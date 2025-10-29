#include "http_session.hpp"
#include <boost/beast/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/url/url_view.hpp> // For parsing URL paths
#include <iostream>

namespace fs = boost::filesystem;

constexpr char to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - ('A' - 'a');
    }
    return c;
}

constexpr bool iequals_ascii(std::string_view a, std::string_view b)
{
    if (a.length() != b.length()) {
        return false;
    }
    for (size_t i = 0; i < a.length(); ++i) {
        if (to_lower_ascii(a[i]) != to_lower_ascii(b[i])) {
            return false;
        }
    }
    return true;
}

// A map of file extensions to MIME types.
constexpr std::array<std::pair<std::string_view, std::string_view>, 20> mime_types {
    { { ".css", "text/css" },
        { ".flv", "video/x-flv" },
        { ".gif", "image/gif" },
        { ".gz", "application/gzip" },
        { ".htm", "text/html" },
        { ".html", "text/html" },
        { ".ico", "image/vnd.microsoft.icon" },
        { ".jpe", "image/jpeg" },
        { ".jpeg", "image/jpeg" },
        { ".jpg", "image/jpeg" },
        { ".js", "application/javascript" },
        { ".json", "application/json" },
        { ".php", "text/html" },
        { ".png", "image/png" },
        { ".svg", "image/svg+xml" },
        { ".swf", "application/x-shockwave-flash" },
        { ".tar", "application/x-tar" },
        { ".txt", "text/plain" },
        { ".xml", "application/xml" },
        { ".zip", "application/zip" } }
};

// Returns the MIME type for a given file extension.
constexpr std::string_view mime_type_for(std::string_view extension)
{
    for (const auto& pair : mime_types) {
        if (iequals_ascii(pair.first, extension)) {
            return pair.second;
        }
    }
    return "application/octet-stream";
}

// This function is kept to maintain the original interface.
beast::string_view mime_type(beast::string_view path)
{
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos)
            return beast::string_view {};
        return path.substr(pos);
    }();
    return mime_type_for(std::string_view(ext.data(), ext.size()));
}

HttpSession::HttpSession(
    net::ip::tcp::socket&& socket,
    std::string doc_root,
    std::function<void(const std::string&)> logger,
    std::shared_ptr<SseService> sse_service)
    : m_stream(std::move(socket))
    , m_doc_root(std::move(doc_root))
    , m_logger(std::move(logger))
{
    m_sse_service = std::move(sse_service);
}

auto HttpSession::run() -> void
{
    // We need to be executing within a strand to perform async operations
    // on the stream. We use a strand to ensure that handlers do not execute
    // concurrently.
    net::dispatch(m_stream.get_executor(),
        beast::bind_front_handler(&HttpSession::do_read, shared_from_this()));
}

auto HttpSession::do_read() -> void
{
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    m_req = {};

    // Set the timeout.
    m_stream.expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(m_stream, m_buffer, m_req,
        beast::bind_front_handler(&HttpSession::on_read, shared_from_this()));
}

auto HttpSession::on_read(beast::error_code ec, std::size_t bytes_transferred) -> void
{
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return do_close();

    if (ec) {
        log("Read error: " + ec.message());
        return;
    }

    // Log the request
    log(std::string(m_req.method_string()) + " " + std::string(m_req.target()));

    // Handle the request
    handle_request();
}

auto HttpSession::handle_request() -> void
{
    // Returns a bad request response
    auto const bad_request = [this](beast::string_view why) {
        return create_error_response(http::status::bad_request, std::string(why));
    };

    // We only support GET and HEAD methods
    if (m_req.method() != http::verb::get && m_req.method() != http::verb::head)
        return send_response(bad_request("Unknown HTTP-method"));

    // Check for SSE endpoint
    if (m_req.target() == "/sse") {
        if (m_sse_service) {
            m_logger("Handing off connection to SSE service.");
            m_sse_service->add_client(std::move(m_stream));
        }
        return; // Connection is now managed by SseService
    }

    // Request path must be absolute and not contain "..".
    if (m_req.target().empty() || m_req.target()[0] != '/' || m_req.target().find("..") != beast::string_view::npos)
        return send_response(bad_request("Illegal request-target"));

    // Build the path to the requested file
    auto path = fs::path { m_doc_root } / std::string(m_req.target().substr(1));
    if (m_req.target().back() == '/')
        path /= "index.html";

    // Attempt to open the file
    auto ec = beast::error_code {};
    http::file_body::value_type body;
    body.open(path.string().c_str(), beast::file_mode::scan, ec);

    // Handle file not found
    if (ec == beast::errc::no_such_file_or_directory)
        return send_response(create_error_response(http::status::not_found, "The resource '" + std::string(m_req.target()) + "' was not found."));

    // Handle other file errors
    if (ec)
        return send_response(create_error_response(http::status::internal_server_error, "An error occurred: '" + ec.message() + "'"));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (m_req.method() == http::verb::head) {
        auto res = http::response<http::empty_body> { http::status::ok, m_req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path.string()));
        res.content_length(size);
        res.keep_alive(m_req.keep_alive());
        return send_response(std::move(res));
    }

    // Respond to GET request
    auto res = http::response<http::file_body> {
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, m_req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path.string()));
    res.content_length(size);
    res.keep_alive(m_req.keep_alive());
    return send_response(std::move(res));
}

auto HttpSession::send_response(http::message_generator&& msg) -> void
{
    auto const keep_alive = msg.keep_alive();

    // Write the response
    beast::async_write(
        m_stream,
        std::move(msg),
        beast::bind_front_handler(&HttpSession::on_write, shared_from_this(), keep_alive));
}

auto HttpSession::on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) -> void
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return log("Write error: " + ec.message());

    if (!keep_alive) {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return do_close();
    }

    // Read another request
    do_read();
}

auto HttpSession::do_close() -> void
{
    // Send a TCP shutdown
    auto ec = beast::error_code {};
    m_stream.socket().shutdown(net::ip::tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}

auto HttpSession::log(const std::string& message) -> void
{
    if (m_logger) {
        m_logger("[Session " + std::to_string(reinterpret_cast<uintptr_t>(this)) + "] " + message);
    }
}

auto HttpSession::create_error_response(http::status status, const std::string& why) -> http::response<http::string_body>
{
    auto res = http::response<http::string_body> { status, m_req.version() };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(m_req.keep_alive());
    res.body() = "<h1>Error</h1><p>" + why + "</p>";
    res.prepare_payload();
    return res;
}