// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_URLVIEW_H
#define PRIME_URLVIEW_H

#include "Config.h"
#include "Dictionary.h"
#include "ScopedPtr.h"
#include "StringUtils.h"
#include "StringView.h"
#include "Value.h" // This can be safely commented out
#include <string>
#include <vector>

namespace Prime {

class URL;
class URLBuilder;
class URLPath;

//
// URLDictionary
//

/// Stores an array of key/value pairs where the pairs are kept in the order they're added, there can be multiple
/// values for the same key, and keys are ASCII case insensitive.
class PRIME_PUBLIC URLDictionary {
public:
    typedef Prime::Dictionary<std::string, std::string> DictionaryType;

    typedef DictionaryType::value_type value_type;
    typedef DictionaryType::iterator iterator;
    typedef DictionaryType::const_iterator const_iterator;

    static bool equalKeys(StringView a, StringView b) PRIME_NOEXCEPT
    {
        return ASCIIEqualIgnoringCase(a, b);
    }

    void clear() PRIME_NOEXCEPT;

    const std::string& get(StringView key) const PRIME_NOEXCEPT;

    const std::string& operator[](StringView key) const PRIME_NOEXCEPT;

    std::vector<std::string> getAll(StringView key) const;

    std::vector<StringView> getAllViews(StringView key) const;

    bool has(StringView key) const PRIME_NOEXCEPT;

    void set(StringView key, StringView value);

    void add(StringView key, StringView value);

    void remove(StringView key);

    iterator find(StringView key) PRIME_NOEXCEPT;

    size_t getSize() const PRIME_NOEXCEPT { return _pairs.size(); }

    bool empty() const PRIME_NOEXCEPT { return _pairs.empty(); }

    const value_type& pair(size_t index) const PRIME_NOEXCEPT { return _pairs.pair(index); }

#ifdef PRIME_HAVE_VALUE
    Value::Dictionary toDictionary() const;
#endif

    iterator begin() PRIME_NOEXCEPT
    {
        return _pairs.begin();
    }
    iterator end() PRIME_NOEXCEPT { return _pairs.end(); }

    const_iterator begin() const PRIME_NOEXCEPT { return _pairs.begin(); }
    const_iterator end() const PRIME_NOEXCEPT { return _pairs.end(); }

    const_iterator cbegin() const PRIME_NOEXCEPT { return _pairs.begin(); }
    const_iterator cend() const PRIME_NOEXCEPT { return _pairs.end(); }

private:
    DictionaryType _pairs;
};

//
// URLView
//

/// A view of an absolute or relative Uniform Resource Locator (as defined in RFC 1808). This has all the
/// shooting-self-in-foot aspects of StringView in that no copies of the strings are made.
class PRIME_PUBLIC URLView {
public:
    class ParseOptions {
    public:
        ParseOptions()
            : _allowRelativeHosts(false)
            , _allowProtocolWithoutLocation(false)
            , _hostOnly(false)
        {
        }

        ParseOptions(const ParseOptions& copy) { operator=(copy); }

        ParseOptions& setAllowRelativeHosts(bool value = true)
        {
            _allowRelativeHosts = value;
            return *this;
        }
        bool getAllowRelativeHosts() const { return _allowRelativeHosts; }

        ParseOptions& setAllowProtocolWithoutLocation(bool value = true)
        {
            _allowProtocolWithoutLocation = value;
            return *this;
        }
        bool getAllowProtocolWithoutLocation() const { return _allowProtocolWithoutLocation; }

        ParseOptions& setHostOnly(bool value = true)
        {
            _hostOnly = value;
            return *this;
        }
        bool getHostOnly() const { return _hostOnly; }

    private:
        bool _allowRelativeHosts;
        bool _allowProtocolWithoutLocation;
        bool _hostOnly;
    };

    URLView();

    URLView(StringView string, const ParseOptions& options = ParseOptions());

    URLView(const char* string, const ParseOptions& options = ParseOptions());

    URLView(const std::string& string, const ParseOptions& options = ParseOptions());

    URLView(const URL& url);

    URLView(const URLBuilder& url);

    URLView(const URLView& copy);

    bool parse(StringView string, const ParseOptions& options = ParseOptions());

