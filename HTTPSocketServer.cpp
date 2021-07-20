// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPSocketServer.h"

#if 0
#define HTTP_SOCKET_SERVER_ENABLE_STDERR_TRANTSCRIPT
#include "MultiStream.h"
#include "StdioStream.h"
#endif

namespace Prime {

HTTPSocketServer::HTTPSocketServer()
{
    _initialised = false;
}

HTTPSocketServer::~HTTPSocketServer()
{
}

void HTTPSocketServer::init(SocketListener* listener, SignalSocket* closeSignal, TaskQueue* taskQueue,
    TaskQueue::TaskGroup* taskGroup, HTTPServer* server, Settings* settings,
    Log* log, const ConnectionWrapper& sslWrapper)
{
    _log = log;
#ifdef PRIME_CXX11_STL
    _settingsObserver.init(settings, [this](Settings* s) {
        this->updateSettings(s);
    });
#else
    _settingsObserver.init(settings, MethodCallback(this, &HTTPSocketServer::updateSettings));
#endif
    _listener = listener;
    _closeSignal = closeSignal;
    _taskQueue = taskQueue;
    _taskGroup = taskGroup;
    _server = server;
    _sslWrapper = sslWrapper;

    _initialised = true;
}

void HTTPSocketServer::updateSettings(Settings* settings)
{
    _disableKeepAlive = settings->get("disableKeepAlive").toBool(false);
    _maxHeaderSizeInBytes = settings->get("maxHeaderSizeInBytes").toUInt(8192);
    _writeBufferSizeInBytes = settings->get("writeBufferSizeInBytes").toUInt(8192);
    _readTimeoutInMilliseconds = (int)(settings->get("readTimeoutInSeconds").toDouble(15.0) * 1000.0);
    _writeTimeoutInMilliseconds = (int)(settings->get("writeTimeoutInSeconds").toDouble(15.0) * 1000.0);
    _keepAliveTimeoutInMilliseconds = (int)(settings->get("keepAliveTimeoutInSeconds").toDouble(5.0) * 1000.0);
    _reverseLookup = settings->get("reverseLookup").toBool(true);
}

void HTTPSocketServer::run()
{
    PRIME_ASSERT(_initialised);

    for (;;) {
        ScopedPtr<Connection> connection(new Connection);
        if (!_listener->accept(connection->connection)) {
            break;
        }

        connection->connection.socket.setCloseSignal(_closeSignal);

        connection->httpSocketServer = this;
        if (_taskGroup) {
#ifdef PRIME_CXX11_STL
            Connection* connectionPointer = connection;
            _taskGroup->queue(_taskQueue, [connectionPointer]() {
                connectionPointer->run();
            });
#else
            _taskGroup->queue(_taskQueue, MethodCallback(connection.get(), &Connection::run));
#endif
        } else {
#ifdef PRIME_CXX11_STL
            Connection* connectionPointer = connection;
            _taskQueue->queue([connectionPointer]() {
                connectionPointer->run();
            });
#else
            _taskQueue->queue(MethodCallback(connection.get(), &Connection::run));
#endif
        }
        connection.detach();
    }

    //Trace("HTTPSocketServer terminated.");
}

//
// HTTPSocketServer::Connection
//

void HTTPSocketServer::Connection::run()
{
    char addressDescription[64];
#ifndef PRIME_NO_IP6
    char serviceDescription[64];
    if (!httpSocketServer->_reverseLookup || !connection.address.getNameInfo(addressDescription, sizeof(addressDescription), serviceDescription, sizeof(serviceDescription), 0, Log::getNullLog()))
#endif
    {
        connection.address.describe(addressDescription, sizeof(addressDescription));
    }

    PrefixLog log;
    log.setLog(httpSocketServer->_log);
    log.setPrefix(MakeString("Client ", addressDescription));

    if (httpSocketServer->_server->getVerboseLevel()) {
        log.trace("Connection opened.");
    }

    SocketStream socketStream(httpSocketServer->_readTimeoutInMilliseconds,
        httpSocketServer->_writeTimeoutInMilliseconds);
    socketStream.takeOwnership(connection.socket);

    RefPtr<NetworkStream> networkStream;
    const char* protocol;

    if (httpSocketServer->_sslWrapper) {
        RefPtr<Stream> stream = httpSocketServer->_sslWrapper(&socketStream, Log::getGlobal());
        if (!stream) {
            return;
        }

        networkStream = UIDCast<NetworkStream>(stream.get());
        protocol = "https";
    } else {
        networkStream = &socketStream;
        protocol = "http";
    }

#ifdef HTTP_SOCKET_SERVER_ENABLE_STDERR_TRANTSCRIPT
    MultiStream debug;
    debug.setReadMode(MultiStream::ReadModeWrite);
    debug.addStream(networkStream);
    debug.addStream(PassRef(new StdioStream(stderr, false)));
    debug.setReadStream(networkStream);
    networkStream = &debug;
#endif

    StreamBuffer readBuffer(networkStream, httpSocketServer->_maxHeaderSizeInBytes);
    StreamBuffer writeBuffer(networkStream, httpSocketServer->_writeBufferSizeInBytes);

    for (;;) {
        bool keepAlive = httpSocketServer->_server->serve(&readBuffer, &writeBuffer, protocol, log, &log, !httpSocketServer->_disableKeepAlive);
        writeBuffer.flush(log);

        if (!keepAlive) {
            break;
        }

        // This sucks - should be using epoll etc. for keep-alive'ing sockets

        NetworkStream::WaitResult waitResult;
        {
            TaskQueue::ScopedYield yield(httpSocketServer->_taskQueue);
            waitResult = networkStream->waitRead(httpSocketServer->_keepAliveTimeoutInMilliseconds, log);
        }

        if (waitResult != NetworkStream::WaitResultOK) {
            if (httpSocketServer->_server->getVerboseLevel()) {
                log->trace("Keep-alive socket not reused.");
            }
            break;
        }

        if (httpSocketServer->_server->getVerboseLevel()) {
            log->trace("Client reusing connection.");
        }
    }

    if (httpSocketServer->_server->getVerboseLevel()) {
        log.trace("Connection closed.");
    }

    delete this;
}
}
