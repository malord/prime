// Copyright 2000-2021 Mark H. P. Lord

#include "NetworkStream.h"

namespace Prime {

PRIME_DEFINE_UID_CAST(NetworkStream)

bool NetworkStream::waitReadTimeout(Log* log)
{
    if (getReadTimeout() < 0) {
        return true;
    }

    switch (waitRead(getReadTimeout(), log)) {
    case WaitResultCancelled:
        return false;

    case WaitResultTimedOut:
        log->error(PRIME_LOCALISE("Network read timeout."));
        return false;

    case WaitResultOK:
        break;
    }

    return true;
}

bool NetworkStream::waitWriteTimeout(Log* log)
{
    if (getWriteTimeout() < 0) {
        return true;
    }

    switch (waitWrite(getWriteTimeout(), log)) {
    case WaitResultCancelled:
        return false;

    case WaitResultTimedOut:
        log->error(PRIME_LOCALISE("Network write timeout."));
        return false;

    case WaitResultOK:
        break;
    }

    return true;
}
}