    const StringView& getProtocol() const { return _protocol; }
    const StringView& getUsername() const { return _username; }
    const StringView& getPassword() const { return _password; }
    const StringView& getHost() const { return _host; }
    const StringView& getPort() const { return _port; }
    const StringView& getPath() const { return _path; }
    const StringView& getParameter() const { return _parameter; }
    const StringView& getQuery() const { return _query; }
    const StringView& getFragment() const { return _fragment; }

    URLView& setProtocol(StringView view)
    {
        _protocol = view;
        return *this;
    }
    URLView& setUsername(StringView view)
    {
        _username = view;
        return *this;
    }
    URLView& setPassword(StringView view)
    {
        _password = view;
        return *this;
    }
    URLView& setHost(StringView view)
    {
        _host = view;
        return *this;
    }
    URLView& setPort(StringView view)
    {
        _port = view;
        return *this;
    }
    URLView& setPath(StringView view)
    {
        _path = view;
        return *this;
    }
    URLView& setParameter(StringView view)
    {
        _parameter = view;
        return *this;
    }
    URLView& setQuery(StringView view)
    {
        _query = view;
        return *this;
    }
    URLView& setFragment(StringView view)
    {
        _fragment = view;
        return *this;
    }

    StringView getPathWithoutSlash() const;

    StringView getEncodedQuery(StringView name) const;
    std::vector<StringView> getEncodedQueryArray(StringView name) const;

    StringView getEncodedParameter(StringView name) const;
    std::vector<StringView> getEncodedParameterArray(StringView name) const;

    std::string getQuery(StringView name) const;
    std::vector<std::string> getQueryArray(StringView name) const;

    std::string getParameter(StringView name) const;
    std::vector<std::string> getParameterArray(StringView name) const;

    /// Returns everything after the host/port (including the leading /).
    std::string getResource() const { return toString(StringOptions().setResourceOnly()); }

    /// Returns the resource, without the fragment.
    std::string getResourceWithoutFragment() const { return toString(StringOptions().setResourceOnly().setDiscardFragment()); }

    std::string getHostWithPort() const;

    /// The location is what RFC 1808 refers to as the "net_loc", the "hostname:port" part of the URL.
    bool hasLocation() const PRIME_NOEXCEPT;

    bool hasProtocol() const PRIME_NOEXCEPT { return !_protocol.empty(); }

    bool isEmpty() const;

    /// Returns a URLView which contains only the protocol, username/password, host and port from this URL.
    URLView getRoot() const;

    /// Splits the path in to a URLPath.
    URLPath getPathComponents() const;

    /// Splits the query string in to a URLDictionary.
    URLDictionary getQueryComponents() const;

    class StringOptions {
    public:
        StringOptions()
            : _discardQuery(false)
            , _discardCredentials(false)
            , _resourceOnly(false)
            , _discardFragment(false)
        {
        }

        StringOptions& setDiscardQuery(bool value = true)
        {
            _discardQuery = value;
            return *this;
        }
        bool getDiscardQuery() const { return _discardQuery; }

        StringOptions& setDiscardCredentials(bool value = true)
        {
            _discardCredentials = value;
            return *this;
        }
        bool getDiscardCredentials() const { return _discardCredentials; }

        StringOptions& setResourceOnly(bool value = true)
        {
            _resourceOnly = value;
            return *this;
        }
        bool getResourceOnly() const { return _resourceOnly; }

        StringOptions& setDiscardFragment(bool value = true)
        {
            _discardFragment = value;
            return *this;
        }
        bool getDiscardFragment() const { return _discardFragment; }

        StringOptions& setLogSafe() { return setDiscardCredentials().setDiscardQuery(); }

    private:
        bool _discardQuery;
        bool _discardCredentials;
        bool _resourceOnly;
        bool _discardFragment;
    };

    std::string toString(const StringOptions& options = StringOptions()) const;

    void toString(std::string& buffer, const StringOptions& options = StringOptions()) const;

    void appendString(std::string& buffer, const StringOptions& options = StringOptions()) const;

private:
    const char* parseHostAndPort(const char* ptr, const char* end);

    StringView _protocol;
    StringView _host;
    StringView _port;
    StringView _path;
    StringView _query;
    StringView _fragment;
    StringView _username;
    StringView _password;
    StringView _parameter;
};

//
// URL
//

/// An absolute or relative Uniform Resource Locator (as defined in RFC 1808). A relative URL can be applied
/// to an absolute URL to generate another absolute URL.
class PRIME_PUBLIC URL {
public:
    typedef URLView::ParseOptions ParseOptions;
    typedef URLView::StringOptions StringOptions;

