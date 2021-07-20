// Copyright 2000-2021 Mark H. P. Lord

//
// Platform independent functions for dealing with different platform's path conventions. e.g.:
//
//     WindowsPath::hasDriveName("c:\\windows") -> true
//     UnixPath::join("/usr/local", "bin") -> "/usr/local/bin"
//     WindowsPath::join("c:\\windows", "system32") -> "c:\\windows\\system32"
//
// The Path typedef is assigned the correct implementation for the target platform (UnixPath, WindowsPath, etc.):
//
//     Path::lastComponent("/path/to/filename.ext") -> "filename.ext"
//
// Functions ending `View` return a StringView referencing characters from the input string.
//

#ifndef PRIME_PATH_H
#define PRIME_PATH_H

#include "StringUtils.h"

namespace Prime {

class PRIME_PUBLIC PathTraitsBase {
public:
    /// Returns true for "." and "..".
    static bool isDotDirectory(StringView name) PRIME_NOEXCEPT
    {
        return name == "." || name == "..";
    }

    /// Convert '/' or '\\' to whichever is correct for this path type.
    static inline char fixSlash(char ch) PRIME_NOEXCEPT
    {
        return ch == '\\' ? '/' : ch;
    }

    /// Convert '/' to the correct path separator for this path type. On platforms where '/' is the path
    /// separator, this does not convert '\\' to '/', unlike fixSlash().
    static inline char fixForwardSlash(char ch) PRIME_NOEXCEPT { return ch; }

    /// This implementation is currently the same for all platforms, to ensure file names are compatible with
    /// as many systems as possible. For future compatibility, rather than using
    /// Path::findInvalidCharacter() (or any function implemented using it), use WindowsPath explicitly.
    static const char* findInvalidCharacter(const char* begin, const char* end) PRIME_NOEXCEPT;
};

/// Can handle drive names, e.g., "game:", as well as drive letters.
class PRIME_PUBLIC WindowsPathTraits : public PathTraitsBase {
public:
    enum { slash = '\\' };

    static const char* const slashes;
    static const char* const separators;

    static bool isSeparator(int ch) PRIME_NOEXCEPT { return ch == '/' || ch == '\\' || ch == ':'; }

    /// Convert '/' or '\\' to whichever is correct for this path type.
    static inline char fixSlash(char ch) PRIME_NOEXCEPT
    {
        return ch == '/' ? '\\' : ch;
    }

    /// Convert '/' to the correct path separator for this path type.
    static inline char fixForwardSlash(char ch) PRIME_NOEXCEPT { return fixSlash(ch); }

    static bool isSlash(int ch) PRIME_NOEXCEPT { return ch == '/' || ch == '\\'; }

    static bool isAbsolute(StringView path) PRIME_NOEXCEPT;

    /// e.g., on Windows \\windows is a relative path (relative to a drive letter).
    static bool isRootRelative(StringView path) PRIME_NOEXCEPT { return !path.empty() && isSlash(path[0]); }

    static size_t getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT;

    /// Note that the drive can be a name, rather than just a letter (e.g., "game:").
    static bool hasDriveName(StringView path, size_t* optionalColonPos = NULL) PRIME_NOEXCEPT;
};

class PRIME_PUBLIC UnixPathTraits : public PathTraitsBase {
public:
    enum { slash = '/' };

    static const char* const slashes;
    static const char* const separators;

    static bool isSeparator(int ch) PRIME_NOEXCEPT { return ch == '/'; }

    static bool isSlash(int ch) PRIME_NOEXCEPT { return ch == '/'; }

    static bool isAbsolute(StringView path) PRIME_NOEXCEPT { return !path.empty() && path[0] == slash; }

    static bool isRootRelative(StringView) PRIME_NOEXCEPT { return false; }

