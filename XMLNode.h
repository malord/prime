// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_XMLNODE_H
#define PRIME_XMLNODE_H

#include "Config.h"
#include "DoubleLinkList.h"
#include "RefCounting.h"
#include "StringUtils.h"
#include <vector>

namespace Prime {

/// A DOM style XML API.
class PRIME_PUBLIC XMLNode : public NonAtomicRefCounted<XMLNode> {
public:
    enum Type {
        TypeElement,
        TypeProcessingInstruction,
        TypeAttribute,
        TypeComment,
        TypeDocType,
        TypeText,
    };

    XMLNode()
        : _type(TypeElement)
        , _prefixPos(0)
        , _parent(NULL)
        , _children(&XMLNode::_siblings)
        , _textNodeType(TextNodeText)
        , _caseInsensitiveTags(false)
    {
    }

    const std::string& getQualifiedName() const { return _qualifiedName; }

    const std::string& getNamespace() const { return _nspace; }

    std::string getPrefix() const { return _qualifiedName.substr(0, _prefixPos); }

    StringView getPrefixView() const { return StringView(_qualifiedName).substr(0, _prefixPos); }

    std::string getName() const { return _qualifiedName.substr(_prefixPos ? _prefixPos + 1 : 0); }

    StringView getNameView() const { return StringView(_qualifiedName).substr(_prefixPos ? _prefixPos + 1 : 0); }

    bool hasName(StringView name) const
    {
        return equalTags(getNameView(), name);
    }

    bool hasNamespace(StringView nspace) const
    {
        return nspace.empty() || StringsEqual(getNamespace(), nspace);
    }

    const std::string& getValue() const { return _value; }

    bool isCDATA() const { return _textNodeType == TextNodeCDATA; }

    void setCDATA(bool cdata) { _textNodeType = cdata ? TextNodeCDATA : TextNodeText; }

    bool isEncodedText() const { return _textNodeType == TextNodeEncoded; }

    void setEncodedText(bool value)
    {
        _textNodeType = value ? TextNodeEncoded : TextNodeText;
    }

    const XMLNode* getParent() const { return _parent; }
    XMLNode* getParent() { return _parent; }

    const XMLNode* getNextSibling() const { return _siblings.getNext(); }
    XMLNode* getNextSibling() { return _siblings.getNext(); }

    const XMLNode* getPreviousSibling() const { return _siblings.getPrevious(); }
    XMLNode* getPreviousSibling() { return _siblings.getPrevious(); }

    bool hasChildren() const { return _children.getFirst() != NULL; }

    size_t getChildCount() const { return _children.size(); }

    const XMLNode* getFirstChild() const { return _children.getFirst(); }
    XMLNode* getFirstChild() { return _children.getFirst(); }

    const XMLNode* getLastChild() const { return _children.getLast(); }
    XMLNode* getLastChild() { return _children.getLast(); }

    std::vector<XMLNode*> getAllChildren();
    std::vector<const XMLNode*> getAllChildren() const;

    const XMLNode* getChild(size_t index) const;
    XMLNode* getChild(size_t index)
    {
        return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->getChild(index));
    }

    Type getType() const { return _type; }

    bool isRoot() const { return _parent == NULL; }

    bool isElement() const { return _type == TypeElement; }

    bool isAttribute() const { return _type == TypeAttribute; }

    bool isText() const { return _type == TypeText; }

    void insertBefore(XMLNode* child, XMLNode* beforeChild);

    bool removeChild(XMLNode* child);

    void removeChildren();

    void detachFromParent();

    /// Performs a deep copy. The caller must delete the returned object.
    RefPtr<XMLNode> createDeepClone() const;

    XMLNode* addTextChild(StringView text);

    void addChild(XMLNode* child);

    bool hasChild(XMLNode* child) { return _children.contains(child); }

    void setName(StringView nspace, StringView name, StringView prefixIfNeeded = "");

    void setQualifiedName(StringView qualifiedName, StringView nspace);

    void setValue(StringView value) { _value.assign(value.begin(), value.end()); }

    void setIntValue(intmax_t value);

    void setFloatValue(FloatMax value);

    template <typename Type>
    Type getIntValue(Type defaultValue) const
    {
        Type value;
        return StringToInt(getValue(), value) ? value : defaultValue;
    }

    template <typename Type>
    Type getFloatValue(Type defaultValue) const
    {
        Type value;
        return StringToReal(getValue(), value) ? value : defaultValue;
    }

    bool getBoolValue(bool defaultValue) const;

    XMLNode* setChild(StringView nspace, StringView name, StringView value);

    XMLNode* addChild(StringView nspace, StringView name, StringView value = "", StringView prefix = "");

    XMLNode* addChild(Type type, StringView value = "");

    XMLNode* addChild(Type type, StringView nspace, StringView name, StringView value, StringView prefix = "");

    void removeTextChildren();

    /// Returns an empty string if there's no match.
    const std::string& getNamespaceForPrefix(StringView prefix) const;

    bool findPrefixForNamespace(std::string& prefix, StringView uri) const;

