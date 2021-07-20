// Copyright 2000-2021 Mark H. P. Lord

#include "LogRecorder.h"
#include "ScopedPtr.h"
#include "StringUtils.h"

namespace Prime {

LogRecorder::LogRecorder()
    : _messages(&Message::_link)
{
    _maxLevel = LevelNone;
}

#ifdef PRIME_COMPILER_RVALUEREF
LogRecorder::LogRecorder(LogRecorder&& rhs) PRIME_NOEXCEPT : _messages(&Message::_link)
{
    move(rhs);
}
#endif

LogRecorder::~LogRecorder()
{
}

bool LogRecorder::logVA(Level level, const char* format, va_list argptr)
{
    std::string formatted;
    StringFormatVA(formatted, format, argptr);
    ScopedPtr<Message> message(new Message(level, formatted));
    _messages.push_back(message);
    message.detach();

    if (level > _maxLevel) {
        _maxLevel = level;
    }
    return false;
}

void LogRecorder::replay(Log* target)
{
    for (const_iterator iter = _messages.cbegin(); iter != _messages.cend(); ++iter) {
        target->log(iter->getLevel(), "%s", iter->getText().c_str());
    }
}

void LogRecorder::move(LogRecorder& from)
{
    _maxLevel = from._maxLevel;
    _messages.move(from._messages);
}

//
// LogRecorder::Message
//

LogRecorder::Message::Message()
{
}

LogRecorder::Message::~Message()
{
}

LogRecorder::Message::Message(Level level, std::string text)
{
    _level = level;
    _text.swap(text);
}

}