    /// Extracts key1=value&key2=value&... in to a URLDictionary.
    static void parseQueryString(URLDictionary& dictionary, StringView queryString);

    /// Extracts key1=value&key2=value&... in to a Value::Dictionary.
    static void parseQueryString(Value::Dictionary& dictionary, StringView queryString);

    /// Takes a Dictionary and produces a URL encoded query string (e.g., key1=value&key2=value&...).
    static std::string buildQueryString(const URLDictionary& dictionary);

    /// Takes a Value::Dictionary and produces a URL encoded query string (e.g., key1=value&key2=value&...).
    static std::string buildQueryString(const Value::Dictionary& dictionary);

    /// Removes . and .. components from a URL path.
    static void tidyPath(std::string& path);

    URL();

    URL(StringView string, const ParseOptions& options = ParseOptions());

    URL(const char* string, const ParseOptions& options = ParseOptions());

    URL(const std::string& string, const ParseOptions& options = ParseOptions());

#ifdef PRIME_COMPILER_RVALUEREF
    URL(std::string&& string, const ParseOptions& options = ParseOptions());
#endif

    URL(const URL& copy);

    URL& operator=(const URL& copy);

#ifdef PRIME_COMPILER_RVALUEREF
    URL(URL&& move);

    URL& operator=(URL&& move);
#endif

    URL(const URLView& view);

    URL& operator=(const URLView& view);

    URL(const URLBuilder& builder);

    URL& operator=(const URLBuilder& builder);

    bool parse(StringView string, const ParseOptions& options = ParseOptions());
    bool parse(const char* string, const ParseOptions& options = ParseOptions());
    bool parse(const std::string& string, const ParseOptions& options = ParseOptions());

#ifdef PRIME_COMPILER_RVALUEREF
    bool parse(std::string&& string, const ParseOptions& options = ParseOptions());
#endif

    const StringView& getProtocol() const
    {
        return _view.getProtocol();
    }
    const StringView& getUsername() const { return _view.getUsername(); }
    const StringView& getPassword() const { return _view.getPassword(); }
    const StringView& getHost() const { return _view.getHost(); }
    const StringView& getPort() const { return _view.getPort(); }
    const StringView& getPath() const { return _view.getPath(); }
    const StringView& getParameter() const { return _view.getParameter(); }
    const StringView& getQuery() const { return _view.getQuery(); }
    const StringView& getFragment() const { return _view.getFragment(); }

    URL& setProtocol(StringView value);
    URL& setUsername(StringView value);
    URL& setPassword(StringView value);
    URL& setHost(StringView value);
    URL& setPort(StringView value);
    URL& setPath(StringView value);
    URL& setParameter(StringView value);
    URL& setQuery(StringView value);
    URL& setFragment(StringView value);

    StringView getPathWithoutSlash() const { return _view.getPathWithoutSlash(); }

    StringView getEncodedQuery(StringView name) const { return _view.getEncodedQuery(name); }
    std::vector<StringView> getEncodedQueryArray(StringView name) const { return _view.getEncodedQueryArray(name); }

    StringView getEncodedParameter(StringView name) const { return _view.getEncodedParameter(name); }
    std::vector<StringView> getEncodedParameterArray(StringView name) const { return _view.getEncodedParameterArray(name); }

    std::string getQuery(StringView name) const { return _view.getQuery(name); }
    std::vector<std::string> getQueryArray(StringView name) const { return _view.getQueryArray(name); }

    std::string getParameter(StringView name) const { return _view.getParameter(name); }
    std::vector<std::string> getParameterArray(StringView name) const { return _view.getParameterArray(name); }

    /// Returns everything after the host/port (including the leading /).
    std::string getResource() const { return _view.getResource(); }

    /// Returns the resource, without the fragment.
    std::string getResourceWithoutFragment() const { return _view.getResourceWithoutFragment(); }

    std::string getHostWithPort() const { return _view.getHostWithPort(); }

    /// The location is what RFC 1808 refers to as the "net_loc", the "hostname:port" part of the URL.
    bool hasLocation() const PRIME_NOEXCEPT { return _view.hasLocation(); }

