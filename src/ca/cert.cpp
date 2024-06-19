//
// Created by Xiaoandi Fu on 2024/3/27.
//
#include "ca/cert.h"
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

    return cert_data;

err:
    std::cerr << "generate cert error" << std::endl;
    if (pkey)  EVP_PKEY_free(pkey);
    if (bio) BIO_free_all(bio);
    return "";
}

int export_root_certificate(char* password){
    OpenSSL_add_all_algorithms();

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx){
        std::cerr << "Error to createing EVP_PKEY_CTX" << std::endl;
        return 1;
    }

    // 初始化密钥生成参数
    if (EVP_PKEY_keygen_init(ctx) <= 0){
        std::cerr << "Error initializing key generation" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }
    // 设置密钥长度
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 4096) <= 0) {
        std::cerr << "Error setting key length" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }

    // 生成密钥对
    EVP_PKEY *pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        std::cerr << "Error generating key pair" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }


    // 创建 X509 证书
    X509 *x509 = X509_new();
    if (!x509) {
        std::cerr << "Error creating X509 object" << std::endl;
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }

    // 设置证书版本
    X509_set_version(x509, 1);

    // 证书序列号
    BIGNUM* serial_number = BN_new();
    BN_rand(serial_number, 64, -1, 0);
    ASN1_INTEGER* serial_asn1 = BN_to_ASN1_INTEGER(serial_number, NULL);

    // 设置证书序列号
    X509_set_serialNumber(x509, serial_asn1);
    ASN1_INTEGER_free(serial_asn1);

    // 设置证书签发者信息
    X509_NAME *issuer_name = X509_NAME_new();
    X509_NAME_add_entry_by_txt(issuer_name, "CN", MBSTRING_ASC, (unsigned char*)"ProxyServer CA", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "C", MBSTRING_ASC, (unsigned char*)"CN", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "ST", MBSTRING_ASC, (unsigned char*)"Guangdong", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "L", MBSTRING_ASC, (unsigned char*)"Guangzhou", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "O", MBSTRING_ASC, (unsigned char*)"HSF", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "OU", MBSTRING_ASC, (unsigned char*)"HSF Inc.", -1, -1, 0);
    X509_set_issuer_name(x509, issuer_name);
    X509_NAME_free(issuer_name);

    // 设置主题名称
    X509_NAME *subject_name = X509_NAME_new();
    X509_NAME_add_entry_by_txt(issuer_name, "CN", MBSTRING_ASC, (unsigned char*)"ProxyServer CA", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "C", MBSTRING_ASC, (unsigned char*)"CN", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "ST", MBSTRING_ASC, (unsigned char*)"Guangdong", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "L", MBSTRING_ASC, (unsigned char*)"Guangzhou", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "O", MBSTRING_ASC, (unsigned char*)"HSF", -1, -1, 0);
    X509_NAME_add_entry_by_txt(issuer_name, "OU", MBSTRING_ASC, (unsigned char*)"HSF Inc.", -1, -1, 0);
    X509_set_subject_name(x509, subject_name);
    X509_NAME_free(subject_name);

    // 设置证书有效期
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L); // 1 year validity

    // 设置证书的公钥
    if (!X509_set_pubkey(x509, pkey)) {
        std::cerr << "Error setting public key" << std::endl;
        X509_free(x509);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }

    // 将证书自签名
    if (!X509_sign(x509, pkey, EVP_sha256())) {
        fprintf(stderr, "Error signing certificate\n");
        X509_free(x509);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        return 1;
    }

    // 将证书写入文件
    BIO *bio_out = BIO_new_file("rootCa.crt", "w");
    if (!PEM_write_bio_X509(bio_out, x509)) {
        fprintf(stderr, "Error writing certificate to file\n");
        X509_free(x509);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        BIO_free_all(bio_out);
        return 1;
    }
    BIO *bio_pkey = BIO_new_file("rootCa.key", "w");
    if (!PEM_write_bio_PrivateKey(bio_pkey, pkey, nullptr, nullptr, 0, nullptr, nullptr)){
        fprintf(stderr, "Error writing privateKey to file\n");
        X509_free(x509);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(ctx);
        BIO_free_all(bio_out);
        return 1;
    }

    // 释放资源
    X509_free(x509);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    BIO_free_all(bio_out);
    BIO_free_all(bio_pkey);

    // 清理 OpenSSL 库
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    return 0;
}

std::string X509_to_string(X509* cert){
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio){
        return "Failed to create BIO";
    }
    if (PEM_write_bio_X509(bio, cert) != 1){
        return "Failed to write X509 to BIO";
    }

    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    if (!mem){
        return "Failed to get BIO memory pointer";
    }
    return {mem->data, mem->length};
}
