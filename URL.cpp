// Copyright 2000-2021 Mark H. P. Lord

#include "URL.h"
#include "Range.h"
#include "StringUtils.h"
#include "Templates.h"
#include "TextEncoding.h"

namespace Prime {

inline StringView RebaseStringView(StringView view, const char* oldBase, const char* newBase)
{
    return StringView(view.begin() - oldBase + newBase, view.end() - oldBase + newBase);
}

//
// URLView
//

URLView::URLView()
{
}

URLView::URLView(const URLView& copy)
    : _protocol(copy._protocol)
    , _host(copy._host)
    , _port(copy._port)
    , _path(copy._path)
    , _query(copy._query)
    , _fragment(copy._fragment)
    , _username(copy._username)
    , _password(copy._password)
    , _parameter(copy._parameter)
{
}

URLView::URLView(const URL& url)
{
    operator=(url);
}

URLView::URLView(StringView string, const ParseOptions& options)
{
    parse(string, options);
}

URLView::URLView(const char* string, const ParseOptions& options)
{
    parse(string, options);
}

URLView::URLView(const std::string& string, const ParseOptions& options)
{
    parse(string, options);
}

const char* URLView::parseHostAndPort(const char* ptr, const char* end)
{
    // Everything up to the next / is the "net_loc".
    const char* slash = std::find(ptr, end, '/');

    _host = StringView(ptr, slash);
    ptr = slash;

    // If there's an '@' in there, it's the username and password.
    StringView::iterator at = std::find(_host.begin(), _host.end(), '@');
    if (at != _host.end()) {
        _username = StringView(_host.begin(), at);
        _host = StringView(at + 1, _host.end());

        // Anything following a : in the username is a password
        StringView::iterator password = std::find(_username.begin(), _username.end(), ':');
        if (password != _username.end()) {
            _password = StringView(password + 1, _username.end());
            _username = StringView(_username.begin(), password);
        } else {
            _password = StringView();
        }
    } else {
        _username = StringView();
        _password = StringView();
    }

    // If there's a ':' in there, it's the port.
    StringView::iterator colon = std::find(_host.begin(), _host.end(), ':');
    if (colon != _host.end()) {
        _port = StringView(colon + 1, _host.end());
        _host = StringView(_host.begin(), colon);
    } else {
        _port = StringView();
    }

    return ptr;
}

bool URLView::parse(StringView string, const ParseOptions& options)
{
    // This is a slightly more liberal implementation of the parsing logic specified in RFC 1808.

    const char* begin = string.begin();
    const char* ptr = begin;
    const char* end = string.end();

    //
    // Extract the fragment
    //

    const char* fragment = std::find(ptr, end, '#');
    if (fragment != end) {
        _fragment = StringView(fragment + 1, end);
        end = fragment;
    } else {
        _fragment = StringView();
    }

    //
    // Extract the scheme (protocol)
    //

    bool foundScheme;
    const char* schemeBegin = ptr;
    static const char notSchemeChars[] = ":/\\#?;@";
    static const char* notSchemeCharsEnd = notSchemeChars + PRIME_COUNTOF(notSchemeChars) - 1;
    while (ptr != end && std::find(notSchemeChars, notSchemeCharsEnd, *ptr) == notSchemeCharsEnd) {
        ++ptr;
    }

    if (ptr != end && *ptr == ':' && (options.getAllowProtocolWithoutLocation() || (end - ptr >= 3 && memcmp(ptr, "://", 3) == 0))) {
        _protocol = StringView(schemeBegin, ptr);
        foundScheme = true;
        ++ptr;
    } else {
        _protocol = StringView();
        foundScheme = false;
        ptr = schemeBegin;
    }

    //
    // Extract the "net_loc" (RFC 1808)
    //

    if ((foundScheme || options.getAllowRelativeHosts() || options.getHostOnly()) && end - ptr >= 2 && strncmp(ptr, "//", 2) == 0) {
        ptr += 2;

        ptr = parseHostAndPort(ptr, end);

    } else if (options.getHostOnly()) {
        ptr = parseHostAndPort(ptr, end);
    }

    //
    // Extract the query
    //

    const char* query = std::find(ptr, end, '?');
    if (query != end) {
        _query = StringView(query + 1, end);
        end = query;
    } else {
        _query = StringView();
    }

    //
    // Extract the parameter
    //

    const char* param = std::find(ptr, end, ';');
    if (param != end) {
        _parameter = StringView(param + 1, end);
        end = param;
    } else {
        _parameter = StringView();
    }

    //
    // We're left with the path
    //

    _path = StringView(ptr, end);

    return true;
}

bool URLView::isEmpty() const
{
    return _protocol.empty() && _username.empty() && _password.empty() && _host.empty() && _port.empty() && _path.empty() && _parameter.empty() && _query.empty() && _fragment.empty();
}

StringView URLView::getPathWithoutSlash() const
{
    StringView path = getPath();
    if (!path.empty() && path[0] == '/') {
        path.remove_prefix(1);
    }
    return path;
}

std::string URLView::toString(const StringOptions& options) const
{
    std::string temp;
    appendString(temp, options);
    return temp;
}

void URLView::toString(std::string& buffer, const StringOptions& options) const
{
    buffer.resize(0);
    appendString(buffer, options);
}

void URLView::appendString(std::string& buffer, const StringOptions& options) const
{
    size_t lengthWas = buffer.size();

    if (!options.getResourceOnly() && !_protocol.empty()) {
        buffer += _protocol;
        buffer += ':';
    }

    bool haveLocation = false;

    if (options.getResourceOnly()) {
        haveLocation = true;
    } else if (!_username.empty() || !_password.empty() || !_host.empty() || !_port.empty()) {
        haveLocation = true;

        buffer += "//";
        if (!_username.empty()) {
            if (options.getDiscardCredentials()) {
                buffer += "...";
            } else {
                buffer += getUsername();
            }
        }

        if (!_password.empty()) {
            buffer += ':';

            if (options.getDiscardCredentials()) {
                buffer += "...";
            } else {
                buffer += getPassword();
            }
        }

        if (!_username.empty() || !_password.empty()) {
            buffer += '@';
        }

        buffer += _host;

        if (!_port.empty()) {
            buffer += ':';
            buffer += _port;
        }
    }

    if (!_path.empty()) {
        if (haveLocation && (_path.empty() || _path[0] != '/')) {
            buffer += '/';
        }

        buffer += _path;
    }

    if (!_parameter.empty()) {
        buffer += ';';

        if (options.getDiscardQuery()) {
            buffer += "...";
        } else {
            buffer += getParameter();
        }
    }

    if (!_query.empty()) {
        buffer += '?';

        if (options.getDiscardQuery()) {
            buffer += "...";
        } else {
            buffer += _query;
        }
    }

    if (!options.getDiscardFragment() && !_fragment.empty()) {
        buffer += '#';

        if (options.getDiscardQuery()) {
            buffer += "...";
        } else {
            buffer += _fragment;
        }
    }

    for (size_t i = lengthWas; i < buffer.size(); ++i) {
        if (!IsURLLegal((unsigned char)buffer[i])) {
            char escaped[4];
            StringFormat(escaped, sizeof(escaped), "%%%02x", (uint8_t)buffer[i]);
            buffer.replace(i, 1, escaped, 3);
            i += 2;
        }
    }
}

std::string URLView::getHostWithPort() const
{
    std::string result;

    if (_port.empty()) {
        StringCopy(result, _host);
    } else {
        StringCopy(result, _host);
        result += ':';
        StringAppend(result, _port);
    }

    return result;
}

bool URLView::hasLocation() const PRIME_NOEXCEPT
{
    return !(StringIsEmpty(getUsername()) && StringIsEmpty(getPassword()) && _host.empty() && _port.empty());
}

URLView URLView::getRoot() const
{
    URLView root;
    root.setProtocol(getProtocol());
    root.setUsername(getUsername());
    root.setPassword(getPassword());
    root.setHost(getHost());
    root.setPort(getPort());

    return root;
}

StringView URLView::getEncodedQuery(StringView name) const
{
    return URLQueryParser::getQueryParameter(getQuery(), name);
}

std::vector<StringView> URLView::getEncodedQueryArray(StringView name) const
{
    return URLQueryParser::getQueryParameters(getQuery(), name);
}

std::string URLView::getQuery(StringView name) const
{
    return URLDecode(getEncodedQuery(name), URLDecodeFlagPlusesAsSpaces);
}

std::vector<std::string> URLView::getQueryArray(StringView name) const
{
    std::vector<StringView> encoded = getEncodedQueryArray(name);
    std::vector<std::string> decoded(encoded.size());
    for (size_t i = 0; i != encoded.size(); ++i) {
        StringCopy(decoded[i], encoded[i]);
    }
    return decoded;
}

StringView URLView::getEncodedParameter(StringView name) const
{
    return URLQueryParser::getQueryParameter(getParameter(), name);
}

std::vector<StringView> URLView::getEncodedParameterArray(StringView name) const
{
    return URLQueryParser::getQueryParameters(getParameter(), name);
}

std::string URLView::getParameter(StringView name) const
{
    return URLDecode(getEncodedParameter(name), URLDecodeFlagPlusesAsSpaces);
}

std::vector<std::string> URLView::getParameterArray(StringView name) const
{
    std::vector<StringView> encoded = getEncodedParameterArray(name);
    std::vector<std::string> decoded(encoded.size());
    for (size_t i = 0; i != encoded.size(); ++i) {
        StringCopy(decoded[i], encoded[i]);
    }
    return decoded;
}

URLPath URLView::getPathComponents() const
{
    return URLPath(getPath());
}

URLDictionary URLView::getQueryComponents() const
{
    URLDictionary result;
    URL::parseQueryString(result, getQuery());
    return result;
}

//
// URL
//

void URL::parseQueryString(URLDictionary& dictionary, StringView queryString)
{
    URLQueryParser qs(queryString);
    URLQueryParser::Parameter qsp;
    while (qs.read(qsp)) {
        dictionary.add(URLDecode(qsp.name, URLDecodeFlagPlusesAsSpaces), URLDecode(qsp.value, URLDecodeFlagPlusesAsSpaces));
    }
}

void URL::parseQueryString(Value::Dictionary& dictionary, StringView queryString)
{
    URLQueryParser qs(queryString);
    URLQueryParser::Parameter qsp;
    while (qs.read(qsp)) {
        Value& value = dictionary.access(URLDecode(qsp.name, URLDecodeFlagPlusesAsSpaces));
        if (value.isUndefined()) {
            value = URLDecode(qsp.value, URLDecodeFlagPlusesAsSpaces);
        } else {
            value.accessVector().push_back(URLDecode(qsp.value, URLDecodeFlagPlusesAsSpaces));
        }
    }
}

std::string URL::buildQueryString(const URLDictionary& dictionary)
{
    std::string buffer;

    for (size_t i = 0; i != dictionary.getSize(); ++i) {
        const std::string& name = dictionary.pair(i).first;
        const std::string& value = dictionary.pair(i).second;

        if (i > 0) {
            buffer += '&';
        }

        URLEncodeAppend(buffer, name, URLEncodeFlagSpacesAsPluses);
        buffer += '=';
        URLEncodeAppend(buffer, value, URLEncodeFlagSpacesAsPluses);
    }

    return buffer;
}

std::string URL::buildQueryString(const Value::Dictionary& dictionary)
{
    std::string buffer;

    for (size_t i = 0; i != dictionary.size(); ++i) {
        const std::string& name = dictionary.pair(i).first;
        const Value& value = dictionary.pair(i).second;

        if (i > 0) {
            buffer += '&';
        }

        URLEncodeAppend(buffer, name, URLEncodeFlagSpacesAsPluses);
        buffer += '=';
        URLEncodeAppend(buffer, value.toString(), URLEncodeFlagSpacesAsPluses);
    }

    return buffer;
}

void URL::tidyPath(std::string& path)
{
    // Make sure we never pop the leading '/' when removing supefluous slashes.
    const ptrdiff_t minNewPathSize = (path.empty() || path[0] != '/') ? 0 : 1;

    std::string::iterator ptr = path.begin() + minNewPathSize;

    // 6a. Remove all occurences of ./
    for (;;) {
        while (ptr != path.end() && *ptr == '/') {
            ++ptr;
        }

        if (ptr == path.end()) {
            break;
        }

        if (*ptr == '.' && (ptr + 1) != path.end() && ptr[1] == '/') {
            path.erase(ptr, ptr + 2);
            ptr = path.begin() + minNewPathSize;
        } else {
            do {
                ++ptr;
            } while (ptr != path.end() && *ptr != '/');
        }
    }

    // 6b. If the path ends with a . as the complete path segment, remove the .
    ptr = FindLastNot(path.begin(), path.end(), '/');
    if (path.end() - ptr == 1 && *ptr == '.') {
        path.resize(path.size() - 1);
    }

    // 6c. Remove all occurences of segment/../
    ptr = path.begin() + minNewPathSize;

    for (;;) {
        while (ptr != path.end() && *ptr == '/') {
            ++ptr;
        }

        if (ptr == path.end()) {
            break;
        }

        std::string::iterator start = ptr;

        do {
            ++ptr;
        } while (ptr != path.end() && *ptr != '/');

        if (ptr == path.end()) {
            break;
        }

        if (ptr - start == 2 && start[0] == '.' && start[1] == '.') {
            continue; // Skip .. appearing at the start
        }

        do {
            ++ptr;
        } while (ptr != path.end() && *ptr == '/');

        if (path.end() - ptr >= 3 && ptr[0] == '.' && ptr[1] == '.' && ptr[2] == '/') {
            path.erase(start, ptr + 3);
            ptr = path.begin() + minNewPathSize;
        }
    }

    // 6d. If the path ends with segment/.. then remove it
    ptr = FindLastNot(path.begin(), path.end(), '/');
    if (path.end() - ptr == 2 && ptr[0] == '.' && ptr[1] == '.') {
        ptr = path.end() - 2;
        while (ptr > path.begin() + minNewPathSize && ptr[-1] == '/') {
            --ptr;
        }
        while (ptr > path.begin() + minNewPathSize && ptr[-1] != '/') {
            --ptr;
        }
        path.erase(ptr, path.end());
    }
}

URL::URL()
{
}

URL::URL(StringView string, const ParseOptions& options)
    : _storage(string.begin(), string.end())
{
    _view.parse(_storage, options);
}

URL::URL(const char* string, const ParseOptions& options)
    : _storage(string)
{
    _view.parse(_storage, options);
}

URL::URL(const std::string& string, const ParseOptions& options)
    : _storage(string)
{
    _view.parse(_storage, options);
}

#ifdef PRIME_COMPILER_RVALUEREF

URL::URL(std::string&& string, const ParseOptions& options)
    : _storage(std::move(string))
{
    _view.parse(_storage, options);
}

#endif

URL::URL(const URLView& view)
{
    view.toString(_storage);
    _view.parse(_storage);
}

#ifdef PRIME_COMPILER_RVALUEREF

URL::URL(URL&& other)
{
    move(other);
}

#endif

URL::URL(const URL& copy)
    : _storage(copy._storage)
{
    _view.parse(_storage);
}

URL& URL::operator=(const URL& other)
{
    if (this != &other) {
        const char* oldBase = other._storage.data();
        _storage = other._storage;
        const char* newBase = _storage.data();

        _view.setProtocol(RebaseStringView(other.getProtocol(), oldBase, newBase));
        _view.setUsername(RebaseStringView(other.getUsername(), oldBase, newBase));
        _view.setPassword(RebaseStringView(other.getPassword(), oldBase, newBase));
        _view.setHost(RebaseStringView(other.getHost(), oldBase, newBase));
        _view.setPort(RebaseStringView(other.getPort(), oldBase, newBase));
        _view.setPath(RebaseStringView(other.getPath(), oldBase, newBase));
        _view.setParameter(RebaseStringView(other.getParameter(), oldBase, newBase));
        _view.setQuery(RebaseStringView(other.getQuery(), oldBase, newBase));
        _view.setFragment(RebaseStringView(other.getFragment(), oldBase, newBase));

        return *this;
    }

    return *this;
}

void URL::move(URL& other)
{
    const char* oldBase = other._storage.data();
    _storage.swap(other._storage);
    const char* newBase = _storage.data();

    _view.setProtocol(RebaseStringView(other.getProtocol(), oldBase, newBase));
    _view.setUsername(RebaseStringView(other.getUsername(), oldBase, newBase));
    _view.setPassword(RebaseStringView(other.getPassword(), oldBase, newBase));
    _view.setHost(RebaseStringView(other.getHost(), oldBase, newBase));
    _view.setPort(RebaseStringView(other.getPort(), oldBase, newBase));
    _view.setPath(RebaseStringView(other.getPath(), oldBase, newBase));
    _view.setParameter(RebaseStringView(other.getParameter(), oldBase, newBase));
    _view.setQuery(RebaseStringView(other.getQuery(), oldBase, newBase));
    _view.setFragment(RebaseStringView(other.getFragment(), oldBase, newBase));

    other._storage.resize(0);
    other._view.setProtocol(other._storage);
    other._view.setUsername(other._storage);
    other._view.setPassword(other._storage);
    other._view.setHost(other._storage);
    other._view.setPort(other._storage);
    other._view.setPath(other._storage);
    other._view.setParameter(other._storage);
    other._view.setQuery(other._storage);
    other._view.setFragment(other._storage);
}

#ifdef PRIME_COMPILER_RVALUEREF

URL& URL::operator=(URL&& other)
{
    if (&other != this) {
        move(other);
    }

    return *this;
}

#endif

URL& URL::operator=(const URLView& view)
{
    view.toString(_storage);
    _view.parse(_storage);

    return *this;
}

URL::URL(const URLBuilder& builder)
{
    operator=(builder);
}

URL& URL::operator=(const URLBuilder& builder)
{
    return operator=(builder.getView());
}

bool URL::parse(StringView string, const ParseOptions& options)
{
    StringCopy(_storage, string);
    return _view.parse(_storage, options);
}

bool URL::parse(const char* string, const ParseOptions& options)
{
    StringCopy(_storage, string);
    return _view.parse(_storage, options);
}

bool URL::parse(const std::string& string, const ParseOptions& options)
{
    _storage = string;
    return _view.parse(_storage, options);
}

#ifdef PRIME_COMPILER_RVALUEREF

bool URL::parse(std::string&& string, const ParseOptions& options)
{
    _storage.swap(string);
    return _view.parse(_storage, options);
}

#endif

// We store the URL as a single string and a URLView in to that string, allowing us to pass only one memory
// allocation around instead of many. The set functions could possibly avoid a memory allocation by replacing
// the component of _storage then shuffling the other components along. I tried it, but corner cases (like
// having to insert a ://, :, @, ;, ? etc) outweighed the slight potential gain of avoiding that allocation.
URL& URL::updateFromView()
{
    std::string temp;
    _view.toString(temp);
    _storage.swap(temp);
    _view.parse(_storage);
    return *this;
}

URL& URL::setProtocol(StringView protocol)
{
    _view.setProtocol(protocol);
    return updateFromView();
}

URL& URL::setHost(StringView host)
{
    _view.setHost(host);
    return updateFromView();
}

URL& URL::setPort(StringView port)
{
    _view.setPort(port);
    return updateFromView();
}

URL& URL::setPath(StringView path)
{
    _view.setPath(path);
    return updateFromView();
}

URL& URL::setQuery(StringView query)
{
    _view.setQuery(query);
    return updateFromView();
}

URL& URL::setFragment(StringView fragment)
{
    _view.setFragment(fragment);
    return updateFromView();
}

URL& URL::setUsername(StringView username)
{
    _view.setUsername(username);
    return updateFromView();
}

URL& URL::setPassword(StringView password)
{
    _view.setPassword(password);
    return updateFromView();
}

URL& URL::setParameter(StringView parameter)
{
    _view.setParameter(parameter);
    return updateFromView();
}

URL URL::resolve(const URLView& embedded) const
{
    return resolve(_view, embedded);
}

URL URL::resolve(const URLView& base, const URLView& embedded)
{
    // Implement section 4 of RFC 1808

    // 1
    if (base.isEmpty()) {
        return URL(embedded);
    }

    // 2a
    if (embedded.isEmpty()) {
        return URL(base);
    }

    // 2b
    if (!StringIsEmpty(embedded.getProtocol())) {
        return URL(embedded);
    }

    URLView result(embedded);
    std::string newPath; // needs to exist outside the scope where it's used

    // 2c
    result.setProtocol(base.getProtocol());

    // 3
    if (!embedded.hasLocation()) {
        result.setUsername(base.getUsername());
        result.setPassword(base.getPassword());
        result.setHost(base.getHost());
        result.setPort(base.getPort());

        // 5
        if (StringIsEmpty(embedded.getPath())) {
            // 5
            result.setPath(base.getPath());

            // 5a
            if (StringIsEmpty(embedded.getParameter())) {
                result.setParameter(base.getParameter());

                // 5b
                if (StringIsEmpty(embedded.getQuery())) {
                    result.setQuery(base.getQuery());
                }
            }
        }
        // 4
        else if (embedded.getPath()[0] != '/') {
            StringCopy(newPath, base.getPath());

            // 6
            std::string::iterator ptr = FindLastNot(newPath.begin(), newPath.end(), '/');

            newPath.replace(ptr, newPath.end(), embedded.getPath().begin(), embedded.getPath().end());

            // 6a, 6b, 6c, 6d
            tidyPath(newPath);

            result.setPath(newPath);
        }
    }

    // 7
    return URL(result);
}

URLPath URL::getPathComponents() const
{
    return URLPath(getPath());
}

void URL::setPathComponents(const URLPath& path)
{
    setPath(path.toString());
}

URLDictionary URL::getQueryComponents() const
{
    URLDictionary result;
    URL::parseQueryString(result, getQuery());
    return result;
}

void URL::setQueryComponents(const URLDictionary& query)
{
    setQuery(URL::buildQueryString(query));
}

//
// URLQueryParser
//

StringView URLQueryParser::getQueryParameter(StringView queryString, StringView name)
{
    URLQueryParser qs(queryString);
    Parameter parameter;
    while (qs.read(parameter)) {
        if (ASCIIEqualIgnoringCase(parameter.name, name)) {
            return parameter.value;
        }
    }

    return StringView();
}

std::vector<StringView> URLQueryParser::getQueryParameters(StringView queryString, StringView name)
{
    std::vector<StringView> values;
    URLQueryParser qs(queryString);
    Parameter parameter;
    while (qs.read(parameter)) {
        if (StringEndsWith(parameter.name, "[]")) {
            parameter.name = parameter.name.substr(0, parameter.name.size() - 2);
        }
        if (ASCIIEqualIgnoringCase(parameter.name, name)) {
            values.push_back(parameter.value);
        }
    }

    return values;
}

bool URLQueryParser::read(Parameter& parameter)
{
    if (_ptr != _end) {
        // Find the next '&' or ';' - that's the limit of this one parameter.
        const char* amp = std::find(_ptr, _end, '&');
        const char* semi = _semi ? std::find(_ptr, amp, ';') : amp;
        const char* next = amp < semi ? amp : semi;

        // Find the first '=', and that separates the name and the value. If we don't find it then the whole
        // thing between _ptr and next is a name.
        const char* eq = std::find(_ptr, next, '=');

        // URL spec says all whitespace should be ignored, but we only ignore it on either side of the
        // & or =.

        parameter.name = StringViewTrim(StringView(_ptr, eq));

        if (eq != next) {
            ++eq;
        }

        parameter.value = StringViewTrim(StringView(eq, next));

        // Skip the separator.
        _ptr = next;
        if (_ptr != _end) {
            ++_ptr;
        }

        return true;
    }

    return false;
}

//
// URLBuilder
//

URLBuilder& URLBuilder::operator=(const URLBuilder& copy)
{
    _protocol = copy._protocol;
    _host = copy._host;
    _port = copy._port;
    _path = copy._path;
    _query = copy._query;
    _fragment = copy._fragment;

    if (!copy._rare.get()) {
        _rare.reset();
    } else {
        _rare.reset(new Rare(*copy._rare));
    }

    return *this;
}

URLBuilder::URLBuilder(const URLView& view)
{
    operator=(view);
}

URLBuilder::URLBuilder(const URL& url)
{
    operator=(url);
}

void URLBuilder::setPassword(StringView password)
{
    if (password.empty() && !_rare.get()) {
        return;
    }

    needRare();
    StringCopy(_rare->password, password);
}

void URLBuilder::setUsername(StringView username)
{
    if (username.empty() && !_rare.get()) {
        return;
    }

    needRare();
    StringCopy(_rare->username, username);
}

void URLBuilder::setParameter(StringView parameter)
{
    if (parameter.empty() && !_rare.get()) {
        return;
    }

    needRare();
    StringCopy(_rare->parameter, parameter);
}

std::string URLBuilder::getHostWithPort() const
{
    if (_port.empty()) {
        return _host;
    }

    return _host + ":" + _port;
}

void URLBuilder::needRare()
{
    if (!_rare.get()) {
        _rare.reset(new Rare);
    }
}

bool URLBuilder::parse(StringView string, const ParseOptions& options)
{
    URLView parser;

    if (!parser.parse(string, options)) {
        return false;
    }

    operator=(parser);
    return true;
}

URLBuilder& URLBuilder::operator=(const URLView& view)
{
    StringCopy(_protocol, view.getProtocol());
    StringCopy(_host, view.getHost());
    StringCopy(_port, view.getPort());
    StringCopy(_path, view.getPath());
    StringCopy(_query, view.getQuery());
    StringCopy(_fragment, view.getFragment());
    if (_rare || !view.getUsername().empty() || !view.getPassword().empty() || !view.getParameter().empty()) {
        needRare();
        StringCopy(_rare->username, view.getUsername());
        StringCopy(_rare->password, view.getPassword());
        StringCopy(_rare->parameter, view.getParameter());
    }

    return *this;
}

URLBuilder& URLBuilder::operator=(const URL& url)
{
    return operator=(url.getView());
}

std::string URLBuilder::toString(const StringOptions& options) const
{
    return getView().toString(options);
}

void URLBuilder::toString(std::string& buffer, const StringOptions& options) const
{
    return getView().toString(buffer, options);
}

bool URLBuilder::isEmpty() const PRIME_NOEXCEPT
{
    return _protocol.empty() && StringIsEmpty(getUsername()) && StringIsEmpty(getPassword()) && _host.empty() && _port.empty() && _path.empty() && StringIsEmpty(getParameter()) && _query.empty() && _fragment.empty();
}

bool URLBuilder::hasLocation() const PRIME_NOEXCEPT
{
    return !(StringIsEmpty(getUsername()) && StringIsEmpty(getPassword()) && _host.empty() && _port.empty());
}

URLPath URLBuilder::getPathComponents() const
{
    return URLPath(getPath());
}

void URLBuilder::setPathComponents(const URLPath& path)
{
    setPath(path.toString());
}

URLDictionary URLBuilder::getQueryComponents() const
{
    URLDictionary result;
    URL::parseQueryString(result, getQuery());
    return result;
}

void URLBuilder::setQueryComponents(const URLDictionary& query)
{
    setQuery(URL::buildQueryString(query));
}

URLBuilder URLBuilder::resolve(const URLBuilder& embedded) const
{
    URL url = URL::resolve(getView(), embedded.getView());
    return URLBuilder(url);
}

URLBuilder URLBuilder::resolve(const URLView& embedded) const
{
    URL url = URL::resolve(getView(), embedded);
    return URLBuilder(url);
}

URLBuilder URLBuilder::resolve(const URL& embedded) const
{
    URL url = URL::resolve(getView(), embedded);
    return URLBuilder(url);
}

void URLBuilder::swap(URLBuilder& rhs)
{
    _protocol.swap(rhs._protocol);
    _host.swap(rhs._host);
    _port.swap(rhs._port);
    _path.swap(rhs._path);
    _query.swap(rhs._query);
    _fragment.swap(rhs._fragment);
    _rare.swap(rhs._rare);
}

void URLBuilder::move(URLBuilder& rhs)
{
#ifdef PRIME_COMPILER_RVALUEREF
    _protocol = std::move(rhs._protocol);
    _host = std::move(rhs._host);
    _port = std::move(rhs._port);
    _path = std::move(rhs._path);
    _query = std::move(rhs._query);
    _fragment = std::move(rhs._fragment);
    _rare = std::move(rhs._rare);
#else
    swap(rhs);
#endif
}

URLView URLBuilder::getView() const
{
    URLView view;
    view.setProtocol(getProtocol());
    view.setHost(getHost());
    view.setPort(getPort());
    view.setPath(getPath());
    view.setQuery(getQuery());
    view.setFragment(getFragment());
    view.setUsername(getUsername());
    view.setPassword(getPassword());
    view.setParameter(getParameter());
    return view;
}

//
// URLPath
//

bool URLPath::isUnsafe(StringView string) PRIME_NOEXCEPT
{
    for (const char* ptr = string.begin(); ptr != string.end(); ++ptr) {
        if (isUnsafe(*ptr)) {
            return true;
        }
    }

    return false;
}

bool URLPath::parse(StringView string)
{
    const char* ptr = string.begin();
    const char* end = string.end();
    bool result = true;

    // TODO: we could write components directly in to _storage
    std::string component;

    _storage.clear();
    _lengths.clear();

    for (;;) {
        const char* slash = std::find(ptr, end, '/');
        size_t length = (size_t)(slash - ptr);

        component.resize(0);
        URLDecodeAppend(component, StringView(ptr, slash), 0);
        if (!component.empty() && component != ".") {
            if (component == "..") {
                if (!_lengths.empty()) {
                    _lengths.pop_back();
                }
            } else {
                if (isUnsafe(component)) {
                    result = false;
                }
                if (component.size() == length && memcmp(component.data(), ptr, length) == 0) {
                    _storage.append(ptr, slash);
                    _lengths.push_back(slash - ptr);

                } else {
                    _storage.append(component.begin(), component.end());
                    _lengths.push_back(component.size());
                }
            }
        }

        ptr = slash;
        if (ptr == end) {
            break;
        }

        if (ptr + 1 == end) {
            // Empty components within a path (e.g., a/b//c/d) are ignored, but a trailing / denotes a directory,
            // so we end with an empty component.
            _lengths.push_back(0);
            break;
        }

        ++ptr;
    }

    return result;
}

std::string URLPath::toString(const StringOptions& options) const
{
    const bool relative = options.getWithoutLeadingSlash();
    const bool urlEscape = !options.getWithoutEscaping();

    if (_lengths.empty()) {
        return relative ? "" : "/";
    }

    std::string path;
    size_t offset = 0;
    bool previousComponent = false;

    for (size_t i = 0; i != _lengths.size(); ++i) {
        StringView component(_storage.data() + offset, _storage.data() + offset + _lengths[i]);
        offset += _lengths[i];

        if (options.getSkipUnsafeComponents() && isUnsafe(component)) {
            continue;
        }

        if (!relative || previousComponent) {
            path += '/';
        }

        if (urlEscape) {
            URLEncodeAppend(path, component, 0);
        } else {
            path.append(component.begin(), component.end());
        }

        previousComponent = true;
    }

    return std::string(path.begin(), path.end());
}

size_t URLPath::offsetOfComponent(size_t index) const
{
    PRIME_DEBUG_ASSERT(index <= getComponentCount());

    size_t offset = 0;
    const size_t* lengths = &_lengths[0];
    while (index--) {
        offset += *lengths++;
    }

    return offset;
}

URLPath URLPath::getTail(size_t skip) const
{
    URLPath result;

    if (PRIME_GUARD(skip <= getComponentCount())) {
        size_t offset = offsetOfComponent(skip);

        result._storage.assign(_storage.data() + offset, _storage.size() - offset);
        result._lengths.assign(_lengths.begin() + skip, _lengths.end());
    }

    return result;
}

URLPath& URLPath::addComponent(StringView component)
{
    _lengths.push_back(component.size());
    _storage.append(component.begin(), component.end());

    return *this;
}

void URLPath::removeLastComponent()
{
    _storage.resize(_storage.size() - _lengths.back());
    _lengths.pop_back();
}

URLPath URLPath::getWithLastComponentRemoved() const
{
    URLPath result(*this);
    result.removeLastComponent();
    return result;
}

StringView URLPath::getComponent(size_t index) const PRIME_NOEXCEPT
{
    if (!PRIME_GUARD(index < getComponentCount())) {
        return StringView();
    }

    return StringView(_storage).substr(offsetOfComponent(index), _lengths[index]);
}

StringView URLPath::getComponentElse(size_t index, StringView defaultValue) const
{
    if (index < _lengths.size()) {
        return getComponent(index);
    }

    return defaultValue;
}

URLPath URLPath::toDirectory() const
{
    if (isDirectory()) {
        return *this;
    }

    URLPath directory = *this;
    directory._lengths.push_back(0);
    return directory;
}

bool URLPath::operator==(const URLPath& rhs) const PRIME_NOEXCEPT
{
    return _lengths == rhs._lengths && _storage == rhs._storage;
}

bool URLPath::operator<(const URLPath& rhs) const PRIME_NOEXCEPT
{
    size_t minComponentCount = std::min(_lengths.size(), rhs._lengths.size());
    for (size_t i = 0; i != minComponentCount; ++i) {
        // Could track the two offsets...
        StringView leftComponent = getComponent(i);
        StringView rightComponent = rhs.getComponent(i);
        if (leftComponent < rightComponent) {
            return true;
        }
        if (leftComponent != rightComponent) {
            return false;
        }
    }

    return _lengths.size() < rhs._lengths.size();
}

URLPath URLPath::getRelative(const URLPath& relative) const
{
    URLPath absolute = *this;
    if (absolute.isDirectory()) {
        absolute.removeLastComponent();
    }

    absolute._lengths.insert(absolute._lengths.end(), relative._lengths.begin(), relative._lengths.end());
    absolute._storage.append(relative._storage);

    return absolute;
}

bool URLPath::startsWith(const URLPath& prefix) const PRIME_NOEXCEPT
{
    size_t count = prefix.getComponentCount();
    if (prefix.isDirectory()) {
        --count;
    }

    if (getComponentCount() < count) {
        return false;
    }

    if (!std::equal(prefix._lengths.begin(), prefix._lengths.begin() + count, _lengths.begin())) {
        return false;
    }

    size_t offset = offsetOfComponent(count);
    if (offset != prefix._storage.size()) {
        return false;
    }

    return memcmp(_storage.data(), prefix._storage.data(), offset) == 0;
}

//
// URLDictionary
//

void URLDictionary::clear() PRIME_NOEXCEPT
{
    _pairs.clear();
}

const std::string& URLDictionary::get(StringView key) const PRIME_NOEXCEPT
{
    for (size_t i = 0; i != _pairs.size(); ++i) {
        const value_type& pair = _pairs.pair(i);

        if (equalKeys(pair.first, key)) {
            return pair.second;
        }
    }

    return emptyString;
}

const std::string& URLDictionary::operator[](StringView key) const PRIME_NOEXCEPT
{
    return get(key);
}

bool URLDictionary::has(StringView key) const PRIME_NOEXCEPT
{
    for (size_t i = 0; i != _pairs.size(); ++i) {
        const value_type& pair = _pairs.pair(i);

        if (equalKeys(pair.first, key)) {
            return true;
        }
    }

    return false;
}

std::vector<std::string> URLDictionary::getAll(StringView key) const
{
    std::vector<std::string> array;

    for (size_t i = 0; i != _pairs.size(); ++i) {
        const value_type& pair = _pairs.pair(i);

        if (equalKeys(pair.first, key)) {
            array.push_back(pair.second);
        }
    }

    return array;
}

std::vector<StringView> URLDictionary::getAllViews(StringView key) const
{
    std::vector<StringView> array;

    for (size_t i = 0; i != _pairs.size(); ++i) {
        const value_type& pair = _pairs.pair(i);

        if (equalKeys(pair.first, key)) {
            array.push_back(pair.second);
        }
    }

    return array;
}

void URLDictionary::set(StringView key, StringView value)
{
    bool first = true;

    for (size_t i = 0; i != _pairs.size(); ++i) {
        value_type& pair = _pairs.pair(i);

        if (equalKeys(pair.first, key)) {
            if (first) {
                StringCopy(pair.second, value);
                first = false;
            } else {
                _pairs.erase(_pairs.begin() + (ptrdiff_t)i);
                --i;
            }
        }
    }

    if (first) {
        add(key, value);
    }
}

void URLDictionary::add(StringView key, StringView value)
{
    _pairs.push_back(value_type());
    value_type& pair = _pairs.back();
    pair.first.assign(key.begin(), key.end());
    StringCopy(pair.second, value);
}

void URLDictionary::remove(StringView key)
{
    for (size_t i = 0; i != _pairs.size(); ++i) {
        value_type& pair = _pairs.pair(i);

        if (equalKeys(pair.first, key)) {
            _pairs.erase(_pairs.begin() + (ptrdiff_t)i);
            --i;
        }
    }
}

URLDictionary::iterator URLDictionary::find(StringView key) PRIME_NOEXCEPT
{
    for (iterator i = begin(); i != end(); ++i) {
        if (equalKeys(i->first, key)) {
            return i;
        }
    }

    return end();
}

#ifdef PRIME_HAVE_VALUE
Value::Dictionary URLDictionary::toDictionary() const
{
    Value::Dictionary dict;
    dict.reserve(_pairs.size());
    for (const_iterator i = begin(); i != end(); ++i) {
        dict.set(i->first, i->second);
    }
    return dict;
}
#endif

//
// StringAppend
//

bool StringAppend(std::string& output, const URLView& url)
{
    url.appendString(output);
    return true;
}

bool StringAppend(std::string& output, const URL& url)
{
    url.appendString(output);
    return true;
}

bool StringAppend(std::string& output, const URLBuilder& url)
{
    output += url.toString();
    return true;
}
}
