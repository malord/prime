// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNIX_UNIXDIRECTORYREADER_H
#define PRIME_UNIX_UNIXDIRECTORYREADER_H

#include "../DirectoryReaderBase.h"
#include "../Log.h"
#include <dirent.h>
#include <sys/types.h>

#ifdef PRIME_OS_QNX
#define PRIME_DIRENT_MISSING_D_TYPE
#endif

#ifdef PRIME_DIRENT_MISSING_D_TYPE
#include <string>
#include <sys/stat.h>
#endif

namespace Prime {

/// Lightweight wrapper around the UNIX readdir() API.
class PRIME_PUBLIC UnixDirectoryReader : public DirectoryReaderBase {
public:
    UnixDirectoryReader();

    ~UnixDirectoryReader();

    /// Open a directory for reading. "path" is the path to a directory and cannot contain a wildcard.
    bool open(const char* path, Log* log, const Options& options = Options());

    bool isOpen() const { return _dir != NULL; }

    void close();

    /// Read the next directory entry. Returns false if there are no more entries to read.
    bool read(Log* log, bool* error = NULL);

    /// Returns the file name, without path, of the directory entry.
    const char* getName() const
    {
        PRIME_ASSERT(_ent);
        return _ent->d_name;
    }

    /// This will return false for a symlink to a directory.
    bool isDirectory() const
    {
        PRIME_ASSERT(_ent);
#ifdef PRIME_DIRENT_MISSING_D_TYPE
        return (S_ISDIR(needStat()->st_mode)) != 0;
#else
        return _ent->d_type == DT_DIR;
#endif
    }

    bool isLink() const
    {
        PRIME_ASSERT(_ent);
#ifdef PRIME_DIRENT_MISSING_D_TYPE
        return (S_ISLNK(needStat()->st_mode)) != 0;
#else
        return _ent->d_type == DT_LNK;
#endif
    }

    bool isHidden() const
    {
        PRIME_ASSERT(_ent);
        return _ent->d_name[0] == '.';
    }

    bool isFile() const
    {
        PRIME_ASSERT(_ent);
#ifdef PRIME_DIRENT_MISSING_D_TYPE
        return (S_ISREG(needStat()->st_mode)) != 0;
#else
        return _ent->d_type == DT_REG;
#endif
    }

    const struct dirent* getEntry() const
    {
        PRIME_ASSERT(_ent);
        return _ent;
    }

private:
#ifdef PRIME_DIRENT_MISSING_D_TYPE
    struct stat* needStat() const;
#endif

    DIR* _dir;
    struct dirent* _ent;
    bool _preventInherit;

#ifdef PRIME_DIRENT_MISSING_D_TYPE
    mutable ScopedPtr<struct stat> _stat;
    std::string _path;
#endif

    PRIME_UNCOPYABLE(UnixDirectoryReader);
};
}

#endif
