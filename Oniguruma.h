// Copyright 2000-2021 Mark H. P. Lord

//
// Unless you definitely need to use Oniguruma specifically, use the Regex typedef defined in Regex.h instead to
// allow the implementation to vary.
//

#ifndef PRIME_ONIGURUMA_H
#define PRIME_ONIGURUMA_H

#include "Config.h"

#ifndef PRIME_NO_ONIGURUMA

#include "StringView.h"
#include "onig/oniguruma.h"
#include <string>

#define PRIME_HAVE_ONIGURUMA

namespace Prime {

//
// Oniguruma
//

/// C++ wrapper around an Oniguruma regular expression. Once constructed, can be re-used by the same thread.
/// This is typedef'd to Regex in Regex.h. Use that unless you're using an Oniguruma specific feature.
class Oniguruma {
public:
    PRIME_STATIC_CONST(unsigned int, maxErrorMessage, ONIG_MAX_ERROR_MESSAGE_LEN);

    Oniguruma() { construct(); }

    enum EnumI { I = 1 };

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _flags(ONIG_OPTION_DEFAULT)
        {
        }

        Options(EnumI)
            : _flags(ONIG_OPTION_DEFAULT | ONIG_OPTION_IGNORECASE)
        {
        }

        Options& setIgnoreCase(bool value = true) { return setFlag(ONIG_OPTION_IGNORECASE, value); }
        Options& setSingleLine(bool value = true) { return setFlag(ONIG_OPTION_SINGLELINE, value); }
        Options& setMultiLine(bool value = true) { return setFlag(ONIG_OPTION_MULTILINE, value); }

        Options& setFlag(unsigned int flag, bool value)
        {
            _flags = (_flags & ~flag) | (value ? flag : 0);
            return *this;
        }

        unsigned int getOnigurumaFlags() const { return _flags; }

    private:
        unsigned int _flags;
    };

    /// Since most patterns are hardcoded, the log parameter is defaulted to the global log.
    explicit Oniguruma(StringView pattern, const Options& options = Options(), Log* log = Log::getGlobal())
    {
        construct();
        compile(pattern, options, log);
    }

    ~Oniguruma() { free(); }

    bool isInitialised() const { return _re != NULL; }

    bool operator!() const { return _re == NULL; }

    /// Since most patterns are hard-coded, the log parameter is defaulted to the global log.
    bool compile(StringView pattern, const Options& options = Options(), Log* log = Log::getGlobal());

    void free();

    class Match {
    public:
        Match()
            : _region(NULL)
            , _count(-1)
        {
        }

        ~Match() { free(); }

        size_t getCount() const
        {
            if (_count < 0) {
                for (_count = 0; _count != _region->num_regs; ++_count) {
                    if (_region->beg[_count] < 0) {
                        break;
                    }
                }
            }

            return (size_t)_count;
        }

        size_t getGroupOffset(size_t index) const { return _region->beg[index]; }
        size_t getGroupEndOffset(size_t index) const { return _region->end[index]; }
        size_t getGroupLength(size_t index) const { return _region->end[index] - _region->beg[index]; }

        StringView getGroupView(StringView string, size_t index) const
        {
            return StringView(string.begin() + getGroupOffset(index), getGroupLength(index));
        }

        std::string getGroup(StringView string, size_t index) const
        {
            return getGroupView(string, index).to_string();
        }

        void free();

    private:
        OnigRegion* needRegion();

        OnigRegion* _region;

        friend class Oniguruma;

        mutable int _count;
    };

    bool search(Match& m, StringView string, const char* start = NULL, const char* range = NULL) const;

    bool match(Match& m, StringView string) const;

    /// e.g., Oniguruma("\r\n").replaceFirstInPlace(string, "");
    bool replaceFirstInPlace(std::string& string, StringView replacement, char escapeChar = '\0') const
    {
        return replaceInPlace(string, replacement, false, escapeChar);
    }

    /// e.g., Oniguruma("\r\n").replaceAllInPlace(string, "\n");
    bool replaceAllInPlace(std::string& string, StringView replacement, char escapeChar = '\0') const
    {
        return replaceInPlace(string, replacement, true, escapeChar);
    }

