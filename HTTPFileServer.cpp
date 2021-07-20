// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPFileServer.h"
#include "Path.h"
#include "PrefixLog.h"
#include "StringStream.h"
#include "TextEncoding.h"

namespace Prime {

//
// HTTPFileServer::Options
//

HTTPFileServer::Options::Options()
    : _expirationSeconds(0)
    , _directoryListings(true)
    , _useGZip(true)
{
}

HTTPFileServer::Options& HTTPFileServer::Options::setMIMETypes(const MIMETypes* mimeTypes)
{
    _mimeTypes = mimeTypes;
    return *this;
}

//
// HTTPFileServer
//

HTTPFileServer::HTTPFileServer()
{
}

HTTPFileServer::~HTTPFileServer()
{
}

void HTTPFileServer::init(FileSystem* fileSystem, const Options& options)
{
    _fileSystem = fileSystem;
    _options = options;

    if (!_options.getMIMETypes()) {
        _options.setMIMETypes(PassRef(new MIMETypes));
    }
}

bool HTTPFileServer::handleRequest(Request& request, Response& response)
{
    std::string path = request.getRemainingPathString();

    FileSystem::FileProperties fileProperties;
    if (!_fileSystem->test(path.c_str(), &fileProperties)) {
        if (!path.empty()) {
            return false;
        }
    }

    if (request.getPath().isDirectory()) {
        if (!path.empty() && !fileProperties.isDirectory) {
            response.error(request, 404);
            return true;
        }

        static const char* indexNames[] = { "index.txt", "index.htm", "index.html" };

        for (size_t indexIndex = 0; indexIndex != COUNTOF(indexNames); ++indexIndex) {
            std::string indexPath = Path::join(path, indexNames[indexIndex]);
            if (sendFile(request, response, indexPath.c_str())) {
                return true;
            }
        }

        if (!_options.getDirectoryListingsEnabled()) {
            response.error(request, 404);
            return true;
        }

        sendDirectoryListing(request, response, path.c_str());
        return true;
    }

    if (fileProperties.isDirectory) {
        response.redirect(request.getPath().toDirectory().toString().c_str());
        return true;
    }

    if (!sendFile(request, response, path.c_str(), &fileProperties)) {
        response.error(request, 404);
        return true;
    }

    return true;
}

bool HTTPFileServer::sendFile(Request& request, Response& response, const char* path,
    const FileSystem::FileProperties* knownProps)
{
    RefPtr<Stream> stream;
    FileSystem::FileProperties fileProperties;
    Response::SendStreamOptions sendOptions;

    if (!knownProps) {
        knownProps = &fileProperties;
        if (!_fileSystem->test(path, &fileProperties)) {
            return false;
        }
    }

    if (response.shouldGZip() && knownProps->crc32 && knownProps->compressionMethod && knownProps->compressionMethod.value() == FileSystem::CompressionMethodDeflate) {

        stream = _fileSystem->open(path, OpenMode().setRead().setBufferSequential(), PrefixLog(request.getLog(), path),
            FileSystem::OpenOptions().setDoNotDecompress().setDoNotVerifyChecksum(),
            &fileProperties);
        if (!stream) {
            response.error(request, 500);
            return true;
        }
        sendOptions.setAlreadyCompressed(true);
        sendOptions.setRawDeflated(true);
        sendOptions.setCRC32(fileProperties.crc32.value());

    } else {
        // openBufferSequential is required for maximum performance from Windows' TransmitFile API.

        if (_options.shouldUseGZip() && response.shouldGZip()) {
            std::string gzPath = path;
            gzPath += ".gz";
            if (_fileSystem->test(gzPath.c_str(), &fileProperties)) {
                stream = _fileSystem->open(gzPath.c_str(),
                    OpenMode().setRead().setBufferSequential(),
                    PrefixLog(request.getLog(), gzPath.c_str()),
                    FileSystem::OpenOptions(), &fileProperties);
                if (!stream) {
                    response.error(request, 500);
                    return true;
                }

                response.setHeader("Content-Encoding", "gzip");
                sendOptions.setAlreadyCompressed(true);
            }
        }

        if (!stream) {
            stream = _fileSystem->open(path, OpenMode().setRead().setBufferSequential(),
                PrefixLog(request.getLog(), path), FileSystem::OpenOptions(), &fileProperties);
            if (!stream) {
                response.error(request, 500);
                return true;
            }
        }
    }

    sendStream(request, response, stream, path, fileProperties, *_options.getMIMETypes(), _options.getExpirationSeconds(), sendOptions);
    return true;
}

void HTTPFileServer::sendDirectoryListing(Request& request, Response& response, const char* path)
{
    StringStream directoryStream;

    // Build a directory listing.
    RefPtr<FileSystem::DirectoryReader> dir = _fileSystem->readDirectory(path, PrefixLog(request.getLog(), path));
    if (!dir) {
        response.error(request, 404);
        return;
    }

    FileSystem::FileProperties fileProperties;
    if (!_fileSystem->test(path, &fileProperties)) {
        fileProperties = FileSystem::FileProperties();
    }

    directoryStream.printf(request.getLog(),
        "<!DOCTYPE html>"
        "<html>"
        "  <head>"
        "  </head>"
        "  <body>"
        "    <table border=\"0\">");

    if (path && *path) {
        directoryStream.printf(request.getLog(), "      <tr><td><a href=\"..\">.. (up)</a></td></tr>");
    }

    while (dir->read(request.getLog())) {
        if (dir->isHidden()) {
            continue;
        }

        std::string name(dir->getName());
        if (name == "." || name == "..") {
            continue;
        }

        const char* slash = dir->isDirectory() ? "/" : "";
        std::string urlName = URLEncode(name);
        std::string htmlName = HTMLEscape(name);
        directoryStream.printf(request.getLog(), "      <tr><td><a href=\"%s%s\">%s%s</a></td></tr>", urlName.c_str(), slash, htmlName.c_str(), slash);
    }

    directoryStream.printf(request.getLog(),
        "    </table>"
        "  </body>"
        "</html>");

    directoryStream.setOffset(0, request.getLog());

    sendStream(request, response, &directoryStream, "index.html", fileProperties, *_options.getMIMETypes(), _options.getExpirationSeconds(), Response::SendStreamOptions());
}

bool HTTPFileServer::sendStream(Request& request, Response& response, Stream* stream, const char* filename,
    const FileSystem::FileProperties& fileProperties, const MIMETypes& mimeTypes,
    int expireAfterSeconds, Response::SendStreamOptions sendOptions)
{
    if (!stream) {
        response.error(request, 404);
        return false;
    }

    // TODO: check If-Newer, range, etc. in the request!
    (void)request;

    //
    // "Content-Type" header
    //

    std::string extension = Path::exetension(filename);

    StringView mimeType = mimeTypes.getMIMETypeForExtension(extension);
    if (!mimeType.empty()) {
        response.setHeader("Content-Type", mimeType);
    }

    if (mimeTypes.isCompressedExtension(extension)) {
        sendOptions.setAlreadyCompressed(true);
    }

    //
    // "Last-Modified", "Date", "Expires" and "Cache-Control: max-age=" headers
    //

    if (fileProperties.modificationTime) {
        response.setHeader("Last-Modified", fileProperties.modificationTime.value());
    }

    response.setExpirationSeconds(expireAfterSeconds);

    //
    // "ETag" header (construct it from the modification date/time)
    //

    if (fileProperties.modificationTime) {
        char timestr[128];
        if (PRIME_GUARD(DateTime(fileProperties.modificationTime.value()).toRFC1123(timestr, sizeof(timestr)))) {
            char base64[192];
            size_t encodedSize = Base64Encode(base64, sizeof(base64), timestr, strlen(timestr), 0);
            if (PRIME_GUARD(encodedSize < sizeof(base64))) {
                base64[encodedSize] = 0;
            } else {
                base64[sizeof(base64) - 1] = 0;
            }
            response.setHeader("ETag", base64);
        }
    }

    //
    // Send the stream
    //

    return response.sendStream(stream, request.getLog(), sendOptions);
}
}
