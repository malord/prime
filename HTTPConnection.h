// Copyright 2000-2021 Mark H. P. Lord

/*

    e.g.:

    Value LoadJSON(const URLView& url)
    {
        auto http = httpFactory->createConnection(url, log);
        if (! http) {
            return Value();
        }

        http->setMethod("GET");
        int responseCode = http->sendRequest(log);
        if (! IsHTTP2xx(responseCode)) {
            log->error(MakeString("HTTP ", responseCode, " ", http->getResponseCodeText()));
            return Value();
        }

        return JSONReader().read(http->getResponseContentStream(), log);
    }

*/

#ifndef PRIME_HTTPCONNECTION_H
#define PRIME_HTTPCONNECTION_H

#include "Dictionary.h"
#include "HTTP.h"
#include "Stream.h"
#include "URL.h"
#include <vector>

namespace Prime {

//
// HTTPConnection
//

/// Encapsulate a single HTTP request. An HTTPConnection is constructed by an HTTPConnectionFactory with a URL
/// to allow the factory to use connection pooling and share settings among connections.
class PRIME_PUBLIC HTTPConnection : public RefCounted {
public:
    /// This response code is returned for any errors that don't come from the server.
    enum { invalidHTTPResponseCode = 599 };

    virtual ~HTTPConnection();

    // Note that the URL is set by the HTTPConnectionFactory which creates this instance.

    virtual void setMethod(StringView method) = 0;

    virtual void setRequestHeader(StringView key, StringView value) = 0;

    /// Set the content to send with the request. The Stream should be rewindable.
    virtual void setRequestBody(Stream* stream) = 0;

    /// Send the request and return an HTTP response code or invalidHTTPResponseCode.
    virtual int sendRequest(Log* log) = 0;

    /// Returns the response code returned by sendRequest().
    virtual int getResponseCode() const = 0;

    /// Returns the reason string for the response code returned by sendRequest(), e.g., "Bad request".
    virtual StringView getResponseCodeText() const = 0;

    /// Returns the URL that was retrieved, which will differ if there was a redirect.
    virtual const URL& getResponseURL() const = 0;

    /// Returns a Stream from which you can read the response content.
    virtual RefPtr<Stream> getResponseContentStream() const = 0;

    /// Returns the length of the content, or -1 if not known. Returns < -1 on error.
    virtual int64_t getResponseContentLength() const = 0;

    /// Returns the content type.
    virtual StringView getResponseContentType() const = 0;

    /// Returns a response header.
    virtual StringView getResponseHeader(StringView key) = 0;

    /// Returns all response headers with the given name.
    virtual std::vector<StringView> getResponseHeaders(StringView key) = 0;

    virtual std::vector<StringView> getResponseHeaderNames() = 0;

    /// Close/abort this connection.
    virtual void close() = 0;

    //
    // Utility methods
    //

    /// Set the request body from a string.
    void setRequestBodyString(StringView string);

    /// Reads the entire response as a string.
    std::string getResponseContentString(Log* log);
};

//
// HTTPConnectionFactory
//

/// Creates HTTPConnections, sharing the same configuration and potentially using connection pooling.
class PRIME_PUBLIC HTTPConnectionFactory : public RefCounted {
public:
    virtual ~HTTPConnectionFactory();

    virtual RefPtr<HTTPConnection> createConnection(const URLView& url, Log* log) = 0;

    /// Not all implementation will support this, but they will hopefully support setMaxRedirects(0) at a minimum.
    virtual void setMaxRedirects(int maxRedirects) = 0;
};
}

#endif