    /// e.g., Oniguruma("\r\n").replaceInPlace(string, "\n", false);
    bool replaceInPlace(std::string& string, StringView replacement, bool all, char escapeChar = '\0') const;

    std::string replaceFirst(StringView source, StringView replacement, char escapeChar = '\0') const
    {
        std::string output(source.begin(), source.end());
        replaceFirstInPlace(output, replacement, escapeChar);
        return output;
    }

    std::string replaceAll(StringView source, StringView replacement, char escapeChar = '\0') const
    {
        std::string output(source.begin(), source.end());
        replaceAllInPlace(output, replacement, escapeChar);
        return output;
    }

private:
    std::string expandReplacementString(StringView source,
        StringView replacement,
        char escapeChar,
        const Oniguruma::Match& m) const;

    void construct()
    {
        _re = NULL;
    }

    regex_t* _re;
};

//
// Oniguruma::Match
//

inline OnigRegion* Oniguruma::Match::needRegion()
{
    free();

    _region = onig_region_new();

    return _region;
}

inline void Oniguruma::Match::free()
{
    if (_region) {
        onig_region_free(_region, 1);
        _region = NULL;
    }
}

//
// Oniguruma
//

inline bool Oniguruma::compile(StringView pattern, const Options& options, Log* log)
{
    free();

    OnigErrorInfo errorInfo;
    int error = onig_new(&_re, (const UChar*)pattern.begin(), (const UChar*)pattern.end(),
        options.getOnigurumaFlags(), ONIG_ENCODING_UTF8, ONIG_SYNTAX_DEFAULT,
        &errorInfo);

    if (error != ONIG_NORMAL) {
        _re = NULL;

        char errorString[ONIG_MAX_ERROR_MESSAGE_LEN];

        onig_error_code_to_str((UChar*)errorString, error, &errorInfo);

        log->error("%s", errorString);
        return false;
    }

    return true;
}

inline void Oniguruma::free()
{
    if (_re) {
        onig_free(_re);
        _re = NULL;
    }
}

inline bool Oniguruma::search(Match& m, StringView string, const char* start, const char* range) const
{
    PRIME_ASSERT(isInitialised());

    return onig_search(_re, (const UChar*)string.begin(), (const UChar*)string.end(),
               (const UChar*)(start ? start : string.begin()), (const UChar*)(range ? range : string.end()),
               m.needRegion(), ONIG_OPTION_NONE)
        >= 0;
}

inline bool Oniguruma::match(Match& m, StringView string) const
{
    if (!search(m, string)) {
        return false;
    }

    return m.getGroupOffset(0) == 0 && m.getGroupEndOffset(0) == string.size();
}

inline std::string Oniguruma::expandReplacementString(StringView source,
    StringView replacement,
    char escapeChar,
    const Oniguruma::Match& m) const
{
    std::string output;

    for (const char* ptr = replacement.begin(); ptr != replacement.end(); ++ptr) {
        if (*ptr != escapeChar) {
            output += *ptr;
            continue;
        }

        ++ptr;
        unsigned int group;
        const char* endPtr;
        if (!ParseInt(StringView(ptr, replacement.end()), endPtr, group, 10)) {
            continue;
        }

        if (group >= m.getCount()) {
            continue;
        }

        ptr = endPtr - 1;
        output += m.getGroupView(source, group);
    }

    return output;
}

inline bool Oniguruma::replaceInPlace(std::string& string, StringView replacement, bool all, char escapeChar) const
{
    Oniguruma::Match m;
    ptrdiff_t offset = 0;
    bool any = false;
    for (;;) {
        if (!this->search(m, StringView(string.data(), string.data() + string.size()), string.data() + offset)) {
            break;
        }

        any = true;

        if (escapeChar) {
            std::string expandedReplacement = expandReplacementString(string, replacement, escapeChar, m);

            string.replace(m.getGroupOffset(0), m.getGroupLength(0), expandedReplacement.data(), expandedReplacement.size());
            offset = m.getGroupOffset(0) + expandedReplacement.size();

        } else {
            string.replace(m.getGroupOffset(0), m.getGroupLength(0), replacement.data(), replacement.size());
            offset = m.getGroupOffset(0) + replacement.size();
        }

        if (!all) {
            break;
        }
    }

    return any;
}
}

#endif // PRIME_NO_ONIGURUMA

#endif