    static size_t getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT;
};

/// Treats /, \\ and : as path separators and supports "mount points" (e.g., "c:", "game:" or "app0:").
class PRIME_PUBLIC GenericPathTraits : public PathTraitsBase {
public:
    enum { slash = '/' };

    static const char* const slashes;
    static const char* const separators;

    static bool isSeparator(int ch) PRIME_NOEXCEPT { return ch == '/' || ch == '\\' || ch == ':'; }

    static bool isSlash(int ch) PRIME_NOEXCEPT { return ch == '/' || ch == '\\'; }

    static bool isAbsolute(StringView path) PRIME_NOEXCEPT;

    static bool isRootRelative(StringView) PRIME_NOEXCEPT { return false; }

    static size_t getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT;

    static bool hasMountPoint(StringView path, size_t* optionalColonPos = NULL) PRIME_NOEXCEPT
    {
        return WindowsPathTraits::hasDriveName(path, optionalColonPos);
    }
};

class PSPPathTraits : public PathTraitsBase {
public:
    enum { slash = '/' };

    static const char* const slashes;
    static const char* const separators;

    static bool isSeparator(int ch) PRIME_NOEXCEPT { return ch == '/' || ch == '\\' || ch == ':'; }

    static bool isSlash(int ch) PRIME_NOEXCEPT { return ch == '/' || ch == '\\'; }

    static bool isAbsolute(StringView path) PRIME_NOEXCEPT;

    /// Returns true if the specified path is relative to a root path. This only applies to Windows where \\wow
    /// refers to "\\wow" on a drive or network share (UNC).
    static bool isRootRelative(StringView path) PRIME_NOEXCEPT;

    /// Returns the length of the root part of the specified path. The length includes the terminating slash, and
    /// *needsSlash is set to true if a slash should be appended.
    static size_t getRootLength(StringView path, bool& needsSlash, bool& hasSlash) PRIME_NOEXCEPT;
};

template <typename Traits>
class BasicPath : public Traits {
public:
    //
    // File name (last component of a path)
    //

    /// Returns a substring holding the last component of a path.
    /// "/usr/bin" -> "bin", /usr/ -> ""
    static StringView lastComponentView(StringView path)
    {
        // Use getRootLength() to make sure we don't venture in to the root of the filename (e.g., the drive
        // letter of the server part of a URL).
        bool needsSlash;
        bool hasSlash;
        const char* begin = path.begin() + Traits::getRootLength(path, needsSlash, hasSlash);

        const char* at = path.end();
        while (at-- != begin) {
            if (Traits::isSeparator(*at)) {
                break;
            }
        }

        return StringView(at + 1, path.end());
    }

    /// Returns a string holding the last component of a path.
    /// "/usr/bin" -> "bin", /usr/ -> ""
    static std::string lastComponent(StringView path)
    {
        return lastComponentView(path).to_string();
    }

    static const char* findLastComponent(const char* path)
    {
        return lastComponentView(path).begin();
    }

    static char* findLastComponent(char* path)
    {
        return (char*)findLastComponent((const char*)path);
    }

    /// Remove the last component of the path, possibly leaving a trailing path separator.
    /// "/usr/bin" -> "/usr/", "/usr/" -> "/usr/"
    static void stripLastComponentInPlace(std::string& path)
    {
        const char* cstr = path.c_str();
        path.resize((size_t)(lastComponentView(cstr).begin() - cstr));
    }

    /// Returns a substring containing the path part of a path and file name.
    /// "/usr/bin" -> "/usr/", "/usr/" -> "/usr/"
    static StringView stripLastComponentView(StringView path)
    {
        return StringView(path.begin(), lastComponentView(path).begin());
    }

    /// Returns a string containing the path part of a path and file name.
    /// "/usr/bin" -> "/usr/", "/usr/" -> "/usr/"
    static std::string stripLastComponent(StringView path)
    {
        return stripLastComponentView(path).to_string();
    }

