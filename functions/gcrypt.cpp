#include <stdio.h>
#include <iostream>

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/base64.h>

#include <openssl/aes.h>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include "crypt.h"

#include "classes/crypto.h"
#include "classes/base64.h"

// const BIO_METHOD *     BIO_f_base64(void);

namespace GCrypt {

    Crypto crypto;
    
    // -----------------------------------------------------------------

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

    // -----------------------------------------------------------------

    char* rsa_encrypt(char *str_in, char *key)
    {

    }

    // -----------------------------------------------------------------

    char* rsa_decrypt(char *str_in, char *key)
    {
        
    }

    // -----------------------------------------------------------------

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
}
