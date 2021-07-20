// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MULTIPARTPARSER_H
#define PRIME_MULTIPARTPARSER_H

#include "StreamBuffer.h"

namespace Prime {

/// Parse mutlipart/form-data and multipart/mixed|alternative
class PRIME_PUBLIC MultipartParser : public RefCounted {
public:
    enum { defaultBufferSize = 65536u };

    /// media-type     = type "/" subtype *( ";" parameter )
    /// type           = token
    /// subtype        = token
    /// parameter      = attribute "=" value
    /// attribute      = token
    /// value          = token | quoted-string
    static std::string parseBoundary(StringView contentTypeHeader);

    MultipartParser();

    ~MultipartParser();

    /// After calling this method, you should call readPart() to get a Stream that can be used to read up to
    /// the next part.
    bool init(StreamBuffer* buffer, StringView boundary, Log* log);

    /// Wrapper around init(buffer) which creates a StreamBuffer.
    bool init(Stream* stream, StringView boundary, size_t bufferSize, Log* log);

    /// Returns a Stream that can be used to read the next part, or NULL if there's an error or there are no
    /// more parts. Use atEnd() to determine whether readPart() succeeded. Note that the part will include the
    /// headers.
    RefPtr<Stream> readPart(Log* log);

    /// Returns true if we reached the end marker.
    bool atEnd() { return _reachedEnd; }

private:
    class PartStream;
    friend class PartStream;

    std::string _boundary;
    RefPtr<StreamBuffer> _buffer;
    bool _firstPart;
    bool _reachedEnd;
};
}

#endif