    /// Remove any file name from path then replace it with a new one.
    static void replaceLastComponentInPlace(std::string& path, StringView newFilename)
    {
        stripLastComponentInPlace(path);
        joinInPlace(path, newFilename);
    }

    /// Returns the result of replaceLastComponentInPlace.
    static std::string replaceLastComponent(StringView path, StringView filename)
    {
        std::string result(path.begin(), path.end());
        replaceLastComponentInPlace(result, filename);
        return result;
    }

    //
    // Extensions
    //

    /// Returns a substring containing the extension part of a path.
    /// "C:\\Windows\\win.ini" -> ".ini", "C:\\Windows" -> ""
    static StringView exetensionView(StringView path)
    {
        const char* end = path.end();
        const char* ptr = end;

        while (ptr-- != path.begin()) {
            if (*ptr == '.') {
                return StringView(ptr, end);
            }

            if (Traits::isSeparator(*ptr)) {
                return StringView(end, end);
            }
        }

        return StringView(end, end);
    }

    /// Returns a string containing the extension part of a path.
    /// "C:\\Windows\\win.ini" -> ".ini", "C:\\Windows" -> ""
    static std::string exetension(StringView path)
    {
        return exetensionView(path).to_string();
    }

    static const char* findExtension(const char* path)
    {
        return exetensionView(path).begin();
    }

    static char* findExtension(char* path)
    {
        return (char*)findExtension((const char*)path);
    }

    /// Remove an extension from a path (doing nothing if there is no extension).
    static void stripExtensionInPlace(std::string& path)
    {
        const char* cstr = path.c_str();
        path.resize((size_t)(findExtension(cstr) - cstr));
    }

    /// Returns a substring containing a path excluding any extension.
    static StringView stripExtensionView(StringView path)
    {
        return StringView(path.begin(), exetensionView(path).begin());
    }

    /// Returns a string containing a path excluding any extension.
    static std::string stripExtension(StringView path)
    {
        return stripExtensionView(path).to_string();
    }

    /// Appends an extension to an existing path, which must have a file name. A . prefix on the extension is
    /// optional, and empty extensions are ignored. Explicitly appending "." will append a .
    static void appendExtensionInPlace(std::string& path, StringView extension)
    {
        if (!extension.empty()) {
            if (extension[0] != '.') {
                path += '.';
            }

            path += extension;
        }
    }

    /// Remove any file extension from path then replace it with a new one.
    static void replaceExtensionInPlace(std::string& path, StringView newExtension)
    {
        stripExtensionInPlace(path);
        appendExtensionInPlace(path, newExtension);
    }

    static std::string replaceExtension(StringView path, StringView extension)
    {
        std::string result(path.begin(), path.end());
        replaceExtensionInPlace(result, extension);
        return result;
    }

    //
    // Trailing slashes
    //

    /// Returns true if the path has a terminating path separator (i.e., a file name can just be appended).
    static bool hasTerminatingSeparator(StringView path)
    {
        return !path.empty() && Traits::isSeparator(path.back());
    }

    /// Returns a pointer to the trailing slash of a path. If the path does not have a trailing slash, or if the
    /// trailing slash is significant to the meaning of the path, returns a pointer to the end of the path.
    static StringView trailingSlashesView(StringView path)
    {
        size_t pathLength = path.size();
        if (!pathLength) {
            return path;
        }

        bool needsSlash, hasSlash;

        // Use the root length to make sure we don't start removing the : from a drive letter or :\ from a
        // drive root.
        size_t rootLength = Traits::getRootLength(path, needsSlash, hasSlash);

        for (;;) {
            if (pathLength == rootLength || !Traits::isSlash(path[pathLength - 1])) {
                return StringView(path.begin() + pathLength, path.end());
            }

            --pathLength;
        }
    }

    static const char* findTrailingSlashes(const char* path)
    {
        return trailingSlashesView(path).begin();
    }

