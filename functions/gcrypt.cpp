#include <stdio.h>
#include <iostream>

// #include <cryptopp/aes.h>
// #include <cryptopp/modes.h>
// #include <cryptopp/base64.h>

#include <openssl/conf.h>
#include <openssl/aes.h>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <openssl/err.h>

// #include <mcrypt.h>

// #include "crypt.h"
#include "gstring.h"

#include "classes/crypto.h"
#include "classes/base64.h"

#include "functions/gcrypt.h"

// const BIO_METHOD *     BIO_f_base64(void);

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

    void handleErrors()
    {

    }

    int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                unsigned char *iv, unsigned char *ciphertext)
    {
        EVP_CIPHER_CTX *ctx;

        int len;

        int ciphertext_len;

        /* Create and initialise the context */
        if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

        /* Initialise the encryption operation. IMPORTANT - ensure you use a key
         * and IV size appropriate for your cipher
         * In this example we are using 256 bit AES (i.e. a 256 bit key). The
         * IV size for *most* modes is the same as the block size. For AES this
         * is 128 bits */
        if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
            handleErrors();

        /* Provide the message to be encrypted, and obtain the encrypted output.
         * EVP_EncryptUpdate can be called multiple times if necessary
         */
        if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
            handleErrors();

        ciphertext_len = len;

        /* Finalise the encryption. Further ciphertext bytes may be written at
         * this stage.
         */
        if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
        ciphertext_len += len;

        /* Clean up */
        EVP_CIPHER_CTX_free(ctx);

        return ciphertext_len;
    }

    // -----------------------------------------------------------------

    int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                unsigned char *iv, unsigned char *plaintext)
    {
        EVP_CIPHER_CTX *ctx;

        int len;

        int plaintext_len;

        /* Create and initialise the context */
        if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

        /* Initialise the decryption operation. IMPORTANT - ensure you use a key
         * and IV size appropriate for your cipher
         * In this example we are using 256 bit AES (i.e. a 256 bit key). The
         * IV size for *most* modes is the same as the block size. For AES this
         * is 128 bits */
        if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
            handleErrors();

        /* Provide the message to be decrypted, and obtain the plaintext output.
         * EVP_DecryptUpdate can be called multiple times if necessary
         */
        if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
            handleErrors();
        plaintext_len = len;

        /* Finalise the decryption. Further plaintext bytes may be written at
         * this stage.
         */
        if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
        plaintext_len += len;

        /* Clean up */
        EVP_CIPHER_CTX_free(ctx);

        return plaintext_len;
    }

    // -----------------------------------------------------------------

    int aes_encrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key, char *iv)
    {
        /* A 256 bit key */
        unsigned char *ukey = (unsigned char *)key;

        /* A 128 bit IV */
        unsigned char *uiv = (unsigned char *)iv;

        unsigned char ciphertext[128];

        int ciphertext_len;

        /* Initialise the library */
        ERR_load_crypto_strings();
        OpenSSL_add_all_algorithms();
        OPENSSL_config(NULL);

        ciphertext_len = encrypt((unsigned char *)str_in, str_in_sz, ukey, uiv, ciphertext);

        memcpy(*str_out, ciphertext, ciphertext_len);

        /* Clean up */
        EVP_cleanup();
        ERR_free_strings();

        return ciphertext_len;
    }

    // -----------------------------------------------------------------

    int aes_decrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key, char *iv)
    {
        /* A 256 bit key */
        unsigned char *ukey = (unsigned char *)key;

        /* A 128 bit IV */
        unsigned char *uiv = (unsigned char *)iv;

        /* Buffer for the decrypted text */
        unsigned char decryptedtext[128];

        int decryptedtext_len;

        /* Decrypt the ciphertext */
        decryptedtext_len = decrypt((unsigned char *)str_in, str_in_sz, ukey, uiv, decryptedtext);

        /* Add a NULL terminator. We are expecting printable text */
        decryptedtext[decryptedtext_len] = '\0';

        memcpy(*str_out, decryptedtext, decryptedtext_len);

        /* Clean up */
        EVP_cleanup();
        ERR_free_strings();

        return decryptedtext_len;
    }
    
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
