//
// Created by Xiaoandi Fu on 2024/4/7.
//

#ifndef PROXY_HTTPSCLIENT_H
#define PROXY_HTTPSCLIENT_H

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

typedef boost::system::error_code error_code;

enum { maxLength = 8192 };

class client{
public:
    client(boost::asio::io_context& ioContext,
           boost::asio::ssl::context& context,
           const tcp::resolver::results_type& endpoints);
    ~client();

private:
    bool verifyCertificate(bool preverified, boost::asio::ssl::verify_context& ctx);
    void connect(const tcp::resolver::results_type& endpoints);
    void handshake();
    void sendRequest();
    void receiveResponse(size_t length);

    boost::asio::ssl::stream<tcp::socket> socket_;
    char request_[maxLength];
    char reply_[maxLength];
};

#endif //PROXY_HTTPSCLIENT_H
