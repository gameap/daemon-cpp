#include <stdio.h>
#include <iostream>

// #include <cryptopp/aes.h>
// #include <cryptopp/modes.h>
// #include <cryptopp/base64.h>

#include <openssl/aes.h>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <openssl/rsa.h>
#include <openssl/pem.h> 

// #include <mcrypt.h>

// #include "crypt.h"
#include "gstring.h"

#include "classes/crypto.h"
#include "classes/base64.h"

#include "functions/gcrypt.h"

// const BIO_METHOD *     BIO_f_base64(void);

/**
 * @deprecated
 */
namespace GCrypt {

    // Crypto crypto;

    // a simple hex-print routine. could be modified to print 16 bytes-per-line
    static void hex_print(const void* pv, size_t len)
    {
        const unsigned char * p = (const unsigned char*)pv;
        if (NULL == pv)
            printf("NULL");
        else
        {
            size_t i = 0;
            for (; i<len;++i)
                printf("%02X ", *p++);
        }
        printf("\n");
    }
        
    // -----------------------------------------------------------------
    /*
    char* aes_encrypt(char *str_in, char *key)
    {
        std::string str_out;
        
        unsigned char *encMsg = NULL;
        int encMsgLen;

        crypto.setAESKey((unsigned char *)&key[0], 32);
        crypto.setAESIv((unsigned char *)"1234567890123456", 16);
        
        if((encMsgLen = crypto.aesEncrypt((const unsigned char*)&str_in[0], strlen(str_in)+1, &encMsg)) == -1) {
            fprintf(stderr, "Encryption failed\n");
        }

        
        return (char*)encMsg;
    }

    // -----------------------------------------------------------------

    char* aes_decrypt(char *str_in, char *key)
    {
        char *decMsg          = NULL;
        int decMsgLen;

        crypto.setAESKey((unsigned char *)&key[0], 32);
        crypto.setAESIv((unsigned char *)"1234567890123456", 16);

        if((decMsgLen = crypto.aesDecrypt((unsigned char*)&str_in[0], strlen(str_in), (unsigned char**)&decMsg)) == -1) {
            fprintf(stderr, "Decryption failed\n");
        }

        return (char *)decMsg;
    }
    */
    
    // -----------------------------------------------------------------

