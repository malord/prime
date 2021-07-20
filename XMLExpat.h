// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLEXPAT_H
#define PRIME_XMLEXPAT_H

#include "XMLPullParser.h"

namespace Prime {

/// An expat adapter for XMLPullParser.
class PRIME_PUBLIC XMLExpat {
public:
    XMLExpat();

    ~XMLExpat();

    void setUserData(void* userData) { _userData = userData; }

    typedef void (*StartElementHandler)(void* userData, const char* name, const char** atts);
    typedef void (*EndElementHandler)(void* userData, const char* name);

    void setElementHandler(StartElementHandler startElementHandler, EndElementHandler endElementHandler);

    typedef void (*CharacterDataHandler)(void* userData, const char* s, int len);

    void setCharacterDataHandler(CharacterDataHandler characterDataHandler);

    /// Instead of pushing XML file contents in a chunk at a time, you provide an XMLPullParser.
    bool run(XMLPullParser& parser);

private:
    void* _userData;

    StartElementHandler _startHandler;
    EndElementHandler _endHandler;
    CharacterDataHandler _characterHandler;

    XMLPullParser* _parser;
};
}

#endif
