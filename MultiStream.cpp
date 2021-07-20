// Copyright 2000-2021 Mark H. P. Lord

#include "MultiStream.h"
#include <algorithm>

namespace Prime {

MultiStream::MultiStream()
{
    _readMode = ReadModeSkip;
}

void MultiStream::reset()
{
    _streams.clear();
}

void MultiStream::addStream(Stream* stream)
{
    _streams.push_back(stream);
}

size_t MultiStream::getStreamCount() const
{
    return _streams.size();
}

void MultiStream::removeStream(size_t index)
{
    _streams.erase(_streams.begin() + index);
}

Stream* MultiStream::getStream(size_t index) const
{
    return _streams[index];
}

void MultiStream::setStream(size_t index, Stream* stream)
{
    _streams.at(index) = stream;
}

ptrdiff_t MultiStream::readSome(void* buffer, size_t maximumBytes, Log* log)
{
    if (!_readStream) {
        return Stream::readSome(buffer, maximumBytes, log);
    }

    ptrdiff_t got = _readStream->readSome(buffer, maximumBytes, log);

    if (got <= 0) {
        return got;
    }

    ptrdiff_t result = got;

    if (_readMode == ReadModeSkip) {
        for (size_t i = 0; i < _streams.size(); ++i) {
            if (_streams[i] == _readStream) {
                continue;
            }
            if (!_streams[i]->skip(got, log)) {
                result = -1;
            }
        }

    } else if (_readMode == ReadModeWrite) {
        for (size_t i = 0; i < _streams.size(); ++i) {
            if (_streams[i] != _readStream) {
                if (!_streams[i]->writeExact(buffer, got, log)) {
                    result = -1;
                }
            }
        }
    }

    return result;
}

ptrdiff_t MultiStream::writeSome(const void* memory, size_t maximumBytes, Log* log)
{
    if (_streams.empty()) {
        return Stream::writeSome(memory, maximumBytes, log);
    }

    ptrdiff_t wrote = _streams[0]->writeSome(memory, maximumBytes, log);

    if (wrote <= 0) {
        return wrote;
    }

    bool ok = true;

    for (size_t i = 1; i < _streams.size(); ++i) {
        if (!_streams[i]->writeExact(memory, wrote, log)) {
            ok = false;
        }
    }

    return ok ? wrote : -1;
}

Stream::Offset MultiStream::seek(Offset offset, SeekMode mode, Log* log)
{
    Offset result = 0;

    for (size_t i = 0; i != _streams.size(); ++i) {
        Offset thisResult = _streams[i]->seek(offset, mode, log);
        if (thisResult < 0) {
            result = -1;
        } else if (result == 0) {
            result = thisResult;
        }
    }

    return result;
}

Stream::Offset MultiStream::getSize(Log* log)
{
    if (!_streams.empty()) {
        return _streams[0]->getSize(log);
    }

    return Stream::getSize(log);
}

bool MultiStream::setSize(Offset size, Log* log)
{
    bool ok = true;

    for (size_t i = 0; i != _streams.size(); ++i) {
        if (!_streams[i]->setSize(size, log)) {
            ok = false;
        }
    }

    _streams.clear();
    return ok;
}

bool MultiStream::close(Log* log)
{
    bool ok = true;

    for (size_t i = 0; i != _streams.size(); ++i) {
        if (!_streams[i]->close(log)) {
            ok = false;
        }
    }

    _streams.clear();
    return ok;
}

void MultiStream::setReadTimeout(int milliseconds)
{
    if (NetworkStream* readStreamNS = UIDCast<NetworkStream>(_readStream.get())) {
        readStreamNS->setReadTimeout(milliseconds);
    }
}

int MultiStream::getReadTimeout() const
{
    if (const NetworkStream* readStreamNS = UIDCast<const NetworkStream>(_readStream.get())) {
        return readStreamNS->getReadTimeout();
    }

    return -1;
}

void MultiStream::setWriteTimeout(int milliseconds)
{
    for (size_t i = 0; i != _streams.size(); ++i) {
        if (NetworkStream* networkStream = UIDCast<NetworkStream>(_streams[i].get())) {
            networkStream->setWriteTimeout(milliseconds);
        }
    }
}

int MultiStream::getWriteTimeout() const
{
    for (size_t i = 0; i != _streams.size(); ++i) {
        if (const NetworkStream* networkStream = UIDCast<const NetworkStream>(_streams[i].get())) {
            return networkStream->getWriteTimeout();
        }
    }

    return -1;
}

NetworkStream::WaitResult MultiStream::waitRead(int milliseconds, Log* log)
{
    if (NetworkStream* readStreamNS = UIDCast<NetworkStream>(_readStream.get())) {
        return readStreamNS->waitRead(milliseconds, log);
    }

    return WaitResultCancelled;
}

NetworkStream::WaitResult MultiStream::waitWrite(int milliseconds, Log* log)
{
    for (size_t i = 0; i != _streams.size(); ++i) {
        if (NetworkStream* networkStream = UIDCast<NetworkStream>(_streams[i].get())) {
            WaitResult result = networkStream->waitWrite(milliseconds, log);
            if (result != WaitResultOK) {
                return result;
            }
        }
    }

    return WaitResultOK;
}
}
