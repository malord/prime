// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTPSOCKETSERVER_H
#define PRIME_HTTPSOCKETSERVER_H

#include "HTTPServer.h"
#ifndef PRIME_CXX11_STL
#include "Callback.h"
#endif
#include "SignalSocket.h"
#include "SocketListener.h"
#include "SocketStream.h"
#include "TaskQueue.h"
#include <functional>

namespace Prime {

/// Accepts connections from a SocketListener then dispatches tasks on a TaskQueue which route the requests to an
/// HTTPServer, taking care of keep-alive.
class PRIME_PUBLIC HTTPSocketServer : public RefCounted {
public:
    // We don't need much stack.
    enum { threadSize = 16u * 1024u };

    HTTPSocketServer();

    ~HTTPSocketServer();

#ifdef PRIME_CXX11_STL
    typedef std::function<RefPtr<Stream>(Stream*, Log*)> ConnectionWrapper;
#else
    typedef Callback2<RefPtr<Stream>, Stream*, Log*> ConnectionWrapper;
#endif

    void init(SocketListener* listener, SignalSocket* closeSignal, TaskQueue* taskQueue,
        TaskQueue::TaskGroup* taskGroup, HTTPServer* server, Settings* settings, Log* log,
        const ConnectionWrapper& sslWrapper = ConnectionWrapper());

    void run();

private:
    void updateSettings(Settings* settings);

    class Connection {
    public:
        RefPtr<HTTPSocketServer> httpSocketServer;
        SocketListener::Connection connection;

        void run();
    };
    friend class Connection;

    Settings::Observer _settingsObserver;
    RefPtr<SocketListener> _listener;
    ConnectionWrapper _sslWrapper;
    RefPtr<HTTPServer> _server;
    RefPtr<TaskQueue> _taskQueue;
    RefPtr<TaskQueue::TaskGroup> _taskGroup;
    RefPtr<SignalSocket> _closeSignal;
    RefPtr<Log> _log;
    bool _initialised;

    bool _disableKeepAlive;
    bool _reverseLookup;
    size_t _maxHeaderSizeInBytes;
    size_t _writeBufferSizeInBytes;
    int _readTimeoutInMilliseconds;
    int _writeTimeoutInMilliseconds;
    int _keepAliveTimeoutInMilliseconds;
};
}

#endif
