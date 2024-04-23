//
// Created by Xiaoandi Fu on 2024/4/3.
//
#include "mock/mock.h"

void MockInterface::onRequest(REQ* req) {}
void MockInterface::onResponse(REQ* req) {}


void MockManager::addMock(MockInterface mock) {
    mocks_.push_back(mock);
}

void MockManager::onRequest(OnRequest func) {
    onRequestFunc_.push_back(func);
}

void MockManager::onResponse(OnResponse func) {
    onResponseFunc_.push_back(func);
}

void MockManager::mockRequest(REQ* req) {
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

void MockManager::mockResponse(REQ* req) {
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