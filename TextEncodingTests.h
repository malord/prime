// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TEXTENCODINGTESTS_H
#define PRIME_TEXTENCODINGTESTS_H

#include "TextEncoding.h"

namespace Prime {

namespace TextEncodingTestsPrivate {

    inline void Base64Test()
    {
        int loops;

#ifdef PRIME_DEBUG
#define NTIMES 1000
#else
#define NTIMES 10000
#endif

        for (loops = 0; loops != NTIMES; ++loops) {
            size_t size;
            unsigned char* buffer;
            size_t maxEncodedSize;
            char* encoded;
            size_t encodedSize;
            size_t maxDecodedSize;
            unsigned char* decoded;
            ptrdiff_t decodedSize;

            size = (rand() % 65535) + 1;

            buffer = (unsigned char*)malloc(size);

            for (size_t i = 0; i != size; ++i) {
                buffer[i] = (unsigned char)rand();
            }

            maxEncodedSize = Base64ComputeMaxEncodedSize(size, 64, 2);

            encoded = (char*)malloc(maxEncodedSize + 1);

            memset(encoded, 0x01, maxEncodedSize + 1);

            encodedSize = Base64Encode(encoded, maxEncodedSize, buffer, size, 64, "\r\n");

            PRIME_ASSERT(encodedSize <= maxEncodedSize);
            PRIME_ASSERT(encoded[encodedSize] == 0x01);

            maxDecodedSize = Base64ComputeMaxDecodedSize(encodedSize);

            decoded = (unsigned char*)malloc(maxDecodedSize);

            decodedSize = Base64Decode(decoded, maxDecodedSize, StringView(encoded, encodedSize));

            PRIME_ASSERT(decodedSize != -1);
            PRIME_ASSERT(decodedSize <= (ptrdiff_t)maxDecodedSize);
            PRIME_ASSERT(decodedSize == (ptrdiff_t)size);

            PRIME_ASSERT(memcmp(decoded, buffer, size) == 0);

            free(decoded);
            free(encoded);
            free(buffer);
        }
    }

    inline void CEscapeTest()
    {
        char buffer[46];
        memset(buffer, '_', sizeof(buffer));
        const char test1[] = "Hello I am a long string \r\t\n\r\t\n\r\t\n\a";
        size_t total = CEscape(buffer, sizeof(buffer) - 1, test1, CEscapeFlagsAllCodes);
        PRIME_ASSERT(buffer[sizeof(buffer) - 1] == '_');
        PRIME_ASSERT(buffer[sizeof(buffer) - 2] == 0);
        PRIME_ASSERT(total == 45);
    }

    inline void Base32Test()
    {
        char buffer[64];
        buffer[Base32Encode(buffer, sizeof(buffer), "Hello", 5, 0, "")] = 0;
        PRIME_TEST(StringsEqual(buffer, "JBSWY3DP"));
        buffer[Base32Encode(buffer, sizeof(buffer), "Hell", 4, 0, "")] = 0;
        PRIME_TEST(StringsEqual(buffer, "JBSWY3A="));
        buffer[Base32Encode(buffer, sizeof(buffer), "Hel", 3, 0, "")] = 0;
        PRIME_TEST(StringsEqual(buffer, "JBSWY==="));
        buffer[Base32Encode(buffer, sizeof(buffer), "He", 2, 0, "")] = 0;
        PRIME_TEST(StringsEqual(buffer, "JBSQ===="));
        buffer[Base32Encode(buffer, sizeof(buffer), "H", 1, 0, "")] = 0;
        PRIME_TEST(StringsEqual(buffer, "JA======"));

        PRIME_TEST(Base32Encode("Hello") == "JBSWY3DP");
        PRIME_TEST(Base32Encode("Hell") == "JBSWY3A=");
        PRIME_TEST(Base32Encode("Hel") == "JBSWY===");
        PRIME_TEST(Base32Encode("He") == "JBSQ====");
        PRIME_TEST(Base32Encode("H") == "JA======");

        buffer[Base32Decode(buffer, sizeof(buffer), "JBSWY3DP")] = 0;
        PRIME_TEST(StringsEqual(buffer, "Hello"));
        buffer[Base32Decode(buffer, sizeof(buffer), "JBSWY3A=")] = 0;
        PRIME_TEST(StringsEqual(buffer, "Hell"));
        buffer[Base32Decode(buffer, sizeof(buffer), "JBSWY===")] = 0;
        PRIME_TEST(StringsEqual(buffer, "Hel"));
        buffer[Base32Decode(buffer, sizeof(buffer), "JBSQ====")] = 0;
        PRIME_TEST(StringsEqual(buffer, "He"));
        buffer[Base32Decode(buffer, sizeof(buffer), "JA======")] = 0;
        PRIME_TEST(StringsEqual(buffer, "H"));

        PRIME_TEST(Base32Decode("JBSWY3DP") == "Hello");
        PRIME_TEST(Base32Decode("JBSWY3A=") == "Hell");
        PRIME_TEST(Base32Decode("JBSWY===") == "Hel");
        PRIME_TEST(Base32Decode("JBSQ====") == "He");
        PRIME_TEST(Base32Decode("JA======") == "H");
        PRIME_TEST(Base32Decode("JBSWY3DP") == "Hello");
        PRIME_TEST(Base32Decode("JBSWY3A") == "Hell");
        PRIME_TEST(Base32Decode("JBSWY") == "Hel");
        PRIME_TEST(Base32Decode("JBSQ") == "He");
        PRIME_TEST(Base32Decode("JA") == "H");
    }
}

inline void TextEncodingTests()
{
    using namespace TextEncodingTestsPrivate;
    Base32Test();
    Base64Test();
    CEscapeTest();
}
}

#endif
