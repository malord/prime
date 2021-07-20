// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_WILDCARDEXPANSIONBASE_H
#define PRIME_WILDCARDEXPANSIONBASE_H

#include "Config.h"

namespace Prime {

/// Base class for the platform specific wildcard expansion.
class PRIME_PUBLIC WildcardExpansionBase {
public:
    class PRIME_PUBLIC Options {
    public:
        Options()
            : _failIfNoMatches(false)
            , _sort(false)
            , _excludeHiddenFiles(false)
        {
        }

        /// All WildcardExpansions must implement this.
        Options& setFailIfNoMatches(bool value = true)
        {
            _failIfNoMatches = value;
            return *this;
        }
        bool getFailIfNoMatches() const { return _failIfNoMatches; }

        /// WildcardExpansions may not implement this.
        Options& setSort(bool value = true)
        {
            _sort = value;
            return *this;
        }
        bool getSort() const { return _sort; }

        /// WildcardExpansions may not implement this.
        Options& setExcludeHiddenFiles(bool value = true)
        {
            _excludeHiddenFiles = value;
            return *this;
        }
        bool getExcludeHiddenFiles() const { return _excludeHiddenFiles; }

    private:
        bool _failIfNoMatches;
        bool _sort;
        bool _excludeHiddenFiles;
    };
};
}

#endif
