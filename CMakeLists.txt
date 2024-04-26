cmake_minimum_required(VERSION 3.23)
project(Proxy)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB_RECURSE native_srcs ./src/*.cpp)

# 编译成库
add_library(ProxyServer ${native_srcs})
target_include_directories(
        ProxyServer
        PRIVATE
        include
        /usr/local/include
        /Users/xiaoandi/github/rapidjson/include
)

target_link_directories(
        ProxyServer
        PRIVATE
        /usr/local/Cellar/openssl@3/3.2.1/lib
        /usr/local/Cellar/boost/1.84.0_1/lib
)

target_link_libraries(
        ProxyServer
        PRIVATE
        crypto
        ssl
        z
)

# main
add_executable(Proxy example/main.cpp)
target_include_directories(
        Proxy
        PUBLIC
        include
        /usr/local/include
        /Users/xiaoandi/github/rapidjson/include
)
target_link_directories(
        Proxy
        PRIVATE
        /usr/local/Cellar/openssl@3/3.2.1/lib
        /usr/local/Cellar/boost/1.84.0_1/lib
)
target_link_libraries(
        Proxy
        PUBLIC
        ProxyServer
)
