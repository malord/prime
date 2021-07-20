// Copyright 2000-2021 Mark H. P. Lord

// TODO: ability to create a queue to run manually (e.g., on a specific thread)

#include "TaskSystem.h"

namespace Prime {

TaskSystem* TaskSystem::_global = NULL;

TaskSystem::~TaskSystem()
{
    if (_global == this) {
        _global = NULL;
    }
}

bool TaskSystem::yieldDoNotCallDirectly()
{
    return false;
}

void TaskSystem::resumeDoNotCallDirectly()
{
    PRIME_ASSERTMSG(0, "resume() without yield()");
}

//
// Global functions
//

bool YieldThreadDoNotCallDirectly()
{
    if (TaskSystem* global = TaskSystem::getGlobal()) {
        return global->yieldDoNotCallDirectly();
    }

    return false;
}

void ResumeThreadDoNotCallDirectly()
{
    if (TaskSystem* global = TaskSystem::getGlobal()) {
        global->resumeDoNotCallDirectly();
    }
}
}
