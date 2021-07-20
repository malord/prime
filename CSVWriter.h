// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_CSVWRITER_H
#define PRIME_CSVWRITER_H

#include "ArrayView.h"
#include "Config.h"
#include "StreamBuffer.h"

namespace Prime {

class PRIME_PUBLIC CSVWriter {
public:
    /// Returns the total size of the escaped output. If the return value >= bufferSize then the output was
    /// truncated (the output will always be null terminated). If buffer is null then nothing is written and
    /// the size is computed.
    static size_t escape(char* buffer, size_t bufferSize, StringView string);

    /// Returns the total size of the escaped output. If the return value >= bufferSize then the output was
    /// truncated (the output will always be null terminated). If buffer is null then nothing is written and
    /// the size is computed.
    static size_t escapeInQuotes(char* buffer, size_t bufferSize, StringView string);

    static std::string escape(StringView string);

    static std::string escapeInQuotes(StringView string);

    CSVWriter();

    enum { defaultBufferSize = 65536u };

    /// Assign the Stream and Log to write to. If buffer is null, allocate a buffer of the specified size,
    /// otherwise use the supplied memory for the buffer.
    CSVWriter(Stream* outputStream, Log* log, size_t bufferSize = defaultBufferSize, void* buffer = NULL);

    void init(Stream* stream, Log* log, size_t bufferSize = defaultBufferSize, void* buffer = NULL);

    Log* getLog() const { return _log; }

    bool getErrorFlag() const { return _error; }

    /// The error flag is set to true if the write callback fails.
    void setErrorFlag(bool value) { _error = value; }

    /// Set the newline sequence. Defaults to \r\n.
    void setNewline(StringView newline);

    /// Returns false if the error flag is set.
    bool writeCell(StringView cell);

    /// Write a printf formatted string to the next cell.
    bool printfCell(const char* format, ...);

    /// Write a printf formatted string given a va_list.
    bool vprintfCell(const char* format, va_list argptr);

    /// Doesn't escape the text.
    bool writeRaw(StringView text);

    /// Returns false if the error flag is set.
    bool endRow();

    /// Write an entire row followed by a newline. Calls endRow().
    bool writeRow(ArrayView<const StringView> cells);

    /// Returns false if the error flag is set.
    bool flush();

private:
    void construct();

    void beginWrite();

    StreamBuffer _streamBuffer;

    RefPtr<Log> _log;

    std::string _newline;

    bool _needComma;

    std::string _cellBuffer;

    bool _error;

    PRIME_UNCOPYABLE(CSVWriter);
};
}

#endif
