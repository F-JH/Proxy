//
// Created by Xiaoandi Fu on 2024/4/2.
//
#include "service/ProxyServer.h"
#include "service/HttpSession.h"

ProxyServer::ProxyServer(boost::asio::io_context &client_context, boost::asio::io_context &server_context, unsigned short port, X509* root_cert, EVP_PKEY* root_key)
    : _acceptor(client_context, tcp::endpoint(tcp::v4(), port)), _socket(client_context), ioContext(&server_context), root_cert_(root_cert), root_key_(root_key){
    do_accept();
}

void ProxyServer::do_accept() {
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
        if (!ec) {
            std::make_shared<HttpSession>(std::move(_socket), ioContext, mockManager_, root_cert_, root_key_)->start();
        }

        do_accept();
    });
}

void ProxyServer::add_mock(MockInterface& mock) {
    mockManager_.addMock(mock);
}

void ProxyServer::add_mock(OnRequest func) {
    mockManager_.onRequest(func);
}

void ProxyServer::add_mock(OnResponse func) {
    mockManager_.onResponse(func);
}
