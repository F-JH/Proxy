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

typedef struct req_res {
    REQUEST request;
    RESPONSE response;
} REQ;

typedef std::function<void(REQUEST* req)> OnRequest;
typedef std::function<void(REQ* req)> OnResponse;

#endif //PROXY_HTTPDEF_H
