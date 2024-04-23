//
// Created by Xiaoandi Fu on 2024/4/3.
//
#include <cstdlib>
#include <functional>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "ca/cert.h"
using boost::asio::ip::tcp;

using namespace boost::asio;
using namespace boost::asio::ip;

typedef boost::system::error_code error_code;

X509* root_cert;
EVP_PKEY* root_key;
char index_html[1024];
int len = 0;

void init() {
    root_cert = load_crt("certificate/rootCa.crt", "Horizon972583048");
    root_key = load_key("certificate/rootCa.key", "Horizon972583048");

    memset(index_html, '\0', 1024);
    FILE* home = fopen("dist/headers", "r");
    if (home != NULL) {
        len = fread(index_html, 1, 1024, home);
        fclose(home);
    }
}

class session : public std::enable_shared_from_this<session> {
public:
    session(boost::asio::ssl::stream<tcp::socket> socket): socket_(std::move(socket)){}
    ~session(){
        std::cout << "~session call" << std::endl;
    }
    void start(){
        do_handshake();
    }
private:
    void do_handshake(){
        auto self(shared_from_this());
        socket_.async_handshake(boost::asio::ssl::stream_base::server, [this, self](const error_code &error){
            if (!error){
                std::cout << "connect from " << socket_.lowest_layer().remote_endpoint().address() << ":" << socket_.lowest_layer().remote_endpoint().port() << std::endl;
                do_read();
            }else {
                std::cerr << error.what() << std::endl;
            }
        });
    }

    void do_read(){
        auto self(shared_from_this());
        socket_.async_read_some(buffer(data_), [this, self](const error_code &error, size_t length){
            std::cerr << error.what() << std::endl;
            if (!error){
                do_write(length);
            }else if (error == boost::asio::error::eof){}
            else {
                std::cerr << error.what() << std::endl;
            }
        });
    }

    void do_write(size_t length){
        auto self(shared_from_this());
        async_write(socket_, buffer(index_html, len), [this, self](const error_code &error, size_t length){
//            if (!error){
//                do_read();
//            }else {
//                std::cerr << error.what() << std::endl;
//            }
//            try{
//                socket_.shutdown();
//            }catch (const std::exception& ignore){
//                std::cerr << ignore.what() << std::endl;
//            }
        });
    }
    boost::asio::ssl::stream<tcp::socket> socket_;
    char data_[1024];
};

class server {
public:
    server(boost::asio::io_context &io_context, unsigned short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), context_(boost::asio::ssl::context::sslv23){
        context_.set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::single_dh_use);
        context_.set_password_callback(std::bind(&server::get_password, this));
//        auto cert = create_fake_certificate_by_domain(root_cert, root_key, "127.0.0.1:8991");
        std::string cert = get_domain_certificate_file(root_cert, root_key, "www.test.com");
        context_.use_certificate_chain(buffer(cert));
        context_.use_private_key(buffer(cert), boost::asio::ssl::context::pem);

        do_accept();
    }
private:
    std::string get_password() const{
        return "Horizon972583048";
    }

    void do_accept(){
        acceptor_.async_accept([this](const error_code &error, tcp::socket socket){
            if (!error){
                std::make_shared<session>(boost::asio::ssl::stream<tcp::socket>(std::move(socket), context_))->start();
            }
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    boost::asio::ssl::context context_;

    std::string domain;
};

int main(){
    OpenSSL_add_all_algorithms();
    init();
    boost::asio::io_context ioContext;

    int port = 8991;

    try{
        std::cout << "serverTest run at port " << port << std::endl;
        server s(ioContext, port);
        ioContext.run();
    }catch (std::exception &e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
