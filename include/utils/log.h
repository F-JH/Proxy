//
// Created by Xiaoandi Fu on 2024/4/8.
//

#ifndef PROXY_LOG_H
#define PROXY_LOG_H

#include <iostream>
#include <ctime>

inline static void log(const char* method, const char* url){
    std::time_t currentTime = std::time(nullptr);
    // 格式化当前时间为字符串
    const int bufferSize = 80;
    char buffer[bufferSize];
    std::strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));
    std::cout << buffer << " [" << method << "] " << url << std::endl;
}

inline static void log(const char* message){
    std::time_t currentTime = std::time(nullptr);
    // 格式化当前时间为字符串
    const int bufferSize = 80;
    char buffer[bufferSize];
    std::strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));

    std::cout << buffer << " " << message;
}

inline static std::string handleUrl(const std::string& domain, const std::string& port, const std::string& path){
    if (port == "443")
        return "https://" + domain + path;
    else
        return "https://" + domain + ":" + port + path;
}

#endif //PROXY_LOG_H
