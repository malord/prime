// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PROPERTYLISTFILEWRITER_H
#define PRIME_PROPERTYLISTFILEWRITER_H

#include "Config.h"
#include "FileSystem.h"
#include "PrefixLog.h"
#include "ScopedPtr.h"
#include <string>

namespace Prime {

/// Provides a write(Value, Log) method which can be used as a callback for an object which wants to save state.
template <typename PropertyListWriterType>
class PropertyListFileWriter : public RefCounted {
public:
    typedef typename PropertyListWriterType::Options PropertyListWriterOptions;

    void init(FileSystem* fileSystem, const char* path, const char* message = NULL,
        const PropertyListWriterOptions& writerOptions = PropertyListWriterOptions())
    {
        _fileSystem = fileSystem;
        _path = path;
        _message = message;
        _writerOptions = writerOptions;
    }

    bool write(const Value& propertyList, Log* log)
    {
        PrefixLog prefixLog(log, _path);
        if (!_message.empty()) {
            prefixLog.trace("%s", _message.c_str());
        }

        if (RefPtr<Stream> stream = _fileSystem->openForWrite(_path.c_str(), prefixLog)) {
            char buffer[1024];
            if (!PropertyListWriterType().write(stream, prefixLog, propertyList, _writerOptions, sizeof(buffer), buffer)) {
                return false;
            }

            return stream->close(prefixLog);
        }

        return false;
    }

private:
    RefPtr<FileSystem> _fileSystem;
    std::string _path;
    std::string _message;
    PropertyListWriterOptions _writerOptions;
};
}

#endif
