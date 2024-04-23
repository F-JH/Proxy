cmake_minimum_required(VERSION 3.23)
project(Proxy)

set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE native_srcs ./src/*.cpp)
add_executable(Proxy ${native_srcs})
target_include_directories(
        Proxy
        PUBLIC
        include
        /usr/local/include
        /Users/xiaoandi/github/rapidjson/include
)

target_link_directories(
        Proxy
        PUBLIC
        /usr/local/Cellar/openssl@3/3.2.1/lib
)

target_link_libraries(
        Proxy
        crypto.3
        ssl
        z
)

#Test
file(GLOB_RECURSE test_srcs ./test/*.cpp)
add_executable(ClientTest test/BeastTest.cpp)
add_executable(ServiceTest src/ca/cert.cpp test/AsioServiceTest.cpp)
#client
target_include_directories(
        ClientTest
        PUBLIC
        include
        /usr/local/include
)

target_link_directories(
        ClientTest
        PUBLIC
        /usr/local/Cellar/openssl@3/3.2.1/lib
        /usr/local/Cellar/boost/1.83.0/lib
)

target_link_libraries(
        ClientTest
        crypto.3
        ssl
        boost_system
        boost_thread-mt
        pthread
)

#serverTest
target_include_directories(
        ServiceTest
        PUBLIC
        include
        /usr/local/include
)

target_link_directories(
        ServiceTest
        PUBLIC
        /usr/local/Cellar/openssl@3/3.2.1/lib
        /usr/local/Cellar/boost/1.83.0/lib
)

target_link_libraries(
        ServiceTest
        crypto.3
        ssl
        boost_system
        boost_thread-mt
        pthread
)
