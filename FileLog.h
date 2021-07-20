// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FILELOG_H
#define PRIME_FILELOG_H

#include "Mutex.h"
#include "PrefixLog.h"
#include "TextLog.h"

namespace Prime {

/// A Log which writes to a file. Supports file size limits. For maximum performance, use this via a LogThreader.
class PRIME_PUBLIC FileLog : public TextLog {
public:
    FileLog();

    ~FileLog();

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _maxFileSize(5 * 1024 * 1024)
            , _maxFiles(10)
            , _truncate(false)
        {
        }

        Options& setMaxFileSize(int64_t value)
        {
            _maxFileSize = value;
            return *this;
        }
        Options& setUnlimitedFileSize() { return setMaxFileSize(INT64_MAX); }
        int64_t getMaxFileSize() const { return _maxFileSize; }
        bool isFileSizeUnlimited() const { return _maxFileSize == INT64_MAX; }

        Options& setMaxFiles(int value)
        {
            _maxFiles = value;
            return *this;
        }
        int getMaxFiles() const { return _maxFiles; }

        Options& setTruncate(bool value = true)
        {
            _truncate = value;
            return *this;
        }
        bool getTruncate() const { return _truncate; }

    private:
        int64_t _maxFileSize;
        int _maxFiles;
        bool _truncate;
    };

    bool init(const char* path, Log* log, const Options& options);

    bool isInitialised() const { return _mutex.isInitialised(); }

    const Options& getOptions() const { return _options; }
    Options& getOptions() { return _options; }

    bool clearLogs(Log* log);

protected:
    virtual void write(Level level, const char* string) PRIME_OVERRIDE;

    std::string getPathForArchive(int n) const;

private:
    RecursiveMutex _mutex;

    std::string _path;
    PrefixLog _log;
    Options _options;
};
}

#endif
