#ifndef GCRYPT_H
#define GCRYPT_H

namespace GCrypt {
    
    // -----------------------------------------------------------------

    char * aes_encrypt(char *str_in, char *key);

    // -----------------------------------------------------------------

    char * aes_decrypt(char *str_in, char *key);

    // -----------------------------------------------------------------

    int rsa_priv_encrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file, char * keypass);

    // -----------------------------------------------------------------

    int rsa_pub_decrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file);
    
    // -----------------------------------------------------------------

    int rsa_pub_encrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file);

    // -----------------------------------------------------------------

    int rsa_priv_decrypt(char ** str_out, char *str_in, size_t str_in_sz, char *key_file, char * keypass);

    // -----------------------------------------------------------------

    char* base64_encode(char* string);
    
    // -----------------------------------------------------------------

    char* base64_decode(char *input);

    // -----------------------------------------------------------------

    int mcrypt_decode(char ** str_out, const char * str_in, size_t str_in_sz, char * key, char * iv);

    // -----------------------------------------------------------------
    
    int mcrypt_encode(char ** str_out, const char * str_in, size_t str_in_sz, char * key, char * iv);

    // -----------------------------------------------------------------

}

#endif
