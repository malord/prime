// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TEMPFILE_H
#define PRIME_TEMPFILE_H

#include "FileStream.h"

#define PRIME_HAVE_TEMPFILE

namespace Prime {

/// Creates a temporary file. Temporary directories are preferred, but temporary files are the only way to
/// perform atomic saves.
class PRIME_PUBLIC TempFile : public Stream {
public:
    TempFile();

    /// Closes the file and, if remove-on-destruct is enabled, removes it.
    ~TempFile();

    /// Create a temporary file given the specified path template, where any X characters at the end of the
    /// string are replaced with random alphanumeric characters (see MakeTempName).
    bool createWithPathTemplate(const char* pathTemplate, Log* log, unsigned int permissions = 0600);

    /// Create a temporary file in the specified path, guaranteeing to not overwrite any existing files.
    bool createInPath(const char* path, Log* log, unsigned int permissions = 0600);

    /// Create a temporary file that will overwrite the specified file when closed. This is the same as creating
    /// a temporary file in the same directory as the target file, then using setRenameOnClose.
    bool createToOverwrite(const char* path, Log* log, unsigned int permissions = 0666);

    /// Return the path of the created file.
    const char* getPath() const { return _path.c_str(); }

    /// Specify whether or not the file should be removed when this object is destructed. By default, it is.
    /// If the file is removed or renamed prior to destruction, the destructor won't attempt to remove anything.
    void setRemoveOnDestruct(bool removeOnDestruct) { _removeOnDestruct = removeOnDestruct; }

    /// Set the file name to rename the file to when it is closed, which overrides setRemoveOnDestruct().
    /// createToOverwrite() calls this for you.
    void setRenameOnClose(const char* renameTo);

    void cancelRenameOnClose();

    /// Close the file, performing automatic rename if requested, deleting the temporary file on failure.
    virtual bool close(Log* log) PRIME_OVERRIDE;

    bool closeAndRemove(Log* log);

    /// Close the file and rename it to "to", ignoring any name set by setRenameOnClose().
    bool closeAndRename(const char* to, Log* log);

    // Stream implementation.
    virtual ptrdiff_t readSome(void* buffer, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual ptrdiff_t writeSome(const void* bytes, size_t maxBytes, Log* log) PRIME_OVERRIDE;
    virtual Offset seek(Offset offset, SeekMode mode, Log* log) PRIME_OVERRIDE;
    virtual Offset getSize(Log* log) PRIME_OVERRIDE;
    virtual bool setSize(Offset newSize, Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;
    virtual bool copyFrom(Stream* source, Log* sourceLog, Offset length, Log* destLog, size_t bufferSize = 0,
        void* buffer = NULL) PRIME_OVERRIDE;

private:
    bool closeOrRemove(Log* log);

    FileStream _fileStream;
    std::string _path;
    std::string _renameTo;
    bool _removeOnDestruct;
};

}

#endif