    bool hasProtocol() const PRIME_NOEXCEPT { return _view.hasProtocol(); }

    bool isEmpty() const PRIME_NOEXCEPT { return _view.isEmpty(); }

    URL resolve(const URLView& relative) const;

    /// Splits the path in to a URLPath.
    URLPath getPathComponents() const;

    void setPathComponents(const URLPath& path);

    /// Splits the query string in to a URLDictionary.
    URLDictionary getQueryComponents() const;

    void setQueryComponents(const URLDictionary& query);

    const std::string& getString() const { return _storage; }

    std::string toString(const StringOptions& options = StringOptions()) const { return _view.toString(options); }

    void toString(std::string& buffer, const StringOptions& options = StringOptions()) const { return _view.toString(buffer, options); }

    void appendString(std::string& buffer, const StringOptions& options = StringOptions()) const { return _view.appendString(buffer, options); }

    void move(URL& other);

    const URLView& getView() const { return _view; }

    operator URLView&() { return _view; }
    operator const URLView&() const { return _view; }

    static URL resolve(const URLView& base, const URLView& relative);

private:
    /// If you update _view, this updates both _storage and _view. e.g.,
    /// _view.setProtocol(whatever); updateFromView();
    URL& updateFromView();

    std::string _storage;
    URLView _view;
};

//
// URLQueryParser
//

class URLQueryParser {
public:
    static StringView getQueryParameter(StringView queryString, StringView name);

    /// Parses both CGI style name=X&name=Y&name=Z and PHP style name[]=X&name[]=Y&name[]=Z
    static std::vector<StringView> getQueryParameters(StringView queryString, StringView name);

    explicit URLQueryParser(StringView string, bool useSemicolons = false)
        : _ptr(string.begin())
        , _end(string.end())
        , _semi(useSemicolons)
    {
    }

    struct Parameter {
        StringView name;
        StringView value;
    };

    bool read(Parameter& parameter);

private:
    const char* _ptr;
    const char* _end;
    bool _semi;
};

//
// URLBuilder
//

/// A mutable version of URL.
class PRIME_PUBLIC URLBuilder {
public:
    typedef URLView::ParseOptions ParseOptions;
    typedef URLView::StringOptions StringOptions;

    URLBuilder() { }

    URLBuilder(const URLBuilder& copy) { operator=(copy); }
    URLBuilder& operator=(const URLBuilder& copy);

#ifdef PRIME_COMPILER_RVALUEREF
    URLBuilder(URLBuilder&& rhs) PRIME_NOEXCEPT
    {
        move(rhs);
    }

    URLBuilder& operator=(URLBuilder&& rhs) PRIME_NOEXCEPT
    {
        if (this != &rhs) {
            move(rhs);
        }
        return *this;
    }
#endif

    explicit URLBuilder(const URLView& view);
    URLBuilder& operator=(const URLView& view);

    explicit URLBuilder(const URL& view);
    URLBuilder& operator=(const URL& view);

    URLBuilder(const char* string, const ParseOptions& options = ParseOptions()) { parse(string, options); }

    URLBuilder(const std::string& string, const ParseOptions& options = ParseOptions()) { parse(string, options); }

    URLBuilder(StringView string, const ParseOptions& options = ParseOptions()) { parse(string, options); }

    bool parse(StringView string, const ParseOptions& options = ParseOptions());

    std::string toString(const StringOptions& options = StringOptions()) const;

    void toString(std::string& buffer, const StringOptions& options = StringOptions()) const;

    const std::string& getProtocol() const PRIME_NOEXCEPT { return _protocol; }
    void setProtocol(StringView protocol) { StringCopy(_protocol, protocol); }

    const std::string& getUsername() const PRIME_NOEXCEPT { return _rare ? _rare->username : emptyString; }
    void setUsername(StringView username);

    const std::string& getPassword() const PRIME_NOEXCEPT { return _rare ? _rare->password : emptyString; }
    void setPassword(StringView password);

    const std::string& getHost() const PRIME_NOEXCEPT { return _host; }
    void setHost(StringView host) { StringCopy(_host, host); }

    std::string getHostWithPort() const;

    const std::string& getPort() const PRIME_NOEXCEPT { return _port; }
    void setPort(StringView port) { StringCopy(_port, port); }

