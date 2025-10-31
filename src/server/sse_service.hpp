#ifndef SSE_SERVICE_HPP
#define SSE_SERVICE_HPP

#include <atomic>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

// Represents a single SSE client connection
class SseClient : public std::enable_shared_from_this<SseClient> {
public:
    SseClient(beast::tcp_stream stream, std::function<void(const std::string&)> logger);

    // Start the SSE connection (send initial headers)
    auto start() -> void;

    // Send an event to this client
    auto send_event(const std::string& event_name, const std::string& data) -> void;

    // Check if the client connection is closed
    auto is_closed() const -> bool;

    // Public for SseService to check socket status
    beast::tcp_stream m_stream;

private:
    auto on_sse_startup(beast::error_code ec, std::size_t bytes_transferred) -> void;
    auto on_write(beast::error_code ec, std::size_t bytes_transferred) -> void;
    auto do_read() -> void;
    auto on_read(beast::error_code ec, std::size_t bytes_transferred) -> void;
    auto close() -> void;

    std::function<void(const std::string&)> m_logger;
    std::string m_write_buffer; // Buffer for outgoing SSE data
    std::atomic<bool> m_is_closed { false };
};

// Manages all SSE client connections
class SseService : public std::enable_shared_from_this<SseService> {
public:
    SseService(net::io_context& ioc, std::function<void(const std::string&)> logger);
    auto add_client(beast::tcp_stream stream) -> void;
    auto send_event_to_all(const std::string& event_name, const std::string& data) -> void;
    auto set_logger(std::function<void(const std::string&)> logger) -> void;

private:
    auto prune_clients() -> void; // Must be called under lock
    net::io_context& m_ioc;
    std::function<void(const std::string&)> m_logger;
    std::vector<std::shared_ptr<SseClient>> m_clients;
    std::mutex m_clients_mutex; // Protect m_clients vector
};

#endif // SSE_SERVICE_HPP