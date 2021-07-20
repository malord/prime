// Copyright 2000-2021 Mark H. P. Lord

// Might be useful to expand this to support quoted arguments. Something like this:
// " begins with space.txt" And this is a comment
// \"begins with quote.txt
// \begins with a b.txt
// \\begins with a backslash.txt
// That way, automated response file generators can just prefix every line with a backslash

#include "ResponseFileLoader.h"
#include "UnownedPtr.h"

namespace Prime {

ResponseFileLoader::ResponseFileLoader()
{
    _first = NULL;
}

ResponseFileLoader::~ResponseFileLoader()
{
    while (_first) {
        ResponseFile* kill = _first;
        _first = kill->nextResponseFile;
        delete kill;
    }
}

void ResponseFileLoader::loadResponseFile(const char* path, char*** argv, Log* log)
{
    UnownedPtr<ResponseFile> rsp(new ResponseFile);
    rsp->argv = NULL;
    rsp->nextResponseFile = _first;
    _first = rsp;

    if (!rsp->loader.loadSupportingStdin(path, log)) {
        log->exitError("Can't read response file: %s", path);
    }

    size_t argc = 0;
    for (char** argp = *argv; *argp; argp++) {
        ++argc;
    }

    for (int loop = 0; loop != 2; ++loop) {
        char** out = loop ? rsp->argv : NULL;
        char* ptr = rsp->loader.begin();
        char* end = rsp->loader.end();
        size_t lineCount = 0;
        for (;;) {
            while (ptr != end && *ptr <= ' ') {
                ++ptr;
            }

            if (ptr == end) {
                break;
            }

            char* line = ptr;

            while (ptr != end && *ptr != '\r' && *ptr != '\n') {
                ++ptr;
            }

            char* lineEnd = ptr;
            while (lineEnd-- > line) {
                if (*lineEnd > ' ') {
                    break;
                }
            }
            ++lineEnd;

            if (line != lineEnd) {
                // Check it's not a comment.
                if (*line != '#') {
                    // We have a line.
                    if (loop == 1) {
                        *out++ = line;
                        *lineEnd = 0;
                    }

                    ++lineCount;
                }
            }
        }

        if (loop == 0) {
            rsp->argv = new char*[argc + lineCount + 1];

        } else {
            for (char** argp = *argv; *argp; ++argp) {
                *out++ = *argp;
            }

            *out++ = NULL;
            PRIME_ASSERT((size_t)(out - rsp->argv) == lineCount + argc + 1);
        }
    }

    *argv = rsp->argv;
}
}
