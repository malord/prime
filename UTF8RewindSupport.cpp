// Copyright 2000-2021 Mark H. P. Lord

#include "UTF8RewindSupport.h"

#ifndef PRIME_NO_UTF8REWIND

#include "StringUtils.h"
#include "utf8rewind/utf8rewind.h"

namespace Prime {

namespace {

    class UTF8RewindCaseConverter : public CaseConverter {
    public:
        virtual size_t toUpperCase(StringView source, char* dest, size_t destSize) PRIME_OVERRIDE
        {
            if (source.empty()) {
                return 0;
            }
            int32_t errors;
            size_t outputSize = utf8toupper(source.data(), source.size(), dest, destSize, UTF8_LOCALE_DEFAULT, &errors);
            if (outputSize == 0 || errors != UTF8_ERR_NONE) {
                return getASCIIConverter()->toUpperCase(source, dest, destSize);
            }
            return outputSize;
        }

        virtual size_t toLowerCase(StringView source, char* dest, size_t destSize) PRIME_OVERRIDE
        {
            if (source.empty()) {
                return 0;
            }
            int32_t errors;
            size_t outputSize = utf8tolower(source.data(), source.size(), dest, destSize, UTF8_LOCALE_DEFAULT, &errors);
            if (outputSize == 0 || errors != UTF8_ERR_NONE) {
                return getASCIIConverter()->toLowerCase(source, dest, destSize);
            }
            return outputSize;
        }

        virtual size_t toTitleCase(StringView source, char* dest, size_t destSize) PRIME_OVERRIDE
        {
            if (source.empty()) {
                return 0;
            }
            int32_t errors;
            size_t outputSize = utf8totitle(source.data(), source.size(), dest, destSize, UTF8_LOCALE_DEFAULT, &errors);
            if (outputSize == 0 || errors != UTF8_ERR_NONE) {
                return getASCIIConverter()->toTitleCase(source, dest, destSize);
            }
            return outputSize;
        }

        virtual size_t fold(StringView source, char* dest, size_t destSize) PRIME_OVERRIDE
        {
            if (source.empty()) {
                return 0;
            }
            int32_t errors;
            size_t outputSize = utf8casefold(source.data(), source.size(), dest, destSize, UTF8_LOCALE_DEFAULT, &errors);
            if (outputSize == 0 || errors != UTF8_ERR_NONE) {
                return getASCIIConverter()->fold(source, dest, destSize);
            }
            return outputSize;
        }
    };
}

UTF8RewindSupport::UTF8RewindSupport()
{
    init();
}

UTF8RewindSupport::~UTF8RewindSupport()
{
    close();
}

void UTF8RewindSupport::init()
{
    static UTF8RewindCaseConverter cc;

    CaseConverter::setGlobal(&cc);
}

void UTF8RewindSupport::close()
{
    CaseConverter::setGlobal(NULL);
}
}

#endif // PRIME_NO_UTF8REWIND
