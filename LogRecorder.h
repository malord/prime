// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LOGRECORDER_H
#define PRIME_LOGRECORDER_H

#include "DoubleLinkList.h"
#include "Log.h"
#include <string>

namespace Prime {

/// Record log output so it can be replayed to another Log when required.
class PRIME_PUBLIC LogRecorder : public Log {
public:
    LogRecorder();

#ifdef PRIME_COMPILER_RVALUEREF
    LogRecorder(LogRecorder&& rhs) PRIME_NOEXCEPT;
#endif

    ~LogRecorder();

    class Message {
    public:
        ~Message();

        Level getLevel() const { return _level; }
        const std::string& getText() const { return _text; }

    private:
        Level _level;
        std::string _text;

        friend class LogRecorder;

        Message();
        Message(Level level, std::string text);

        DoubleLink<Message> _link;
    };

    typedef DoubleLinkList<Message, DeletingLinkListElementManager<Message>>::const_iterator const_iterator;
    typedef DoubleLinkList<Message, DeletingLinkListElementManager<Message>>::iterator iterator;

    const_iterator cbegin() const { return _messages.cbegin(); }
    const_iterator cend() const { return _messages.cend(); }

    const_iterator begin() const { return _messages.cbegin(); }
    const_iterator end() const { return _messages.cend(); }

    iterator begin() { return _messages.begin(); }
    iterator end() { return _messages.end(); }

    void replay(Log* target);

    bool empty() const { return _messages.empty(); }

    void clear() { _messages.clear(); }

    /// Returns LevelNone if no logs have been written.
    Level getMaxLevel() const { return _maxLevel; }

    void move(LogRecorder& from);

    // Log implementation.
    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

private:
    DoubleLinkList<Message, DeletingLinkListElementManager<Message>> _messages;
    Level _maxLevel;
};
}

#endif
