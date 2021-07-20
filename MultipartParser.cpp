// Copyright 2000-2021 Mark H. P. Lord

#include "MultipartParser.h"
#include "HTTP.h"
#include "StringUtils.h"

namespace Prime {

//
// MultipartParser::PartStream
//

class MultipartParser::PartStream : public Stream {
public:
    PartStream(MultipartParser* parent)
        : _parent(parent)
        , _bytesChecked(0)
    {
    }

    virtual ptrdiff_t readSome(void* output, size_t maximumBytes, Log* log) PRIME_OVERRIDE
    {
        StreamBuffer* buffer = _parent->_buffer;

        if (!_bytesChecked) {
            StringView boundary(_parent->_boundary);

            if (!buffer->getBytesAvailable()) {
                if (buffer->fetchMore(log) <= 0) {
                    return -1;
                }
            }

            const char* matchPtr = reinterpret_cast<const char*>(memchr(buffer->getReadPointer(), boundary[0], buffer->getBytesAvailable()));
            if (matchPtr == buffer->getReadPointer()) {
                // We only check for the boundary when it may begin at offset zero in the buffer.
                if (!buffer->requireNumberOfBytes(boundary.size(), log)) {
                    return -1;
                }
                if (memcmp(buffer->getReadPointer(), &boundary[0], boundary.size()) == 0) {
                    // We've found the boundary.
                    return 0;
                }

                // Didn't match, so just consume the first byte.
                _bytesChecked = 1;

            } else {
                _bytesChecked = matchPtr ? static_cast<size_t>(matchPtr - buffer->getReadPointer()) : buffer->getBytesAvailable();
            }
        }

        size_t thisTime = std::min(_bytesChecked, maximumBytes);
        memcpy(output, buffer->getReadPointer(), thisTime);
        buffer->advanceReadPointer(thisTime);
        _bytesChecked -= thisTime;
        return static_cast<ptrdiff_t>(thisTime);
    }

    virtual bool close(Log* log) PRIME_OVERRIDE
    {
        bool success = _parent->_buffer->close(log);
        _parent->_buffer = NULL;
        return success;
    }

private:
    RefPtr<MultipartParser> _parent;
    size_t _bytesChecked;
};

//
// MultipartParser
//

// media-type     = type "/" subtype *( ";" parameter )
// type           = token
// subtype        = token
// parameter      = attribute "=" value
// attribute      = token
// value          = token | quoted-string
std::string MultipartParser::parseBoundary(StringView header)
{
    StringView type = HTTPParseToken(header, header);
    if (!ASCIIEqualIgnoringCase(type, "multipart")) {
        return std::string();
    }
    if (HTTPSkip(header, "/", header)) {
        (void)HTTPParseToken(header, header); // ignore the subtype
    }

    for (;;) {
        if (!HTTPSkip(header, ";", header)) {
            break;
        }

        StringView name = HTTPParseToken(header, header);
        if (HTTPSkip(header, "=", header)) {
            std::string value = HTTPParseTokenOrQuotedString(header, header);

            if (ASCIIEqualIgnoringCase(name, "boundary")) {
                return value;
            }
        }
    }

    return std::string();
}

MultipartParser::MultipartParser()
{
    _reachedEnd = false;
}

MultipartParser::~MultipartParser()
{
}

bool MultipartParser::init(StreamBuffer* buffer, StringView boundary, Log* log)
{
    RefPtr<StreamBuffer> oldBuffer;
    std::swap(_buffer, oldBuffer); // _buffer will be NULL if we fail

    if (boundary.empty()) {
        log->error(PRIME_LOCALISE("multipart missing boundary"));
        return false;
    }

    _boundary.reserve(boundary.size() + 4);
    _boundary = "\r\n--";
    _boundary += boundary;

    _firstPart = true;
    _buffer = buffer;

    return true;
}

bool MultipartParser::init(Stream* stream, StringView boundary, size_t bufferSize, Log* log)
{
    return init(PassRef(new StreamBuffer(stream, bufferSize)), boundary, log);
}

RefPtr<Stream> MultipartParser::readPart(Log* log)
{
    if (!PRIME_GUARD(_buffer)) {
        return NULL;
    }
    if (_reachedEnd) {
        return NULL;
    }

    StringView boundary(_boundary);
    if (_firstPart) {
        _firstPart = false;
        boundary = boundary.substr(2);
    }

    for (;;) {
        const char* ptr = _buffer->getReadPointer();
        const char* matchPtr = reinterpret_cast<const char*>(memchr(ptr, boundary[0], _buffer->getBytesAvailable()));
        _buffer->setReadPointer(matchPtr ? matchPtr : _buffer->getTopPointer());
        if (_buffer->getBytesAvailable() < boundary.size() + 2) {
            if (!_buffer->requireNumberOfBytes(boundary.size() + 2, log)) {
                log->error(PRIME_LOCALISE("multipart content missing boundary"));
                return NULL;
            }

            continue;
        }

        if (memcmp(matchPtr, &boundary[0], boundary.size()) == 0) {
            _buffer->setReadPointer(matchPtr + boundary.size());

            if (memcmp(_buffer->getReadPointer(), "\r\n", 2) == 0) {
                _buffer->advanceReadPointer(2);
                return PassRef(new PartStream(this));
            }

            if (memcmp(_buffer->getReadPointer(), "--", 2) == 0) {
                _buffer->advanceReadPointer(2);
                _reachedEnd = true;
                return NULL;
            }

            log->error(PRIME_LOCALISE("multipart boundary missing newline"));
            return NULL;
        }

        // Consume the - and try again
        _buffer->setReadPointer(matchPtr + 1);
    }
}
}
