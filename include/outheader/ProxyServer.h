//
// Created by Xiaoandi Fu on 2024/4/24.
//
// 给外部的头文件
#ifndef PROXY_PROXYSERVER_H
#define PROXY_PROXYSERVER_H

#include <functional>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

typedef boost::beast::http::request<boost::beast::http::string_body> REQUEST;
typedef boost::beast::http::response<boost::beast::http::string_body> RESPONSE;

typedef struct req_res {
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::http::response<boost::beast::http::string_body> response;
} REQ;

typedef void (*OnRequest)(REQUEST*);
typedef void (*OnResponse)(REQ*);

class MockInterface {
public:
    virtual void onRequest(REQ* req);
    virtual void onResponse(REQ* req);
};

class ProxyServer {
public:
    ProxyServer(unsigned short port, const char* certPath, const char* keyPath, const char* pwd);

    void add_mock(MockInterface& mock);
    void add_mock(OnRequest func);
    void add_mock(OnResponse func);

    void run();
};

#endif //PROXY_PROXYSERVER_H
