// Copyright 2000-2021 Mark H. P. Lord

#include "JSONWriter.h"
#include "ScopedPtr.h"
#include "StringStream.h"
#include "StringUtils.h"
#include "TextEncoding.h"

namespace Prime {

static inline bool IsDangerousCharacter(uint32_t uch)
{
    return uch == 0x2028 || uch == 0x2029 || uch == 0x0085;
}

JSONWriter::JSONWriter()
{
}

bool JSONWriter::write(Stream* stream, Log* log, const Value& value, const Options& options, size_t bufferSize, void* buffer)
{
    if (!begin(stream, log, options, bufferSize, buffer)) {
        return false;
    }

    if (!write(&_streamBuffer, value, 0)) {
        return false;
    }

    return end();
}

bool JSONWriter::begin(Stream* stream, Log* log, const Options& options, size_t bufferSize, void* buffer)
{
    _log = log;
    _options = options;

    PRIME_ASSERT(bufferSize > 3);
    _streamBuffer.init(stream, bufferSize, buffer);

    return true;
}

bool JSONWriter::end()
{
    if (_options.getWantTrailingNewline() && !writeNewline(&_streamBuffer)) {
        return false;
    }

    if (!_streamBuffer.flush(_log)) {
        return false;
    }

    return true;
}

bool JSONWriter::write(Stream* stream, Log* log, StringView string, const Options& options, size_t bufferSize, void* buffer)
{
    if (!begin(stream, log, options, bufferSize, buffer)) {
        return false;
    }

    if (!writeString(&_streamBuffer, string)) {
        return false;
    }

    return end();
}

bool JSONWriter::write(Stream* stream, Log* log, const char* string, const Options& options, size_t bufferSize, void* buffer)
{
    return write(stream, log, StringView(string), options, bufferSize, buffer);
}

bool JSONWriter::write(Stream* stream, Log* log, const std::string& string, const Options& options, size_t bufferSize, void* buffer)
{
    return write(stream, log, StringView(string), options, bufferSize, buffer);
}

bool JSONWriter::write(Stream* stream, Log* log, const Data& data, const Options& options, size_t bufferSize, void* buffer)
{
    if (!begin(stream, log, options, bufferSize, buffer)) {
        return false;
    }

    if (!writeData(&_streamBuffer, data)) {
        return false;
    }

    return end();
}

bool JSONWriter::write(Stream* stream, Log* log, const Value::Vector& vector, const Options& options, size_t bufferSize, void* buffer)
{
    if (!begin(stream, log, options, bufferSize, buffer)) {
        return false;
    }

    if (!writeVector(&_streamBuffer, vector, 0)) {
        return false;
    }

    return end();
}

bool JSONWriter::write(Stream* stream, Log* log, const Value::Dictionary& dictionary, const Options& options, size_t bufferSize, void* buffer)
{
    if (!begin(stream, log, options, bufferSize, buffer)) {
        return false;
    }

    if (!writeDictionary(&_streamBuffer, dictionary, 0)) {
        return false;
    }

    return end();
}

bool JSONWriter::write(Stream* stream, Log* log, const std::vector<std::string>& stringVector, const Options& options, size_t bufferSize, void* buffer)
{
    if (!begin(stream, log, options, bufferSize, buffer)) {
        return false;
    }

    if (!writeStringVector(&_streamBuffer, stringVector, 0)) {
        return false;
    }

    return end();
}

bool JSONWriter::write(StreamBuffer* streamBuffer, const Value& value, int indent)
{
    switch (value.getType()) {
    case Value::TypeUndefined:
        if (!_options.getAllowUndefined()) {
            if (!_options.getAllowUndefinedAsNull()) {
                _log->warning(PRIME_LOCALISE("Invalid value written as null."));
            }
            return streamBuffer->writeBytes("null", 4, _log);
        } else {
            return streamBuffer->writeBytes("undefined", 9, _log);
        }
        break;

    case Value::TypeNull:
        if (!_options.getAllowNull()) {
            _log->warning(PRIME_LOCALISE("Null value written."));
        }
        return streamBuffer->writeBytes("null", 4, _log);

    case Value::TypeBool:
        if (value.getBool()) {
            return streamBuffer->writeBytes("true", 4, _log);
        } else {
            return streamBuffer->writeBytes("false", 5, _log);
        }

    case Value::TypeInteger:
        return writeInteger(streamBuffer, value.getInteger());

    case Value::TypeReal:
        return writeReal(streamBuffer, value.getReal());

    case Value::TypeDate:
        return writeDate(streamBuffer, value.getDate());

    case Value::TypeTime:
        return writeTime(streamBuffer, value.getTime());

    case Value::TypeDateTime:
        return writeDateTime(streamBuffer, value.getUnixTime());

    case Value::TypeData:
        return writeData(streamBuffer, value.getData());

    case Value::TypeString:
        return writeString(streamBuffer, value.getString());

    case Value::TypeVector:
        return writeVector(streamBuffer, value.getVector(), indent);

    case Value::TypeDictionary:
        return writeDictionary(streamBuffer, value.getDictionary(), indent);

    case Value::TypeObject: {
        Value serialised = value.toValue();
        if (serialised.isUndefined() || serialised.isObject()) {
            _log->error(PRIME_LOCALISE("Unserialisable object cannot be written as JSON."));
            return false;
        }

        return write(streamBuffer, serialised, indent);
    }
    }

    PRIME_ASSERT(0);
    return false;
}

bool JSONWriter::writeInteger(StreamBuffer* streamBuffer, Value::Integer n)
{
    char buffer[128];
    StringFormat(buffer, sizeof(buffer), "%" PRIME_PRId_VALUE, n);
    return streamBuffer->writeString(buffer, _log);
}

bool JSONWriter::writeReal(StreamBuffer* streamBuffer, Value::Real d)
{
    char buffer[128];
    StringFormat(buffer, sizeof(buffer), "%" PRIME_PRIg_VALUE, d);
    return streamBuffer->writeString(buffer, _log);
}

bool JSONWriter::writeDate(StreamBuffer* streamBuffer, const Date& date)
{
    char buffer[128];
    buffer[0] = '"';
    date.toISO8601(buffer + 1, sizeof(buffer) - 2);
    StringAppend(buffer, sizeof(buffer), "\"");

    return streamBuffer->writeString(buffer, _log);
}

bool JSONWriter::writeTime(StreamBuffer* streamBuffer, const Time& time)
{
    char buffer[128];
    buffer[0] = '"';
    time.toISO8601(buffer + 1, sizeof(buffer) - 2);
    StringAppend(buffer, sizeof(buffer), "\"");

    return streamBuffer->writeString(buffer, _log);
}

bool JSONWriter::writeDateTime(StreamBuffer* streamBuffer, const UnixTime& unixTime)
{
    char buffer[128];
    buffer[0] = '"';
    DateTime(unixTime).toISO8601(buffer + 1, sizeof(buffer) - 2, "T", "Z");
    StringAppend(buffer, sizeof(buffer), "\"");

    return streamBuffer->writeString(buffer, _log);
}

bool JSONWriter::writeData(StreamBuffer* streamBuffer, const Data& data)
{
    if (!streamBuffer->writeByte('"', _log)) {
        return false;
    }

    if (!data.empty()) {
        // Could use a Base64Encoder here to save on memory

        size_t maxBase64Size = Base64ComputeMaxEncodedSize(data.size(), 0, 0);

        ScopedArrayPtr<char> base64(new char[maxBase64Size]);

        size_t encodedSize = Base64Encode(base64.get(), maxBase64Size, &data[0], data.size(), 0);
        PRIME_ASSERT(encodedSize <= maxBase64Size);

        if (!streamBuffer->writeBytes(base64.get(), encodedSize, _log)) {
            return false;
        }
    }

    return streamBuffer->writeByte('"', _log);
}

bool JSONWriter::writeString(StreamBuffer* streamBuffer, StringView string)
{
    const char* begin = string.begin();
    const char* ptr = begin;
    const char* end = string.end();

    if (!streamBuffer->writeByte('"', _log)) {
        return false;
    }

    const char* escape = NULL;
    size_t escapeLength = 0;
    size_t skipLength = 0;
    char escapeBuffer[32];

    for (;;) {
        while (ptr != end && (unsigned char)*ptr >= ' ' && (unsigned char)*ptr < 0x7f && *ptr != '\\' && *ptr != '"') {
            ++ptr;
        }

        if (ptr != end) {
            escapeLength = 2;
            skipLength = 1;

            switch (*ptr) {
            case '\a':
                escape = "\\a";
                break;

            case '\b':
                escape = "\\b";
                break;

            case '\f':
                escape = "\\f";
                break;

            case '\n':
                escape = "\\n";
                break;

            case '\r':
                escape = "\\r";
                break;

            case '\t':
                escape = "\\t";
                break;

            case '\v':
                escape = "\\v";
                break;

            case '\0':
                escape = "\\0";
                break;

            case '"':
                escape = "\\\"";
                break;

            case '\\':
                escape = "\\\\";
                break;

            default: {
                unsigned int length;
                if ((unsigned char)*ptr >= 0x80 && UTF8IsValid(ptr, end, &length)) {
                    // We have a valid Unicode character. It may or may not need escaping.

                    uint32_t uch = UTF8Decode(ptr, length);

                    if (!_options.getUTF8() || IsDangerousCharacter(uch)) {
                        if (UTF16CanEncode(uch)) {
                            uint16_t utf16[4];
                            unsigned int utf16Length = UTF16Encode(utf16, uch);
                            escapeBuffer[0] = 0;
                            for (unsigned int i = 0; i != utf16Length; ++i) {
                                StringAppendFormat(escapeBuffer, sizeof(escapeBuffer), "\\u%04x", (unsigned int)utf16[i]);
                            }

                            escape = escapeBuffer;
                            escapeLength = utf16Length * 6;
                            skipLength = length;
                            break;
                        }

                        // Can't encode as UTF-16! Write the first byte escaped (fall through).

                    } else {
                        // Safe to write as UTF-8.
                        escape = ptr;
                        escapeLength = length;
                        skipLength = length;
                        break;
                    }
                }

                StringFormat(escapeBuffer, sizeof(escapeBuffer), "\\u%04x", ((unsigned char)*ptr) & 0xff);
                escape = escapeBuffer;
                escapeLength = 6;
                break;
            }
            }
        }

        if (ptr != begin) {
            if (!streamBuffer->writeBytes(begin, ptr - begin, _log)) {
                return false;
            }
        }

        if (ptr == end) {
            break;
        }

        if (!streamBuffer->writeBytes(escape, escapeLength, _log)) {
            return false;
        }

        ptr += skipLength;
        begin = ptr;
    }

    return streamBuffer->writeByte('"', _log);
}

bool JSONWriter::writeVector(StreamBuffer* streamBuffer, const Value::Vector& array, int indent)
{
    if (array.empty()) {
        return streamBuffer->writeBytes("[]", 2, _log);
    }

    if (!streamBuffer->writeByte('[', _log)) {
        return false;
    }

    size_t n = array.size();
    for (size_t i = 0; i != n; ++i) {
        if (!writeNewline(streamBuffer) || !writeIndent(streamBuffer, indent + 1)) {
            return false;
        }

        const Value& element = array[i];

        if (!write(streamBuffer, element, indent + 1)) {
            return false;
        }

        if (i != n - 1) {
            streamBuffer->writeByte(',', _log);
        }
    }

    if (!writeNewline(streamBuffer) || !writeIndent(streamBuffer, indent) || !streamBuffer->writeByte(']', _log)) {
        return false;
    }

    return true;
}

bool JSONWriter::writeStringVector(StreamBuffer* streamBuffer, const std::vector<std::string>& array, int indent)
{
    if (array.empty()) {
        return streamBuffer->writeBytes("[]", 2, _log);
    }

    if (!streamBuffer->writeByte('[', _log)) {
        return false;
    }

    size_t n = array.size();
    for (size_t i = 0; i != n; ++i) {
        if (!writeNewline(streamBuffer) || !writeIndent(streamBuffer, indent + 1)) {
            return false;
        }

        const std::string& element = array[i];

        if (!writeString(streamBuffer, element)) {
            return false;
        }

        if (i != n - 1) {
            streamBuffer->writeByte(',', _log);
        }
    }

    if (!writeNewline(streamBuffer) || !writeIndent(streamBuffer, indent) || !streamBuffer->writeByte(']', _log)) {
        return false;
    }

    return true;
}

bool JSONWriter::writeDictionary(StreamBuffer* streamBuffer, const Value::Dictionary& dictionary, int indent)
{
    size_t n = dictionary.size();

    if (!n) {
        return streamBuffer->writeBytes("{}", 2, _log);
    }

    if (!streamBuffer->writeByte('{', _log)) {
        return false;
    }

    const char* colon;
    size_t colonSize;

    if (_options.getSingleLineMode()) {
        colon = ":";
        colonSize = 1;
    } else {
        colon = ": ";
        colonSize = 2;
    }

    for (size_t i = 0; i != n; ++i) {
        const Value::Dictionary::value_type& pair = dictionary.pair(i);

        if (!writeNewline(streamBuffer) || !writeIndent(streamBuffer, indent + 1)) {
            return false;
        }

        if (!write(streamBuffer, pair.first, indent + 1)) {
            return false;
        }

        if (!streamBuffer->writeBytes(colon, colonSize, _log)) {
            return false;
        }

        if (!write(streamBuffer, pair.second, indent + 1)) {
            return false;
        }

        if (i != n - 1) {
            streamBuffer->writeByte(',', _log);
        }
    }

    if (!writeNewline(streamBuffer) || !writeIndent(streamBuffer, indent) || !streamBuffer->writeByte('}', _log)) {
        return false;
    }

    return true;
}

bool JSONWriter::writeIndent(StreamBuffer* streamBuffer, int indent)
{
    if (_options.getSingleLineMode()) {
        return true;
    }

    while (indent--) {
        if (!streamBuffer->writeByte('\t', _log)) {
            return false;
        }
    }

    return true;
}

bool JSONWriter::writeNewline(StreamBuffer* streamBuffer)
{
    if (_options.getSingleLineMode()) {
        return true;
    }

    return streamBuffer->writeByte('\n', _log);
}

//
// AppendJSON
//

template <typename Input>
static void AppendJSONTemplate(std::string& out, const Input& value, bool formatted = false)
{
    JSONWriter::Options options;
    options.setSingleLineMode(!formatted);
    options.setWantTrailingNewline(false);
    options.setAllowEverything();

    StringStream stream;
    stream.getString().swap(out);
    char buffer[1024];
    if (!JSONWriter().write(&stream, Log::getNullLog(), value, options, sizeof(buffer), buffer)) {
        RuntimeError("JSONWriter().write failed");
    }
    stream.getString().swap(out);
}

void AppendJSON(std::string& out, const Value& value, bool formatted)
{
    AppendJSONTemplate(out, value, formatted);
}

void AppendJSON(std::string& out, StringView string, bool formatted)
{
    AppendJSONTemplate(out, string, formatted);
}

void AppendJSON(std::string& out, const char* string, bool formatted)
{
    AppendJSONTemplate(out, StringView(string), formatted);
}

void AppendJSON(std::string& out, const std::string& string, bool formatted)
{
    AppendJSONTemplate(out, StringView(string), formatted);
}

void AppendJSON(std::string& out, const Data& data, bool formatted)
{
    AppendJSONTemplate(out, data, formatted);
}

void AppendJSON(std::string& out, const Value::Vector& vector, bool formatted)
{
    AppendJSONTemplate(out, vector, formatted);
}

void AppendJSON(std::string& out, const Value::Dictionary& dictionary, bool formatted)
{
    AppendJSONTemplate(out, dictionary, formatted);
}

void AppendJSON(std::string& out, const std::vector<std::string>& stringVector, bool formatted)
{
    AppendJSONTemplate(out, stringVector, formatted);
}
}
