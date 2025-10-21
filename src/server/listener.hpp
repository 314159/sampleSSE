#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <functional>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Forward declaration
class HttpSession;

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        std::string doc_root,
        std::function<void(const std::string&)> logger);

    // A listener is a unique resource, so it should not be copyable or movable.
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;
    Listener(Listener&&) = delete;
    Listener& operator=(Listener&&) = delete;

    // Start accepting incoming connections
    auto run() -> void;

private:
    auto do_accept() -> void;
    auto on_accept(beast::error_code ec, tcp::socket socket) -> void;

    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    std::string m_doc_root;
    std::function<void(const std::string&)> m_logger;
};

#endif // LISTENER_HPP