    const XMLNode* getChild(StringView nspace, StringView name) const;
    XMLNode* getChild(StringView nspace, StringView name)
    {
        return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->getChild(nspace, name));
    }

    bool hasChild(StringView nspace, StringView name) const
    {
        return getChild(nspace, name) != NULL;
    }

    const XMLNode* getElement(StringView nspace, StringView name) const;
    XMLNode* getElement(StringView nspace, StringView name)
    {
        return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->getElement(nspace, name));
    }

    std::vector<RefPtr<const XMLNode>> getElements(StringView nspace, StringView name) const;

    bool hasElement(StringView nspace, StringView name) const
    {
        return getElement(nspace, name) != NULL;
    }

    const XMLNode* getNextChildElement(const XMLNode* startChildOrNull, StringView nspace, StringView name) const;
    XMLNode* getNextChildElement(XMLNode* startChildOrNull, StringView nspace, StringView name)
    {
        return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->getNextChildElement(startChildOrNull, nspace, name));
    }

    const XMLNode* getNextSiblingElement(StringView nspace, StringView name) const;
    XMLNode* getNextSiblingElement(StringView nspace, StringView name)
    {
        return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->getNextSiblingElement(nspace, name));
    }

    const XMLNode* getAttribute(StringView nspace, StringView name) const;
    XMLNode* getAttribute(StringView nspace, StringView name)
    {
        return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->getAttribute(nspace, name));
    }

    bool hasAttribute(StringView nspace, StringView name) const
    {
        return getAttribute(nspace, name) != NULL;
    }

    const std::string& getAttributeValue(StringView nspace, StringView name) const;

    template <typename Type>
    Type getIntAttribute(StringView nspace, StringView name, Type defaultValue) const
    {
        const XMLNode* attr = getAttribute(nspace, name);
        if (!attr) {
            return defaultValue;
        }

        return attr->getIntValue(defaultValue);
    }

    template <typename Type>
    Type getFloatAttribute(StringView nspace, StringView name, Type defaultValue) const
    {
        const XMLNode* attr = getAttribute(nspace, name);
        if (!attr) {
            return defaultValue;
        }

        return attr->getFloatValue(defaultValue);
    }

    bool getBoolAttribute(StringView nspace, StringView name, bool defaultValue) const
    {
        const XMLNode* attr = getAttribute(nspace, name);
        if (!attr) {
            return defaultValue;
        }

        return attr->getBoolValue(defaultValue);
    }

    XMLNode* setAttribute(StringView nspace, StringView name, StringView value, StringView prefix = "");

    /// You can only call this if there's definitely not already an attribute with the same name.
    XMLNode* addAttribute(StringView nspace, StringView name, StringView value, StringView prefix = "");

    bool removeAttribute(StringView nspace, StringView name);

    const XMLNode* getDescendant(StringView nspace, StringView name) const;
    XMLNode* getDescendant(StringView nspace, StringView name)
    {
        return const_cast<XMLNode*>(const_cast<const XMLNode*>(this)->getDescendant(nspace, name));
    }

    std::string getAllText() const;

    std::string getAllTextDeep() const;

    std::string getAllText(StringView nspace, StringView name) const;

    std::string getAllTextDeep(StringView nspace, StringView name) const;

    bool hasAttributes() const;

    /// Recursively enable or disable case insensitive tags. The default is false (disabled).
    void setCaseInsensitiveTags(bool value);

    bool hasCaseInsensitiveTags() const { return _caseInsensitiveTags; }

    bool equalTags(StringView a, StringView b) const
    {
        return _caseInsensitiveTags ? ASCIIEqualIgnoringCase(a, b) : StringsEqual(a, b);
    }

    static bool equalNamespaces(StringView a, StringView b)
    {
        return ASCIIEqualIgnoringCase(a, b);
    }

    static bool equalURIs(StringView a, StringView b)
    {
        return ASCIIEqualIgnoringCase(a, b);
    }

    static bool equalSearchNamespaces(StringView search, StringView a)
    {
        return search == "*" || ASCIIEqualIgnoringCase(search, a);
    }

    /// Recursively find all elements with the specified name.
    std::vector<const XMLNode*> getAllElements(StringView nspace, StringView name) const;

    void getAllElements(std::vector<const XMLNode*>& vector, StringView nspace, StringView name) const;

    /// Recursively find all elements with the specified name.
    std::vector<XMLNode*> getAllElements(StringView nspace, StringView name);

    void getAllElements(std::vector<XMLNode*>& vector, StringView nspace, StringView name);

private:
    void getAllTextDeep(std::string& out) const;

    XMLNode* addAttribute2(StringView nspace, StringView name, StringView value, StringView prefix);

    void setName2(StringView nspace, StringView name, StringView prefixIfNeeded = "");

    Type _type;

    std::string _nspace; // A URI
    std::string _qualifiedName; // prefix:name
    size_t _prefixPos; // location of :, zero if there's no :

    std::string _value;

    XMLNode* _parent;
    DoubleLink<XMLNode> _siblings;
    DoubleLinkList<XMLNode, RefCountingLinkListElementManager<XMLNode>> _children;

    enum TextNodeType {
        TextNodeText,
        TextNodeCDATA,
        TextNodeEncoded
    };

    TextNodeType _textNodeType;
    bool _caseInsensitiveTags;

    XMLNode(const XMLNode&)
        : _children(&XMLNode::_siblings)
    {
        PRIME_ASSERT(0);
    }

    XMLNode& operator=(const XMLNode&)
    {
        PRIME_ASSERT(0);
        return *this;
    }
};
}

#endif