    const std::string& getPath() const PRIME_NOEXCEPT { return _path; }
    void setPath(StringView path) { StringCopy(_path, path); }

    const char* getPathWithoutSlash() const PRIME_NOEXCEPT
    {
        const char* path = getPath().c_str();
        return *path == '/' ? path + 1 : path;
    }

    const std::string& getQuery() const PRIME_NOEXCEPT { return _query; }

    void setQuery(StringView query) { StringCopy(_query, query); }

    /// Splits the path in to a URLPath.
    URLPath getPathComponents() const;

    void setPathComponents(const URLPath& path);

    /// Splits the query string in to a URLDictionary.
    URLDictionary getQueryComponents() const;

    void setQueryComponents(const URLDictionary& query);

    const std::string& getParameter() const PRIME_NOEXCEPT { return _rare ? _rare->parameter : emptyString; }
    void setParameter(StringView parameter);

    const std::string& getFragment() const PRIME_NOEXCEPT { return _fragment; }
    void setFragment(StringView fragment) { StringCopy(_fragment, fragment); }

    /// Returns everything after the host/port (including the leading /).
    std::string getResource() const { return toString(StringOptions().setResourceOnly()); }

    /// Returns the resource, without the fragment.
    std::string getResourceWithoutFragment() const { return toString(StringOptions().setResourceOnly().setDiscardFragment()); }

    /// Resolve another URL (as though linked to with <a>).
    URLBuilder resolve(const URLBuilder& relative) const;

    /// Resolve another URL (as though linked to with <a>).
    URLBuilder resolve(const URLView& relative) const;

    /// Resolve another URL (as though linked to with <a>).
    URLBuilder resolve(const URL& relative) const;

    bool isEmpty() const PRIME_NOEXCEPT;

    /// The location is what RFC 1808 refers to as the "net_loc", the "hostname:port" part of the URL.
    bool hasLocation() const PRIME_NOEXCEPT;

    bool hasProtocol() const PRIME_NOEXCEPT { return !_protocol.empty(); }

    void swap(URLBuilder& rhs);

    void move(URLBuilder& rhs);

    URLView getView() const;

private:
    void needRare();

    std::string _protocol;
    std::string _host;
    std::string _port;
    std::string _path;
    std::string _query;
    std::string _fragment;

    struct Rare {
        std::string username;
        std::string password;
        std::string parameter;
    };

    ScopedPtr<Rare> _rare;
};

//
// URLPath
//

/// A path within a URL. Initialised from a URL encoded path, deals with decoding %XX sequences within path
/// components and removing "." and "..".
class PRIME_PUBLIC URLPath {
public:
    /// A path component will be skipped if it contains these characters.
    static bool isUnsafe(char ch) PRIME_NOEXCEPT { return ch == '/' || ch == '\\' || ch == ':' || ch == '\0'; }

    static bool isUnsafe(StringView string) PRIME_NOEXCEPT;

    URLPath() PRIME_NOEXCEPT { }

    URLPath(const URLPath& copy)
        : _storage(copy._storage)
        , _lengths(copy._lengths)
    {
    }

#ifdef PRIME_COMPILER_RVALUEREF
    URLPath(URLPath&& rhs) PRIME_NOEXCEPT : _storage(std::move(rhs._storage)),
                                            _lengths(std::move(rhs._lengths))
    {
    }
#endif

    /// Path components containing unsafe characters will be stripped.
    URLPath(const char* path) { parse(path); }

    URLPath(const std::string& path) { parse(path); }

    URLPath(StringView path) { parse(path); }

    URLPath& operator=(const URLPath& copy)
    {
        _storage = copy._storage;
        _lengths = copy._lengths;
        return *this;
    }

#ifdef PRIME_COMPILER_RVALUEREF
    URLPath& operator=(URLPath&& rhs) PRIME_NOEXCEPT
    {
        if (this != &rhs) {
            _storage = std::move(rhs._storage);
            _lengths = std::move(rhs._lengths);
        }
        return *this;
    }
#endif

    void swap(URLPath& other)
    {
        _storage.swap(other._storage);
        _lengths.swap(other._lengths);
    }

    /// Returns false if unsafe characters were found in the path. However, the path will still have been parsed.
    bool parse(StringView string);

