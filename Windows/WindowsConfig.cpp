// Copyright 2000-2021 Mark H. P. Lord

#include "WindowsConfig.h"

namespace Prime {

std::string WCharToChar(UINT codepage, WCharStringView wstring)
{
    std::string string;

    int len = WideCharToMultiByte(codepage, 0, wstring.begin(), (int)wstring.size(), 0, 0, 0, 0);
    if (len >= 0) {
        string.resize(len);
        WideCharToMultiByte(codepage, 0, wstring.begin(), (int)wstring.size(), &string[0], len, 0, 0);
    } else {
        DeveloperWarning("WideCharToMultiByte failure.");
    }

    return string;
}

WCharString CharToWChar(UINT codepage, StringView string)
{
    WCharString wstring;

    int len = MultiByteToWideChar(codepage, 0, string.begin(), (int)string.size(), 0, 0);
    if (len >= 0) {
        wstring.resize(len);
        MultiByteToWideChar(codepage, 0, string.begin(), (int)string.size(), &wstring[0], len);
    } else {
        DeveloperWarning("MultiByteToWideChar failure.");
    }

    return wstring;
}

#ifdef PRIME_OS_WINDOWS_UNICODE

std::string TCharToChar(TCharStringView string)
{
    return WCharToChar(CP_UTF8, string);
}

TCharString CharToTChar(StringView string)
{
    return CharToWChar(CP_UTF8, string);
}

#elif defined(PRIME_WIN98_UTF8)

std::string TCharToChar(TCharStringView string)
{
    return WCharToChar(CP_UTF8, CharToWChar(CP_ACP, string));
}

TCharString CharToTChar(StringView string)
{
    return WCharToChar(CP_ACP, CharToWChar(CP_UTF8, string));
}

#else

std::string TCharToChar(TCharStringView string)
{
    return std::string(string.begin(), string.end());
}

TCharString CharToTChar(StringView string)
{
    return TCharString(string.begin(), string.end());
}

#endif

}
