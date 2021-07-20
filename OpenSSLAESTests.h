// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OPENSSLAESTESTS_H
#define PRIME_OPENSSLAESTESTS_H

#include "OpenSSLSupport.h"

#ifndef PRIME_NO_OPENSSL

#include "OpenSSLAES.h"
#include "TextEncoding.h"

namespace Prime {

namespace OpenSSLAESTestsPrivate {

    inline void Tests()
    {
        Log* log = Log::getGlobal();

        static const char passphrase[] = "bananahammock_bananahammock";
        static StringView plaintext("14489/100.00000");

        PRIME_TEST(OpenSSLSupport::initSSL(log));

        Optional<std::string> encrypted = OpenSSLAES256::encryptWithMD5Key(passphrase, plaintext, log);
        PRIME_TEST(encrypted);
        if (encrypted) {
            PRIME_TEST(OpenSSLAES256::decryptWithMD5Key(passphrase, *encrypted, log).value_or("") == plaintext);
            puts(Base32Encode(*encrypted).c_str());
        }
        PRIME_TEST(Base32Encode("14489/100.00000") == "GE2DIOBZF4YTAMBOGAYDAMBQ");
    }
}

inline void OpenSSLAESTests()
{
    OpenSSLAESTestsPrivate::Tests();
}
}

#else // PRIME_NO_OPENSSL

namespace Prime {

inline void OpenSSLAESTests()
{
}
}

#endif // PRIME_NO_OPENSSL

#endif
