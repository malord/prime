// Copyright 2000-2021 Mark H. P. Lord

#include "GZipFormat.h"
#include "ByteOrder.h"
#include <string.h>

namespace Prime {

namespace GZip {

    //
    // Header
    //

    Header::Header()
    {
        id[0] = id0;
        id[1] = id1;
        compressionMethod = (uint8_t)CompressionMethodDeflate;
        flags = 0;
        modificationTime = 0;
        extraFlags = 0;
        system = SystemUnknown;
        extraLength = 0;
        extra = NULL;
        filename = NULL;
        comment = NULL;
        headerCRC16 = 0;
    }

    void Header::setFilename(const char* value)
    {
        filename = value;
        if (value) {
            flags |= (uint8_t)FlagFilename;
        } else {
            flags &= (uint8_t)(~FlagFilename);
        }
    }

    void Header::setComment(const char* value)
    {
        comment = value;
        if (value) {
            flags |= (uint8_t)FlagComment;
        } else {
            flags &= (uint8_t)(~FlagComment);
        }
    }

    bool Header::setExtra(const void* data, size_t length)
    {
        if (length != (uint16_t)length) {
            return false;
        }

        extra = data;
        extraLength = (uint16_t)length;

        if (extraLength) {
            flags |= (uint8_t)FlagExtra;
        } else {
            flags &= (uint8_t)(~FlagExtra);
        }

        return true;
    }

    bool Header::decode(const void* memory, size_t maxBytes)
    {
        if (maxBytes < 10) {
            return false;
        }

        const uint8_t* bytes = (const uint8_t*)memory;

        id[0] = bytes[0];
        id[1] = bytes[1];
        compressionMethod = bytes[2];
        flags = bytes[3];
        modificationTime = Read32LE(bytes + 4);
        extraFlags = bytes[8];
        system = bytes[9];

        extraLength = 0;
        extra = NULL;

        filename = NULL;
        comment = NULL;
        headerCRC16 = 0;

        if (id[0] != 31 || id[1] != 139) {
            return false;
        }

        const uint8_t* ptr = bytes + 10;
        const uint8_t* end = bytes + maxBytes;

        if (flags & FlagExtra) {
            if (end - ptr < 2) {
                return true;
            }

            extraLength = Read16LE(ptr);
            ptr += 2;

            if ((size_t)(end - ptr) < extraLength) {
                return true;
            }

            extra = ptr;
            ptr += extraLength;
        }

        if (flags & FlagFilename) {
            const void* nulls = memchr(ptr, 0, end - ptr);
            if (!nulls) {
                return true;
            }

            filename = (const char*)ptr;
            ptr = (const uint8_t*)nulls + 1;
        }

        if (flags & FlagComment) {
            const void* nulls = memchr(ptr, 0, end - ptr);
            if (!nulls) {
                return true;
            }

            comment = (const char*)ptr;
            ptr = (const uint8_t*)nulls + 1;
        }

        if (flags & FlagHeaderCRC16) {
            if (end - ptr < 2) {
                return true;
            }

            headerCRC16 = Read16LE(ptr);
            //ptr += 2;
        }

        return true;
    }

    size_t Header::encode(void* memory) const
    {
        uint8_t* bytes = (uint8_t*)memory;

        unsigned int actualFlags = flags & ~(FlagExtra | FlagFilename | FlagComment);
        if (filename) {
            actualFlags |= FlagFilename;
        }
        if (comment) {
            actualFlags |= FlagComment;
        }
        if (extraLength != 0) {
            actualFlags |= FlagExtra;
        }

        if (memory) {
            bytes[0] = 31;
            bytes[1] = 139;
            bytes[2] = compressionMethod;
            bytes[3] = (uint8_t)actualFlags;
            Write32LE(bytes + 4, modificationTime);
            bytes[8] = extraFlags;
            bytes[9] = system;
        }
        bytes += 10;

        if (actualFlags & FlagExtra) {
            if (memory) {
                Write16LE(bytes, extraLength);
                memcpy(bytes + 2, extra, extraLength);
            }

            bytes += 2 + extraLength;
        }

        if (filename) {
            size_t filenameSize = strlen(filename) + 1;
            if (memory) {
                memcpy(bytes, filename, filenameSize);
            }
            bytes += filenameSize;
        }

        if (comment) {
            size_t commentSize = strlen(comment) + 1;
            if (memory) {
                memcpy(bytes, comment, commentSize);
            }
            bytes += commentSize;
        }

        if (actualFlags & FlagHeaderCRC16) {
            if (memory) {
                Write16LE(bytes, headerCRC16);
            }
            bytes += 2;
        }

        return bytes - (uint8_t*)memory;
    }

    //
    // Footer
    //

    void Footer::decode(const void* memory)
    {
        const uint8_t* bytes = (const uint8_t*)memory;
        crc32 = Read32LE(bytes);
        originalSize = Read32LE(bytes + 4);
    }

    void Footer::encode(void* memory) const
    {
        uint8_t* bytes = (uint8_t*)memory;
        Write32LE(bytes, crc32);
        Write32LE(bytes + 4, originalSize);
    }
}
}
