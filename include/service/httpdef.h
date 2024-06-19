//
// Created by Xiaoandi Fu on 2024/4/23.
//

#ifndef PROXY_HTTPDEF_H
#define PROXY_HTTPDEF_H
#include <functional>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
using namespace boost::beast::http;

typedef boost::beast::http::request<boost::beast::http::string_body> REQUEST;
typedef boost::beast::http::response<boost::beast::http::string_body> RESPONSE;

typedef struct response_ {
    REQUEST request;
    RESPONSE response;
    boost::beast::string_view url;
} RES;

typedef struct request_ {
    REQUEST& request;
    std::string& domain;
    std::string& port;

    request_(REQUEST& req, std::string& _domain, std::string& _port)
        : request(req), domain(_domain), port(_port) {}
} REQ;

typedef std::function<void(REQ*)> OnRequest;
typedef std::function<void(RES*)> OnResponse;

#endif //PROXY_HTTPDEF_H
