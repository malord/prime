// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPHeaderBuilder.h"
#include "ChunkedReader.h"
#include "StringStream.h"
#include "Substream.h"
#include "TextEncoding.h"
#include <algorithm>

namespace Prime {

//
// HTTPHeaderBuilder
//

HTTPHeaderBuilder::HTTPHeaderBuilder()
{
    reset();
}

HTTPHeaderBuilder::~HTTPHeaderBuilder()
{
}

void HTTPHeaderBuilder::reset()
{
    _headers.clear();
    _method = HTTPMethodUnknown;
    _majorVersion = _minorVersion = -1;
    _responseCode = -1;
    _closed = false;
}

bool HTTPHeaderBuilder::loadFromParser(const HTTPParser& parser, Log* log, StringView protocol)
{
    PRIME_UNUSED(log);

    _method = parser.getMethod();
    _url = parser.getAbsoluteURL(protocol);
    _requestURL = parser.getRequestURL().toString();
    _majorVersion = parser.getMajorVersion();
    _minorVersion = parser.getMinorVersion();
    _responseCode = parser.getResponseCode();
    StringCopy(_responseCodeText, parser.getResponseCodeText());
    _closed = parser.connectionWasClosed();

    const ArrayView<const HTTPParser::Header>& allHeaders = parser.getAllHeaders();
    _headers.clear();
    for (size_t i = 0; i != allHeaders.size(); ++i) {
        const HTTPParser::Header& header = allHeaders[i];
        _headers.add(header.name, header.value);
    }

    return true;
}

bool HTTPHeaderBuilder::parse(HTTPParser::ParseMode mode, StreamBuffer* stream, Log* log, StringView protocol)
{
    HTTPParser parser;
    if (!parser.parse(mode, stream, false, log)) {
        _closed = parser.connectionWasClosed();
        return false;
    }

    return loadFromParser(parser, log, protocol);
}

bool HTTPHeaderBuilder::parse(HTTPParser::ParseMode mode, StringView source, Log* log, StringView protocol)
{
    HTTPParser parser;
    if (!parser.parse(mode, source, false, log)) {
        _closed = parser.connectionWasClosed();
        return false;
    }

    return loadFromParser(parser, log, protocol);
}

bool HTTPHeaderBuilder::isKeepAlive() const
{
    const std::string& header = get("Connection");

    if (isVersionOrNewer(1, 1)) {
        // As of HTTP/1.1, default Connection is keep-alive. It used to be close.
        if (header.empty()) {
            return true;
        }
    }

    return HTTPParser::equalHeaders(header, "keep-alive");
}

std::string HTTPHeaderBuilder::getRawHeaders() const
{
    StringStream stream;

    sendHeaders(&stream, Log::getNullLog());

    return PRIME_MOVE(stream.getString());
}

void HTTPHeaderBuilder::set(StringView name, const UnixTime& time)
{
    char buffer[128];
    if (PRIME_GUARD(DateTime(time).toRFC1123(buffer, sizeof(buffer)))) {
        set(name, buffer);
    }
}

bool HTTPHeaderBuilder::send(Stream* stream, Log* log) const
{
    if (isResponse()) {
        if (!stream->printf(log, "HTTP/1.1 %d %s\r\n", getResponseCode(), getResponseCodeText().c_str())) {
            return false;
        }
    } else {
        if (!stream->printf(log, "%s %s HTTP/1.1\r\n", GetHTTPMethodName(_method),
                _url.getResourceWithoutFragment().c_str())) {
            return false;
        }

        if (!has("Host")) {
            if (!stream->printf(log, "Host: %s\r\n", _url.getHostWithPort().c_str())) {
                return false;
            }
        }
    }

    return sendHeaders(stream, log);
}

bool HTTPHeaderBuilder::sendHeaders(Stream* stream, Log* log) const
{
    for (size_t i = 0; i != _headers.getSize(); ++i) {
        const URLDictionary::value_type& pair = _headers.pair(i);
        if (!stream->printf(log, "%s: %s\r\n", pair.first.c_str(), pair.second.c_str())) {
            return false;
        }
    }

    if (!stream->printf(log, "\r\n")) {
        return false;
    }

    return true;
}

RefPtr<Stream> HTTPHeaderBuilder::readBody(StreamBuffer* networkStream, Log* log)
{
    if (ASCIIEqualIgnoringCase(get("transfer-encoding"), "chunked")) {
        if (!StringIsWhitespace(get("trailing"))) {
            // TODO: deal with Trailing headers
            log->error(PRIME_LOCALISE("Trailing headers not supported."));
            return NULL;
        }
        return PassRef(new ChunkedReader(networkStream));
    }

    int64_t length = -1;
    if (StringToInt(get("content-length"), length, 10) && length >= 0) {
        return PassRef(new Substream(networkStream, 0, false, length, log, false));
    }

    const std::string& connection = get("connection");
    if (ASCIIEqualIgnoringCase(connection, "close") || connection.empty()) {
        return networkStream;
    }

    log->error(PRIME_LOCALISE("Unable to read response."));
    return NULL;
}

void HTTPHeaderBuilder::setResponse(int responseCode, StringView responseCodeText, bool keepalive)
{
    setResponseCode(responseCode);
    StringCopy(_responseCodeText, responseCodeText);
    set("Connection", keepalive ? "keepalive" : "close");
}

bool HTTPHeaderBuilder::sendResponse(Stream* stream, Log* log, int responseCode, StringView responseCodeText,
    StringView data, bool keepalive)
{
    setResponse(responseCode, responseCodeText, keepalive);
    set("Content-Length", ToString(data.size()));
    return send(stream, log) && stream->writeExact(data.data(), data.size(), log);
    ;
}
}
