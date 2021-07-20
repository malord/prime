// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_TEXTREADER_H
#define PRIME_TEXTREADER_H

#include "Config.h"
#include "Stream.h"
#include "StringView.h"
#include <vector>

namespace Prime {

/// Helper class for reading a text file. Can either read from a whole file already loaded in to memory or
/// through a Stream (in which case buffering is provided). Differs from StreamBuffer in that it keeps track of
/// the current line and column number and has a "marker" mechanism to allow rewinding during parsing.
class PRIME_PUBLIC TextReader : public RefCounted {
public:
    static int charToInt(char ch) { return (int)(unsigned char)(ch); }

    static char intToChar(int ch) { return (char)(unsigned char)(ch); }

    enum { defaultBufferSize = 512u * 1024u };

    class Marker;
    friend class Marker;

    /// A rewind marker. When you construct a Marker it guarantees that the TextReader will not purge the
    /// current position from the buffer, allowing you to rewind to that point if needed.
    /// Markers can be nested and must be released in the order in which they were constructed. The Marker
    /// destructor will automatically rewind a marker, so you must call release() if you want to destruct a
    /// Marker without rewinding through the file.
    class Marker {
        friend class TextReader;

    public:
        Marker(TextReader* textReader)
            : _textReader(textReader)
            , _locked(true)
            , _prevMarker(textReader->_firstMarker)
            , _ptr(textReader->_ptr)
        {
            textReader->_firstMarker = this;
        }

        ~Marker()
        {
            if (_locked) {
                rewind();
            }
        }

        /// Release the marker. This prevents it from rewinding when destructed.
        void release()
        {
            PRIME_ASSERT(_locked);
            PRIME_ASSERTMSG(this == _textReader->_firstMarker, "Marker not released in order.");

            _textReader->_firstMarker = _prevMarker;
            _locked = false;
        }

        /// Rewind to this marker. This also releases the marker.
        void rewind()
        {
            PRIME_ASSERT(_locked);
            release();
            _textReader->_ptr = _ptr;
        }

        bool isLocked() const { return _locked; }

    private:
        TextReader* _textReader;
        bool _locked;
        Marker* _prevMarker;

        // The TextReader will update this if it modifies the buffer.
        const char* _ptr;
    };

    TextReader();

    ~TextReader();

    /// Set a Log to write to. Mandatory.
    void setLog(Log* log) { _log = log; }

    Log* getLog() const { return &_locationLog; }

    /// Disabled by default.
    void setSkipBOM(bool skipBOM) { _skipBOM = skipBOM; }

    /// Begin parsing the supplied text.
    void setText(StringView source, unsigned int line = 0, unsigned int column = 0);

    /// Set a Stream, which can provide us with more bytes, and allocate a buffer (the buffer will grow if
    /// necessary).
    void setStream(Stream* stream, size_t bufferSize, unsigned int line = 0, unsigned int column = 0);

    /// Returns a string of the form "(line:column)".
    const char* getLocation() const;

    /// Returns the zero-based line number.
    unsigned int getLine() const;

    /// Returns the zero-based column number.
    unsigned int getColumn() const;

    enum {
        /// Special character value used to indiciate a read error.
        ErrorChar = -2,

        /// Special character value used to indicate the end of the file.
        EOFChar = -1
    };

    /// Return the next character that will be read, EOFChar if the end of the file has been reached or
    /// ErrorChar if a read error occurred. The character is not consumed.
    int peekChar()
    {
        return (_ptr == _top) ? slowPeekChar(0) : charToInt(*_ptr);
    }

    /// Return a character from the buffer, EOFChar if the index is beyond the end of the file or ErrorChar
    /// if a read error occurred. The character is not consumed.
    int peekChar(unsigned int at)
    {
        return (_ptr + at >= _top) ? slowPeekChar(at) : charToInt(_ptr[at]);
    }

    /// Returns true if the buffer contents match the string.
    bool hasString(const char* string);

