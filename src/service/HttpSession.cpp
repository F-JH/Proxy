//
// Created by Xiaoandi Fu on 2024/4/16.
//
#include "service/HttpSession.h"

typedef struct urldec{
    std::string protocol;
    std::string domain;
    std::string port;
    std::string path;
} URL;

void handleUrl(std::string url, URL* parse){
    size_t protocol = url.find("://");
    parse->protocol = url.substr(0, protocol);
    size_t domain = url.find("/", protocol + 3);
    std::string domainAll = url.substr(protocol + 3, domain - protocol - 3);
    parse->path = url.substr(domain);
    size_t point = domainAll.find(':');
    if (point != domainAll.npos){
        parse->domain = domainAll.substr(0, point);
        parse->port = domainAll.substr(point + 1);
    }else {
        parse->domain = domainAll;
        parse->port = "80";
    }
}

std::string HttpSession::getPassword() const {
    return "Horizon972583048";
}

HttpSession::HttpSession(tcp::socket clientSocket, io_context* ioContext, MockManager& mockManager, X509* rootCert, EVP_PKEY* rootKey) : clientSocket_(std::move(clientSocket)), mockManager_(mockManager), rootCert_(rootCert), rootKey_(rootKey){
    serverSocket_ = nullptr;
    ioContext_ = ioContext;
    isFirstWriteServer = true;
    serverReadCnt = 0;
}

HttpSession::~HttpSession() {
    if (serverSocket_ != nullptr) delete serverSocket_;
}

void HttpSession::start() {
    clientRead();
}

void HttpSession::clientRead() {
    auto self(shared_from_this());
//    REQUEST* request = new REQUEST({});
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
            if (serverSocket_ == nullptr){
                // 连接服务端，检查是否是https
                if (req->request.method() == boost::beast::http::verb::connect){
                    std::string connctResponse = "HTTP/1.1 200 Connection established\r\n\r\n";
                    std::string target = req->request.target();
                    size_t pos = target.find(':');
                    if (pos != target.npos){
                        domain = target.substr(0, pos);
                        port = target.substr(pos + 1);
                    }else {
                        domain = target;
                        port = "443";
                    }
                    asioWrite(connctResponse);
                    delete req;
                }else {
                    URL urlDecode;
                    handleUrl(req->request.target(), &urlDecode);
                    domain = urlDecode.domain;
                    port = urlDecode.port;
                    log(((std::string)to_string(req->request.method())).c_str(), ((std::string) req->request.target()).c_str());
                    tcp::resolver resolver(*ioContext_);
                    try{
                        tcp::resolver::results_type endpoints = resolver.resolve(domain, port);
//                        serverSocket_ = new tcp::socket(*ioContext_);
                        serverSocket_ = new boost::beast::tcp_stream(*ioContext_);
                        auto self(shared_from_this());
                        serverSocket_->async_connect(endpoints->endpoint(), [this, self, req](const error_code& error){
                            if (error){
                                stopClient();
                            }else {
                                serverWrite(req);
                            }
                        });
                    }catch (std::exception& e){
                        delete req;
                        std::cerr << e.what() << std::endl;
                    }
                }
                return;
            }
            serverWrite(req);
        }
    });
}

void HttpSession::asioWrite(std::string res) {
    auto self(shared_from_this());
    boost::asio::async_write(clientSocket_, buffer(res, res.size()), transfer_all(), [this, self](const error_code& error, size_t length){
        if(error){
            std::cerr << error.what() << std::endl;
            stopClient();
        }else {
            boost::asio::ssl::context https_context(boost::asio::ssl::context::sslv23);
            https_context.set_options(
                    boost::asio::ssl::context::default_workarounds
                    | boost::asio::ssl::context::no_sslv2
                    | boost::asio::ssl::context::single_dh_use);
            https_context.set_password_callback(std::bind(&HttpSession::getPassword, this));
            std::string cert = get_domain_certificate_file(rootCert_, rootKey_, domain);
            https_context.use_certificate_chain(buffer(cert));
            https_context.use_private_key(buffer(cert), boost::asio::ssl::context::pem);
            std::make_shared<HttpsSession>(std::move(boost::beast::ssl_stream<boost::beast::tcp_stream>(std::move(clientSocket_), https_context)), *ioContext_, std::move(https_context), mockManager_, domain, port)->connect();
        }
    });
}

void HttpSession::clientWrite(REQ* req) {
    // 写入客户端之前的mock操作
//    mockManager_.mockResponse(response);

    auto self(shared_from_this());
    boost::beast::http::async_write(clientSocket_, req->response, [this, self, req](const error_code& error, size_t length){
        delete req;
        if (!error && serverReadCnt == 1 && isServerStop){
            stopClient();
        }
    });
}

void HttpSession::serverRead(REQ* req) {
    serverReadCnt++;
    auto self(shared_from_this());
    serverFlatBuffer_.clear();
    boost::beast::http::async_read(*serverSocket_, serverFlatBuffer_,  req->response, [this, self, req](const error_code& error, size_t length){
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

void HttpSession::serverWrite(REQ* req) {
    // 写入服务端之前的mock操作
//    mockManager_.mockRequest(request);

    auto self(shared_from_this());
    boost::beast::http::async_write(*serverSocket_, req->request, [this, self, req](const error_code& error, size_t length){
        if (!error){
            serverRead(req);
            clientRead();
        }else {
            delete req;
        }
    });
}

void HttpSession::stopClient() {
    if (serverSocket_ != nullptr){
        try{
            serverSocket_->close();
        }catch (std::exception& ignore){}
    }
}

void HttpSession::stopServer() {
    try{
        clientSocket_.close();
    }catch (std::exception& ignore){}
}