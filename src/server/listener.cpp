#include "listener.hpp"
#include "http_session.hpp"
#include <boost/asio/strand.hpp>
#include <iostream>

Listener::Listener(
    net::io_context& ioc,
    tcp::endpoint endpoint,
    std::string doc_root,
    std::function<void(const std::string&)> logger)
    : m_ioc(ioc)
    , m_acceptor(ioc)
    , m_doc_root(std::move(doc_root))
    , m_logger(std::move(logger))
{
    auto ec = beast::error_code {};

    // Open the acceptor
    m_acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        m_logger("Listener error (open): " + ec.message());
        return;
    }

    // Allow address reuse
    m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        m_logger("Listener error (set_option): " + ec.message());
        return;
    }

    // Bind to the server address
    m_acceptor.bind(endpoint, ec);
    if (ec) {
        m_logger("Listener error (bind): " + ec.message());
        return;
    }

    // Start listening for connections
    m_acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        m_logger("Listener error (listen): " + ec.message());
        return;
    }
}

auto Listener::run() -> void
{
    do_accept();
}

auto Listener::do_accept() -> void
{
    // The new connection gets its own strand
    m_acceptor.async_accept(
        net::make_strand(m_ioc),
        beast::bind_front_handler(
            &Listener::on_accept,
            shared_from_this()));
}

auto Listener::on_accept(beast::error_code ec, tcp::socket socket) -> void
{
    if (ec) {
        m_logger("Accept error: " + ec.message());
        // Don't stop listening on recoverable errors
    } else {
        // Create the HTTP session and run it
        std::make_shared<HttpSession>(
            std::move(socket),
            m_doc_root,
            m_logger)
            ->run();
    }

    // Accept another connection
    do_accept();
}