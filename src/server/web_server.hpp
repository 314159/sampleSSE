#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <boost/asio/io_context.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// Forward declaration
class Listener;

class WebServer {
public:
    WebServer(
        std::string address,
        unsigned short port,
        std::string doc_root,
        int thread_count = 1);

    // Rule of 5: Non-copyable, but movable
    // Correction: io_context makes this class non-movable as well.
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    WebServer(WebServer&&) = delete;
    WebServer& operator=(WebServer&&) = delete;

    ~WebServer();

    // Start the server in a background thread
    auto run() -> void;

    // Stop the server
    auto stop() -> void;

    // Optional: Callback for logging messages to the GUI
    auto set_logger(std::function<void(const std::string&)> logger) -> void;

private:
    auto log(const std::string& message) -> void;

    boost::asio::io_context m_ioc;
    std::string m_address;
    unsigned short m_port;
    std::string m_doc_root;
    int m_thread_count;

    std::shared_ptr<Listener> m_listener;
    std::vector<std::thread> m_threads;
    std::function<void(const std::string&)> m_logger;
};

#endif // WEB_SERVER_HPP
