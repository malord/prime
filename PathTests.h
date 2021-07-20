// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_PATHTESTS_H
#define PRIME_PATHTESTS_H

#include "Path.h"

namespace Prime {

inline void PathTests()
{
    std::string path = "C:\\Program Files (x86)\\Hrunk\\Random App 17\\Random App.exe";

    std::string path2 = WindowsPath::stripLastComponent(path);
    PRIME_TEST(path2 == "C:\\Program Files (x86)\\Hrunk\\Random App 17\\");

    std::string path4 = WindowsPath::stripTrailingSlashes(path2);
    PRIME_TEST(path4 == "C:\\Program Files (x86)\\Hrunk\\Random App 17");

    std::string path3 = WindowsPath::stripExtension(path);
    PRIME_TEST(path3 == "C:\\Program Files (x86)\\Hrunk\\Random App 17\\Random App");

    std::string path5 = WindowsPath::join(path4, "Icon.ico");
    PRIME_TEST(path5 == "C:\\Program Files (x86)\\Hrunk\\Random App 17\\Icon.ico");

    PRIME_TEST(WindowsPath::stripTrailingSlashes("c:\\\\\\\\\\\\") == "c:\\");
    PRIME_TEST(WindowsPath::stripTrailingSlashes("c:windows\\\\\\\\\\\\") == "c:windows");
    PRIME_TEST(WindowsPath::stripTrailingSlashes("\\\\server\\share\\\\\\\\\\\\") == "\\\\server\\share\\");

    // Removed these, they weren't used
    // UnixPath::basename(&temp, "/usr/bin/"); PRIME_TEST(temp == "bin");
    // UnixPath::basename(&temp, "/usr/bin"); PRIME_TEST(temp == "bin");
    // UnixPath::dirname(&temp, "/usr/bin/"); PRIME_TEST(temp == "/usr");
    // UnixPath::dirname(&temp, "/usr/bin"); PRIME_TEST(temp == "/usr");

    PRIME_TEST(UnixPath::stripTrailingSlashes("//////") == "/");
    PRIME_TEST(UnixPath::stripTrailingSlashes("/a/////") == "/a");
    PRIME_TEST(UnixPath::stripTrailingSlashes("/") == "/");
    PRIME_TEST(UnixPath::stripTrailingSlashes("") == "");
    PRIME_TEST(UnixPath::stripTrailingSlashes("a///") == "a");

    PRIME_TEST(GenericPath::isAbsolute("game:/path/to/date"));
    PRIME_TEST(GenericPath::isAbsolute("game:path/to/date"));
    PRIME_TEST(GenericPath::isAbsolute("/path/to/date"));
    PRIME_TEST(!GenericPath::isAbsolute("path/to/date"));

    PRIME_TEST(GenericPath::join("game:", "path/to/data") == "game:path/to/data");
    PRIME_TEST(GenericPath::join("game:", "app0:path/to/data") == "app0:path/to/data");

    PRIME_TEST(GenericPath::join("", "path/to/data") == "path/to/data");
    PRIME_TEST(GenericPath::join("a", "path/to/data") == "a/path/to/data");
    PRIME_TEST(GenericPath::join("/", "path/to/data") == "/path/to/data");
}
}

#endif
