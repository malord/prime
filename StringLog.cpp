// Copyright 2000-2021 Mark H. P. Lord

#include "StringLog.h"
#include <string.h>

namespace Prime {

StringLog::StringLog()
{
	init(&_stringStream);
}

StringLog::~StringLog()
{
}
}
