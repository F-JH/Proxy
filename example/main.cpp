#include <fstream>
#include <iostream>
#include "ProxyServer.h"
#include "gzip.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

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

int main() {

    try {
        std::cout << "proxy servere run at " << PORT << std::endl;
        // 添加mock操作，这里是onResponse

//        std::thread update(updateJson);
//                    auto allocator = document.GetAllocator();
//                    for (auto it = data.Begin(); it != data.End(); it++){
//                        if (it->IsObject()){
//                            rapidjson::Value& obj = *it;
//                            obj["payOrderTradeAmt"] = n;
//                            std::string name = "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong" + std::to_string(n);
//                            obj["sourceReferrerName"].SetString(name.c_str(), name.length(), allocator);
//
//                            n += 100;
//                        }
//                    }


        ProxyServer proxyServer(PORT, "certificate/rootCa.crt", "certificate/rootCa.key", "Horizon972583048");


        proxyServer.run();
    } catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
