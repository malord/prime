// Copyright 2000-2021 Mark H. P. Lord

#include "../File.h"
#include "../FileLocations.h"
#include "../Path.h"

#if !defined(PRIME_OS_MAC) && !defined(PRIME_OS_IOS)

namespace Prime {

std::string GetExecutableFilePath(const char* argv0, Log* log)
{
    std::string executableFilePath;
    if (!NormalisePath(executableFilePath, argv0, log)) {
        executableFilePath = argv0;
    }

    return executableFilePath;
}

std::string GetToolsPath(const char* argv0, Log* log)
{
    std::string installPath = GetExecutableFilePath(argv0, log);
    Path::stripLastComponentInPlace(installPath);
    Path::stripTrailingSlashesInPlace(installPath);

    return installPath;
}

std::string GetResourcesPath(const char* argv0, Log* log)
{
    return GetToolsPath(argv0, log);
}

static std::string GetHomePath(Log*)
{
    std::string homePath;
    if (const char* home = getenv("HOME")) {
        homePath = home;
    } else {
        // TODO: use glob()?
        return std::string();
    }

    Path::stripTrailingSlashesInPlace(homePath);

    return homePath;
}

std::string GetSavePath(const char* appID, Log* log)
{
    std::string savePath = Path::join(GetHomePath(log), ".config/" + AppIDToLowerCase(appID));

    return savePath;
}

std::string GetPluginsPath(const char* appID, Log* log)
{
    return GetSavePath(appID, log);
}

std::string GetTemporaryPath(Log*)
{
    std::string tempPath;
    if (const char* tmpdir = getenv("TMPDIR")) {
        tempPath = tmpdir;
    } else {
        tempPath = "/tmp";
    }

    Path::stripTrailingSlashesInPlace(tempPath);

    return tempPath;
}

std::string GetCachePath(const char* appID, Log* log)
{
    return Path::join(GetTemporaryPath(log), AppIDToLowerCase(appID));
}

std::string GetSystemPluginsPath(const char* appID, Log*)
{
    return std::string();
}

std::string GetDesktopPath(Log*)
{
    return std::string();
}
}

#endif