    static char* findTrailingSlashes(char* path)
    {
        return (char*)findTrailingSlashes((const char*)path);
    }

    static bool hasTrailingSlashes(StringView path)
    {
        return trailingSlashesView(path).begin() != path.end();
    }

    /// Remove any trailing slashes from a path.
    static void stripTrailingSlashesInPlace(std::string& path)
    {
        const char* cstr = path.c_str();
        path.resize((size_t)(trailingSlashesView(cstr).begin() - cstr));
    }

    static StringView stripTrailingSlashesView(StringView path)
    {
        return StringView(path.begin(), trailingSlashesView(path).begin());
    }

    static std::string stripTrailingSlashes(StringView path)
    {
        return stripTrailingSlashesView(path).to_string();
    }

    //
    // Trailing dot
    //

    /// Return a substring containing any trailing dot in a path.
    static StringView trailingDotView(StringView path)
    {
        const char* ptr = path.end();
        while (ptr > path.begin() && ptr[-1] == '.') {
            --ptr;
        }

        return StringView(ptr, path.end());
    }

    /// If the path ends with a '.', remove it.
    static void stripTrailingDotInPlace(std::string& path)
    {
        const char* cstr = path.c_str();
        path.resize((size_t)(trailingDotView(cstr).begin() - cstr));
    }

    //
    // Join
    //

protected:
    /// Work out where two paths should be joined and whether a slash  should be inserted.
    static void computeJoin(StringView base, StringView join, size_t& joinPoint, char& joinSlash, size_t& joinSkip)
    {
        joinPoint = 0;
        joinSlash = 0;
        joinSkip = 0;

        if (Traits::isAbsolute(join)) {
            return;
        }

        size_t pathLength = base.size();

        bool needsSlash;
        bool hasSlash;

        if (Traits::isRootRelative(join)) {
            joinPoint = Traits::getRootLength(base, needsSlash, hasSlash);
        } else {
            joinPoint = pathLength;

            hasSlash = (pathLength && hasTerminatingSeparator(base));

            needsSlash = pathLength && !hasSlash;
        }

        // Prevent double path separators.
        bool joinHasPathSeparator = !join.empty() && Traits::isSeparator(join[0]);

        if (needsSlash && joinHasPathSeparator) {
            needsSlash = false;
        } else if (hasSlash && joinHasPathSeparator) {
            joinSkip = 1;
        }

        if (needsSlash) {
            joinSlash = Traits::slash;
        }
    }

public:
    /// Join joinee on to base. e.g. joinInPlace("/usr/local", "bin") would yield "/usr/local/bin".
    static bool joinInPlace(char* base, size_t baseSize, StringView joinee)
    {
        size_t joinPoint;
        char joinSlash[] = "/";
        size_t joinSkip;

        computeJoin(base, joinee, joinPoint, joinSlash[0], joinSkip);

        base[joinPoint] = 0;

        bool success = true;

        if (joinSlash[0]) {
            if (!StringAppend(base, baseSize, joinSlash)) {
                success = false;
            }
        }

        return success && StringAppend(base, baseSize, joinee.substr(joinSkip));
    }

    /// Join joinee on to base. e.g. joinInPlace("/usr/local", "bin") would yield "/usr/local/bin".
    static void joinInPlace(std::string& base, StringView joinee)
    {
        size_t joinPoint;
        char joinSlash;
        size_t joinSkip;

        computeJoin(base, joinee, joinPoint, joinSlash, joinSkip);

        base.resize(joinPoint);

        if (joinSlash) {
            base += joinSlash;
        }

        base += joinee.substr(joinSkip);
    }

