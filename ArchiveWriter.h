// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_ARCHIVEWRITER_H
#define PRIME_ARCHIVEWRITER_H

#include "ArchiveReader.h"

namespace Prime {

class PRIME_PUBLIC ArchiveWriter : public RefCounted {
public:
    class PRIME_PUBLIC Delegate : public RefCounted {
    public:
        virtual ~Delegate();

        /// Return the archive file name as though a single-part archive were being created. An ArchiveWriter
        /// needs this in order to compute the file name(s) for finishPart().
        virtual const char* getArchiveFilename() const = 0;

        /// Create a temporary file to hold an archive part while the archive is being created. The temporary file
        /// will be renamed when the ArchiveWriter calls finishPart(), or will be removed if the archive creation
        /// is cancelled.
        virtual RefPtr<Stream> createPartFile(unsigned int partNumber, Log* log) = 0;

        /// Create a temporary read/write file to use during archiving. The temporary file will be deleted when
        /// the Stream is close()d or release()d.
        virtual RefPtr<Stream> createTemporaryFile() = 0;

        /// An ArchiveWriter must call this from its finish() method to rename the temporary part file to its
        /// correct name. The Streams returned by createPartFile() must be closed before this is called.
        virtual bool finishPart(unsigned int partNumber, const char* partFilename, Log* log) = 0;

        /// The ArchiveWriter must invoke this method for sequential part names that should no longer exist,
        /// until false is returned. i.e., if an archive used to have two parts (e.g., archive.001, archive.002)
        /// and the newly written archive only has one part, use of this method ensures archive.002 is removed.
        virtual bool deleteStrayPart(const char* partFilename, Log* log) = 0;

        /// Flags for openFile().
        enum {
            /// Don't decompress the data, i.e., provide the raw compressed data. Use this to transfer an
            /// already-compressed file without recompressing.
            OpenFileDoNotDecompress = 1u,
        };

        /// Open a file for reading, given the ID that was given to addFile(). The ArchiveWriter can call
        /// this method any time between the call to addFile() and the end of finish().
        virtual RefPtr<Stream> openFile(const Value& identifier, unsigned int flags, Log* log) = 0;
    };

    ArchiveWriter();

    virtual ~ArchiveWriter();

    /// Begin writing an archive. The delegate is retained.
    virtual bool begin(Delegate* delegate, const Value::Dictionary& options, Log* log) = 0;

    /// Add a file to the archive. The ArchiveWriter may does not necessarily write the file during this call, but
    /// when it does it will invoke the delegate's openFile() method to read the file, supplying the ID to
    /// identify the file.
    virtual bool addFile(ArchiveReader* archiveReader, const ArchiveReader::DirectoryEntry& directoryEntry,
        const Value& identifier, Log* log)
        = 0;

    /// Finish writing the archive.
    virtual bool finish(Log* log) = 0;
};
}

#endif
