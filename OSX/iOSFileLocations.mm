// Copyright 2000-2021 Mark H. P. Lord

#include "../FileLocations.h"
#include "../Path.h"
#include "ScopedAutoreleasePool.h"

namespace Prime {

static std::string FoundationPath(NSSearchPathDirectory directory, NSSearchPathDomainMask domainMask, Log* log, BOOL expandTilde = YES)
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(directory, domainMask, expandTilde);

    if (![paths count]) {
        log->error("NSSearchPathForDirectoriesInDomains error");
        return std::string();
    }

    return [[paths objectAtIndex:0] fileSystemRepresentation];
}

std::string GetExecutableFilePath(const char*, Log* log)
{
    ScopedAutoreleasePool autoreleasePool;

    NSBundle* mainBundle = [NSBundle mainBundle];

    if (!mainBundle) {
        log->error("No mainBundle.");
        return std::string();
    }

    return [[mainBundle executablePath] fileSystemRepresentation];
}

std::string GetToolsPath(const char* argv0, Log* log)
{
    std::string installPath = GetExecutableFilePath(argv0, log);
    if (!installPath.empty()) {
        Path::stripLastComponentInPlace(installPath);
        Path::stripTrailingSlashesInPlace(installPath);
    }
    return installPath;
}

std::string GetResourcesPath(const char*, Log* log)
{
    ScopedAutoreleasePool autoreleasePool;

    NSBundle* mainBundle = [NSBundle mainBundle];

    if (!mainBundle) {
        log->error("No mainBundle.");
        return std::string();
    }

    return [[mainBundle resourcePath] fileSystemRepresentation];
}

std::string GetSavePath(const char*, Log* log)
{
    ScopedAutoreleasePool autoreleasePool;
    std::string savePath = FoundationPath(NSDocumentDirectory, NSUserDomainMask, log);
    if (!savePath.empty()) {
        Path::stripTrailingSlashesInPlace(savePath);
    }
    return savePath;
}

std::string GetPluginsPath(const char* appID, Log* log)
{
    return GetSavePath(appID, log);
}

std::string GetTemporaryPath(Log* log)
{
    ScopedAutoreleasePool autoreleasePool;
    NSString* tempPath = NSTemporaryDirectory();

    if (!tempPath) {
        log->error("NSTemporaryDirectory error");
        return std::string();
    }

    return [tempPath fileSystemRepresentation];
}

std::string GetCachePath(const char* appID, Log* log)
{
    ScopedAutoreleasePool autoreleasePool;
    std::string cachePath = FoundationPath(NSCachesDirectory, NSUserDomainMask, log);
    if (!cachePath.empty()) {
        Path::joinInPlace(cachePath, AppIDToLowerCase(appID));
        Path::stripTrailingSlashesInPlace(cachePath);
    }
    return cachePath;
}

std::string GetSystemPluginsPath(const char*, Log* log)
{
    log->error("No plugins path on iOS.");
    return std::string();
}

std::string GetDesktopPath(Log* log)
{
    log->error("No desktop path on iOS.");
    return std::string();
}
}
