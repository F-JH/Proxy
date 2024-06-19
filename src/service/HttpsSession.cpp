//
// Created by Xiaoandi Fu on 2024/4/19.
//
#include "service/HttpsSession.h"

HttpsSession::HttpsSession(boost::beast::ssl_stream<boost::beast::tcp_stream> clientSocket, boost::asio::io_context &ioContext, boost::asio::ssl::context context, MockManager<OnRequest, OnResponse>& mockManager, std::string domain, std::string port)
        : clientSocket_(std::move(clientSocket)), serverSocket_(ioContext, context), resolver(ioContext), mockManager_(mockManager) {

    serverReadCnt = 0;
    isServerStop = false;
    isFirstWriteServer = true;
    isConnect = false;
    domain_= domain;
    port_ = port;
    serverSocket_.set_verify_mode(boost::asio::ssl::verify_none);
    SSL_set_tlsext_host_name(serverSocket_.native_handle(), domain.c_str());
}

HttpsSession::~HttpsSession(){
}

void HttpsSession::connect(RES* res) {
    REQ req {res->request, domain_, port_};
    mockManager_.mockRequest(&req);

    auto endpoints = resolver.resolve(req.domain, req.port); // 这里是同步获取dns，考虑异步
    auto self(shared_from_this());
    boost::beast::get_lowest_layer(serverSocket_).async_connect(endpoints, [this, self, res](const error_code& error, const tcp::endpoint& ep){
        if (error){
            std::cerr << "connect | " << error.what() << std::endl;
        }else {
            handshakeWithServer(res);
        }
    });
}

void HttpsSession::handshakeWithServer(RES* res) {
    auto self(shared_from_this());
    serverSocket_.async_handshake(boost::asio::ssl::stream_base::client, [this, self, res](const error_code &error){
        if (error){
            std::cerr << "handshakeWithServer" << " | " << error.what() << std::endl;
        }else {
            isConnect = true;
            serverWrite(res);
//            handshakeWithClient();
        }
    });
}

void HttpsSession::handshakeWithClient() {
    auto self(shared_from_this());
    clientSocket_.async_handshake(boost::asio::ssl::stream_base::server, [this, self](const error_code &error){
        if (!error){
            clientRead();
        }else {
            std::cerr << "handshakeWithClient" << " | " << error.what() << std::endl;
        }
    });
}

void HttpsSession::clientRead() {
    auto self(shared_from_this());
    clientFlatBuffer_.clear();
    RES* res = new RES;
    res->request = {};
    res->response = {};
    boost::beast::http::async_read(clientSocket_, clientFlatBuffer_, res->request, [this, self, res](const error_code& error, size_t length){
        if (error){
            delete res;
            if (error == boost::asio::error::operation_aborted
                || error == boost::asio::error::interrupted
                || error == boost::asio::error::try_again
                || error == boost::asio::error::would_block){
                clientRead();
            }else {
                stopServer();
            }
        }else if (!isConnect){
            connect(res);
        }else {
            std::string method = to_string(res->request.method());
            url = handleUrl(domain_, port_, (std::string)res->request.target());
            res->url = url;
            log(method.c_str(), url.c_str());
            serverWrite(res);
        }
    });
}

void HttpsSession::clientWrite(RES* res) {
    mockManager_.mockResponse(res);

    auto self(shared_from_this());
    boost::beast::http::async_write(clientSocket_, res->response, [this, self, res](const error_code& error, size_t length){
        delete res;
        if (!error && serverReadCnt == 1 && isServerStop){
            stopClient();
        }
    });
}

void HttpsSession::serverRead(RES* res) {
    serverReadCnt++;
    auto self(shared_from_this());
    serverFlatBuffer_.clear();
    boost::beast::http::async_read(serverSocket_, serverFlatBuffer_, res->response, [this, self, res](const error_code& error, size_t length){
        if (error){
            if (error == boost::asio::error::operation_aborted
                || error == boost::asio::error::interrupted
                || error == boost::asio::error::try_again
                || error == boost::asio::error::would_block) {
                res->response.clear();
                res->response = {};
                serverRead(res);
            }else {
                isServerStop = true;
                if (length > 0){
                    clientWrite(res);
                }else if (serverReadCnt == 1){
                    // 无其他在处理中的clientWrite，直接关闭
                    delete res;
                    stopClient();
                }else {
                    delete res;
                    serverReadCnt--;
                }
            }
        }else {
            clientWrite(res);
        }
    });
}

void HttpsSession::serverWrite(RES* res) {
//    mockManager_.mockRequest(req);

    auto self(shared_from_this());
    boost::beast::http::async_write(serverSocket_,  res->request, [this, self, res](const error_code& error, size_t length){
        if (!error){
            serverRead(res);
            clientRead();
        }else {
            delete res;
        }
    });
}

void HttpsSession::stopServer() {
    try{
        serverSocket_.shutdown();
    }catch (std::exception& ignore){}
}

void HttpsSession::stopClient() {
    try{
        clientSocket_.shutdown();
    }catch (std::exception& ignore){}
}
