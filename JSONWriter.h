// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_JSONWRITER_H
#define PRIME_JSONWRITER_H

#include "StreamBuffer.h"
#include "Value.h"

namespace Prime {

/// Serialises a Value in JSON format.
class PRIME_PUBLIC JSONWriter {
public:
    JSONWriter();

    class PRIME_PUBLIC Options {
    public:
        Options()
            : _allowNull(true)
            , _allowUndefined(false)
            , _allowUndefinedAsNull(false)
            , _utf8(true)
            , _singleLine(false)
            , _trailingNewline(true)
        {
        }

        Options& setAllowEverything()
        {
            _allowNull = true;
            _allowUndefined = true;
            return *this;
        }

        /// Don't log a warning if a null Value is written. This is true by default.
        Options& setAllowNull(bool value)
        {
            _allowNull = value;
            return *this;
        }
        bool getAllowNull() const { return _allowNull; }

        /// Write an invalid Value as "undefined", which is non-standard JSON. This is false by default.
        Options& setAllowUndefined(bool value)
        {
            _allowUndefined = value;
            return *this;
        }
        bool getAllowUndefined() const { return _allowUndefined; }

        /// Write an invalid Value as null, and don't warn about it. This option is ignored if getAllowUndefined()
        /// is true.
        Options& setAllowUndefinedAsNull(bool value)
        {
            _allowUndefinedAsNull = value;
            return *this;
        }
        bool getAllowUndefinedAsNull() const { return _allowUndefinedAsNull; }

        /// We're writing UTF-8 to UTF-8, so only escape certain dangerous characters.
        Options& setUTF8(bool value)
        {
            _utf8 = value;
            return *this;
        }
        bool getUTF8() const { return _utf8; }

        /// Don't write newlines or indents.
        Options& setSingleLineMode(bool value)
        {
            _singleLine = value;
            return *this;
        }
        bool getSingleLineMode() const { return _singleLine; }

        /// Write a trailing \n (useful when writing to a text file). Defaults to true. Ignored if getSingleLineMode()
        /// is true.
        Options& setWantTrailingNewline(bool value)
        {
            _trailingNewline = value;
            return *this;
        }
        bool getWantTrailingNewline() const { return _trailingNewline; }

    private:
        bool _allowNull;
        bool _allowUndefined;
        bool _allowUndefinedAsNull;
        bool _utf8;
        bool _singleLine;
        bool _trailingNewline;
    };

    /// Write the Value's contents to the stream as JSON. If buffer is NULL, a buffer of bufferSize is allocated
    /// and freed by the function.
    bool write(Stream* stream, Log* log, const Value& value, const Options& options, size_t bufferSize, void* buffer = NULL);

    bool write(Stream* stream, Log* log, StringView string, const Options& options, size_t bufferSize, void* buffer = NULL);
    bool write(Stream* stream, Log* log, const char* string, const Options& options, size_t bufferSize, void* buffer = NULL);
    bool write(Stream* stream, Log* log, const std::string& string, const Options& options, size_t bufferSize, void* buffer = NULL);
    bool write(Stream* stream, Log* log, const Data& data, const Options& options, size_t bufferSize, void* buffer = NULL);
    bool write(Stream* stream, Log* log, const Value::Vector& vector, const Options& options, size_t bufferSize, void* buffer = NULL);
    bool write(Stream* stream, Log* log, const Value::Dictionary& dictionary, const Options& options, size_t bufferSize, void* buffer = NULL);
    bool write(Stream* stream, Log* log, const std::vector<std::string>& stringVector, const Options& options, size_t bufferSize, void* buffer = NULL);

private:
    bool begin(Stream* stream, Log* log, const Options& options, size_t bufferSize, void* buffer);
    bool end();

    bool write(StreamBuffer* streamBuffer, const Value& value, int indent);
    bool writeInteger(StreamBuffer* streamBuffer, Value::Integer n);
    bool writeReal(StreamBuffer* streamBuffer, Value::Real d);
    bool writeDate(StreamBuffer* streamBuffer, const Date& date);
    bool writeTime(StreamBuffer* streamBuffer, const Time& time);
    bool writeDateTime(StreamBuffer* streamBuffer, const UnixTime& unixTime);
    bool writeData(StreamBuffer* streamBuffer, const Data& data);
    bool writeString(StreamBuffer* streamBuffer, StringView string);
    bool writeVector(StreamBuffer* streamBuffer, const Value::Vector& array, int indent);
    bool writeDictionary(StreamBuffer* streamBuffer, const Value::Dictionary& dictionary, int indent);
    bool writeStringVector(StreamBuffer* streamBuffer, const std::vector<std::string>& array, int indent);

    bool writeIndent(StreamBuffer* streamBuffer, int indent);
    bool writeNewline(StreamBuffer* streamBuffer);

    Options _options;
    Log* _log;
    StreamBuffer _streamBuffer;

    PRIME_UNCOPYABLE(JSONWriter);
};

PRIME_PUBLIC void AppendJSON(std::string& out, const Value& value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const Value::Integer value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const Value::Real value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, StringView value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const char* value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const std::string& value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const Value::Vector& value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const Value::Dictionary& value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const Data& value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const UnixTime& value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const DateTime& value, bool multiline);
PRIME_PUBLIC void AppendJSON(std::string& out, const std::vector<std::string>& value, bool multiline);

/// To support new types, create an AppendJSON override.
template <typename Type>
inline std::string ToJSON(const Type& value, bool multiline = true)
{
    std::string temp;
    AppendJSON(temp, value, multiline);
    return temp;
}
}

#endif
