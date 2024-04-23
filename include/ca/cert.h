//
// Created by Xiaoandi Fu on 2024/3/27.
//

#ifndef CERT_H
#define CERT_H

#include <map>
#include <cerrno>
#include <iostream>
#include <string>
#include <vector>

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#define MAX_LEGTH 4096

int password_callback(char* buf, int size, int rwflag, void* userdata);
X509* load_crt(const char* crt_path, const char* password);

EVP_PKEY* load_key(const char* key_path, const char* password);

X509* generate_cert_template(std::string domain);

std::pair<std::vector<unsigned char>, std::vector<unsigned char>> create_fake_certificate_by_domain(X509* root_cert, EVP_PKEY* root_key, const std::string& domain);

std::string get_domain_certificate_file(X509* root_cert, EVP_PKEY* root_key, const std::string& domain);

#endif //CERT_H

