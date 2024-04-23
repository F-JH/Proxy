//
// Created by Xiaoandi Fu on 2024/4/2.
//

#ifndef PROXY_PROXYSERVER_H
#define PROXY_PROXYSERVER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "mock/mock.h"

using namespace boost::asio;
using namespace boost::asio::ip;

class ProxyServer {
public:
    ProxyServer(boost::asio::io_context &client_context, boost::asio::io_context &server_context, unsigned short port, X509* root_cert, EVP_PKEY* root_key);

    void add_mock(MockInterface& mock);
    void add_mock(OnRequest func);
    void add_mock(OnResponse func);
private:
    std::string get_password() const;
    void do_accept();

    tcp::acceptor _acceptor;
    tcp::socket _socket;
    io_context* ioContext;

    X509* root_cert_;
    EVP_PKEY* root_key_;

    MockManager mockManager_;
};

#endif //PROXY_PROXYSERVER_H
