// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_HASHSTREAM_H
#define PRIME_HASHSTREAM_H

#include "Config.h"
#include "Stream.h"

namespace Prime {

/// A Stream implementation that computes a hash for all bytes that pass through it. If the hash and size of the
/// data are known, the hash is verified when the last byte has been read/written. If the hash is known but the
/// size of the data is not, then the hash is verified either when an attempt is made to read past the end of
/// the file, or when close() is called. If the hash is not known, this class can be used to compute it.
/// Hasher can be any type which has a reset() method, a process() method which runs an array of bytes through
/// the hash algorithm, and a get() method which returns the current hash.
template <typename Hasher> // e.g., HashStream<SHA256>
class HashStream : public Stream {
public:
    HashStream() { disableVerification(); }

    ~HashStream() { }

    /// Set the underlying Stream.
    explicit HashStream(Stream* stream) PRIME_NOEXCEPT
    {
        disableVerification();
        setStream(stream);
    }

    /// Set the underlying Stream. This does not reset the hash.
    void setStream(Stream* stream) PRIME_NOEXCEPT { _stream = stream; }

    /// Set the known-good hash. When the end of the file is reached, or when we're destructed or closed,
    /// the hash is verified. If size >= 0 then the hash will be tested after that number of bytes have been
    /// read/written.
    void beginVerification(typename Hasher::Result correctHash, Offset size = -1) PRIME_NOEXCEPT
    {
        _verify = true;
        _correctHash = correctHash;
        _sizeKnown = size >= 0;
        _knownSize = size;
        _sizeSoFar = 0;
        _errorFound = false;
    }

    /// Verification will be disabled unless you've called beginVerification().
    void disableVerification() PRIME_NOEXCEPT
    {
        _verify = false;
        _sizeKnown = false;
        _errorFound = false;
    }

    bool isVerifying() const PRIME_NOEXCEPT { return _verify; }

    bool isSizeKnown() const PRIME_NOEXCEPT { return _sizeKnown; }

    /// Retrieve the hash up to this point.
    typename Hasher::Result getHash() const { return _hasher.get(); }

    /// Reset the hash to its initial value.
    void resetHash() PRIME_NOEXCEPT { _hasher.reset(); }

    // Stream overrides.
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE
    {
        PRIME_ASSERT(_stream.get());

        if (_errorFound) {
            return -1;
        }

        ptrdiff_t result = _stream->readSome(buffer, maximumBytes, log);

        if (result < 0) {
            return result;
        }

        if (result == 0) {
            if (verifyAtEnd() && !verifyHash(log)) {
                return -1;
            }

        } else {
            if (!updateHash(buffer, result, log)) {
                return -1;
            }
        }

        return result;
    }

    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE
    {
        PRIME_ASSERT(_stream.get());

        if (_errorFound) {
            return -1;
        }

        ptrdiff_t result = _stream->writeSome(memory, maximumBytes, log);

        if (result < 0) {
            return result;
        }

        if (!updateHash(memory, result, log)) {
            return -1;
        }

        return result;
    }

    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE
    {
        PRIME_ASSERT(_stream.get());

        // Seeking is not a problem if we're just getting the offset.
        if (mode != SeekModeRelative || offset != 0) {
            if (_sizeKnown) {
                log->trace(PRIME_LOCALISE("Hash verification disabled due to seek."));
                disableVerification();
            }
        }

        return _stream->seek(offset, mode, log);
    }

    virtual Offset getSize(Log* log) PRIME_OVERRIDE
    {
        PRIME_ASSERT(_stream.get());

        return _stream->getSize(log);
    }

    virtual bool setSize(Offset size, Log* log) PRIME_OVERRIDE
    {
        PRIME_ASSERT(_stream.get());

        return _stream->setSize(size, log);
    }

    virtual bool flush(Log* log) PRIME_OVERRIDE
    {
        PRIME_ASSERT(_stream.get());

        if (!_stream->flush(log)) {
            return false;
        }

        return !_errorFound;
    }

    /// Verify the checksum without closing the underlying stream.
    bool end(Log* log)
    {
        PRIME_ASSERT(_stream);

        bool success = true;

        if (verifyAtEnd() && !verifyHash(log)) {
            success = false;
        }

        disableVerification();
        return success;
    }

    virtual bool close(Log* log) PRIME_OVERRIDE
    {
        if (!_stream) {
            return true;
        }

        bool success = _stream->close(log);
        _stream.release();

        if (verifyAtEnd() && !verifyHash(log)) {
            success = false;
        }

        if (_errorFound) {
            success = false;
        }

        return success;
    }

private:
    bool updateHash(const void* bytes, size_t byteCount, Log* log)
    {
        _hasher.process(bytes, byteCount);

        if (_verify && _sizeKnown) {
            _sizeSoFar += byteCount;
            if (_sizeSoFar == _knownSize) {
                return verifyHash(log);
            }

            if (_sizeSoFar > _knownSize) {
                if (!_errorFound) {
                    _errorFound = true;
                    log->error(PRIME_LOCALISE("Data is corrupt (incorrect length)."));
                    return false;
                }
            }
        }

        return true;
    }

    bool verifyHash(Log* log) const
    {
        PRIME_ASSERT(_verify);

        if (getHash() == _correctHash) {
            return true;
        }

        if (!_errorFound) {
            _errorFound = true;
            log->error(PRIME_LOCALISE("Data is corrupt (hash mismatch)."));
        }

        return false;
    }

    bool verifyAtEnd() const PRIME_NOEXCEPT { return _verify && !_sizeKnown; }

    RefPtr<Stream> _stream;
    bool _verify;
    typename Hasher::Result _correctHash;
    mutable bool _errorFound; // Never log more than one error.
    Offset _knownSize;
    Offset _sizeSoFar;
    bool _sizeKnown;

    Hasher _hasher;

    PRIME_UNCOPYABLE(HashStream);
};
}

#endif
