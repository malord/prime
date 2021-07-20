// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MULTISTREAM_H
#define PRIME_MULTISTREAM_H

#include "NetworkStream.h"
#include <vector>

namespace Prime {

/// Route writes to multiple Streams. Useful for making a debug copy of network communications.
class PRIME_PUBLIC MultiStream : public NetworkStream {
public:
    MultiStream();

    /// Clear the list of Streams.
    void reset();

    void addStream(Stream* stream);

    void removeStream(size_t index);

    size_t getStreamCount() const;

    Stream* getStream(size_t index) const;

    void setStream(size_t index, Stream* stream);

    /// Reads come from this stream only. If no read stream is set, reading is disabled. The read stream can also
    /// be in the list of streams (so it receives writes, seeks, setSize and close).
    void setReadStream(Stream* value) { _readStream = value; }

    Stream* getReadStream() const { return _readStream; }

    enum ReadMode {
        /// Read from the read stream then skip forward in all the other streams. The default.
        ReadModeSkip,

        /// Read from the read stream and write what was read to the other streams. For logging transcripts.
        ReadModeWrite
    };

    void setReadMode(ReadMode value) { _readMode = value; }

    // Stream implementation.
    virtual ptrdiff_t readSome(void* buffer, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset size, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;

    // NetworkStream implementation.
    virtual void setReadTimeout(int milliseconds) PRIME_OVERRIDE;
    virtual int getReadTimeout() const PRIME_OVERRIDE;
    virtual void setWriteTimeout(int milliseconds) PRIME_OVERRIDE;
    virtual int getWriteTimeout() const PRIME_OVERRIDE;
    virtual WaitResult waitRead(int milliseconds, Log* log) PRIME_OVERRIDE;
    virtual WaitResult waitWrite(int milliseconds, Log* log) PRIME_OVERRIDE;

private:
    std::vector<RefPtr<Stream>> _streams;
    RefPtr<Stream> _readStream;

    ReadMode _readMode;

    PRIME_UNCOPYABLE(MultiStream);
};
}

#endif
