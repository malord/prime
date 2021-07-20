// Copyright 2000-2021 Mark H. P. Lord

#include "../FileLocations.h"
#include "../Path.h"
#include "ScopedAutoreleasePool.h"

namespace Prime {

static std::string FoundationPath(NSSearchPathDirectory directory, NSSearchPathDomainMask domainMask, BOOL expandTilde = YES)
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(directory, domainMask, expandTilde);

    if (![paths count]) {
        return std::string();
    }

    return [[paths objectAtIndex:0] fileSystemRepresentation];
}

std::string GetExecutableFilePath(const char*, Log*)
{
    ScopedAutoreleasePool autoreleasePool;

    NSBundle* mainBundle = [NSBundle mainBundle];

    if (!mainBundle) {
        return std::string();
    }

    return [[mainBundle executablePath] fileSystemRepresentation];
}

std::string GetToolsPath(const char* argv0, Log* log)
{
    std::string toolsPath = GetExecutableFilePath(argv0, log);
    Path::stripLastComponentInPlace(toolsPath);
    Path::stripTrailingSlashesInPlace(toolsPath);
    return toolsPath;
}

std::string GetResourcesPath(const char*, Log*)
{
    ScopedAutoreleasePool autoreleasePool;

    NSBundle* mainBundle = [NSBundle mainBundle];

    if (!mainBundle) {
        return std::string();
    }

    std::string dataPath = [[mainBundle resourcePath] fileSystemRepresentation];

    Path::stripTrailingSlashesInPlace(dataPath);

    return dataPath;
}

std::string GetSavePath(const char* appID, Log*)
{
    ScopedAutoreleasePool autoreleasePool;
    std::string savePath = Path::join(FoundationPath(NSApplicationSupportDirectory, NSUserDomainMask), AppIDToRelativePath(appID));
    Path::stripTrailingSlashesInPlace(savePath);

    return savePath;
}

std::string GetPluginsPath(const char* appID, Log* log)
{
    return GetSavePath(appID, log);
}

std::string GetCachePath(const char* appID, Log*)
{
    ScopedAutoreleasePool autoreleasePool;
    std::string cachePath = Path::join(FoundationPath(NSCachesDirectory, NSLocalDomainMask), AppIDToRelativePath(appID));
    Path::stripTrailingSlashesInPlace(cachePath);

    return cachePath;
}

std::string GetTemporaryPath(Log*)
{
    ScopedAutoreleasePool autoreleasePool;

    NSString* nsTempPath = NSTemporaryDirectory();

    if (!nsTempPath) {
        return std::string();
    }

    std::string tempPath = [nsTempPath fileSystemRepresentation];

    Path::stripTrailingSlashesInPlace(tempPath);

    return tempPath;
}

std::string GetSystemPluginsPath(const char* appID, Log*)
{
    ScopedAutoreleasePool autoreleasePool;
    std::string systemSavePath = Path::join(FoundationPath(NSApplicationSupportDirectory, NSLocalDomainMask), AppIDToRelativePath(appID));
    Path::stripTrailingSlashesInPlace(systemSavePath);

    return systemSavePath;
}

std::string GetDesktopPath(Log*)
{
    ScopedAutoreleasePool autoreleasePool;
    std::string desktopPath = FoundationPath(NSDesktopDirectory, NSLocalDomainMask);
    Path::stripTrailingSlashesInPlace(desktopPath);

    return desktopPath;
}
}
