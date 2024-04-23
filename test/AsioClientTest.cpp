//
// Created by Xiaoandi Fu on 2024/4/2.
//

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;

io_context context;

#define MEM_FN(x)       boost::bind(&self_type::x, shared_from_this())
#define MEM_FN1(x,y)    boost::bind(&self_type::x, shared_from_this(),y)
#define MEM_FN2(x,y,z)  boost::bind(&self_type::x, shared_from_this(),y,z)

class talk_to_svr : public boost::enable_shared_from_this<talk_to_svr> {
    typedef talk_to_svr self_type;
    void start(ip::tcp::endpoint ep) {
        sock_.async_connect(ep, MEM_FN1(on_connect, _1));
    }
    talk_to_svr(const std::string &message) : sock_(context), started_(true), message_(message) {}
public:
    typedef boost::system::error_code error_code;
    typedef boost::shared_ptr<talk_to_svr> ptr;

    void do_read() {
        async_read(sock_, buffer(read_buffer_), MEM_FN2(read_complete,_1,_2), MEM_FN2(on_read,_1,_2));
    }
    void do_write(const std::string & msg) {
        if ( !started() ) return;
        std::copy(msg.begin(), msg.end(), write_buffer_);
        sock_.async_write_some( buffer(write_buffer_, msg.size()), MEM_FN2(on_write,_1,_2));
    }

    size_t read_complete(const boost::system::error_code & err, size_t bytes) {
        if (err) return 0;
        bool found = std::find(read_buffer_, read_buffer_ + bytes, '\n') < read_buffer_ + bytes;
        return found ? 0 : 1;
    }

    void on_connect(const error_code & err) {
        if ( !err)      do_write(message_ + "\n");
        else            stop();
    }

    void on_read(const error_code & err, size_t bytes) {
        if (!err) {
            std::string copy(read_buffer_, bytes - 1);
            std::cout << copy << std::endl;
            if (copy == "exit") {
                stop();
            }else {
                do_write(copy + "\n");
            }
        }else {
            stop();
        }
    }
    void on_write(const error_code & err, size_t bytes) {
        do_read();
    }

    static ptr start(ip::tcp::endpoint ep, const std::string &message) {
        ptr new_(new talk_to_svr(message));
        new_->start(ep);
        return new_;
    }
    void stop() {
        if (!started_) return;
        started_ = false;
        sock_.close();
    }
    bool started() { return started_; }
private:
    ip::tcp::socket sock_;
    enum { max_msg = 1024 };
    char read_buffer_[max_msg];
    char write_buffer_[max_msg];
    bool started_;
    std::string message_;
};

int main(){
//    ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
    tcp::resolver resolver(context);
    tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "8001");
    talk_to_svr::start(endpoints->endpoint(), "headers");
    context.run();
}
