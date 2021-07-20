// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_THREADSAFESTRINGSTREAM_H
#define PRIME_THREADSAFESTRINGSTREAM_H

#include "Mutex.h"
#include "Stream.h"

namespace Prime {

/// Wraps calls to an underlying Stream in mutex locks. readAtOffset() and writeAtOffset() become thread safe,
/// as does write() if no seeking is performed.
class PRIME_PUBLIC ThreadSafeStream : public Stream {
public:
    ThreadSafeStream();

    ~ThreadSafeStream();

    bool init(Stream* wrap, Log* log);

    /// Locks a ThreadSafeStream and provides direct access to the underlying Stream.
    /// e.g., FunctionThatUsesStream(ThreadSafeStream::ScopedLock(threadSafeStream))
    class ScopedLock {
    public:
        explicit ScopedLock(ThreadSafeStream* tss)
        {
            _tss = tss;
            _underlyingStream = tss->lockStream();
        }

        ScopedLock()
            : _tss(NULL)
            , _underlyingStream(NULL)
        {
        }

        ~ScopedLock() { unlock(); }

        Stream* lock(ThreadSafeStream* tss)
        {
            if (tss != _tss) {
                unlock();

                _tss = tss;
                _underlyingStream = tss->lockStream();
            }

            return _underlyingStream;
        }

        void unlock()
        {
            if (_tss) {
                _tss->unlockStream();
                _underlyingStream = NULL;
            }
        }

        Stream* get() const { return _underlyingStream; }

        operator Stream*() const { return _underlyingStream; }

        /// e.g., ThreadSafeStream::ScopedLock(tss)->write("Hello", 5, log)
        Stream* operator->() const { return _underlyingStream; }

    private:
        operator void*() const { return NULL; }

        ThreadSafeStream* _tss; // Not refcounted
        Stream* _underlyingStream;
    };

    // Stream implementation.
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t readAtOffset(Offset offset, void* buffer, size_t requiredBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeAtOffset(Offset offset, const void* bytes, size_t byteCount, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset size, Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;
    virtual bool copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize = 0, void* buffer = NULL) PRIME_OVERRIDE;
    virtual Stream* getUnderlyingStream() const PRIME_OVERRIDE { return _underlyingStream; }

private:
    /// Returns a pointer to the underlying stream, with the mutex held. Used by ScopedLock.
    Stream* lockStream();

    void unlockStream();

    friend class ScopedLock;

    RefPtr<Stream> _underlyingStream;
    RecursiveMutex _mutex;
};
}

#endif