    static std::string join(StringView base, StringView other)
    {
        size_t joinPoint;
        char joinSlash;
        size_t otherSkip;
        computeJoin(base, other, joinPoint, joinSlash, otherSkip);

        size_t otherSize = other.size();
        size_t outputSize = joinPoint + (otherSize - otherSkip) + (joinSlash ? 1 : 0);

        std::string output;
        if (outputSize) {
            output.reserve(outputSize);
            StringCopy(output, base);
            output.resize(outputSize, 0);

            char* outputPtr = &output[0] + joinPoint;

            if (joinSlash) {
                *outputPtr++ = joinSlash;
            }

            std::copy(other.begin() + otherSkip, other.begin() + otherSize, outputPtr);
        }

        return output;
    }

    //
    // Normalising paths
    //

    /// Returns a substring containing all of a path excluding any leading slashes.
    static StringView stripLeadingSlashesView(StringView path)
    {
        const char* ptr;
        for (ptr = path.begin(); ptr != path.end() && Traits::isSlash(*ptr); ++ptr) { }

        return StringView(ptr, path.end());
    }

    static std::string stripLeadingSlashes(StringView path)
    {
        return stripLeadingSlashesView(path).to_string();
    }

    static const char* skipSlashes(const char* path)
    {
        return stripLeadingSlashesView(path).begin();
    }

    static char* skipSlashes(char* path)
    {
        return (char*)skipSlashes((const char*)path);
    }

    /// Convert '/' or '\\' to whichever is correct for this path type.
    template <typename Iter>
    static void fixSlashesInPlace(Iter begin, Iter end)
    {
        for (; begin != end; ++begin) {
            *begin = Traits::fixSlash(*begin);
        }
    }

    /// Convert '/' or '\\' to whichever is correct for this path type.
    static void fixSlashesInPlace(char* path)
    {
        for (; *path; ++path) {
            *path = Traits::fixSlash(*path);
        }
    }

    /// Convert '/' or '\\' to whichever is correct for this path type.
    static void fixSlashesInPlace(std::string& path)
    {
        fixSlashesInPlace(path.begin(), path.end());
    }

    static std::string fixSlashes(StringView path)
    {
        std::string result(path.begin(), path.end());
        std::transform(result.begin(), result.end(), result.begin(), Traits::fixSlash);
        return result;
    }

    /// Convert '/' to the correct path separator for this path type.
    template <typename Iter>
    static void fixForwardSlashesInPlace(Iter begin, Iter end)
    {
        for (; begin != end; ++begin) {
            *begin = Traits::fixForwardSlash(*begin);
        }
    }

    /// Convert '/' to the correct path separator for this path type.
    static void fixForwardSlashesInPlace(char* path)
    {
        for (; *path; ++path) {
            *path = Traits::fixForwardSlash(*path);
        }
    }

    /// Convert '/' to the correct path separator for this path type.
    static void fixForwardSlashesInPlace(std::string& path)
    {
        fixForwardSlashesInPlace(path.begin(), path.end());
    }

    static std::string fixForwardSlashes(StringView path)
    {
        std::string result(path.begin(), path.end());
        std::transform(result.begin(), result.end(), result.begin(), Traits::fixForwardSlash);
        return result;
    }

    /// Reduce all occurrences of double slashes to single slashes (e.g., "/usr//bin" => "/usr/bin").
    template <typename Iter>
    static Iter eraseDuplicateSlashesInPlace(Iter begin, Iter end)
    {
        Iter out = begin;
        while (begin != end) {
            *out++ = *begin;
            if (Traits::isSlash(*begin)) {
                do {
                    ++begin;
                } while (begin != end && Traits::isSlash(*begin));
            } else {
                ++begin;
            }
        }

        return out;
    }

    /// Reduce all occurrences of double slashes to single slashes (e.g., "/usr//bin" => "/usr/bin").
    static void eraseDuplicateSlashesInPlace(char* path)
    {
        *eraseDuplicateSlashesInPlace(path, path + strlen(path)) = '\0';
    }

