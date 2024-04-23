//
// Created by Xiaoandi Fu on 2024/4/8.
//

#ifndef PROXY_LOG_H
#define PROXY_LOG_H

#include <iostream>

inline static void log(const char* method, const char* url){
    std::cout << "[" << method << "]" << " " << url << std::endl;
}

inline static std::string handleUrl(const std::string& domain, const std::string& port, const std::string& path){
    if (port == "443")
        return "https://" + domain + path;
    else
        return "https://" + domain + ":" + port + path;
}

#endif //PROXY_LOG_H
