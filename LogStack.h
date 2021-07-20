// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_LOGSTACK_H
#define PRIME_LOGSTACK_H

#include "Log.h"
#include <vector>

namespace Prime {

/// Route log messages to the last pushed Log.
class PRIME_PUBLIC LogStack : public Log {
public:
    /// e.g.,
    /// LogStack::Pusher pushLog(_logStack,
    ///                          MakeRef<PrefixLog>(_logStack.getTop(), MakeString(lineNumber)));
    class Pusher {
    public:
        Pusher(LogStack& stack, Log* log)
            : _stack(stack)
        {
            stack.push(log);
        }

        ~Pusher()
        {
            _stack.pop();
        }

    private:
        LogStack& _stack;
    };

    LogStack()
    {
    }

    void clear();

    void push(Log* log);

    Log* getTop() const { return _logs.empty() ? NULL : _logs.back(); }

    RefPtr<Log> pop();

    virtual bool logVA(Level level, const char* format, va_list argptr) PRIME_OVERRIDE;

private:
    std::vector<RefPtr<Log>> _logs;

    PRIME_UNCOPYABLE(LogStack);
};
}

#endif
