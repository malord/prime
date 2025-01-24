// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STRINGLOG_H
#define PRIME_STRINGLOG_H

#include "Mutex.h"
#include "StreamLog.h"
#include "StringStream.h"

namespace Prime {

/// A Log which writes to a string. This wraps a StreamLog and a StringStream.
class PRIME_PUBLIC StringLog : public StreamLog {
public:
    StringLog();

    ~StringLog();

    /// Direct access to the std::string. The string can be modified.
    std::string& getString() { return _stringStream.getString(); }

    /// Direct access to the std::string.
    const std::string& getString() const { return _stringStream.getString(); }

    /// Get the data as a C string (i.e., null terminated).
    const char* c_str() const { return _stringStream.c_str(); }

private:
    StringStream _stringStream;
};
}

#endif
