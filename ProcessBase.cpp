// Copyright 2000-2021 Mark H. P. Lord

#include "ProcessBase.h"

namespace Prime {

//
// ProcessBase::Argument
//

ProcessBase::Argument::Argument()
    : _wildcard(false)
    , _verbatim(false)
{
}

ProcessBase::Argument::Argument(std::string argument)
    : _argument(PRIME_MOVE(argument))
    , _wildcard(false)
    , _verbatim(false)
{
}

ProcessBase::Argument::Argument(const char* argument)
    : _argument(argument)
    , _wildcard(false)
    , _verbatim(false)
{
}

ProcessBase::Argument::~Argument()
{
}

ProcessBase::Argument::Argument(const Argument& copy) { operator=(copy); }

ProcessBase::Argument& ProcessBase::Argument::operator=(const Argument& copy)
{
    if (this != &copy) {
        _verbatim = copy._verbatim;
        _wildcard = copy._wildcard;
        _argument = copy._argument;
    }

    return *this;
}

ProcessBase::Argument& ProcessBase::Argument::operator=(std::string argument)
{
    set(PRIME_MOVE(argument));
    return *this;
}

ProcessBase::Argument& ProcessBase::Argument::operator=(const char* argument)
{
    set(PRIME_MOVE(argument));
    return *this;
}

ProcessBase::Argument& ProcessBase::Argument::set(std::string argument)
{
    _argument.swap(argument);
    return *this;
}
}
