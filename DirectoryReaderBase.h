// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DIRECTORYREADERBASE_H
#define PRIME_DIRECTORYREADERBASE_H

#include "Config.h"

namespace Prime {

/// Base class for the platform specific DirectoryReaders.
class PRIME_PUBLIC DirectoryReaderBase {
public:
    class PRIME_PUBLIC Options {
    public:
        Options()
            : _childProcessInherit(false)
        {
        }

        /// If supported by the platform, allow child processes to inherit the directory handle.
        Options& setChildProcessInherit(bool value = true)
        {
            _childProcessInherit = value;
            return *this;
        }
        bool getChildProcessInherit() const { return _childProcessInherit; }

    private:
        bool _childProcessInherit;
    };
};
}

#endif
