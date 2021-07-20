// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TEMPDIRECTORY_H
#define PRIME_TEMPDIRECTORY_H

#include "File.h"
#include "RefCounting.h"
#include <string>

namespace Prime {

/// Creates a uniquely named directory and optionally removes it (recursively) on destruction.
class PRIME_PUBLIC TempDirectory {
public:
    TempDirectory();

    /// Calls remove(). To prevent this, call cancelRemoveOnDestruct().
    ~TempDirectory();

    /// Performs the same action as the destructor.
    void close();

    /// Create a uniquely named directory in the specified path. Use GetTemporaryPath to determine a temporary
    /// file location for the application. If removeOnDestruct is true, the Log is retained to use when the
    /// directory is removed.
    bool createInPath(const char* path, bool removeOnDestruct, Log* log, unsigned int permissions = 0700);

    /// Returns the path to the directory that was created.
    const char* getPath() const { return _path.c_str(); }

    /// Recursively remove the directory.
    bool remove(Log* log);

    void cancelRemoveOnDestruct() { _removeOnDestructLog.release(); }

private:
    std::string _path;
    RefPtr<Log> _removeOnDestructLog;
};

}

#endif
