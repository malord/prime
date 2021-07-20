// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FILELOCATIONS_H
#define PRIME_FILELOCATIONS_H

#include "Log.h"
#include <string>

namespace Prime {

/// Returns the full path and file name of the executable.
PRIME_PUBLIC std::string GetExecutableFilePath(const char* argv0, Log* log);

/// Returns the path to the folder where any ancillary executables are located.
PRIME_PUBLIC std::string GetToolsPath(const char* argv0, Log* log);

/// Returns the path to the folder where the application's resources (as installed with the executable) are
/// located.
PRIME_PUBLIC std::string GetResourcesPath(const char* argv0, Log* log);

/// Returns the path to the folder where the application should save any user specific data. appID is, e.g.,
/// com.Prime.Editor, which may become the path Prime/Editor or com.prime.editor depending on OS conventions.
PRIME_PUBLIC std::string GetSavePath(const char* appID, Log* log);

/// Returns the path to the folder where the application can find user installed plugins. This may be within
/// the save path. See GetSavePath() for a description of appID.
PRIME_PUBLIC std::string GetPluginsPath(const char* appID, Log* log);

/// Returns the path to a folder where the application can store temporary files. These files can be deleted at
/// any time. The files aren't in an app specific folder, so unique names should be used.
PRIME_PUBLIC std::string GetTemporaryPath(Log* log);

/// Returns the path to the user's cache folder. An application can save files here, but they may be deleted by
/// the OS to free space. See GetSavePath() for a description of appID.
PRIME_PUBLIC std::string GetCachePath(const char* appID, Log* log);

/// Returns the path to the folder where the application can find plugins installed for all users on the local
/// machine. See GetSavePath() for a description of appID.
PRIME_PUBLIC std::string GetSystemPluginsPath(const char* appID, Log* log);

/// Returns the path to the user's Desktop or home folder.
PRIME_PUBLIC std::string GetDesktopPath(Log* log);

/// Convert an appID to a relative path. com.Prime.Editor would become Prime/Editor.
PRIME_PUBLIC std::string AppIDToRelativePath(const char* appID);

/// Convert an appID to lower case. com.Prime.Editor would become com.prime.editor.
PRIME_PUBLIC std::string AppIDToLowerCase(const char* appID);

// GetHomePath() isn't implemented because it shouldn't be used - use GetSavePath() as the location to store
// settings file or GetDesktopPath() to put files where the user should see them.
// GetDocumentsPath() isn't implemented because it's better to present the user with a file picker instead.
}

#endif
