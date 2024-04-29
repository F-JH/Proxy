#include <iostream>
#include "outheader/ProxyServer.h"
#include "outheader/gzip.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#define PORT 8999

int main() {

    try {
        std::cout << "proxy servere run at " << PORT << std::endl;
        ProxyServer proxyServer(PORT, "certificate/rootCa.crt", "certificate/rootCa.key", "Horizon972583048");

        // 添加mock操作，这里是onResponse
        proxyServer.add_mock([](REQ* req){
            if (req->request.target().find("/admin/forwardDaasApi") != req->request.target().npos){
                std::string requestBody;
                if (isGzip(req->request)){
                    requestBody = unCompressGzip(req->request.body());
                }else {
                    requestBody = req->request.body();
                }

                rapidjson::Document request;
                request.Parse(requestBody.c_str());
                rapidjson::Value& col = request["params"]["columnList"][0];
                if (std::strcmp(col.GetString(), "payOrderTradeAmt") == 0){
                    std::cerr << "mock payOrderTradeAmt" << std::endl;
                    std::string bodyString;
                    bool hasGzip = isGzip(req->response);
                    if (hasGzip){
                        // 对body进行gzip解压
                        bodyString = unCompressGzip(req->response.body());
                    }else {
                        bodyString = req->response.body();
                    }
                    rapidjson::Document document;
                    document.Parse(bodyString.c_str());
                    rapidjson::Value& data = document["data"]["data"];

                    int n = 100;
                    auto allocator = document.GetAllocator();
                    for (auto it = data.Begin(); it != data.End(); it++){
                        if (it->IsObject()){
                            rapidjson::Value& obj = *it;
                            obj["payOrderTradeAmt"] = n;
                            std::string name = "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong" + std::to_string(n);
                            obj["sourceReferrerName"].SetString(name.c_str(), name.length(), allocator);

                            n += 100;
                        }
                    }

                    rapidjson::StringBuffer rapidBuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(rapidBuffer);
                    document.Accept(writer);

                    if (hasGzip){
                        // 将body重新用gzip压缩
                        req->response.body() = compressGzip(rapidBuffer.GetString());
                    }else{
                        req->response.body() = rapidBuffer.GetString();
                    }
                }
            }
        });

        proxyServer.run();
    } catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
