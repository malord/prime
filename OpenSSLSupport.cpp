// Copyright 2000-2021 Mark H. P. Lord

#include "OpenSSLSupport.h"

#ifndef PRIME_NO_OPENSSL

#include "Mutex.h"
#include "Thread.h"
#include <openssl/err.h>
#include <openssl/ssl.h>

#ifdef _MSC_VER
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "Ws2_32.lib")
#endif

#ifndef OPENSSL_THREADS
#error OpenSSL not built with thread support!
#endif

namespace Prime {

bool OpenSSLSupport::_initialised = false;
bool OpenSSLSupport::_ready = false;

static RecursiveMutex* GetOpenSSLSupportMutex()
{
    static RecursiveMutex mutex(Log::getGlobal());
    return &mutex;
}

static int numLocks = 0;
static Mutex* mutexes = NULL;

static unsigned long ThreadIDCallback()
{
#ifdef PRIME_OS_UNIX
    return (unsigned long)Thread::getCallingThreadID().getPthreadID();
#else
    return (unsigned long)Thread::getCallingThreadID();
#endif
}

static void LockingCallback(int mode, int type, const char* file, int line)
{
    (void)file;
    (void)line;
    PRIME_ASSERT(type < numLocks);
    if (mode & CRYPTO_LOCK) {
        mutexes[type].lock();
    } else {
        mutexes[type].unlock();
    }
}

bool OpenSSLSupport::initSSL(Log* log)
{
    RecursiveMutex::ScopedLock lock(GetOpenSSLSupportMutex());
    (void)log;

    if (_initialised) {
        return _ready;
    }

    numLocks = CRYPTO_num_locks();
    mutexes = new Mutex[numLocks];
    for (int i = 0; i != numLocks; ++i) {
        if (!mutexes[i].init(log)) {
            delete[] mutexes;
            mutexes = NULL;
            return false;
        }
    }

    CRYPTO_set_id_callback(&ThreadIDCallback);
    CRYPTO_set_locking_callback(&LockingCallback);

    SSL_load_error_strings();
    SSL_library_init();

    _initialised = true;
    _ready = true;

    log->trace("SSL library initialised.");

    return _ready;
}

void OpenSSLSupport::closeSSL(Log* log)
{
    RecursiveMutex::ScopedLock lock(GetOpenSSLSupportMutex());

    if (!_initialised) {
        return;
    }

    log->trace("SSL library shut down.");

    ERR_free_strings();

    _initialised = false;
    _ready = false;

    if (mutexes) {
        delete[] mutexes;
        mutexes = NULL;
    }
}

}

#endif // PRIME_NO_OPENSSL
