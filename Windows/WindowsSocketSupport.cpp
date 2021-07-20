// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsSocketSupport.h"

namespace Prime {

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#if PRIME_WINVER >= PRIME_WINDOWS_VISTA
#pragma comment(lib, "Mswsock.lib")
#endif
#endif

bool WindowsSocketSupport::initSockets(Log* log)
{
    static WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log->error(PRIME_LOCALISE("WinSock initialisation failed."));
        return false;
    }

    return true;
}

void WindowsSocketSupport::shutdownSockets()
{
    WSACleanup();
}

void WindowsSocketSupport::logSocketError(Log* log, int err, Log::Level level)
{
    // This doesn't work on non-NT-based Windows versions, it just displays the error code.
    // If you ever want to change this, you need to use FormatMessage and pass the HMODULE
    // for the Winsock DLL.
    log->logWindowsError((DWORD)err, NULL, level);
}

void WindowsSocketSupport::logGetAddrInfoError(Log* log, int err, Log::Level level)
{
    (void)err;
    logSocketError(log, WSAGetLastError(), level);
    //log->logWindowsError((DWORD) err);
}

bool WindowsSocketSupport::setSocketNonBlocking(Handle handle, bool nonBlocking)
{
    unsigned long parm = nonBlocking ? 1 : 0;
    return ioctlSocket(handle, FIONBIO, (char*)&parm) != -1;
}
}