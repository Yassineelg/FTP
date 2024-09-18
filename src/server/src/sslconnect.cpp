#include "sslconnect.hpp"

SslConnect::SslConnect() : ctx(nullptr) {}

SslConnect::~SslConnect() {
    cleanup();
}

bool SslConnect::initialize() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

static int passphrase_callback(char* buf, int size, int rwflag, void* userdata) {
    const char* passphrase = static_cast<const char*>(userdata);
    size_t len = strlen(passphrase);
    if (len > static_cast<size_t>(size)) {
        len = size;
    }
    memcpy(buf, passphrase, len);
    return len;
}

bool SslConnect::configure(const char* cert_file, const char* key_file) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match the public certificate\n");
        return false;
    }
    return true;
}

SSL_CTX *SslConnect::getContext() {
    return ctx;
}

SSL* SslConnect::acceptConnection(int client_socket) {
    std::cout << "Creating a new SSL object...\n";
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        std::cerr << "Unable to create SSL object\n";
        return nullptr;
    }

    SSL_set_fd(ssl, client_socket);

    int ssl_accept_result = SSL_accept(ssl);

    if (ssl_accept_result <= 0) {
        int ssl_error = SSL_get_error(ssl, ssl_accept_result);
        std::cerr << "SSL_accept failed with error code: " << ssl_error << "\n";
        SSL_free(ssl);
        return nullptr;
    }

    return ssl;
}

int SslConnect::sslSend(SSL* ssl, const char* message) {
    return SSL_write(ssl, message, strlen(message));
}

int SslConnect::sslReceive(SSL* ssl, char* buffer, int size) {
    return SSL_read(ssl, buffer, size);
}

void SslConnect::closeConnection(SSL* ssl) {
    SSL_shutdown(ssl);
    close(SSL_get_fd(ssl));
    SSL_free(ssl);
}

void SslConnect::cleanup() {
    if (ctx) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
    EVP_cleanup();
}