    /// Reduce all occurrences of double slashes to single slashes (e.g., "/usr//bin" => "/usr/bin").
    static std::string stripDuplicateSlashes(StringView path)
    {
        std::string result(path.begin(), path.end());
        result.erase(eraseDuplicateSlashesInPlace(result.begin(), result.end()), result.end());
        return result;
    }

    /// Replace all invalid characters with another character.
    static void replaceInvalidCharactersInPlace(char* begin, char* end, char replacement)
    {
        for (;;) {
            begin = (char*)Traits::findInvalidCharacter(begin, end);
            if (begin == end) {
                break;
            }

            *begin++ = replacement;
        }
    }

    static std::string replaceInvalidCharacters(StringView source, char replacement)
    {
        std::string result(source.begin(), source.end());
        if (!result.empty()) {
            replaceInvalidCharactersInPlace(&result[0], &result[0] + result.size(), replacement);
        }

        return result;
    }

    enum FixPathOptions {
        FixPathKeepTrailingSlashes = 1u << 0,
    };

    /// Remove duplicate slashes (/a//b/c -> /a/b/c), "." and ".." relative paths and any trailing slashes
    /// and convert '\\' and '/' to the correct slash for this platform.
    static std::string tidy(StringView input, unsigned int options = 0)
    {
        const char* ptr = input.begin();
        const char* end = input.end();

        std::string fixed;

        while (ptr != end) {
            if (Traits::isSeparator(*ptr)) {
                char sep = Traits::fixSlash(*ptr);
                fixed += sep;

                // Skip duplicate slashes.
                do {
                    ++ptr;
                } while (ptr != end && sep == Traits::fixSlash(*ptr));
            }

            if (ptr != end && *ptr == '.') {
                if (ptr + 1 == end || Traits::isSeparator(ptr[1])) {
                    // Ignore ./ anywhere or . at the end of the path (a/./b -> a/b, a/. -> a/)
                    ptr = stripLeadingSlashesView(StringView(ptr + 1, end)).begin();
                    continue;

                } else if (ptr[1] == '.' && (ptr + 2 == end || Traits::isSeparator(ptr[2]))) {
                    // Pop a component from the path if we have .. or ../
                    const char* fixedBegin = fixed.c_str();
                    const char* last = trailingSlashesView(fixedBegin).begin();
                    bool needsSlash, hasSlash;
                    size_t rootLength = Traits::getRootLength(fixedBegin, needsSlash, hasSlash);
                    while (last > fixedBegin + rootLength) {
                        --last;
                        if (!Traits::isSeparator(*last)) {
                            continue;
                        }

                        while (last != fixedBegin + rootLength) {
                            --last;
                            if (!Traits::isSeparator(*last)) {
                                ++last;
                                break;
                            }
                        }

                        ++last;
                        break;
                    }

                    fixed.resize((size_t)(last - fixedBegin));
                    ptr = stripLeadingSlashesView(StringView(ptr + 2, end)).begin();
                    continue;
                }
            }

            const char* start = ptr;

            while (ptr != end && !Traits::isSeparator(*ptr)) {
                ++ptr;
            }

            fixed.append(start, (size_t)(ptr - start));
        }

        if (!(options & FixPathKeepTrailingSlashes)) {
            const char* fixedBegin = fixed.c_str();
            const char* trailing = findTrailingSlashes(fixedBegin);
            fixed.resize((size_t)(trailing - fixedBegin));
        }

        return fixed;
    }

private:
    BasicPath() PRIME_NOEXCEPT { }
};

typedef BasicPath<WindowsPathTraits> WindowsPath;
typedef BasicPath<UnixPathTraits> UnixPath;
typedef BasicPath<GenericPathTraits> GenericPath;
typedef BasicPath<PSPPathTraits> PSPPath;

#if defined(PRIME_OS_WINDOWS)
typedef WindowsPath Path;
#elif defined(PRIME_OS_UNIX)
typedef UnixPath Path;
#else
typedef GenericPath Path;
#endif
}

#endif // PRIME_PATHS_H