    size_t getComponentCount() const PRIME_NOEXCEPT { return _lengths.size(); }

    bool isEmpty() const PRIME_NOEXCEPT { return _lengths.empty(); }

    bool isEmptyOrRoot() const PRIME_NOEXCEPT
    {
        return _lengths.empty() || (_lengths.size() == 1 && _lengths[0] == 0);
    }

    StringView getComponent(size_t index) const PRIME_NOEXCEPT;

    template <typename Index>
    const StringView operator[](Index index) const PRIME_NOEXCEPT { return getComponent(static_cast<size_t>(index)); }

    StringView getComponentElse(size_t index, StringView defaultValue = "") const;

    const StringView getLastComponent() const PRIME_NOEXCEPT
    {
        return _lengths.empty() ? emptyString : StringView(_storage).substr(_storage.size() - _lengths.back());
    }

    /// Returns true if the last component of the path is empty, e.g.,for /test/ the components would be
    /// "test", "". An empty path is not a directory.
    bool isDirectory() const PRIME_NOEXCEPT
    {
        return !_lengths.empty() && _lengths.back() == 0;
    }

    /// Adds an empty final component if necessary to make this path a path to a directory.
    URLPath toDirectory() const;

    //const std::vector<std::string>& getComponents() const PRIME_NOEXCEPT { return _components; }

    /// Unless you're constructing a URLPath from scratch, and know that the last component is not empty,
    /// you probably want to use getRelative().
    URLPath& addComponent(StringView component);

    /// e.g., path.getRelative("login/")
    URLPath getRelative(const URLPath& relative) const;

    class StringOptions {
    public:
        /// The default options create a string ready for URL::setPath.
        StringOptions()
            : _withoutLeadingSlash(false)
            , _withoutEscaping(false)
            , _skipUnsafe(false)
        {
        }

        /// Return "admin/users/" instead of "/admin/users/".
        StringOptions& setWithoutLeadingSlash(bool value = true) PRIME_NOEXCEPT
        {
            _withoutLeadingSlash = value;
            return *this;
        }
        bool getWithoutLeadingSlash() const PRIME_NOEXCEPT { return _withoutLeadingSlash; }

        /// Do not apply URL escaping.
        StringOptions& setWithoutEscaping(bool value = true) PRIME_NOEXCEPT
        {
            _withoutEscaping = value;
            return *this;
        }
        bool getWithoutEscaping() const PRIME_NOEXCEPT { return _withoutEscaping; }

        /// Skip unsafe components in the path.
        StringOptions& setSkipUnsafeComponents(bool value = true) PRIME_NOEXCEPT
        {
            _skipUnsafe = value;
            return *this;
        }
        bool getSkipUnsafeComponents() const { return _skipUnsafe; }

    private:
        bool _withoutLeadingSlash;
        bool _withoutEscaping;
        bool _skipUnsafe;
    };

    std::string toString(const StringOptions& options = StringOptions()) const;

    /// Returns a URLPath containing all the components except the first "skip" components.
    URLPath getTail(size_t skip = 1) const;

    void removeLastComponent();

    URLPath getWithLastComponentRemoved() const;

    bool operator==(const URLPath& rhs) const PRIME_NOEXCEPT;

    bool operator<(const URLPath& rhs) const PRIME_NOEXCEPT;

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const URLPath&)

    bool startsWith(const URLPath& prefix) const PRIME_NOEXCEPT;

    //std::vector<std::string>::const_iterator begin() const PRIME_NOEXCEPT { return _components.begin(); }
    //std::vector<std::string>::const_iterator cbegin() const PRIME_NOEXCEPT { return _components.begin(); }

    //std::vector<std::string>::const_iterator end() const PRIME_NOEXCEPT { return _components.end(); }
    //std::vector<std::string>::const_iterator cend() const PRIME_NOEXCEPT { return _components.end(); }

private:
    size_t offsetOfComponent(size_t index) const;

    std::string _storage;
    std::vector<size_t> _lengths;
};

//
// StringAppend
//

PRIME_PUBLIC bool StringAppend(std::string& output, const URLView& url);
PRIME_PUBLIC bool StringAppend(std::string& output, const URL& url);
PRIME_PUBLIC bool StringAppend(std::string& output, const URLBuilder& url);

// TODO: ostream
}

#endif
