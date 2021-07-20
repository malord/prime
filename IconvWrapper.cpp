// Copyright 2000-2021 Mark H. P. Lord

#include "IconvWrapper.h"

#ifdef PRIME_HAVE_ICONVWRAPPER

#include <errno.h>

namespace Prime {

#ifndef PRIME_NO_ICONV
static const iconv_t invalidIconvHandle = (iconv_t)-1;
#endif

#ifndef PRIME_NO_MINICONV
static const miniconv_t invalidMiniconvHandle = (miniconv_t)-1;
#endif

IconvWrapper::IconvWrapper()
{
    init();
}

IconvWrapper::IconvWrapper(const char* toEncoding, const char* fromEncoding, const Options& options)
{
    init();

    open(toEncoding, fromEncoding, options);
}

void IconvWrapper::init()
{
#ifndef PRIME_NO_ICONV
    _iconvHandle = invalidIconvHandle;
#endif

#ifndef PRIME_NO_MINICONV
    _miniconvHandle = invalidMiniconvHandle;
#endif
}

IconvWrapper::~IconvWrapper()
{
    close();
}

bool IconvWrapper::open(const char* toEncoding, const char* fromEncoding, const Options& options)
{
    close();

    _fromCode = fromEncoding;
    _toCode = toEncoding;

    if (options.getIgnoreErrors()) {
        _toCode += "//IGNORE";
    } else if (options.isTransliterationEnabled()) {
        _toCode += "//TRANSLIT";
    }

#ifdef PRIME_NO_ICONV
    _miniconvHandle = miniconv_open(_toCode.c_str(), _fromCode.c_str());
    if (_miniconvHandle == invalidMiniconvHandle) {
        return false;
    }
#elif defined(PRIME_NO_MINICONV)
    _iconvHandle = iconv_open(_toCode.c_str(), _fromCode.c_str());
    if (_iconvHandle == invalidIconvHandle) {
        return false;
    }
#else
    if (options.getPreferIconv()) {
        _iconvHandle = iconv_open(_toCode.c_str(), _fromCode.c_str());
        if (_iconvHandle == invalidIconvHandle) {
            _miniconvHandle = miniconv_open(_toCode.c_str(), _fromCode.c_str());
            if (_miniconvHandle == invalidMiniconvHandle) {
                return false;
            }
        }
    } else {
        _miniconvHandle = miniconv_open(_toCode.c_str(), _fromCode.c_str());
        if (_miniconvHandle == invalidMiniconvHandle) {
            _iconvHandle = iconv_open(_toCode.c_str(), _fromCode.c_str());
            if (_iconvHandle == invalidIconvHandle) {
                return false;
            }
        }
    }
#endif

    return true;
}

void IconvWrapper::close()
{
#ifndef PRIME_NO_ICONV
    if (_iconvHandle != invalidIconvHandle) {
        iconv_close(_iconvHandle);
        _iconvHandle = invalidIconvHandle;
    }
#endif

#ifndef PRIME_NO_MINICONV
    if (_miniconvHandle != invalidMiniconvHandle) {
        miniconv_close(_miniconvHandle);
        _miniconvHandle = invalidMiniconvHandle;
    }
#endif
}

bool IconvWrapper::isOpen() const
{
#ifndef PRIME_NO_ICONV
    if (_iconvHandle != invalidIconvHandle) {
        return true;
    }
#endif

#ifndef PRIME_NO_MINICONV
    if (_miniconvHandle != invalidMiniconvHandle) {
        return true;
    }
#endif

    return false;
}

size_t IconvWrapper::iconv(const char** inBuffer, size_t& inBytesLeft, char** outBuffer, size_t& outBytesLeft, int& errorNumber)
{
    PRIME_ASSERT(isOpen());

#ifndef PRIME_NO_ICONV
    if (_iconvHandle != invalidIconvHandle) {
#ifdef PRIME_OS_WINDOWS
        size_t result = ::iconv(_iconvHandle, inBuffer, &inBytesLeft, outBuffer, &outBytesLeft);
#else
        size_t result = ::iconv(_iconvHandle, (char**)inBuffer, &inBytesLeft, outBuffer, &outBytesLeft);
#endif

        errorNumber = errno;

        return result;
    }
#endif

#ifndef PRIME_NO_MINICONV
    if (_miniconvHandle != invalidMiniconvHandle) {
        return miniconv2(_miniconvHandle, inBuffer, inBytesLeft, outBuffer, outBytesLeft, errorNumber);
    }
#endif

    return (size_t)-1;
}

bool IconvWrapper::iconv(const char** inBuffer, size_t& inBytesLeft, char** outBuffer, size_t& outBytesLeft, Log* log)
{
    int errorNumber;
    size_t result = iconv(inBuffer, inBytesLeft, outBuffer, outBytesLeft, errorNumber);
    if (result != (size_t)-1) {
        return true;
    }

    if (errorNumber == EINVAL || errorNumber == E2BIG) {
        return true;
    }

    log->logErrno(errorNumber);
    return false;
}
}

#endif // PRIME_HAVE_ICONVWRAPPER
