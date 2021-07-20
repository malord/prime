// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HTTPMULTISOCKETSERVER_H
#define PRIME_HTTPMULTISOCKETSERVER_H

#include "Callback.h"
#include "HTTPServer.h"
#include "HTTPSocketServer.h"
#include "Settings.h"
#include "SignalSocket.h"
#include "TaskQueue.h"
#include "Thread.h"

namespace Prime {

/// Given a Settings which contains a server configuration, sets up one or more HTTPServers listening on threads
/// and pushing requests to a Handler.
class PRIME_PUBLIC HTTPMultiSocketServer {
public:
    HTTPMultiSocketServer();

    ~HTTPMultiSocketServer();

    bool init(HTTPServer::Handler* handler, TaskQueue* taskQueue, Settings* settings, Log* log,
        const HTTPSocketServer::ConnectionWrapper& sslWrapper = HTTPSocketServer::ConnectionWrapper());

    void close(Log* log);

    HTTPServer::Handler* getHandler() { return _handler; }

    bool hasLoopbackAddress() const { return _hasLoopbackAddress; }
    const SocketAddress& getLoopbackAddress() const
    {
        PRIME_ASSERT(_hasLoopbackAddress);
        return _loopbackAddress;
    }

    /// After calling init, call this to get a message to display to the console telling the user where to connect.
    const char* getConnectMessage() const { return _connectMessage.c_str(); }

private:
    CallbackList0 _terminationCallbacks;
    RefPtr<HTTPServer::Handler> _handler;
    RefPtr<TaskQueue> _taskQueue;
    RefPtr<TaskQueue::TaskGroup> _taskGroup;
    SignalSocket _closeSignal;
    std::vector<RefPtr<Thread>> _socketServerThreads;
    bool _hasLoopbackAddress;
    SocketAddress _loopbackAddress;
    std::string _connectMessage;
};
}

#endif
