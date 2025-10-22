#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include "sse_service.hpp" // Include SSE service header
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/config.hpp>
#include <functional>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(
        net::ip::tcp::socket&& socket,
        std::string doc_root,
        std::function<void(const std::string&)> logger,
        std::shared_ptr<SseService> sse_service); // Add SSE service

    // Start the session
    auto run() -> void;

private:
    auto do_read() -> void;
    auto on_read(beast::error_code ec, std::size_t bytes_transferred) -> void;
    auto handle_request() -> void;
    auto send_response(http::message_generator&& msg) -> void;
    auto on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) -> void;
    auto do_close() -> void;
    auto log(const std::string& message) -> void;

    // Helper to create error responses
    auto create_error_response(http::status status, const std::string& why) -> http::response<http::string_body>;

    beast::tcp_stream m_stream;
    beast::flat_buffer m_buffer;
    std::string m_doc_root;
    http::request<http::string_body> m_req;
    std::function<void(const std::string&)> m_logger;
    std::shared_ptr<SseService> m_sse_service; // SSE service instance
};

#endif // HTTP_SESSION_HPP
