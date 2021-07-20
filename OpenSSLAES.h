// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPENSSLAES_H
#define PRIME_OPENSSLAES_H

#include "OpenSSLSupport.h"

#ifndef PRIME_NO_OPENSSL

#include "Optional.h"
#include "StringView.h"

namespace Prime {

//
// OpenSSLKey
//

class OpenSSLKey {
public:
    enum { maxKeyLength = 64 };
    enum { maxIVLength = 16 };

    OpenSSLKey();

    ~OpenSSLKey();

    bool createKey(StringView passphrase, Log* log, const char* cipherName,
        const char* digestName, const char* salt);

    const unsigned char* getKey() const { return _key; }
    int getKeyLength() const { return _keyLength; }

    const unsigned char* getIV() const { return _iv; }
    int getIVLength() const { return _ivLength; }

private:
    unsigned char _key[maxKeyLength];
    int _keyLength;

    unsigned char _iv[maxIVLength];
    int _ivLength;
};

//
// OpenSSLAES256
//

/// AES encryption using OpenSSL.
class PRIME_PUBLIC OpenSSLAES256 {
public:
    /// Replicate the old openssl tool behaviour.
    static bool createKey(OpenSSLKey& key, StringView passphrase, Log* log, const char* digestName = "md5",
        const char* salt = NULL);

    static Optional<std::string> encryptWithMD5Key(StringView passphrase, StringView plaintext, Log* log);
    static Optional<std::string> decryptWithMD5Key(StringView passphrase, StringView ciphertext, Log* log);

    OpenSSLAES256();

    ~OpenSSLAES256();

    bool encrypt(std::string& ciphertext, StringView plaintext, const OpenSSLKey& key, Log* log) const;

    bool decrypt(std::string& plaintext, StringView ciphertext, const OpenSSLKey& key, Log* log) const;

    Optional<std::string> encrypt(StringView plaintext, const OpenSSLKey& key, Log* log) const;

    Optional<std::string> decrypt(StringView ciphertext, const OpenSSLKey& key, Log* log) const;
};
}

#endif // PRIME_NO_OPENSSL

#endif
