// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WILDCARDEXPANSIONLOADER_H
#define PRIME_WILDCARDEXPANSIONLOADER_H

#include "Config.h"
#include <string>
#include <vector>

namespace Prime {

/// Wrap a WildcardExpansion implementation and load all the results during the call to open(). You should
/// use this if you're modifying the contents of a directory while a find is in progress.
template <typename WildcardExpansion>
class WildcardExpansionLoader {
public:
    typedef typename WildcardExpansion::Options Options;

    WildcardExpansionLoader() { construct(); }

    /// Immediately invokes find().
    explicit WildcardExpansionLoader(const char* pattern, const Options& options, Log* log)
    {
        construct();
        _wildcard.find(pattern, options, log);
    }

    ~WildcardExpansionLoader() { }

    /// Begins finding file names which match the specified pattern.
    bool find(const char* pattern, const Options& options, Log* log)
    {
        close();

        if (!_wildcard.find(pattern, options, log)) {
            return false;
        }

        if (!load(log)) {
            return false;
        }

        _wildcard.close();
        return true;
    }

    /// Load all the filenames from our WildcardExpansion. If the WildcardExpansion type has custom "read"
    /// methods, you can invoke them (via get()) then call load() to load the list.
    bool load(Log* log)
    {
        while (const char* filename = _wildcard.read(log)) {
            _matches.push_back(filename);
        }

        return true;
    }

    /// Returns the next match.
    const char* read(Log*)
    {
        if (_at >= _matches.size()) {
            return NULL;
        }

        return _matches[_at++].c_str();
    }

    /// Free the list.
    void close()
    {
        _matches.clear();
        _at = 0;
    }

    //
    // Access the underlying WildcardExpansion
    //

    WildcardExpansion& get() { return _wildcard; }
    const WildcardExpansion& get() const { return _wildcard; }

private:
    void construct()
    {
        _at = 0;
    }

    WildcardExpansion _wildcard;
    std::vector<std::string> _matches;
    size_t _at;

    PRIME_UNCOPYABLE(WildcardExpansionLoader);
};

}

#endif
