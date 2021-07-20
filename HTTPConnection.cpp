// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPConnection.h"
#include "StringStream.h"
#include <algorithm>

namespace Prime {

//
// HTTPConnectionFactory
//

HTTPConnectionFactory::~HTTPConnectionFactory()
{
}

//
// HTTPConnection
//

HTTPConnection::~HTTPConnection()
{
}

void HTTPConnection::setRequestBodyString(StringView string)
{
    setRequestBody(PassRef(new StringStream(string)));
}

std::string HTTPConnection::getResponseContentString(Log* log)
{
    std::string response;

    StringStream responseStream;
    if (RefPtr<Stream> stream = getResponseContentStream()) {
        if (!responseStream.copyFrom(stream, log, std::max<int64_t>(-1, getResponseContentLength()), log)) {
            return "";
        }
    }

    responseStream.getString().swap(response);

    return response;
}
}
