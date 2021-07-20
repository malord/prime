// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_QUOTEDPRINTABLEENCODER_H
#define PRIME_QUOTEDPRINTABLEENCODER_H

#include "Config.h"
#include "ScopedPtr.h"
#include "Stream.h"
#include "StreamBuffer.h"

namespace Prime {

/// Encode output with quoted-printable encoding.
class PRIME_PUBLIC QuotedPrintableEncoder : public Stream {
public:
    enum TextMode {
        /// The file should be encoded verbatim, but CRLF coded newlines should be written as CRLF.
        /// This is the default.
        TextModeBinaryCRLF,

        /// The file should be encoded verbatim. CRLF is escaped.
        TextModeBinary,

        /// The content is textual and any newline sequences (\n, \r\n, \r) should be converted to CRLF.
        TextModeText
    };

    class PRIME_PUBLIC Options {
    public:
        Options() { construct(); }

        explicit Options(size_t lineLength)
        {
            construct();
            setLineLength(lineLength);
        }

        Options& setTextMode(TextMode value = TextModeText)
        {
            _textMode = value;
            return *this;
        }
        Options& setBinaryMode() { return setTextMode(TextModeBinary); }
        Options& setBinaryCRLFMode() { return setTextMode(TextModeBinaryCRLF); }

        TextMode getTextMode() const { return _textMode; }

        Options& setLineLength(size_t value)
        {
            PRIME_ASSERT(value >= 6);
            _maxLineLength = value;
            return *this;
        }
        size_t getLineLength() const { return _maxLineLength; }

        /// Whether or not to escape '.' at the start of a line (for SMTP servers). Defaults to true.
        Options& setEscapeDot(bool value)
        {
            _escapeDot = value;
            return *this;
        }
        bool getEscapeDot() const { return _escapeDot; }

        /// Whether or not to escape '-' at the start of a line (for MIME). Defaults to true.
        Options& setEscapeDash(bool value)
        {
            _escapeDash = value;
            return *this;
        }
        bool getEscapeDash() const { return _escapeDash; }

    private:
        void construct()
        {
            _maxLineLength = 76;
            _textMode = TextModeBinaryCRLF;
            _escapeDot = true;
            _escapeDash = true;
        }

        size_t _maxLineLength;
        TextMode _textMode;
        bool _escapeDot;
        bool _escapeDash;
    };

    QuotedPrintableEncoder();

    QuotedPrintableEncoder(Stream* stream, const Options& options);

    ~QuotedPrintableEncoder();

    void begin(Stream* stream, const Options& options);

    /// Does nothing and returns true if an end-write isn't needed.
    bool end(Log* log);

    virtual ptrdiff_t writeSome(const void* memory, size_t maximumBytes, Log* log) PRIME_OVERRIDE;
    virtual bool close(Log* log) PRIME_OVERRIDE;
    virtual bool flush(Log* log) PRIME_OVERRIDE;

private:
    void construct();

    bool flushLine(Log* log, bool crlf = true);

    bool _started;

    RefPtr<Stream> _stream;
    ScopedArrayPtr<uint8_t> _line;
    size_t _maxLineLength;
    size_t _lineLength;
    unsigned int _putBack;
    Options _options;
};
}

#endif
