// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ICONVWRAPPER_H
#define PRIME_ICONVWRAPPER_H

#include "Config.h"
#include "Log.h"

#if !(defined(PRIME_NO_MINICONV) && defined(PRIME_NO_ICONV))

#define PRIME_HAVE_ICONVWRAPPER

#ifndef PRIME_NO_MINICONV
#include "miniconv.h"
#endif
#ifndef PRIME_NO_ICONV
#include <iconv.h>
#endif

namespace Prime {

/// A wrapper around the platform's iconv API (possibly with a fallback to miniconv).
class PRIME_PUBLIC IconvWrapper {
public:
    /// Options for open().
    class PRIME_PUBLIC Options {
    private:
        bool _preferIconv;
        bool _ignore;
        bool _transliterate;

    public:
        Options()
            : _preferIconv(true)
            , _ignore(true)
            , _transliterate(false)
        {
        }

        /// By default, iconv will be used over miniconv if enabled. This overrides that behaviour.
        Options& setPreferIconv(bool value)
        {
            _preferIconv = value;
            return *this;
        }
        bool getPreferIconv() const { return _preferIconv; }

        /// Attempt to ignore errors in the stream.
        Options& setIgnoreErrors(bool value)
        {
            _ignore = value;
            return *this;
        }
        bool getIgnoreErrors() const { return _ignore; }

        /// Attempt to transliterate characters that have no direct counterpart.
        Options& setTransliterationEnabled(bool value)
        {
            _transliterate = value;
            return *this;
        }
        bool isTransliterationEnabled() const { return _transliterate; }
    };

    /// Construct in an unopened state.
    IconvWrapper();

    /// Constructor that immediately calls open.
    IconvWrapper(const char* toEncoding, const char* fromEncoding, const Options& options = Options());

    ~IconvWrapper();

    /// Create a new iconv conversion descriptor. If one already exists, closes that one. Returns false on error.
    bool open(const char* toEncoding, const char* fromEncoding, const Options& options = Options());

    /// Close the conversion descriptor.
    void close();

    /// Returns a true value if we have been successfully initialised i.e., Open succeeded).
    bool isOpen() const;

    /// Perform an iconv. See iconv(3) for a description of this method and its return value.
    size_t iconv(const char** inBuffer, size_t& inBytesLeft, char** outBuffer, size_t& outBytesLeft, int& errorNumber);

    /// Perform an iconv. Returns true on success and false on error (setting errorNumber to the errno). Deals
    /// with EINVAL.
    bool iconv(const char** inBuffer, size_t& inBytesLeft, char** outBuffer, size_t& outBytesLeft, Log* log);

private:
    void init();

#ifndef PRIME_NO_ICONV
    iconv_t _iconvHandle;
#endif

#ifndef PRIME_NO_MINICONV
    miniconv_t _miniconvHandle;
#endif

    std::string _toCode;
    std::string _fromCode;

    PRIME_UNCOPYABLE(IconvWrapper);
};
}

#endif

#endif
