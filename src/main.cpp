#include <thread>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "ca/cert.h"
#include "utils/gzip.h"
#include "service/httpdef.h"
#include "service/ProxyServer.h"
#define PORT 8999

X509* root_cert;
EVP_PKEY* root_key;

void init() {
    root_cert = load_crt("certificate/rootCa.crt", "Horizon972583048");
    root_key = load_key("certificate/rootCa.key", "Horizon972583048");
}

void worker(boost::asio::io_context &context, const char* name){
    context.run();
    while (context.stopped()){
        context.restart();
        context.run();
    }
}

int main() {
    OpenSSL_add_all_algorithms();
    init();

    try {
        boost::asio::io_context clientContext;
        boost::asio::io_context serverContext;

        std::cout << "proxy servere run at " << PORT << std::endl;
        ProxyServer proxyServer(clientContext, clientContext, PORT, root_cert, root_key);

        // 添加mock操作，这里是onResponse
        proxyServer.add_mock([](REQ* req){
            if (req->request.target().find("/admin/api/dataintegrate/common/forwardDaasApi") != req->request.target().npos){
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
                    std::string bodyString;
                    if (isGzip(req->response)){
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
                    req->response.body() = compressGzip(rapidBuffer.GetString());
                }
            }
        });

        std::thread client([&clientContext](){ worker(clientContext, "client");});
        client.join();
    } catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
