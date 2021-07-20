// Copyright 2000-2021 Mark H. P. Lord

#include "XMLNode.h"
#include "Convert.h"
#include "ScopedPtr.h"
#include "Templates.h"

namespace Prime {

void XMLNode::insertBefore(XMLNode* child, XMLNode* beforeChild)
{
    PRIME_ASSERT(!child->getParent());
    child->_parent = this;
    _children.insertBefore(child, beforeChild);
}

const XMLNode* XMLNode::getChild(size_t index) const
{
    const XMLNode* node;
    for (node = getFirstChild(); index && node; --index, node = node->getNextSibling()) { }

    return node;
}

std::vector<XMLNode*> XMLNode::getAllChildren()
{
    std::vector<XMLNode*> out;
    size_t count = getChildCount();
    out.reserve(count);
    for (size_t i = 0; i != count; ++i) {
        out.push_back(getChild(i));
    }
    return out;
}

std::vector<const XMLNode*> XMLNode::getAllChildren() const
{
    std::vector<const XMLNode*> out;
    size_t count = getChildCount();
    out.reserve(count);
    for (size_t i = 0; i != count; ++i) {
        out.push_back(getChild(i));
    }
    return out;
}

bool XMLNode::removeChild(XMLNode* child)
{
    if (child->_parent != this) {
        return false;
    }

    child->_parent = NULL;
    _children.erase(child);
    return true;
}

void XMLNode::removeChildren()
{
    for (XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
        child->_parent = NULL;
    }

    _children.clear();
}

void XMLNode::detachFromParent()
{
    if (_parent) {
        _parent->_children.detach(this);
    }
}

RefPtr<XMLNode> XMLNode::createDeepClone() const
{
    RefPtr<XMLNode> clone = PassRef(new XMLNode);
    clone->_type = _type;
    clone->_nspace = _nspace;
    clone->_qualifiedName = _qualifiedName;
    clone->_prefixPos = _prefixPos;
    clone->_value = _value;
    clone->_textNodeType = _textNodeType;
    clone->_caseInsensitiveTags = _caseInsensitiveTags;
    clone->_parent = NULL;

    for (const XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
        clone->addChild(child->createDeepClone());
    }

    return clone;
}

void XMLNode::addChild(XMLNode* child)
{
    PRIME_ASSERT(!child->getParent());
    child->_parent = this;
    _children.push_back(child);
}

void XMLNode::setIntValue(intmax_t value)
{
    char buffer[32];
    StringFormat(buffer, sizeof(buffer), "%" PRIdMAX, value);
    setValue(buffer);
}

void XMLNode::setFloatValue(FloatMax value)
{
    char buffer[128];
    StringFormat(buffer, sizeof(buffer), "%" PRIME_PRIg_FLOATMAX, value);
    setValue(buffer);
}

bool XMLNode::getBoolValue(bool defaultValue) const
{
    StringView value = getValue();

    static const StringView truthiness[] = { "yes", "no", "true", "false", "on", "off", "1", "0" };

    for (size_t i = 0; i != COUNTOF(truthiness); ++i) {
        if (ASCIIEqualIgnoringCase(truthiness[i], value)) {
            return (i & 1) == 0;
        }
    }

    return defaultValue;
}

XMLNode* XMLNode::setChild(StringView nspace, StringView name, StringView value)
{
    XMLNode* child = getChild(nspace, name);
    if (!child) {
        return addChild(nspace, name, value);
    }

    child->removeChildren();

    child->addTextChild(value);

    return child;
}

XMLNode* XMLNode::addChild(StringView nspace, StringView name, StringView value, StringView prefix)
{
    RefPtr<XMLNode> child = PassRef(new XMLNode);
    child->_type = TypeElement;
    child->addTextChild(value);
    addChild(child.get());
    child->setName(nspace, name, prefix);
    return child.get(); // We hold a reference.
}

void XMLNode::setName(StringView nspace, StringView name, StringView prefixIfNeeded)
{
    setName2(nspace, name, prefixIfNeeded);

    PRIME_ASSERT(equalNamespaces(getNamespace(), nspace));
    PRIME_ASSERT(equalTags(getName(), name) || (StringIsEmpty(getNamespace()) && equalTags(getQualifiedName(), name)));
    PRIME_ASSERT(StringIsEmpty(nspace) || equalNamespaces(getNamespaceForPrefix(getPrefix()), nspace));
}

void XMLNode::setName2(StringView nspace, StringView name, StringView prefixIfNeeded)
{
    // No namespace, so qualified name is the local name.
    if (StringIsEmpty(nspace)) {
        setQualifiedName(name, nspace);
        return;
    }

    PRIME_DEBUG_ASSERTMSG(!Contains(name, ':'), "XMLNode name contains a colon.");

    // Find our element (which might be us).
    XMLNode* element = isAttribute() ? getParent() : this;
    PRIME_ASSERT(element->isElement()); // Attribute's parent not an element?

    std::string foundPrefix;
    if (element->findPrefixForNamespace(foundPrefix, nspace)) {
        if (foundPrefix.empty()) {
            // nspace is the defualt namespace.
            _nspace.assign(nspace.begin(), nspace.end());
            _qualifiedName.assign(name.begin(), name.end());
            _prefixPos = 0;
        } else {
            // nspace already has a prefix, so use that.
            _nspace.assign(nspace.begin(), nspace.end());
            _qualifiedName = foundPrefix + ':' + name;
            _prefixPos = foundPrefix.size();
        }
        return;
    }

    if (!StringIsEmpty(prefixIfNeeded)) {
        // We've been given a prefix to use.

        const std::string xmlnsPrefix = "xmlns:" + prefixIfNeeded;

        XMLNode* prefixNode = element->getAttribute("", xmlnsPrefix);
        if (!prefixNode || equalNamespaces(prefixNode->getValue(), nspace)) {
            if (!prefixNode) {
                element->setAttribute("", xmlnsPrefix, nspace);
            }

            _nspace.assign(nspace.begin(), nspace.end());
            _qualifiedName = prefixIfNeeded + ':' + name;
            _prefixPos = prefixIfNeeded.size();
            return;
        }

        // prefix already used by another namespace.
    }

    // If we're an element, try setting the default namespace.
    if (isElement()) {
        PRIME_ASSERT(this == element);
        XMLNode* existingXMLns = getAttribute("", "xmlns");

        _nspace.assign(nspace.begin(), nspace.end());
        _qualifiedName.assign(name.begin(), name.end());
        _prefixPos = 0;

        if (existingXMLns) {
            if (!equalNamespaces(existingXMLns->getValue(), nspace)) {
                // Change the default namespace then re-set the names on all our attributes and child elements.
                existingXMLns->setValue(nspace);

                for (XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
                    if (child->isAttribute() || child->isElement()) {
                        child->setName(child->getNamespace(), child->getName());
                    }
                }
            }
        } else {
            addAttribute("", "xmlns", nspace);
        }

        return;
    }

    // We're an attribute. We're going to have to make up a prefix.
    for (int attempt = 0; attempt != INT_MAX; ++attempt) {
        char xmlnsPrefix[32];
        StringFormat(xmlnsPrefix, sizeof(xmlnsPrefix), "xmlns:ns%d", attempt);

        if (element->getAttribute("", xmlnsPrefix)) {
            // Already exists.
            continue;
        }

        element->setAttribute("", xmlnsPrefix, nspace);

        _nspace.assign(nspace.begin(), nspace.end());
        _qualifiedName = MakeString(&xmlnsPrefix[6], ':', name);
        _prefixPos = strlen(xmlnsPrefix) - 6;
        return;
    }
}

void XMLNode::setQualifiedName(StringView qualifiedName, StringView nspace)
{
    _nspace.assign(nspace.begin(), nspace.end());
    _qualifiedName.assign(qualifiedName.begin(), qualifiedName.end());
    StringView::const_iterator colon = std::find(qualifiedName.begin(), qualifiedName.end(), ':');
    _prefixPos = (colon == qualifiedName.end()) ? 0 : (size_t)(colon - qualifiedName.begin());
}

XMLNode* XMLNode::addTextChild(StringView text)
{
    RefPtr<XMLNode> textNode = PassRef(new XMLNode);
    textNode->_type = TypeText;
    textNode->_value.assign(text.begin(), text.end());
    addChild(textNode.get());
    return textNode.get(); // We hold a reference.
}

XMLNode* XMLNode::addChild(Type type, StringView value)
{
    RefPtr<XMLNode> child = PassRef(new XMLNode);
    child->_type = type;
    child->_value.assign(value.begin(), value.end());
    addChild(child.get());
    return child.get(); // We hold a reference.
}

XMLNode* XMLNode::addChild(Type type, StringView nspace, StringView name, StringView value, StringView prefix)
{
    RefPtr<XMLNode> child = PassRef(new XMLNode);
    child->_type = type;
    child->_value.assign(value.begin(), value.end());
    addChild(child.get());
    child->setName(nspace, name, prefix);
    return child.get(); // We hold a reference.
}

void XMLNode::removeTextChildren()
{
    XMLNode* next = NULL;
    for (XMLNode* child = getFirstChild(); child; child = next) {
        next = child->getNextSibling();

        if (child->isText()) {
            removeChild(child);
        }
    }
}

const std::string& XMLNode::getNamespaceForPrefix(StringView prefix) const
{
    for (const XMLNode* node = this; node; node = node->getParent()) {
        if (!node->isElement()) {
            continue;
        }

        for (const XMLNode* attr = node->getFirstChild(); attr; attr = attr->getNextSibling()) {
            if (!attr->isAttribute()) {
                continue;
            }

            if (StringIsEmpty(prefix)) {
                // Looking for the default namespace.
                if (StringsEqual(attr->getQualifiedName(), "xmlns")) {
                    return attr->getValue();
                }
            } else {
                // Looking for a prefix.
                if (StringStartsWith(attr->getQualifiedName(), "xmlns:")) {
                    if (StringsEqual(attr->getNameView(), prefix)) {
                        return attr->getValue();
                    }
                }
            }
        }
    }

    return emptyString;
}

bool XMLNode::findPrefixForNamespace(std::string& prefix, StringView uri) const
{
    if (!_prefixPos && _nspace == uri) {
        prefix = "";
        return true;
    }

    for (const XMLNode* node = this; node; node = node->getParent()) {
        if (!node->isElement()) {
            continue;
        }

        for (const XMLNode* attr = node->getFirstChild(); attr; attr = attr->getNextSibling()) {
            if (!attr->isAttribute()) {
                continue;
            }

            if (StringStartsWith(attr->getQualifiedName(), "xmlns:") && equalURIs(attr->getValue(), uri)) {
                prefix = attr->getName();
                return true;
            }
        }
    }

    return false;
}

const XMLNode* XMLNode::getChild(StringView nspace, StringView name) const
{
    for (const XMLNode* node = getFirstChild(); node; node = node->getNextSibling()) {
        if (equalSearchNamespaces(nspace, node->getNamespace()) && equalTags(node->getNameView(), name)) {
            return node;
        }
    }

    return NULL;
}

const XMLNode* XMLNode::getElement(StringView nspace, StringView name) const
{
    const XMLNode* node = getChild(nspace, name);
    if (!node || !node->isElement()) {
        return NULL;
    }

    return node;
}

std::vector<RefPtr<const XMLNode>> XMLNode::getElements(StringView nspace, StringView name) const
{
    std::vector<RefPtr<const XMLNode>> list;

    for (const XMLNode* node = getFirstChild(); node; node = node->getNextSibling()) {
        if (node->isElement() && equalSearchNamespaces(nspace, node->getNamespace()) && equalTags(node->getNameView(), name)) {
            list.push_back(node);
        }
    }

    return list;
}

const XMLNode* XMLNode::getNextChildElement(const XMLNode* startChildOrNull, StringView nspace, StringView name) const
{
    if (!startChildOrNull) {
        return getElement(nspace, name);
    }

    return startChildOrNull->getNextSiblingElement(nspace, name);
}

const XMLNode* XMLNode::getNextSiblingElement(StringView nspace, StringView name) const
{
    const XMLNode* node = this;
    for (;;) {
        node = node->getNextSibling();
        if (!node) {
            return NULL;
        }

        if (!node->isElement()) {
            continue;
        }

        if (equalSearchNamespaces(nspace, node->getNamespace()) && equalTags(node->getNameView(), name)) {
            return node;
        }
    }
}

const XMLNode* XMLNode::getAttribute(StringView nspace, StringView name) const
{
    const XMLNode* node = getChild(nspace, name);
    if (!node || !node->isAttribute()) {
        return NULL;
    }

    return node;
}

const std::string& XMLNode::getAttributeValue(StringView nspace, StringView name) const
{
    const XMLNode* attr = getAttribute(nspace, name);
    if (!attr) {
        return emptyString;
    }

    return attr->getValue();
}

XMLNode* XMLNode::setAttribute(StringView nspace, StringView name, StringView value, StringView prefix)
{
    if (XMLNode* attr = getAttribute(nspace, name)) {
        attr->setValue(value);
        return attr;
    }

    return addAttribute2(nspace, name, value, prefix);
}

XMLNode* XMLNode::addAttribute(StringView nspace, StringView name, StringView value, StringView prefix)
{
    return addAttribute2(nspace, name, value, prefix);
}

XMLNode* XMLNode::addAttribute2(StringView nspace, StringView name, StringView value, StringView prefix)
{
    RefPtr<XMLNode> newAttr = PassRef(new XMLNode);
    newAttr->_type = TypeAttribute;
    newAttr->_value.assign(value.begin(), value.end());
    addChild(newAttr.get());
    newAttr->setName(nspace, name, prefix);
    return newAttr.get(); // We hold a reference.
}

bool XMLNode::removeAttribute(StringView nspace, StringView name)
{
    XMLNode* attr = getAttribute(nspace, name);
    if (!attr) {
        return false;
    }

    return removeChild(attr);
}

const XMLNode* XMLNode::getDescendant(StringView nspace, StringView name) const
{
    if (const XMLNode* found = getChild(nspace, name)) {
        return found;
    }

    for (const XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
        if (const XMLNode* foundInChild = child->getDescendant(nspace, name)) {
            return foundInChild;
        }
    }

    return NULL;
}

std::string XMLNode::getAllText() const
{
    std::string result;

    for (const XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
        if (!child->isText()) {
            continue;
        }

        result.append(child->getValue());
    }

    return result;
}

std::string XMLNode::getAllTextDeep() const
{
    std::string result;

    getAllTextDeep(result);

    return result;
}

void XMLNode::getAllTextDeep(std::string& out) const
{
    for (const XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {

        if (child->isText()) {
            out.append(child->getValue());
        }

        child->getAllTextDeep(out);
    }
}

std::string XMLNode::getAllText(StringView nspace, StringView name) const
{
    const XMLNode* child = getChild(nspace, name);
    if (!child) {
        return "";
    }

    return child->getAllText();
}

std::string XMLNode::getAllTextDeep(StringView nspace, StringView name) const
{
    const XMLNode* child = getChild(nspace, name);
    if (!child) {
        return "";
    }

    return child->getAllTextDeep();
}

bool XMLNode::hasAttributes() const
{
    for (const XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
        if (child->isAttribute()) {
            return true;
        }
    }

    return false;
}

void XMLNode::setCaseInsensitiveTags(bool value)
{
    _caseInsensitiveTags = true;
    for (XMLNode* child = _children.getFirst(); child; child = _children.getNext(child)) {
        child->setCaseInsensitiveTags(value);
    }
}

std::vector<const XMLNode*> XMLNode::getAllElements(StringView nspace, StringView name) const
{
    std::vector<const XMLNode*> nodes;
    getAllElements(nodes, nspace, name);
    return nodes;
}

void XMLNode::getAllElements(std::vector<const XMLNode*>& vector, StringView nspace, StringView name) const
{
    if (isElement()) {
        if (hasName(name) && hasNamespace(nspace)) {
            vector.push_back(this);
        }

        for (const XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
            child->getAllElements(vector, nspace, name);
        }
    }
}

std::vector<XMLNode*> XMLNode::getAllElements(StringView nspace, StringView name)
{
    std::vector<XMLNode*> nodes;
    getAllElements(nodes, nspace, name);
    return nodes;
}

void XMLNode::getAllElements(std::vector<XMLNode*>& vector, StringView nspace, StringView name)
{
    if (isElement()) {
        if (hasName(name) && hasNamespace(nspace)) {
            vector.push_back(this);
        }

        for (XMLNode* child = getFirstChild(); child; child = child->getNextSibling()) {
            child->getAllElements(vector, nspace, name);
        }
    }
}
}
