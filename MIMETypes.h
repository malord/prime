// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_MIMETYPES_H
#define PRIME_MIMETYPES_H

#include "Config.h"
#include "Log.h"
#include "Value.h"
#include <map>

namespace Prime {

/// MIME types database.
class PRIME_PUBLIC MIMETypes : public RefCounted {
public:
    MIMETypes();

    /// Load from a dictionary of extension : mime_type pairs.
    void load(const Value::Dictionary& dict);

    StringView getMIMETypeForExtension(StringView extension) const;

    void setCompressedExtension(StringView extension, bool compressed);

    bool isCompressedExtension(StringView extension) const;

    void setCompressedExtensions(const std::vector<std::string>& extensions);

private:
    struct Extension {
        std::vector<std::string> mimeTypes;
        bool compressed;

        Extension()
            : compressed(false)
        {
        }
    };

    typedef std::map<std::string, Extension, std::less<std::string>> Map;
    Map _map;
};
}

#endif
