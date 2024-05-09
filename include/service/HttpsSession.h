//
// Created by Xiaoandi Fu on 2024/4/19.
//

#ifndef PROXY_HTTPSSESSION_H
#define PROXY_HTTPSSESSION_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <vector>
#include <string>

#include "utils/log.h"
#include "mock/mock.h"

using namespace boost::asio;
using namespace boost::beast;
using namespace boost::asio::ip;
using namespace boost::asio::error;
using namespace boost::beast::http;

typedef boost::system::error_code error_code;

class HttpsSession : public std::enable_shared_from_this<HttpsSession> {
public:
    HttpsSession(boost::beast::ssl_stream<boost::beast::tcp_stream> clientSocket, io_context& ioContext, boost::asio::ssl::context context, MockManager<OnRequest, OnResponse>& mockManager, std::string domain, std::string port);
    ~HttpsSession();
    void connect();
private:
    template<class Body, class Allocator>
    boost::beast::http::message_generator handle_request(http::request<Body, http::basic_fields<Allocator>>&& req);
    void handshakeWithClient();
    void handshakeWithServer();
    void clientRead();
    void clientWrite(REQ* req);
    void serverRead(REQ* req);
    void serverWrite(REQ* req);
    bool verifyCertificate(bool preverified, boost::asio::ssl::verify_context& ctx);
    void stopClient();
    void stopServer();

    boost::beast::ssl_stream<boost::beast::tcp_stream> clientSocket_;
    boost::beast::ssl_stream<boost::beast::tcp_stream> serverSocket_;

    boost::beast::flat_buffer clientFlatBuffer_;
    boost::beast::flat_buffer serverFlatBuffer_;

    tcp::resolver resolver;
    std::string domain_;
    std::string port_;
    int serverReadCnt;
    bool isFirstWriteServer;
    bool isServerStop;

    MockManager<OnRequest, OnResponse> mockManager_;
    std::string url;
};

#endif //PROXY_HTTPSSESSION_H
