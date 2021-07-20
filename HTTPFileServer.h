// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTPFILESERVER_H
#define PRIME_HTTPFILESERVER_H

#include "FileSystem.h"
#include "HTTPServer.h"
#include "MIMETypes.h"
#include <set>

namespace Prime {

/// A static file server that can be plugged in to an HTTPServer.
class PRIME_PUBLIC HTTPFileServer : public HTTPServer::Handler {
public:
    typedef HTTPServer::Request Request;
    typedef HTTPServer::Response Response;

    static bool sendStream(Request& request, Response& response, Stream* stream, const char* filename,
        const FileSystem::FileProperties& fileProperties, const MIMETypes& mimeTypes,
        int expireAfterSeconds, Response::SendStreamOptions sendOptions);

    HTTPFileServer();

    ~HTTPFileServer();

    class PRIME_PUBLIC Options {
    public:
        Options();

        Options& setExpirationSeconds(int value)
        {
            _expirationSeconds = value;
            return *this;
        }
        int getExpirationSeconds() const { return _expirationSeconds; }

        Options& setDirectoryListingsEnabled(bool value)
        {
            _directoryListings = value;
            return *this;
        }
        bool getDirectoryListingsEnabled() const { return _directoryListings; }

        Options& setMIMETypes(const MIMETypes* mimeTypes);
        const MIMETypes* getMIMETypes() const { return _mimeTypes; }

        /// If enabled, any file request for filename.ext will first search for filename.ext.gz, and send that
        /// instead. Enabled by default.
        Options& setUseGZip(bool value)
        {
            _useGZip = value;
            return *this;
        }
        bool shouldUseGZip() const { return _useGZip; }

    private:
        int _expirationSeconds;
        bool _directoryListings;
        RefPtr<const MIMETypes> _mimeTypes;
        bool _useGZip;
    };

    void init(FileSystem* fileSystem, const Options& options);

    bool getDirectoryListingsEnabled() const { return _options.getDirectoryListingsEnabled(); }

    /// Sends a file, preferring to send a .gz compressed version if it exists first.
    bool sendFile(Request& request, Response& response, const char* path,
        const FileSystem::FileProperties* knownProps = NULL);

    void sendDirectoryListing(Request& request, Response& response, const char* path);

    // Handler implementation.
    virtual bool handleRequest(Request& request, Response& response) PRIME_OVERRIDE;

private:
    RefPtr<FileSystem> _fileSystem;
    Options _options;
};
}

#endif
