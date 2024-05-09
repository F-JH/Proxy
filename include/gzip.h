//
// Created by Xiaoandi Fu on 2024/4/22.
//

#ifndef PROXY_GZIP_H
#define PROXY_GZIP_H

#include <iostream>
#include <string>
#include <zlib.h>

#include "ProxyServer.h"

#define CHUNK 1024

inline static void test(){
    std::cout << "hello world" << std::endl;
}

inline static bool isGzip(boost::beast::http::request<boost::beast::http::string_body>& request){
    for (auto const& field : request.base()){
        if (field.name_string() == "Content-Encoding" && field.value() == "gzip"){
            return true;
        }
    }

    return false;
}

inline static bool isGzip(boost::beast::http::response<boost::beast::http::string_body>& response){
    for (auto const& field : response.base()){
        if (field.name_string() == "Content-Encoding" && field.value() == "gzip"){
            return true;
        }
    }

    return false;
}

inline static std::string unCompressGzip(const std::string& compressedData){

    z_stream stream;

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = compressedData.size();
    stream.next_in = (Bytef *) compressedData.data();

    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) return "";

    std::string unCompressedData;
    char buffer[CHUNK];
    do {
        stream.avail_out = CHUNK;
        stream.next_out = (Bytef *)buffer;

        int ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR){
            inflateEnd(&stream);
            return "";
        }

        unCompressedData.append(buffer, CHUNK - stream.avail_out);
    }while (stream.avail_out == 0);
    inflateEnd(&stream);

    return unCompressedData;
}

inline static std::string compressGzip(const std::string& src){
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK){
        return "";
    }

    stream.next_in = (Bytef *) src.c_str();
    stream.avail_in = src.size();

    std::string dest;
    char buffer[CHUNK];

    do{
        stream.next_out = (Bytef *)buffer;
        stream.avail_out = CHUNK;
        //压缩
        if (deflate(&stream, Z_FINISH) == Z_STREAM_ERROR){
            deflateEnd(&stream);
            return "";
        }

        dest.append(buffer, CHUNK - stream.avail_out);
    }while (stream.avail_out == 0);
    deflateEnd(&stream);

    return dest;
}

#endif //PROXY_GZIP_H
