// Copyright 2000-2021 Mark H. P. Lord

#include "FileLocations.h"
#include "Path.h"

namespace Prime {

std::string AppIDToRelativePath(const char* appID)
{
    // Always remove everything up to the first '.'.
    const char* p = strchr(appID, '.');
    if (p) {
        appID = p + 1;
    }

    // Remove "co.".
    if (strncmp(appID, "co.", 3) == 0) {
        appID += 3;
    }

    std::string result(appID);
    std::replace(result.begin(), result.end(), '.', (char)Path::slash);
    return result;
}

std::string AppIDToLowerCase(const char* appID)
{
    std::string result(appID);
    ASCIIToLowerInPlace(result);
    return result;
}
}
