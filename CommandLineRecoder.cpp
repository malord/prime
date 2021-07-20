// Copyright 2000-2021 Mark H. P. Lord

#include "CommandLineRecoder.h"

namespace Prime {
char* CommandLineRecoder::noArgs[] = { (char*)"no_args", NULL };
}

#if defined(PRIME_OS_WINDOWS)

#include "StringUtils.h"
#include "Windows/WindowsConfig.h"
#include <vector>

namespace Prime {

namespace {

    void ParseWindowsCommandLine(const char* cmdline, int* argc, char*** argv)
    {
        // Windows 2000 and later have CommandLineToArgvW(), but this was written before I was willing to make
        // Windows 2000 a minimum requirement. Which just made me feel old.

        std::vector<char*> args;
        std::string arg;

        for (const char* ptr = cmdline;;) {
            bool inQuotes;

            while (*ptr == ' ' || *ptr == '\t') {
                ++ptr;
            }

            if (!*ptr) {
                break;
            }

            arg.resize(0);

            inQuotes = false;

            // These are the Windows command line parsing rules. A double quote can be escaped either by two
            // double quotes ("") or by a backslash followed by a double quote (\"). So to resolve an ambiguity,
            // if you have 2n backslashes followed by a double quote then you get n backslashes and the double
            // quote is processed as normal. If you have 2n+1 backslashes then you get n backslashes and an
            // escaped double quote.
            for (;;) {
                bool shouldCopy;
                int slashCount;

                shouldCopy = true;
                slashCount = 0;

                while (*ptr == '\\') {
                    ++ptr;
                    ++slashCount;
                }

                if (*ptr == '"') {
                    if (slashCount % 2 == 0) {
                        if (inQuotes && ptr[1] == '"') {
                            ++ptr;
                        } else {
                            shouldCopy = false;
                            inQuotes = !inQuotes;
                        }
                    }

                    slashCount /= 2;
                }

                while (slashCount--) {
                    arg += '\\';
                }

                if (!*ptr || (!inQuotes && (*ptr == ' ' || *ptr == '\t'))) {
                    break;
                }

                if (shouldCopy) {
                    arg += *ptr;
                }

                ++ptr;
            }

            args.push_back(NewString(arg.c_str()));
        }

        char** newArgv = new char*[args.size() + 1];

        for (size_t i = 0; i != args.size(); ++i) {
            newArgv[i] = (char*)args[i];
        }

        newArgv[args.size()] = 0;

        *argc = (int)args.size();
        *argv = newArgv;
    }
}

void CommandLineRecoder::fixCommandLineEncoding(int* argc, char*** argv, char**& allocedArgv)
{
    freeEncodedCommandLine(allocedArgv);
    ParseWindowsCommandLine(TCharToChar(GetCommandLine()).c_str(), argc, argv);
    allocedArgv = *argv;
}

void CommandLineRecoder::freeEncodedCommandLine(char**& allocedArgv)
{
    if (!allocedArgv) {
        return;
    }

    for (char** ptr = allocedArgv; *ptr; ++ptr) {
        delete[] * ptr;
    }

    delete[] allocedArgv;
    allocedArgv = NULL;
}

}

#else

namespace Prime {

void CommandLineRecoder::fixCommandLineEncoding(int*, char***, char**&)
{
}

void CommandLineRecoder::freeEncodedCommandLine(char**&)
{
}

}

#endif
