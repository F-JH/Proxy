//
// Created by Xiaoandi Fu on 2024/4/2.
//
#include "ca/cert.h"
#include "ProxyServer.h"
#include "service/HttpSession.h"

ProxyServer::ProxyServer(unsigned short port, const char* certPath, const char* keyPath, const char* pwd)
    : mockManager_(){
    OpenSSL_add_all_algorithms();
    _socket = new tcp::socket(serverContext);
    _acceptor = new tcp::acceptor(serverContext, tcp::endpoint(tcp::v4(), port));
    root_cert_ = load_crt(certPath, pwd);
    root_key_ = load_key(keyPath, pwd);
}

ProxyServer::~ProxyServer(){
    delete _acceptor;
    delete _socket;
}

void ProxyServer::do_accept() {
    _acceptor->async_accept(*_socket, [this](boost::system::error_code ec) {
        if (!ec) {
            std::make_shared<HttpSession>(std::move(*_socket), &sessionContext, mockManager_, (X509*)root_cert_, (EVP_PKEY*)root_key_)->start();
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

void ProxyServer::run() {
    do_accept();

    std::thread server([&](){
        [[maybe_unused]] auto work_guard = make_work_guard(serverContext);
        serverContext.run();
    });
    std::thread session([&](){
        [[maybe_unused]] auto work_guard = make_work_guard(sessionContext);
        sessionContext.run();
    });

    server.join();
    session.join();
}
