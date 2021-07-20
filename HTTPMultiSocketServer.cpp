// Copyright 2000-2021 Mark H. P. Lord

#include "HTTPMultiSocketServer.h"
#include "HTTPServer.h"
#include "SocketListener.h"

namespace Prime {

HTTPMultiSocketServer::HTTPMultiSocketServer()
{
}

HTTPMultiSocketServer::~HTTPMultiSocketServer()
{
}

bool HTTPMultiSocketServer::init(HTTPServer::Handler* handler, TaskQueue* taskQueue, Settings* settings, Log* log,
    const HTTPSocketServer::ConnectionWrapper& sslWrapper)
{
    _taskQueue = taskQueue;
    _handler = handler;

    bool allowNonSSL = settings->get("allowNonSSL").toBool(false);

    _taskGroup = _taskQueue->createTaskGroup();
    if (!_taskGroup) {
        return false;
    }

    if (!_closeSignal.init(log)) {
        log->error(PRIME_LOCALISE("Couldn't configure the close signal socket."));
        return false;
    }

    _terminationCallbacks += MethodCallback(&_closeSignal, static_cast<void (SignalSocket::*)()>(&SignalSocket::signal));

    Value::Vector addresses = settings->get("address").toVector();

    if (addresses.empty()) {
        log->note("No addresses specified in configuration - using INADDR_ANY:8000");
        addresses.push_back("0.0.0.0:8000");
    }

    RefPtr<Settings> socketServerSettings = settings->getSettings("Server");
    const int defaultThreadCount = socketServerSettings->get("threadCount").toInt(1);
    const int redirectThreadCount = socketServerSettings->get("redirectThreadCount").toInt(1);
    const int loopbackThreadCount = socketServerSettings->get("loopbackThreadCount").toInt(1);

    bool foundDefaultLoopbackAddress = false;
    SocketAddress defaultLoopbackAddress;

    for (size_t addressIndex = 0; addressIndex != addresses.size(); ++addressIndex) {
        Value addressValue = addresses[addressIndex];
        const Value::Dictionary& addressDictionary = addressValue.getDictionary();

        std::string address;
        bool ssl = false;
        bool loopback = false;
        Value::Dictionary redirect;
        int threadCount = 1;

        if (addressDictionary.empty()) {
            address = addressValue.toString();
            threadCount = defaultThreadCount;
        } else {
            address = addressDictionary["address"].toString();
            if (address.empty()) {
                log->error(PRIME_LOCALISE("Invalid address: %s"), addressValue.toString().c_str());
                return false;
            }

            ssl = addressDictionary["ssl"].toBool(false);

            redirect = addressDictionary["redirect"].getDictionary();

            loopback = addressDictionary["loopback"].toBool(false);

            int thisDefaultThreadCount;
            if (!redirect.empty()) {
                thisDefaultThreadCount = redirectThreadCount;
            } else if (loopback) {
                thisDefaultThreadCount = loopbackThreadCount;
            } else {
                thisDefaultThreadCount = defaultThreadCount;
            }

            threadCount = ToInt(addressDictionary["threadCount"], thisDefaultThreadCount);
        }

        if (allowNonSSL && !ssl && redirect["protocol"].toString() == "https") {
            redirect = Value::Dictionary();
        }

        RefPtr<SocketListener> listener = PassRef(new SocketListener);
        std::vector<std::string> connectTo;
        if (!listener->init(address.c_str(),
                SocketListener::Options().setDefaultPort(80).setCloseSignal(&_closeSignal),
                log, &connectTo)) {
            return false;
        }

        if (!loopback && !ssl) {
            std::string message = "Connect to: ";
            for (size_t i = 0; i != connectTo.size(); ++i) {
                if (i != 0) {
                    if (i == connectTo.size() - 1) {
                        message += " or ";
                    } else {
                        message += ", ";
                    }
                }

                message += "http://";
                message += connectTo[i];
            }

            _connectMessage.swap(message);
        }

        if (loopback) {
            _hasLoopbackAddress = true;
            _loopbackAddress = listener->getLocalAddress();
        }

        if (!ssl && listener->getLocalAddress().isLocalhost()) {
            defaultLoopbackAddress = listener->getLocalAddress();
            foundDefaultLoopbackAddress = true;
        }

        RefPtr<HTTPServer::Handler> thisHandler;

        if (!redirect.empty()) {
            RefPtr<HTTPServer::Redirecter> redirecter = PassRef(new HTTPServer::Redirecter);
            redirecter->setPort(redirect["port"].toInt(-1));
            redirecter->setProtocol(redirect["protocol"].getString());

            thisHandler = redirecter;
        } else {
            thisHandler = _handler;
        }

        RefPtr<HTTPServer> server = PassRef(new HTTPServer);
        if (!server->init(thisHandler, log, settings->getSettings("HTTPServer"))) {
            return false;
        }

        RefPtr<HTTPSocketServer> socketServer = PassRef(new HTTPSocketServer);
        socketServer->init(listener, &_closeSignal, _taskQueue, _taskGroup, server,
            settings->getSettings("HTTPSocketServer"), log,
            ssl ? sslWrapper : HTTPSocketServer::ConnectionWrapper());

        for (int i = 0; i != threadCount; ++i) {
            RefPtr<Thread> thread = PassRef(new Thread);
            if (!thread->create(MethodCallback(socketServer, &HTTPSocketServer::run),
                    HTTPSocketServer::threadSize, log, "HTTPSocketServer")) {
                log->error(PRIME_LOCALISE("Couldn't create thread."));
                return false;
            }
            _socketServerThreads.push_back(thread);
        }
    }

    if (foundDefaultLoopbackAddress) {
        char buf[128];
        if (!defaultLoopbackAddress.describe(buf, sizeof(buf))) {
            buf[0] = 0;
        }
        _hasLoopbackAddress = true;
        _loopbackAddress = defaultLoopbackAddress;
        log->trace("Using default loopback address: %s", buf);
    }

    return true;
}

void HTTPMultiSocketServer::close(Log* log)
{
    _terminationCallbacks();

    log->verbose("No longer accepting new connections, waiting for existing connections...");

    if (_taskGroup) {
        _taskGroup->wait();
    }

    while (!_socketServerThreads.empty()) {
        _socketServerThreads.back()->join();
        _socketServerThreads.pop_back();
    }

    log->verbose("All connections closed.");
}
}
