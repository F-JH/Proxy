#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

//#include "../example/mock_function.h"
#include "ProxyServer.h"
#include "gzip.h"


#define PORT 8999

std::string mockJson = "";

void setContentLength(RESPONSE* response, size_t length){
    std::string len = std::to_string(length);
    response->set(boost::beast::http::field::content_length, len);
}

inline bool exists_file(const std::string& name){
    std::ifstream f(name.c_str());
    bool result = f.good();
    f.close();
    return result;
}

std::string to_string(rapidjson::Document& json){
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    return buffer.GetString();
}

int main(int argc, char* argv[]) {
    if (!exists_file("rootCa.crt") || !exists_file("rootCa.key")){
        export_root_certificate(""); // 生成根证书
    }

    try {
        std::cout << "proxy servere run at " << PORT << std::endl;
        ProxyServer proxyServer(PORT, "rootCa.crt", "rootCa.key", "");

        // mock响应数据
        proxyServer.add_mock([&](RES* res){
            if (res->url.contains("example.com/exampleApi")){
                std::string bodyString;
                if (isGzip(res->response)){
                    bodyString = unCompressGzip(res->response.body());
                }else {
                    bodyString = res->response.body();
                }

                // 修改返回数据
                rapidjson::Document body;
                body.Parse(bodyString.c_str());
                body["data"]["example"] = "example";
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                body.Accept(writer);
                setContentLength(&res->response, strlen(buffer.GetString()));

                if (isGzip(res->response)){
                    res->response.body() = compressGzip(buffer.GetString());
                }else {
                    res->response.body() = buffer.GetString();
                }
            }
        });

        // mock请求数据,重定向域名
        proxyServer.add_mock([&](REQ* req){
            if (req->domain.contains("example-test.com")){
                req->domain = "example.com";
            }
        });

        proxyServer.run();
    } catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
