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

HttpSession::HttpSession(tcp::socket clientSocket, io_context* ioContext, MockManager<OnRequest, OnResponse>& mockManager, X509* rootCert, EVP_PKEY* rootKey) : clientSocket_(std::move(clientSocket)), mockManager_(mockManager), rootCert_(rootCert), rootKey_(rootKey){
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
        }else {
            if (serverSocket_ == nullptr){
                // 连接服务端，检查是否是https
                if (res->request.method() == boost::beast::http::verb::connect){
                    std::string connctResponse = "HTTP/1.1 200 Connection established\r\n\r\n";
                    std::string target = res->request.target();
                    size_t pos = target.find(':');
                    if (pos != target.npos){
                        domain = target.substr(0, pos);
                        port = target.substr(pos + 1);
                    }else {
                        domain = target;
                        port = "443";
                    }
                    asioWrite(connctResponse);
                    delete res;
                }else {
                    URL urlDecode;
                    handleUrl(res->request.target(), &urlDecode);
                    domain = urlDecode.domain;
                    port = urlDecode.port;
                    log(((std::string)to_string(res->request.method())).c_str(), ((std::string) res->request.target()).c_str());

                    // mock
                    REQ req (res->request, domain, port);
                    mockManager_.mockRequest(&req);

                    tcp::resolver resolver(*ioContext_);
                    try{
                        tcp::resolver::results_type endpoints = resolver.resolve(domain, port);
                        serverSocket_ = new boost::beast::tcp_stream(*ioContext_);
                        auto self(shared_from_this());
                        serverSocket_->async_connect(endpoints->endpoint(), [this, self, res](const error_code& error){
                            if (error){
                                stopClient();
                            }else {
                                serverWrite(res);
                            }
                        });
                    }catch (std::exception& e){
                        delete res;
                        std::cerr << e.what() << std::endl;
                    }
                }
                return;
            }
            serverWrite(res);
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
            std::make_shared<HttpsSession>(std::move(boost::beast::ssl_stream<boost::beast::tcp_stream>(std::move(clientSocket_), https_context)), *ioContext_, std::move(https_context), mockManager_, domain, port)->handshakeWithClient();
        }
    });
}

void HttpSession::clientWrite(RES* res) {
    // 写入客户端之前的mock操作
    mockManager_.mockResponse(res);
    auto self(shared_from_this());
    boost::beast::http::async_write(clientSocket_, res->response, [this, self, res](const error_code& error, size_t length){
        delete res;
        if (!error && serverReadCnt == 1 && isServerStop){
            stopClient();
        }
    });
}

void HttpSession::serverRead(RES* res) {
    serverReadCnt++;
    auto self(shared_from_this());
    serverFlatBuffer_.clear();
    boost::beast::http::async_read(*serverSocket_, serverFlatBuffer_,  res->response, [this, self, res](const error_code& error, size_t length){
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

void HttpSession::serverWrite(RES* res) {
    auto self(shared_from_this());
    boost::beast::http::async_write(*serverSocket_, res->request, [this, self, res](const error_code& error, size_t length){
        if (!error){
            serverRead(res);
            clientRead();
        }else {
            delete res;
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