//
// Created by Xiaoandi Fu on 2024/4/16.
//

#ifndef PROXY_HTTPSESSION_H
#define PROXY_HTTPSESSION_H

#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>

#include "ca/cert.h"
#include "mock/mock.h"
#include "utils/log.h"
#include "service/HttpsSession.h"

using namespace boost::asio;
using namespace boost::asio::ip;

typedef boost::beast::error_code error_code;

class HttpSession : public std::enable_shared_from_this<HttpSession>{
public:
    HttpSession(tcp::socket clientSocket, io_context* ioContext, MockManager<OnRequest, OnResponse>& mockManager, X509* rootCert, EVP_PKEY* rootKey);
    void start();
    ~HttpSession();
private:
    std::string getPassword() const;
    void clientRead();
    void clientWrite(REQ* req);
    void serverRead(REQ* req);
    void serverWrite(REQ* req);
    void stopClient();
    void stopServer();
    void asioWrite(std::string res);

    boost::beast::tcp_stream clientSocket_;
    boost::beast::tcp_stream* serverSocket_;
    boost::beast::flat_buffer clientFlatBuffer_;
    boost::beast::flat_buffer serverFlatBuffer_;

    X509* rootCert_;
    EVP_PKEY* rootKey_;
    io_context* ioContext_;
    int serverReadCnt;
    bool isFirstWriteServer;
    bool isServerStop;
    std::string domain;
    std::string port;

    MockManager<OnRequest, OnResponse> mockManager_;
};
#endif //PROXY_HTTPSESSION_H
