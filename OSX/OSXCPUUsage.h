// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_OSX_OSXCPUUSAGE_H
#define PRIME_OSX_OSXCPUUSAGE_H

#include "Config.h"

namespace Prime {

class PRIME_PUBLIC OSXCPUUsage {
public:
    OSXCPUUsage();

    ~OSXCPUUsage();

    int read();
};
}

#endif
