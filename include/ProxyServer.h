//
// Created by Xiaoandi Fu on 2024/4/2.
//

#ifndef PROXY_PROXYSERVER_H
#define PROXY_PROXYSERVER_H

#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "mock/mock.h"

using namespace boost::asio;
using namespace boost::asio::ip;

class ProxyServer {
public:
    ProxyServer(unsigned short port, const char* certPath, const char* keyPath, const char* pwd);
    ~ProxyServer();

    void add_mock(MockInterface& mock);
    void add_mock(OnRequest func);
    void add_mock(OnResponse func);

    void run();
private:
    std::string get_password() const;
    void do_accept();

    tcp::acceptor* _acceptor;
    tcp::socket* _socket;
    io_context serverContext;
    io_context sessionContext;

    void* root_cert_;
    void* root_key_;

    MockManager<OnRequest, OnResponse> mockManager_;
};

extern int export_root_certificate(char* password);

#endif //PROXY_PROXYSERVER_H
