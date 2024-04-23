//
// Created by Xiaoandi Fu on 2024/3/27.
//
#include "ca/cert.h"
#include <sys/stat.h>
#include <openssl/err.h>

#define MAX_LEGTH 4096

std::map<std::string, X509*> templates;
std::map<const std::string, const std::string> pem_certs;

int password_callback(char* buf, int size, int rwflag, void* userdata) {
    const char *password = reinterpret_cast<const char*>(userdata);
    if (password != nullptr) {
        strncpy(buf, password, size);
        return static_cast<int>(strlen(buf));
    }
    return 0;
}

X509* load_crt(const char* crt_path, const char* password) {
    FILE* fp;
    fp = fopen(crt_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "fopen crt fail\n");
        return NULL;
    }
    X509* cert = PEM_read_X509(fp, NULL, password_callback, &password);
    if (cert == NULL) {
        fprintf(stderr, "pem read x509 fail\n");
        return NULL;
    }
    fclose(fp);

    return cert;
}

EVP_PKEY* load_key(const char* key_path, const char* password) {
    FILE* fp;
    fp = fopen(key_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "fopen key fail\n");
        return NULL;
    }
    EVP_PKEY* pkey = PEM_read_PrivateKey(fp, NULL, password_callback, &password);
    if (pkey == NULL) {
        fprintf(stderr, "read key fail\n");
        return NULL;
    }
    fclose(fp);

    return pkey;
}

X509* generate_cert_template(std::string domain) {
    // 构建证书模板
    X509* cert = X509_new();
    X509_NAME* subject_name = X509_NAME_new();
    X509_NAME_add_entry_by_txt(subject_name, "C", MBSTRING_ASC, (const unsigned char*)"CN", -1, -1, 0);
    X509_NAME_add_entry_by_txt(subject_name, "O", MBSTRING_ASC, (const unsigned char*)"https-mitm-proxy", -1, -1, 0);
    X509_NAME_add_entry_by_txt(subject_name, "OU", MBSTRING_ASC, (const unsigned char*)"https-mitm-proxy", -1, -1, 0);
    X509_NAME_add_entry_by_txt(subject_name, "L", MBSTRING_ASC, (const unsigned char*)"ShenZhen", -1, -1, 0);
    X509_NAME_add_entry_by_txt(subject_name, "CN", MBSTRING_ASC, (const unsigned char*)domain.c_str(), -1, -1, 0);

    X509_set_subject_name(cert, subject_name);
    X509_set_issuer_name(cert, subject_name);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 31536000L); // 有效期一年
    // X509_set_pubkey(cert, pkey);
    X509_add_ext(cert, X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, "non-critical,CA:TRUE"), -1);
    X509_add_ext(cert, X509V3_EXT_conf_nid(NULL, NULL, NID_ext_key_usage, "clientAuth,serverAuth"), -1);
    X509_add_ext(cert, X509V3_EXT_conf_nid(NULL, NULL, NID_key_usage, "digitalSignature,dataEncipherment"), -1);
    X509_add_ext(cert, X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, domain.c_str()), -1);

    // 释放资源
    X509_NAME_free(subject_name);
    templates[domain] = cert;

    return cert;
}

std::string get_domain_certificate_file(X509* root_cert, EVP_PKEY* root_key, const std::string& domain){
    std::string certificate_file = "certificate/sub_certificate/" + domain + ".pem";
    std::string subject_alt_name = "DNS:" + domain;
    std::string cert_data;
    auto item = pem_certs.find(domain);
    if (item != pem_certs.end()){
        return item->second;
    }

    X509* template_cert = NULL;
    BIO* bio = NULL;
    auto it = templates.find(domain);
    if (it != templates.end()) {
        template_cert = it->second;
    }else {
        template_cert = generate_cert_template(domain);
        templates[domain] = template_cert;
    }
    // 证书序列号
    BIGNUM* serial_number = BN_new();
    BN_rand(serial_number, 64, -1, 0);
    ASN1_INTEGER* serial_asn1 = BN_to_ASN1_INTEGER(serial_number, NULL);

    // 生成RSA密钥对
    RSA* rsa = RSA_new();
    BIGNUM* exponent = BN_new();
    BN_set_word(exponent, RSA_F4);
    RSA_generate_key_ex(rsa, 4096, exponent, NULL);
    EVP_PKEY* pkey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pkey, rsa);

    // 证书签发
    X509* cert = X509_new();
    X509_set_version(cert, 2);
    X509_set_serialNumber(cert, serial_asn1);
    X509_set_subject_name(cert, X509_get_subject_name(template_cert));

    // 添加SAN
    X509_add_ext(cert, X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, subject_alt_name.c_str()), -1);


    X509_set_issuer_name(cert, X509_get_subject_name(root_cert));
    X509_set_pubkey(cert, pkey);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 31536000L);
    X509_sign(cert, root_key, EVP_sha512());

    // 保存证书
    bio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(bio, cert);
    PEM_write_bio_PrivateKey(bio, pkey, NULL, NULL, 0, NULL, NULL);

    char bio_buf[4096];
    {
        int len;
        while ((len = BIO_read(bio, bio_buf, sizeof(bio_buf))) > 0){
            cert_data.append(bio_buf, len);
        }
        pem_certs.insert({domain, cert_data});
    }

    // 正常释放资源
    EVP_PKEY_free(pkey);
    BIO_free_all(bio);

//    return certificate_file;
    return cert_data;

err:
    std::cerr << "generate cert error" << std::endl;
    if (pkey)  EVP_PKEY_free(pkey);
    if (bio) BIO_free_all(bio);
    return "";
}
