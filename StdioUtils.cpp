// Copyright 2000-2021 Mark H. P. Lord

#include "StdioUtils.h"
#ifdef PRIME_OS_WINDOWS
#include "Windows/WindowsConfig.h"
#endif
#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#ifdef PRIME_OS_UNIX
#include "Unix/UnixCloseOnExec.h"
#endif

namespace Prime {

FILE* StdioOpen(const char* path, const char* mode, bool forceNoInherit)
{
#ifdef PRIME_OS_WINDOWS_UNICODE
    (void)forceNoInherit;
    return _wfopen(CharToTChar(path).c_str(), CharToTChar(mode).c_str());
#else
#ifdef PRIME_OS_UNIX
    UnixCloseOnExec::ScopedLock execLock;
#endif

    // There's a sporadically supported fopen 'e' flag for close-on-exec, but I'm not using it.

    FILE* fp = fopen(path, mode);

#ifdef PRIME_OS_UNIX
    if (fp && forceNoInherit) {
        UnixCloseOnExec::closeOnExec(fileno(fp));
    }
#else
    (void)forceNoInherit;
#endif

    return fp;
#endif
}

ptrdiff_t StdioRead(FILE* fp, void* ptr, ptrdiff_t size)
{
    size_t got;

    PRIME_ASSERT(fp);
    PRIME_ASSERT(size >= 0);

    got = fread(ptr, 1, size, fp);

    if (got == (size_t)size) {
        return got;
    }

    return ferror(fp) ? -1 : (ptrdiff_t)got;
}

ptrdiff_t StdioWrite(FILE* fp, const void* ptr, ptrdiff_t size)
{
    size_t wrote;

    PRIME_ASSERT(fp);
    PRIME_ASSERT(size >= 0);

    wrote = fwrite(ptr, 1, size, fp);

    if (wrote == (size_t)size) {
        return wrote;
    }

    return ferror(fp) ? -1 : (ptrdiff_t)wrote;
}

int StdioSeek(FILE* fp, int64_t offset, int whence)
{
#if defined(PRIME_OS_WINDOWS) && !PRIME_MSC_AND_OLDER(1300)
    return _fseeki64(fp, offset, whence);
#elif defined(PRIME_OS_UNIX)
    return fseeko(fp, offset, whence);
#else
    PRIME_ASSERTMSG((long int)offset == offset, "Seeking outside fseek supported offset");
    return fseek(fp, (long int)offset, whence);
#endif
}

int64_t StdioTell(FILE* fp)
{
#if defined(PRIME_OS_WINDOWS) && !PRIME_MSC_AND_OLDER(1300)
    return _ftelli64(fp);
#elif defined(PRIME_OS_UNIX)
    return ftello(fp);
#else
    return ftell(fp);
#endif
}

void StdioSetModeBinary(FILE* fp)
{
#if defined(PRIME_OS_WINDOWS)
    _setmode(_fileno(fp), _O_BINARY);
#elif defined(PRIME_OS_DOS)
    setmode(fileno(fp), O_BINARY);
#else
    (void)fp;
#endif
}

void StdioSetModeText(FILE* fp)
{
#if defined(PRIME_OS_WINDOWS)
    _setmode(_fileno(fp), _O_TEXT);
#elif defined(PRIME_OS_DOS)
    setmode(fileno(fp), O_TEXT);
#else
    (void)fp;
#endif
}
}
