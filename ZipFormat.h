// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ZIPFORMAT_H
#define PRIME_ZIPFORMAT_H

#include "Config.h"

namespace Prime {

namespace Zip {

    /// MS-DOS file attributes.
    enum FileAttributes {
        FileAttributeNormal = 0x00,
        FileAttributeReadOnly = 0x01,
        FileAttributeHidden = 0x02,
        FileAttributeSystem = 0x04,
        FileAttributeLabel = 0x08,
        FileAttributeDirectory = 0x10,
        FileAttributeArchive = 0x20,
        FileAttributeUnused = 0x40
    };

    /// Zip compression methods.
    enum CompressionMethod {
        CompressionMethodStore = 0,
        CompressionMethodDeflate = 8
    };

    /// Convert a zlib compression (0 through 9) to a zip file compression method.
    inline uint8_t ZlibCompressionToZipMethod(int compression)
    {
        return compression != 0 ? (uint8_t)CompressionMethodDeflate : (uint8_t)CompressionMethodStore;
    }

    struct CentralDirectoryEntry;

    /// Local directory entry in a zip file.
    struct PRIME_PUBLIC LocalDirectoryEntry {

        /// Parse our contents from memory.
        bool decode(const void* memory);

        /// Write our contents to memory.
        void encode(void* memory, const char* filename = NULL, const void* extra = NULL) const;

        /// Copy our contents from a central directory entry.
        void copyCentralDirectoryEntry(const CentralDirectoryEntry& cent);

        /// Compute the size of our data when encoded, including the file name and extra data.
        size_t computeEncodedSize() const { return encodedSize + filenameLength + extraLength; }

        /// Signature of a local directory entry, "PK34"
        enum { validSignature = UINT32_C(0x04034b50) };

        /// Size in bytes of the encoded structure.
        enum { encodedSize = 4 * 4 + 7 * 2 };

        uint32_t signature; // Should be validSignature
        uint16_t extracterVersion;
        uint16_t bitFlag;
        uint16_t method;
        uint16_t modificationTime;
        uint16_t modificationDate;
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t decompressedSize;
        uint16_t filenameLength;
        uint16_t extraLength;

        // Followed by filename (variable size)
        // Followed by extra field (variable size)
    };

    /// Central directory entry in a zip file.
    struct PRIME_PUBLIC CentralDirectoryEntry {

        /// Parse our contents from memory.
        bool decode(const void* memory);

        /// Write our contents to memory.
        void encode(void* memory, const char* filename = NULL, const void* extra = NULL, const void* comment = NULL) const;

        /// Copy our contents from a local directory entry.
        void copyLocalDirectoryEntry(const LocalDirectoryEntry& lent);

        /// Compute our encoded size, including file name, extra data and comment.
        size_t computeEncodedSize() const { return encodedSize + filenameLength + extraLength + commentLength; }

        /// Signature of a central directory entry, "PK12"
        enum { validSignature = UINT32_C(0x02014b50) };

        /// Size in bytes of the encoded structure.
        enum { encodedSize = 6 * 4 + 11 * 2 };

        uint32_t signature;
        uint16_t madeByVersion;
        uint16_t extracterVersion;
        uint16_t bitFlag;
        uint16_t method;
        uint16_t modificationTime;
        uint16_t modificationDate;
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t decompressedSize;
        uint16_t filenameLength;
        uint16_t extraLength;
        uint16_t commentLength;
        uint16_t diskNumber;
        uint16_t internalAttributes;
        uint32_t externalAttributes;
        uint32_t offset; // Offset of the local directory entry.

        // Followed by filename (variable size)
        // Followed by extra field (variable size)
        // Followed by file comment (variable size)
    };

    /// The zip file footer.
    struct PRIME_PUBLIC EndRecord {

        /// Parse our contents from memory.
        bool decode(const void* memory);

        /// Write our contents to memory.
        void encode(void* memory) const;

        /// Signature of a zip file footer, "PK56".
        enum { validSignature = UINT32_C(0x06054b50) };

        /// Size in bytes of the encoded structure.
        enum { encodedSize = 3 * 4 + 5 * 2 };

        uint32_t signature;
        uint16_t thisDiskNumber;
        uint16_t cdirDiskNumber;
        uint16_t cdirThisDisk;
        uint16_t cdirEntryCount;
        uint32_t cdirSize;
        uint32_t cdirOffset;
        uint16_t commentLength;

        // This is followed by the zip file comment
    };

    //
    // Date/time
    //

    PRIME_PUBLIC void DecodeDateTime(uint16_t zipDate, uint16_t zipTime, int& year, int& month, int& day,
        int& hour, int& minute, int& second);

    PRIME_PUBLIC void EncodeDateTime(uint16_t* zipDate, uint16_t* zipTime, int year, int month, int day,
        int hour, int minute, int second);
}
}

#endif
