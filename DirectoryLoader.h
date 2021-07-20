// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DIRECTORYLOADER_H
#define PRIME_DIRECTORYLOADER_H

#include "Config.h"
#include <string>
#include <vector>

namespace Prime {

/// Wrap a DirectoryReader implementation and load all the results during the call to open(). This ensures that
/// if the directory is modified during the directory read, the application still sees the original results.
template <typename DirectoryReader>
class DirectoryLoader {
public:
    typedef typename DirectoryReader::Options Options;

    DirectoryLoader() { construct(); }

    ~DirectoryLoader() { }

    /// Open a directory for reading. "path" is the path to a directory and cannot contain a wildcard. If you
    /// need to call a DirectoryReader specific alternative to open(), call it directly via get(), then call
    /// load().
    bool open(const char* path, Log* log, const Options& options = Options())
    {
        close();

        if (!_dir.open(path, log, options)) {
            return false;
        }

        if (!load(log)) {
            return false;
        }

        _dir.close();
        return true;
    }

    /// Load all the directory entries from the underlying DirectoryReader (which you must open via get()).
    bool load(Log* log)
    {
        bool error;
        while (_dir.read(log, &error)) {
            _entries.push_back(Entry());
            Entry& entry = _entries.back();
            entry.name = _dir.getName();
            entry.isDirectory = _dir.isDirectory();
            entry.isHidden = _dir.isHidden();
            entry.isLink = _dir.isLink();
            entry.isFile = _dir.isFile();
        }

        if (error) {
            return false;
        }

        _isOpen = true;
        return true;
    }

    bool isOpen() const { return _isOpen; }

    void close()
    {
        _entries.clear();
        _isOpen = false;
        _at = (size_t)-1;
    }

    /// Read the next directory entry. Returns false if there are no more entries to read.
    bool read(Log* log, bool* error = NULL)
    {
        (void)log;
        (void)error;

        PRIME_ASSERT(_isOpen);

        ++_at;

        return _at < _entries.size();
    }

    /// Returns the file name, without path, of the directory entry.
    const char* getName() const { return _entries[_at].name.c_str(); }

    /// This will return false for a symlink to a directory.
    bool isDirectory() const { return _entries[_at].isDirectory; }

    bool isLink() const { return _entries[_at].isLink; }

    bool isHidden() const { return _entries[_at].isHidden; }

    bool isFile() const { return _entries[_at].isFile; }

    //
    // Access the underlying DirectoryReader
    //

    DirectoryReader& get() { return _dir; }
    const DirectoryReader& get() const { return _dir; }

private:
    void construct()
    {
        _at = (size_t)-1;
        _isOpen = false;
    }

    struct Entry {
        std::string name;
        bool isHidden;
        bool isDirectory;
        bool isLink;
        bool isFile;
    };

    DirectoryReader _dir;
    std::vector<Entry> _entries;
    size_t _at;
    bool _isOpen;

    PRIME_UNCOPYABLE(DirectoryLoader);
};
}

#endif
