// Copyright 2000-2021 Mark H. P. Lord

#include "OSXCPUUsage.h"
#include <mach/mach_error.h>
#include <mach/mach_host.h>

namespace Prime {

OSXCPUUsage::OSXCPUUsage()
{
}

OSXCPUUsage::~OSXCPUUsage()
{
}

int OSXCPUUsage::read()
{
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

    host_cpu_load_info_data_t info;
    kern_return_t krc = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&info, &count);

    if (krc != KERN_SUCCESS) {
        DeveloperWarning("OSXCPUUsage: %s", mach_error_string(krc));
        return false;
    }

    uint64_t user = (uint64_t)info.cpu_ticks[CPU_STATE_USER];
    //uint64_t nice = (uint64_t) info.cpu_ticks[CPU_STATE_NICE];
    uint64_t kernel = (uint64_t)info.cpu_ticks[CPU_STATE_SYSTEM];
    uint64_t idle = (uint64_t)info.cpu_ticks[CPU_STATE_IDLE];

    int percentage = (int)((user + kernel) * UINT64_C(100) / (user + kernel + idle));

    return percentage;
}
}
