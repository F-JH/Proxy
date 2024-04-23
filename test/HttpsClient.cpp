//
// Created by Xiaoandi Fu on 2024/4/7.
//
#include "HttpsClient.h"

client::~client() {
    std::cout << "~client" << std::endl;
}

client::client(boost::asio::io_context &ioContext, boost::asio::ssl::context &context,
               const tcp::resolver::results_type &endpoints) : socket_(ioContext, context){
    socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    socket_.set_verify_callback(std::bind(&client::verifyCertificate,  this, _1, _2));

    connect(endpoints);
}

bool client::verifyCertificate(bool preverified, boost::asio::ssl::verify_context &ctx) {
//    char subjectName[256];
//    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
//    X509_NAME_oneline(X509_get_subject_name(cert), subjectName, 256);
//    std::cout << "Verifying " << subjectName << "\n";

    return true;
}

void client::connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(socket_.lowest_layer(), endpoints, [this](const error_code& error, const tcp::endpoint& endpoint){
        if (!error){
            handshake();
        }else {
            std::cerr << error.what() << std::endl;
        }
    });
}

void client::handshake() {
    socket_.async_handshake(boost::asio::ssl::stream_base::client, [this](const error_code& error){
        if (!error){
            sendRequest();
        }else {
            std::cerr << error.what() << std::endl;
        }
    });
}

void client::sendRequest() {
    std::string msg = "GET https://xiaoandi.myshoplinestg.com/admin/ HTTP/1.1\r\nHost: xiaoandi.myshoplinestg.com\r\nUser-Agent: curl/7.87.0\r\nAccept: */*\r\n\r\n";
    boost::asio::async_write(socket_, boost::asio::buffer(msg.c_str(), msg.size()), [this](const boost::system::error_code& error, std::size_t length){
        if (!error){
            receiveResponse(length);
        }else {
            std::cerr << "Write failed: " << error.message() << "\n";
        }
    });
}

void client::receiveResponse(size_t length) {
    socket_.async_read_some(boost::asio::buffer(reply_, 8192),[this](const boost::system::error_code& error, std::size_t length){
        if (!error){
            std::cout << "Reply: ";
            std::cout.write(reply_, length);
            std::cout << "\n";
        }else{
            std::cerr << "Read failed: " << error.message() << "\n";
        }
    });
}

int main(){
    boost::asio::io_context ioContext;
    tcp::resolver resolver(ioContext);
    auto endpoints = resolver.resolve("xiaoandi.myshoplinestg.com", "https");

    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    client c(ioContext, ctx, endpoints);
    ioContext.run();
}