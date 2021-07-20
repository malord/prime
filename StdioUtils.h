// Copyright 2000-2021 Mark H. P. Lord

//
// Wrap stdio file functions to:
// * provide a UNIX read()/write() style interface,
// * take care of calling ferror(),
// * support 64-bit file offsets.
//

#ifndef PRIME_STDIOUTILS_H
#define PRIME_STDIOUTILS_H

#include "Config.h"
#include <stdio.h>

namespace Prime {

/// On Unicode Windows, the file name is converted from UTF-8 to WCHAR.
PRIME_PUBLIC FILE* StdioOpen(const char* path, const char* mode, bool forceNoInherit = true);

/// Returns < 0 on error, otherwise returns the number of bytes read, dealing with the call to ferror().
PRIME_PUBLIC ptrdiff_t StdioRead(FILE* fp, void* ptr, ptrdiff_t size);

/// Returns < 0 on error, otherwise returns the number of bytes written, dealing with the call to ferror().
PRIME_PUBLIC ptrdiff_t StdioWrite(FILE* fp, const void* ptr, ptrdiff_t size);

/// Supports large offsets on platforms where stdio does.
PRIME_PUBLIC int StdioSeek(FILE* fp, int64_t offset, int whence);

/// Supports large offsets on platforms where stdio does.
PRIME_PUBLIC int64_t StdioTell(FILE* fp);

PRIME_PUBLIC void StdioSetModeBinary(FILE* fp);

PRIME_PUBLIC void StdioSetModeText(FILE* fp);
}

#endif
