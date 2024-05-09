//
// Created by Xiaoandi Fu on 2024/4/3.
//
#ifndef PROXY_MOCK_H
#define PROXY_MOCK_H

#include <string>
#include <vector>
#include <iostream>

#include "service/httpdef.h"

class HttpSession;
class HttpsSession;

// mock接口
class MockInterface {
public:
    virtual void onRequest(REQ* req);
    virtual void onResponse(REQ* req);
};

template<typename onrequest, typename onresponse>
class MockManager {
    friend class HttpSession;
    friend class HttpsSession;
public:
    void addMock(MockInterface mock){
        mocks_.push_back(mock);
    }
    void onRequest(onrequest func){
        onRequestFunc_.push_back(func);
    }
    void onResponse(onresponse func){
        onResponseFunc_.push_back(func);
    }
private:
    void mockRequest(REQ* req){
        for (auto mock : mocks_){
            try{
                mock.onRequest(req);
            }catch (std::exception& e){
                std::cerr << e.what() << std::endl;
            }
        }

        for (auto func : onRequestFunc_){
            try{
                func(&(req->request));
            }catch (std::exception& e){
                std::cerr << e.what() << std::endl;
            }
        }
    }
    void mockResponse(REQ* req){
        for (auto mock : mocks_){
            try{
                mock.onResponse(req);
            }catch (std::exception& e){
                std::cerr << e.what() << std::endl;
            }
        }

        for (auto func : onResponseFunc_){
            try{
                func(req);
            }catch (std::exception& e){
                std::cerr << e.what() << std::endl;
            }
        }
    }

    std::vector<const MockInterface> mocks_;
    std::vector<onrequest> onRequestFunc_;
    std::vector<onresponse> onResponseFunc_;
};

#endif //PROXY_MOCK_H
