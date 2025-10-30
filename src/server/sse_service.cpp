#include "sse_service.hpp"
#include <algorithm> // For std::remove_if
#include <boost/asio/post.hpp> // For net::post
#include <boost/asio/write.hpp> // For boost::asio::async_write

// SseClient implementation
SseClient::SseClient(beast::tcp_stream stream, std::function<void(const std::string&)> logger)
    : m_stream(std::move(stream))
    , m_logger(std::move(logger))
{
}

auto SseClient::start() -> void
{
    // Send initial SSE headers
    auto res = http::response<http::empty_body> {};
    res.version(11); // HTTP/1.1
    res.result(http::status::ok);
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/event-stream");
    res.set(http::field::cache_control, "no-cache");
    res.set(http::field::connection, "keep-alive");
    res.keep_alive(true);

    // Use a message_generator for the header
    auto msg = http::message_generator { std::move(res) };

    // Write the headers
    beast::async_write( // Use beast::async_write for message_generator
        m_stream,
        std::move(msg),
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_write(ec, bytes_transferred);
        });
}

auto SseClient::send_event(const std::string& event_name, const std::string& data) -> void
{
    // SSE format: "event: <event_name>\ndata: <data>\n\n"
    m_write_buffer.clear();
    if (!event_name.empty()) {
        m_write_buffer += "event: ";
        m_write_buffer += event_name;
        m_write_buffer += "\n";
    }
    m_write_buffer += "data: ";
    m_write_buffer += data;
    m_write_buffer += "\n\n";

    // Post the write operation to the stream's executor to ensure thread safety
    net::post(m_stream.get_executor(),
        [self = shared_from_this()]() {
            net::async_write(
                self->m_stream.socket(),
                net::buffer(self->m_write_buffer),
                [self](const beast::error_code& ec, size_t bytes_transferred) {
                    self->on_write(ec, bytes_transferred);
                });
        });
}

auto SseClient::on_write(beast::error_code ec, std::size_t /**/) -> void
{
    if (ec) {
        m_logger("SSE Client write error: " + ec.message());
        close(); // Close connection on error
    }
    // For SSE, we don't read after writing, we just keep the connection open
    // and wait for more events to send.
}

auto SseClient::close() -> void
{
    auto ec = beast::error_code {};
    if (m_stream.socket().shutdown(tcp::socket::shutdown_send, ec)) {
        m_logger("SSE Client shutdown error: " + ec.message());
    }
    // The client will be removed from SseService's list when its shared_ptr count drops to 0
}

// SseService implementation
SseService::SseService(net::io_context& ioc, std::function<void(const std::string&)> logger)
    : m_ioc(ioc)
    , m_logger(std::move(logger))
{
}

auto SseService::add_client(beast::tcp_stream stream) -> void
{
    auto client = std::make_shared<SseClient>(std::move(stream), m_logger);
    {
        auto lock = std::lock_guard<std::mutex>(m_clients_mutex);
        m_clients.push_back(client);
    }
    client->start(); // Send initial headers
    m_logger("SSE client connected. Total clients: " + std::to_string(m_clients.size()));
}

auto SseService::send_event_to_all(const std::string& event_name, const std::string& data) -> void
{
    auto lock = std::lock_guard<std::mutex>(m_clients_mutex);
    // Remove disconnected clients
    std::erase_if(m_clients, [](const std::shared_ptr<SseClient>& client) {
        return !client->m_stream.socket().is_open();
    });

    if (m_clients.empty()) {
        m_logger("No SSE clients to send event '" + event_name + "' to.");
        return;
    }

    m_logger("Sending SSE event '" + event_name + "' with data '" + data + "' to " + std::to_string(m_clients.size()) + " clients.");
    for (const auto& client : m_clients) {
        client->send_event(event_name, data);
    }
}

auto SseService::set_logger(std::function<void(const std::string&)> logger) -> void
{
    m_logger = std::move(logger);
}