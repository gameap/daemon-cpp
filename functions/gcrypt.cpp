#include <stdio.h>
#include <iostream>

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/base64.h>

#include <openssl/aes.h>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <mcrypt.h>

#include "crypt.h"
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

    char* rsa_encrypt(char *str_in, char *key)
    {

    }

    // -----------------------------------------------------------------

    char* rsa_decrypt(char *str_in, char *key)
    {
        
    }

    // -----------------------------------------------------------------

    size_t mcrypt_decode(char ** str_out, const char * str_in, size_t str_in_sz, char * key)
    {
        MCRYPT td;
        int keysize = 32; /* 256 bits */

        td = mcrypt_module_open("rijndael-256", NULL, "cbc", NULL);

        if (td == MCRYPT_FAILED) {
            std::cerr << "Mcrypt load failed" << std::endl;
            return -1;
        }

        // IV = (char *)malloc(mcrypt_enc_get_iv_size(td));
        std::string IV(mcrypt_enc_get_iv_size(td), 0);
        // IV = random(mcrypt_enc_get_iv_size(td));
        IV = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

        if (mcrypt_generic_init(td, key, keysize,&IV[0]) < 0) {
            std::cerr << "Error mcrypt_generic_init!" << std::endl;
            return -1;
        }

        size_t siz;
        siz = (str_in_sz / mcrypt_enc_get_block_size(td)) * mcrypt_enc_get_block_size(td);

        char buf[siz*8];

        memcpy(buf, &str_in[0], str_in_sz);
        std::cout << "buf: " << std::endl;
        hex_print(buf, str_in_sz);

        if (mdecrypt_generic(td, &buf, siz) < 0) {
            std::cerr << "mdecrypt_generic error: " << std::endl;
            return -1;
        }

        *str_out = buf;

        // std::cout << "Decoded: " << std::endl;
        // hex_print(buf, str_in_sz);
        
        return siz;
    }

    // -----------------------------------------------------------------
    
    size_t mcrypt_encode(char ** str_out, const char * str_in, size_t str_in_sz, char * key)
    {
        MCRYPT td;
        int keysize = 32; /* 256 bits */

        td = mcrypt_module_open("rijndael-256", NULL, "cbc", NULL);

        if (td == MCRYPT_FAILED) {
            std::cerr << "Mcrypt load failed" << std::endl;
            return -1;
        }

        // IV = (char *)malloc(mcrypt_enc_get_iv_size(td));
        std::string IV(mcrypt_enc_get_iv_size(td), 0);
        // IV = random(mcrypt_enc_get_iv_size(td));
        IV = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

        if (mcrypt_generic_init(td, key, keysize,&IV[0]) < 0) {
            std::cerr << "Error mcrypt_generic_init!" << std::endl;
            return -1;
        }

        size_t siz;
        siz = (str_in_sz / mcrypt_enc_get_block_size(td)) * mcrypt_enc_get_block_size(td);

        char buf[siz*8];
        strcpy(buf, str_in);
        // memcpy(buf, &str_in[0], str_in_sz);
        std::cout << "buf: " << std::endl;
        hex_print(buf, str_in_sz);
        
        if (mcrypt_generic(td, &buf, siz) < 0) {
            std::cerr << "mcrypt_generic error: " << std::endl;
            return -1;
        }

        *str_out = buf;

        // std::cout << "Encoded: " << std::endl;
        // hex_print(str_out, siz);

        return siz;
    }

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
