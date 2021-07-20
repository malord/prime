// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_GZIPFORMAT_H
#define PRIME_GZIPFORMAT_H

#include "Config.h"

namespace Prime {

namespace GZip {

    enum CompressionMethods {
        CompressionMethodDeflate = 8
    };

    enum Flags {
        FlagText = 1u << 0,
        FlagHeaderCRC16 = 1u << 1,
        FlagExtra = 1u << 2,
        FlagFilename = 1u << 3,
        FlagComment = 1u << 4
    };

    enum ExtraFlags {
        ExtraFlagDeflateMax = 2u,
        ExtraFlagDeflateFast = 4u
    };

    enum Systems {
        SystemFAT = 0,
        SystemAmiga = 1,
        SystemVMS = 2,
        SystemUnix = 3,
        SystemVMCMS = 4,
        SystemAtaritOS = 5,
        SystemHPFS = 6,
        SystemMacOS = 7,
        SystemZSystem = 8,
        SystemCPM = 9,
        SystemTOPS20 = 10,
        SystemNTFS = 11,
        SystemQDOS = 12,
        SystemAcornRISCOS = 13,
        SystemUnknown = 255
    };

    struct PRIME_PUBLIC Header {
        enum { id0 = 31 };
        enum { id1 = 139 };

        uint8_t id[2]; // 31, 139 (0x1f, 0x8b)
        uint8_t compressionMethod; // CompressionMethods
        uint8_t flags; // Flags
        uint32_t modificationTime; // UNIX
        uint8_t extraFlags; // ExtraFlags
        uint8_t system; // Systems

        // if (Header.flags & FlagExtra)
        uint16_t extraLength;

        // Followed by extra data (decode() points this to within its supplied memory buffer)
        const void* extra;

        // Followed by filename (null terminated) (decode() points this to within its supplied memory buffer)
        const char* filename;

        // Followed by comment (null terminated) (decode() points this to within its supplied memory buffer)
        const char* comment;

        uint16_t headerCRC16;

        /// The encoded size of a Header if the default flags are used (as initialised by the constructor).
        enum { defaultEncodedSize = 10u };

        /// Parse our contents from memory.
        bool decode(const void* memory, size_t maxBytes);

        /// Write our contents to memory. Or, if memory is NULL, return the required size.
        size_t encode(void* memory) const;

        /// Initialises the header so that it can be immediately encoded and written to a stream.
        Header();

        /// Sets filename and the relevant flag. The file name is not copied.
        void setFilename(const char* value);

        /// Sets comment and the relevant flag. The comment is not copied.
        void setComment(const char* value);

        /// Sets extraLength, extra and the relevant flag. The data is not copied.
        bool setExtra(const void* data, size_t length);
    };

    struct PRIME_PUBLIC Footer {
        uint32_t crc32;
        uint32_t originalSize;

        enum { encodedSize = 8u };

        /// Parse our contents from memory.
        void decode(const void* memory);

        /// Write our contents to memory.
        void encode(void* memory) const;
    };
}
}

#endif