    int rsa_priv_encrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file, char * keypass)
    {
        RSA * priv_key = NULL;
        FILE * priv_key_file;
        
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        ERR_load_crypto_strings();

        priv_key_file = fopen(key_file, "rb");

        if (priv_key_file == NULL) {
            std::cerr << "Keyfile read failed" << std::endl;
            return -1;
        }

        priv_key = PEM_read_RSAPrivateKey(priv_key_file, NULL, NULL, keypass);
        fclose(priv_key_file);

        if (!priv_key) {
            std::cerr << "priv_key error: " << std::endl;
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }

        int key_size = RSA_size(priv_key);
        unsigned char *ustr_out = (unsigned char *)malloc(key_size);
        
        int len = RSA_private_encrypt(str_in_sz, (unsigned char *)&str_in[0], ustr_out, priv_key, RSA_PKCS1_PADDING);

        if (len == -1) {
            std::cerr << "RSA_private_encrypt error (rsa_priv_encrypt)." << std::endl;
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }
        
        *str_out = (char *)ustr_out;
        return len;
    }

    // -----------------------------------------------------------------

    int rsa_pub_decrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file)
    {
        RSA * pub_key = NULL;
        FILE * pub_key_file;
        
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        ERR_load_crypto_strings();
        
        pub_key_file = fopen(key_file, "rb");
        pub_key = PEM_read_RSA_PUBKEY(pub_key_file, NULL, NULL, NULL);
        fclose(pub_key_file);

        if (!pub_key) {
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }

        int key_size = RSA_size(pub_key);

        if (key_size == 0) {
            std::cerr << "Key size error" << std::endl;
            return -1;
        }
        
        unsigned char *ustr_out = (unsigned char *)malloc(key_size);
        int len = RSA_public_decrypt(str_in_sz, (unsigned char *)&str_in[0], ustr_out, pub_key, RSA_PKCS1_PADDING);
        
        if (len == -1) {
            std::cerr << "RSA_public_decrypt error (rsa_pub_decrypt)." << std::endl;
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }
        
        *str_out = (char *)ustr_out;
        return len;
    }
    
    // -----------------------------------------------------------------

    int rsa_pub_encrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file)
    {
        RSA * pub_key = NULL;
        FILE * pub_key_file;
        
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        ERR_load_crypto_strings();
        
        pub_key_file = fopen(key_file, "rb");

        if (pub_key_file == NULL) {
            std::cerr << "Keyfile read failed" << std::endl;
            return -1;
        }

        pub_key = PEM_read_RSA_PUBKEY(pub_key_file, NULL, NULL, NULL);
        fclose(pub_key_file);

        if (!pub_key) {
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }

        int key_size = RSA_size(pub_key);
        unsigned char *ustr_out = (unsigned char *)malloc(key_size);

        // std::cout << "key_size: " << key_size << std::endl;
        // std::cout << "str_in_sz: " << str_in_sz << std::endl;
        
        int len = RSA_public_encrypt(str_in_sz, (unsigned char *)&str_in[0], ustr_out, pub_key, RSA_PKCS1_PADDING);

        if (len == -1) {
            std::cerr << "RSA_public_encrypt error (rsa_pub_encrypt)." << std::endl;
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }
        
        *str_out = (char *)ustr_out;
        return len;
    }

    // -----------------------------------------------------------------

    int rsa_priv_decrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file, char * keypass)
    {
        RSA * priv_key = NULL;
        FILE * priv_key_file;
        
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        ERR_load_crypto_strings();
        
        priv_key_file = fopen(key_file, "rb");
        priv_key = PEM_read_RSAPrivateKey(priv_key_file, NULL, NULL, keypass);
        fclose(priv_key_file);

        if (!priv_key) {
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }

        int key_size = RSA_size(priv_key);
        unsigned char *ustr_out = (unsigned char *)malloc(key_size);
        
        int len = RSA_private_decrypt(str_in_sz, (unsigned char *)&str_in[0], ustr_out, priv_key, RSA_PKCS1_PADDING);
        
        if (len == -1) {
            std::cerr << "RSA_private_decrypt error (rsa_priv_decrypt)." << std::endl;
            std::cerr << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return -1;
        }
        
        *str_out = (char *)ustr_out;
        return len;
    }

    // -----------------------------------------------------------------
	/*
    int mcrypt_decode(char ** str_out, const char * str_in, size_t str_in_sz, char * key, char * iv)
    {
        MCRYPT td;
        int keysize = 32; // 256 bits

        td = mcrypt_module_open("rijndael-256", NULL, "cbc", NULL);

        if (td == MCRYPT_FAILED) {
            std::cerr << "Mcrypt load failed" << std::endl;
            return -1;
        }
        
        if (mcrypt_generic_init(td, key, keysize, iv) < 0) {
            std::cerr << "Error mcrypt_generic_init!" << std::endl;
            return -1;
        }

        size_t siz;
        siz = (str_in_sz / mcrypt_enc_get_block_size(td)) * mcrypt_enc_get_block_size(td);

        char buf[siz*8];

        memcpy(buf, &str_in[0], str_in_sz);
        
        if (mdecrypt_generic(td, &buf, siz) < 0) {
            std::cerr << "mdecrypt_generic error: " << std::endl;
            return -1;
        }

        *str_out = buf;

        return siz;
    }

    // -----------------------------------------------------------------
    
    int mcrypt_encode(char ** str_out, const char * str_in, size_t str_in_sz, char * key, char * iv)
    {
        MCRYPT td;
        int keysize = 32; // 256 bits

        td = mcrypt_module_open("rijndael-256", NULL, "cbc", NULL);

        if (td == MCRYPT_FAILED) {
            std::cerr << "Mcrypt load failed" << std::endl;
            return -1;
        }

        if (mcrypt_generic_init(td, key, keysize, iv) < 0) {
            std::cerr << "Error mcrypt_generic_init!" << std::endl;
            return -1;
        }

        size_t siz;
        siz = (str_in_sz / mcrypt_enc_get_block_size(td)) * mcrypt_enc_get_block_size(td);

        char buf[siz*8];
        strcpy(buf, str_in);
        
        if (mcrypt_generic(td, &buf, siz) < 0) {
            std::cerr << "mcrypt_generic error: " << std::endl;
            return -1;
        }

        *str_out = buf;

        return siz;
    }
	*/
    // -----------------------------------------------------------------
    
    /*
    char* base64_encode(char* string)
    {
        return base64Encode((const unsigned char *)string, strlen(string));
    }

    // -----------------------------------------------------------------

    char* base64_decode(char *input)
    {
        unsigned char *buffer;
        base64Decode((const char *)input,
            strlen(input),
            &buffer);
        
        return (char *)buffer;
    }
    */
}
