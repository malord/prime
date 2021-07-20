// Copyright 2000-2021 Mark H. P. Lord

#include "ZipFormat.h"
#include "ByteOrder.h"
#include <string.h>

namespace Prime {

namespace Zip {

    //
    // LocalDirectoryEntry
    //

    bool LocalDirectoryEntry::decode(const void* memory)
    {
        const char* ptr = (const char*)memory;

        Read32LE(ptr, signature);
        Read16LE(ptr + 4, extracterVersion);
        Read16LE(ptr + 6, bitFlag);
        Read16LE(ptr + 8, method);
        Read16LE(ptr + 10, modificationTime);
        Read16LE(ptr + 12, modificationDate);
        Read32LE(ptr + 14, crc32);
        Read32LE(ptr + 18, compressedSize);
        Read32LE(ptr + 22, decompressedSize);
        Read16LE(ptr + 26, filenameLength);
        Read16LE(ptr + 28, extraLength);

        return signature == validSignature;
    }

    void LocalDirectoryEntry::encode(void* memory, const char* filename, const void* extra) const
    {
        char* ptr = (char*)memory;

        Write32LE(ptr, signature);
        Write16LE(ptr + 4, extracterVersion);
        Write16LE(ptr + 6, bitFlag);
        Write16LE(ptr + 8, method);
        Write16LE(ptr + 10, modificationTime);
        Write16LE(ptr + 12, modificationDate);
        Write32LE(ptr + 14, crc32);
        Write32LE(ptr + 18, compressedSize);
        Write32LE(ptr + 22, decompressedSize);
        Write16LE(ptr + 26, filenameLength);
        Write16LE(ptr + 28, extraLength);

        if (filename) {
            memcpy(ptr + encodedSize, filename, filenameLength);
        }

        if (extra) {
            memcpy(ptr + encodedSize + filenameLength, extra, extraLength);
        }
    }

    void LocalDirectoryEntry::copyCentralDirectoryEntry(const CentralDirectoryEntry& cent)
    {
        signature = cent.signature;
        extracterVersion = cent.extracterVersion;
        bitFlag = cent.bitFlag;
        method = cent.method;
        modificationTime = cent.modificationTime;
        modificationDate = cent.modificationDate;
        crc32 = cent.crc32;
        compressedSize = cent.compressedSize;
        decompressedSize = cent.decompressedSize;
        filenameLength = cent.filenameLength;
        extraLength = cent.extraLength;
    }

    //
    // CentralDirectoryEntry
    //

    bool CentralDirectoryEntry::decode(const void* memory)
    {
        const char* ptr = (const char*)memory;

        Read32LE(ptr, signature);
        Read16LE(ptr + 4, madeByVersion);
        Read16LE(ptr + 6, extracterVersion);
        Read16LE(ptr + 8, bitFlag);
        Read16LE(ptr + 10, method);
        Read16LE(ptr + 12, modificationTime);
        Read16LE(ptr + 14, modificationDate);
        Read32LE(ptr + 16, crc32);
        Read32LE(ptr + 20, compressedSize);
        Read32LE(ptr + 24, decompressedSize);
        Read16LE(ptr + 28, filenameLength);
        Read16LE(ptr + 30, extraLength);
        Read16LE(ptr + 32, commentLength);
        Read16LE(ptr + 34, diskNumber);
        Read16LE(ptr + 36, internalAttributes);
        Read32LE(ptr + 38, externalAttributes);
        Read32LE(ptr + 42, offset);

        return signature == validSignature;
    }

    void CentralDirectoryEntry::encode(void* memory, const char* filename, const void* extra, const void* comment) const
    {
        char* ptr = (char*)memory;

        Write32LE(ptr, signature);
        Write16LE(ptr + 4, madeByVersion);
        Write16LE(ptr + 6, extracterVersion);
        Write16LE(ptr + 8, bitFlag);
        Write16LE(ptr + 10, method);
        Write16LE(ptr + 12, modificationTime);
        Write16LE(ptr + 14, modificationDate);
        Write32LE(ptr + 16, crc32);
        Write32LE(ptr + 20, compressedSize);
        Write32LE(ptr + 24, decompressedSize);
        Write16LE(ptr + 28, filenameLength);
        Write16LE(ptr + 30, extraLength);
        Write16LE(ptr + 32, commentLength);
        Write16LE(ptr + 34, diskNumber);
        Write16LE(ptr + 36, internalAttributes);
        Write32LE(ptr + 38, externalAttributes);
        Write32LE(ptr + 42, offset);

        if (filename) {
            memcpy(ptr + encodedSize, filename, filenameLength);
        }

        if (extra) {
            memcpy(ptr + encodedSize + filenameLength, extra, extraLength);
        }

        if (comment) {
            memcpy(ptr + encodedSize + filenameLength + extraLength, comment, commentLength);
        }
    }

    void CentralDirectoryEntry::copyLocalDirectoryEntry(const LocalDirectoryEntry& lent)
    {
        signature = lent.signature;
        madeByVersion = extracterVersion = lent.extracterVersion;
        bitFlag = lent.bitFlag;
        method = lent.method;
        modificationTime = lent.modificationTime;
        modificationDate = lent.modificationDate;
        crc32 = lent.crc32;
        compressedSize = lent.compressedSize;
        decompressedSize = lent.decompressedSize;
        filenameLength = lent.filenameLength;
        extraLength = lent.extraLength;
        commentLength = 0;
        diskNumber = 0;
        internalAttributes = 0;
        externalAttributes = 0;
        offset = 0;
    }

    //
    // EndRecord
    //

    bool EndRecord::decode(const void* memory)
    {
        const char* ptr = (const char*)memory;

        Read32LE(ptr, signature);
        Read16LE(ptr + 4, thisDiskNumber);
        Read16LE(ptr + 6, cdirDiskNumber);
        Read16LE(ptr + 8, cdirThisDisk);
        Read16LE(ptr + 10, cdirEntryCount);
        Read32LE(ptr + 12, cdirSize);
        Read32LE(ptr + 16, cdirOffset);
        Read16LE(ptr + 20, commentLength);

        return signature == validSignature;
    }

    void EndRecord::encode(void* memory) const
    {
        char* ptr = (char*)memory;

        Write32LE(ptr, signature);
        Write16LE(ptr + 4, thisDiskNumber);
        Write16LE(ptr + 6, cdirDiskNumber);
        Write16LE(ptr + 8, cdirThisDisk);
        Write16LE(ptr + 10, cdirEntryCount);
        Write32LE(ptr + 12, cdirSize);
        Write32LE(ptr + 16, cdirOffset);
        Write16LE(ptr + 20, commentLength);
    }

    //
    // Date/time
    //

    void DecodeDateTime(uint16_t zipDate, uint16_t zipTime, int& year, int& month, int& day, int& hour, int& minute,
        int& second)
    {
        year = (zipDate >> 9) + 1980;
        month = (zipDate >> 5) & 15;
        day = (zipDate & 31);

        hour = (zipTime >> 11);
        minute = (zipTime >> 5) & 63;
        second = (zipTime & 31) * 2;
    }

    void EncodeDateTime(uint16_t* zipDate, uint16_t* zipTime, int year, int month, int day, int hour, int minute, int second)
    {
        *zipDate = (uint16_t)(((year - 1980) << 9)
            | ((month) << 5)
            | ((day)));

        *zipTime = (uint16_t)(((hour) << 11)
            | ((minute) << 5)
            | ((second) >> 1));
    }
}
}