    /// Returns true if the buffer contents starting at the specified offset match the string.
    bool hasString(unsigned int at, const char* string);

    /// Read the next character from the buffer and return its value. Returns EOFChar if there are no more
    /// characters to read, ErrorChar if a read error occurred.
    int readChar()
    {
        if (_ptr == _top) {
            return slowReadChar();
        }

        return charToInt(*_ptr++);
    }

    /// Put the last read character back in to the buffer.
    void putBack()
    {
        PRIME_DEBUG_ASSERT(_ptr != _buffer);
        --_ptr;
    }

    /// Consume a character that has been peeked.
    void skipChar()
    {
        PRIME_DEBUG_ASSERT(_ptr != _top);
        ++_ptr;
    }

    /// Consume a number of characters that have been peeked.
    void skipChars(size_t n)
    {
        PRIME_ASSERT(n <= (size_t)(_top - _ptr));
        _ptr += n;
    }

    /// Read more characters in to the buffer, shifting the contents of the buffer if necessary (and possible).
    /// Returns the number of characters added to the buffer, or -1 if a read error occurs or the buffer is full.
    ptrdiff_t fetchMore();

    /// Set the location of the token. If derived classes don't call this, getLocation() will be returning the
    /// location of the file pointer at the time an error was detected, which may not be helpful.
    /// To get correct token locations, call this method immediately before each token is parsed.
    void setTokenStartToCurrentPointer() { _tokenStart = _ptr; }

    /// Returns the current read pointer, which points to the next character that will be read. If the returned
    /// pointer equals getTopPointer(), there are no bytes available to read.
    const char* getReadPointer() const { return _ptr; }

    /// Returns a pointer to the character just after the end of the buffered characters.
    const char* getTopPointer() const { return _top; }

private:
    /// Structure used to store information on our current location.
    struct Location {
        unsigned int line;
        unsigned int column;
        char previousChar;
        const char* ptr;
    };

    /// A Log which prefixes every message with the current location of a TextReader.
    class LocationLog : public Log {
    public:
        LocationLog();

        void setTextReader(TextReader* textReader) { _textReader = textReader; }

        virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

    private:
        TextReader* _textReader;
    };

    friend class LocationLog;

    void allocBuffer(size_t bufferSize);

    bool growDynamicBuffer();

    bool isDynamicBuffer() const
    {
        return _buffer && _buffer == &_dynamicBuffer[0];
    }

    void rebasePointers(char* from, char* to);

    /// Read an index that is probably beyond the contents of the buffer.
    int slowPeekChar(unsigned int at);

    /// Read the next character even if we're at the end of the buffer.
    int slowReadChar()
    {
        int c = slowPeekChar(0);
        if (c >= 0) {
            ++_ptr;
        }

        return c;
    }

    void setLocation(unsigned int line = 0, unsigned int column = 0);

    /// Update integer line and column numbers given text to count. The lastChar is needed in case the last time
    /// UpdateLocation was called it ended with a \\r and we need to ignore the corresponding \\n.
    static void updateLocation(const char* from, const char* to, Location& loc);

    /// Update _tokenLocation.
    void updateTokenLocation() const;

    mutable const char* _tokenStart;

    enum ErrorState {
        ErrorStateNone,
        ErrorStateEOF,
        ErrorStateError
    };

    /// Has any error occurred?
    ErrorState _errorState;

    Marker* _firstMarker;

    const char* _ptr;
    const char* _top;

    char* _buffer;
    char* _bufferEnd;

    RefPtr<Log> _log;
    mutable LocationLog _locationLog;

    RefPtr<Stream> _stream;
    bool _streamCheckForBOM;

    bool _skipBOM;

    // The line and column currently at the start of the buffer.
    Location _beginLocation;
    mutable Location _tokenLocation;

    std::vector<char> _dynamicBuffer;
    mutable char _locationBuffer[30];

    PRIME_UNCOPYABLE(TextReader);
};
}

#endif
