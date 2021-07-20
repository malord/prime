// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_RESPONSEFILELOADER_H
#define PRIME_RESPONSEFILELOADER_H

#include "CommandLineParser.h"
#include "FileLoader.h"

namespace Prime {

/// A simple ResponseFileLoader for CommandLineParser that uses FileLoader to load response files.
/// Lines beginning with # are comments.
class PRIME_PUBLIC ResponseFileLoader : public CommandLineParser::ResponseFileLoader {
public:
    ResponseFileLoader();

    ~ResponseFileLoader();

    virtual void loadResponseFile(const char* path, char*** argv, Log* log) PRIME_OVERRIDE;

private:
    struct ResponseFile {
        FileLoader loader;
        ResponseFile* nextResponseFile;
        char** argv;

        ~ResponseFile()
        {
            delete[] argv;
        }
    };

    ResponseFile* _first;

    PRIME_UNCOPYABLE(ResponseFileLoader);
};
}

#endif
