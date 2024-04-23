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


class MockManager {
    friend class HttpSession;
    friend class HttpsSession;
public:
    void addMock(MockInterface mock);
    void onRequest(OnRequest func);
    void onResponse(OnResponse func);
private:
    void mockRequest(REQ* req);
    void mockResponse(REQ* req);

    std::vector<const MockInterface> mocks_;
    std::vector<const OnRequest> onRequestFunc_;
    std::vector<const OnResponse> onResponseFunc_;
};

#endif //PROXY_MOCK_H
