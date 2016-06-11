#ifndef GCRYPT_H
#define GCRYPT_H

namespace GCrypt {
    // -----------------------------------------------------------------

    char * aes_encrypt(char *str_in, char *key);

    // -----------------------------------------------------------------

    char * aes_decrypt(char *str_in, char *key);

    // -----------------------------------------------------------------

    char * rsa_encrypt(char *str_in, char *key);

    // -----------------------------------------------------------------

    char * rsa_decrypt(char *str_in, char *key);

    // -----------------------------------------------------------------

    char* base64_encode(char* string);
    
    // -----------------------------------------------------------------

    char* base64_decode(char *input);

    size_t mcrypt_decode(char ** str_out, const char * str_in, size_t str_in_sz, char * key);
    size_t mcrypt_encode(char ** str_out, const char * str_in, size_t str_in_sz, char * key);

}

#endif
