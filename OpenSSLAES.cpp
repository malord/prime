// Copyright 2000-2021 Mark H. P. Lord

#include "OpenSSLAES.h"

#ifndef PRIME_NO_OPENSSL

#include <openssl/evp.h>

namespace Prime {

//
// OpenSSLKey
//

OpenSSLKey::OpenSSLKey()
    : _keyLength(0)
    , _ivLength(0)
{
}

OpenSSLKey::~OpenSSLKey()
{
}

bool OpenSSLKey::createKey(StringView passphrase, Log* log, const char* cipherName, const char* digestName,
    const char* salt)
{
    PRIME_COMPILE_TIME_ASSERT(maxKeyLength >= EVP_MAX_KEY_LENGTH);
    PRIME_COMPILE_TIME_ASSERT(maxIVLength >= EVP_MAX_IV_LENGTH);

    const EVP_CIPHER* cipher = EVP_get_cipherbyname(cipherName);
    if (!cipher) {
        log->error(PRIME_LOCALISE("Missing %s cipher."), cipherName);
        return false;
    }

    const EVP_MD* digest = EVP_get_digestbyname(digestName);
    if (!digest) {
        log->error(PRIME_LOCALISE("Missing %s digest."), digestName);
        return false;
    }

    if (!EVP_BytesToKey(cipher, digest, (const unsigned char*)salt,
            (const unsigned char*)passphrase.data(), Narrow<int>(passphrase.size()),
            1, _key, _iv)) {
        log->error(PRIME_LOCALISE("Failed to create key from passphrase."));
        return false;
    }

    _keyLength = EVP_CIPHER_key_length(cipher); //->key_len;
    _ivLength = EVP_CIPHER_iv_length(cipher); //->iv_len;
    return true;
}

//
// OpenSSLAES256
//

bool OpenSSLAES256::createKey(OpenSSLKey& key, StringView passphrase, Log* log, const char* digestName,
    const char* salt)
{
    return key.createKey(passphrase, log, "aes-256-cbc", digestName, salt);
}

Optional<std::string> OpenSSLAES256::encryptWithMD5Key(StringView passphrase, StringView plaintext, Log* log)
{
    OpenSSLKey key;
    if (!createKey(key, passphrase, log)) {
        return nullopt;
    }

    return OpenSSLAES256().encrypt(plaintext, key, log);
}

Optional<std::string> OpenSSLAES256::decryptWithMD5Key(StringView passphrase, StringView ciphertext, Log* log)
{
    OpenSSLKey key;
    if (!createKey(key, passphrase, log)) {
        return nullopt;
    }

    return OpenSSLAES256().decrypt(ciphertext, key, log);
}

OpenSSLAES256::OpenSSLAES256()
{
}

OpenSSLAES256::~OpenSSLAES256()
{
}

bool OpenSSLAES256::encrypt(std::string& ciphertext, StringView plaintext, const OpenSSLKey& key, Log* log) const
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        log->error(PRIME_LOCALISE("Couldn't create cipher context."));
        return false;
    }

    const EVP_CIPHER* cipher = EVP_aes_256_cbc();

    if (key.getKeyLength() != EVP_CIPHER_key_length(cipher) || key.getIVLength() != EVP_CIPHER_iv_length(cipher)) {
        log->error(PRIME_LOCALISE("Incorrect key size."));
        return false;
    }

    ciphertext.resize((Narrow<int>(plaintext.size()) / EVP_CIPHER_block_size(cipher) + 1) * EVP_CIPHER_block_size(cipher));

    if (EVP_EncryptInit_ex(ctx, cipher, NULL, key.getKey(), key.getIV()) != 1) {
        log->error(PRIME_LOCALISE("Couldn't initialise encryption engine."));
        return false;
    }

    int len = Narrow<int>(plaintext.size());
    if (EVP_EncryptUpdate(ctx, (unsigned char*)&ciphertext[0], &len,
            (const unsigned char*)plaintext.data(), Narrow<int>(plaintext.size()))
        != 1) {
        log->error(PRIME_LOCALISE("Couldn't encrypt."));
        return false;
    }

    int len2 = 0;
    if (EVP_EncryptFinal_ex(ctx, (unsigned char*)&ciphertext[len], &len2) != 1) {
        log->error(PRIME_LOCALISE("Couldn't finalise encryption."));
        return false;
    }
    len += len2;

    ciphertext.resize(len);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool OpenSSLAES256::decrypt(std::string& plaintext, StringView ciphertext, const OpenSSLKey& key, Log* log) const
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        log->error(PRIME_LOCALISE("Couldn't create cipher context."));
        return false;
    }

    const EVP_CIPHER* cipher = EVP_aes_256_cbc();

    if (key.getKeyLength() != EVP_CIPHER_key_length(cipher) || key.getIVLength() != EVP_CIPHER_iv_length(cipher)) {
        log->error(PRIME_LOCALISE("Incorrect key size."));
        return false;
    }

    plaintext.resize(ciphertext.size() + EVP_CIPHER_block_size(cipher));

    if (EVP_DecryptInit_ex(ctx, cipher, NULL, key.getKey(), key.getIV()) != 1) {
        log->error(PRIME_LOCALISE("Couldn't initialise decryption engine."));
        return false;
    }

    int len = Narrow<int>(ciphertext.size());
    if (EVP_DecryptUpdate(ctx, (unsigned char*)&plaintext[0], &len,
            (const unsigned char*)ciphertext.data(), Narrow<int>(ciphertext.size()))
        != 1) {
        log->error(PRIME_LOCALISE("Couldn't decrypt."));
        return false;
    }

    int len2 = 0;
    if (EVP_DecryptFinal_ex(ctx, (unsigned char*)&plaintext[len], &len2) != 1) {
        log->error(PRIME_LOCALISE("Couldn't finalise decryption."));
        return false;
    }
    len += len2;

    plaintext.resize(len);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

Optional<std::string> OpenSSLAES256::encrypt(StringView plaintext, const OpenSSLKey& key, Log* log) const
{
    std::string output;
    if (!encrypt(output, plaintext, key, log)) {
        return nullopt;
    }
    return output;
}

Optional<std::string> OpenSSLAES256::decrypt(StringView ciphertext, const OpenSSLKey& key, Log* log) const
{
    std::string output;
    if (!decrypt(output, ciphertext, key, log)) {
        return nullopt;
    }
    return output;
}
}

#endif
