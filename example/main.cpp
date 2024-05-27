#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "ProxyServer.h"
#include "gzip.h"

#define PORT 8999

std::string mockJson = "";

void setContentLength(RESPONSE* response, size_t length){
    std::string len = std::to_string(length);
    response->set(boost::beast::http::field::content_length, len);
}

void updateJson(){
    while (true){
        std::ifstream mockFile("example/mock.json");
        if (!mockFile.is_open()){
            std::cerr << "Failed to open the mock.json" << std::endl;
            return;
        }
        std::string line;
        std::string jsonTemp;
        while (std::getline(mockFile, line)){
            jsonTemp += line + "\n";
        }
        mockFile.close();
        mockJson = jsonTemp;
        sleep(5);
    }
}

inline bool exists_file(const std::string& name){
    std::ifstream f(name.c_str());
    bool result = f.good();
    f.close();
    return result;
}

int main(int argc, char* argv[]) {
    if (!exists_file("rootCa.crt") || !exists_file("rootCa.key")){
        export_root_certificate("");
    }

    try {
        std::cout << "proxy servere run at " << PORT << std::endl;
        ProxyServer proxyServer(PORT, "rootCa.crt", "rootCa.key", "");

        proxyServer.add_mock([&](REQ* req){
            if (req->url.contains("example.com/example_api")){
                std::string requestData;
                if (isGzip(req->request)){
                    requestData = unCompressGzip(req->request.body());
                }else {
                    requestData = req->request.body();
                }

                rapidjson::Document data;
                data.Parse(requestData.c_str());

                if (data["data"][0] == "example"){
                    rapidjson::Document mockData;
                    mockData.Parse(mockJson.c_str());
                    if (!mockData.IsObject()){
                        std::cerr << "mockData is not a json object !" << std::endl;
                        return;
                    }
                    rapidjson::StringBuffer rapidBuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(rapidBuffer);
                    mockData["mymock"].Accept(writer);

                    if (isGzip(req->response)){
                        req->response.body() = compressGzip(rapidBuffer.GetString());
                    }else {
                        req->response.body() = rapidBuffer.GetString();
                    }
                    setContentLength(&req->response, strlen(rapidBuffer.GetString()));
                }
            }
        });

        proxyServer.run();
    } catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
