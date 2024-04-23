//
// Created by Xiaoandi Fu on 2024/4/19.
//
#include "service/HttpsSession.h"

HttpsSession::HttpsSession(boost::beast::ssl_stream<boost::beast::tcp_stream> clientSocket, boost::asio::io_context &ioContext, boost::asio::ssl::context context, MockManager& mockManager, std::string domain, std::string port)
        : clientSocket_(std::move(clientSocket)), serverSocket_(ioContext, context), resolver(ioContext), mockManager_(mockManager) {

    serverReadCnt = 0;
    isServerStop = false;
    isFirstWriteServer = true;
    domain_= domain;
    port_ = port;
    serverSocket_.set_verify_mode(boost::asio::ssl::verify_none);
    SSL_set_tlsext_host_name(serverSocket_.native_handle(), domain.c_str());
}

HttpsSession::~HttpsSession(){
}

void HttpsSession::connect() {
    auto endpoints = resolver.resolve(domain_, "https"); // 这里是同步获取dns，考虑异步
    auto self(shared_from_this());
    boost::beast::get_lowest_layer(serverSocket_).async_connect(endpoints, [this, self](const error_code& error, const tcp::endpoint& ep){
        if (error){
            std::cerr << "connect | " << error.what() << std::endl;
        }else {
            handshakeWithServer();
        }
    });
}

void HttpsSession::handshakeWithServer() {
    auto self(shared_from_this());
    serverSocket_.async_handshake(boost::asio::ssl::stream_base::client, [this, self](const error_code &error){
        if (error){
            std::cerr << "handshakeWithServer" << " | " << error.what() << std::endl;
        }else {
            handshakeWithClient();
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
    REQ* req = new REQ;
    req->request = {};
    req->response = {};
    boost::beast::http::async_read(clientSocket_, clientFlatBuffer_, req->request, [this, self, req](const error_code& error, size_t length){
        if (error){
            delete req;
            if (error == boost::asio::error::operation_aborted
                || error == boost::asio::error::interrupted
                || error == boost::asio::error::try_again
                || error == boost::asio::error::would_block){
                clientRead();
            }else {
                stopServer();
            }
        }else {
            std::string method = to_string(req->request.method());
            url = handleUrl(domain_, port_, (std::string)req->request.target());
            log(method.c_str(), url.c_str());
            serverWrite(req);
        }
    });
}

void HttpsSession::clientWrite(REQ* req) {
    mockManager_.mockResponse(req);

    auto self(shared_from_this());
    boost::beast::http::async_write(clientSocket_, req->response, [this, self, req](const error_code& error, size_t length){
        delete req;
        if (!error && serverReadCnt == 1 && isServerStop){
            stopClient();
        }
    });
}

void HttpsSession::serverRead(REQ* req) {
    serverReadCnt++;
    auto self(shared_from_this());
    serverFlatBuffer_.clear();
    boost::beast::http::async_read(serverSocket_, serverFlatBuffer_, req->response, [this, self, req](const error_code& error, size_t length){
        if (error){
            if (error == boost::asio::error::operation_aborted
                || error == boost::asio::error::interrupted
                || error == boost::asio::error::try_again
                || error == boost::asio::error::would_block) {
                req->response.clear();
                req->response = {};
                serverRead(req);
            }else {
                isServerStop = true;
                if (length > 0){
                    clientWrite(req);
                }else if (serverReadCnt == 1){
                    // 无其他在处理中的clientWrite，直接关闭
                    delete req;
                    stopClient();
                }else {
                    delete req;
                    serverReadCnt--;
                }
            }
        }else {
            clientWrite(req);
        }
    });
}

void HttpsSession::serverWrite(REQ* req) {
    mockManager_.mockRequest(req);

    auto self(shared_from_this());
    boost::beast::http::async_write(serverSocket_,  req->request, [this, self, req](const error_code& error, size_t length){
        if (!error){
            serverRead(req);
            clientRead();
        }else {
            delete req;
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
