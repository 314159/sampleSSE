#include "web_server.hpp"
#include "listener.hpp"
#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

WebServer::WebServer(
    std::string address,
    unsigned short port,
    std::string doc_root,
    int thread_count)
    : m_ioc { thread_count }
    , m_address(std::move(address))
    , m_port(port)
    , m_doc_root(std::move(doc_root))
    , m_thread_count(thread_count > 0 ? thread_count : 1)
{
    // The default logger just prints to cerr
    m_logger = [](const std::string& msg) {
        std::cerr << msg << std::endl;
    };
}

WebServer::~WebServer()
{
    // If the server is running, stop it. This ensures RAII for the threads.
    if (!m_ioc.stopped()) {
        stop();
    }
}

auto WebServer::run() -> void
{
    log("Server starting...");

    auto const addr = net::ip::make_address(m_address);

    // Create and launch a listening port
    m_listener = std::make_shared<Listener>( //
        m_ioc,
        tcp::endpoint { addr, m_port },
        m_doc_root,
        m_logger);
    m_listener->run();

    // Run the I/O service on the requested number of threads
    m_threads.reserve(m_thread_count);
    for (int i = 0; i < m_thread_count; ++i) {
        m_threads.emplace_back([this] { m_ioc.run(); });
    }
    log("Server started successfully. Thread count: " + std::to_string(m_thread_count));
}

auto WebServer::stop() -> void
{
    log("Server stopping...");

    // This will cause all active I/O operations to fail with an error
    // that can be ignored, and for io_context::run() to return.
    m_ioc.stop();

    // Wait for all threads in the pool to exit
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_threads.clear();
    log("Server stopped.");
}

auto WebServer::set_logger(std::function<void(const std::string&)> logger) -> void
{
    m_logger = std::move(logger);
}

auto WebServer::log(const std::string& message) -> void
{
    if (m_logger) {
        m_logger(message);
    }
}