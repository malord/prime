// Copyright 2000-2021 Mark H. P. Lord

// Invasive, non-circular, double-linked list.

#ifndef PRIME_DOUBLELINKLIST_H
#define PRIME_DOUBLELINKLIST_H

#include "Config.h"

namespace Prime {

//
// LinkListElementManagers
// Manage the lifetime of elements added to or removed from a linked list
//

/// Does not delete elements when they're removed from a linked list.
template <typename Type>
class DetachingLinkListElementManager {
public:
    DetachingLinkListElementManager() PRIME_NOEXCEPT { }

    /// Called when an element has been added to a list.
    void added(Type*) PRIME_NOEXCEPT { }

    /// The specified element has been removed from the list.
    void removed(Type*) PRIME_NOEXCEPT { }

    PRIME_UNCOPYABLE(DetachingLinkListElementManager);
};

/// Deletes elements when they're removed from a linked list.
template <typename Type>
class DeletingLinkListElementManager {
public:
    DeletingLinkListElementManager() PRIME_NOEXCEPT { }

    /// Called when an element has been added to a list.
    void added(Type*) PRIME_NOEXCEPT { }

    /// The specified element has been removed from the list.
    void removed(Type* element) PRIME_NOEXCEPT
    {
        delete element;
    }

    PRIME_UNCOPYABLE(DeletingLinkListElementManager);
};

/// Reference counted linked list elements for elements that have RefPtrRetain() and RefPtrRelease() overloads.
template <typename Type>
class RefCountingLinkListElementManager {
public:
    RefCountingLinkListElementManager() PRIME_NOEXCEPT { }

    /// Called when an element has been added to a list.
    void added(Type* element) PRIME_NOEXCEPT
    {
        RefPtrRetain(element);
    }

    /// The specified element has been removed from the list.
    void removed(Type* element) PRIME_NOEXCEPT
    {
        RefPtrRelease(element);
    }

    PRIME_UNCOPYABLE(RefCountingLinkListElementManager);
};

//
// DoubleLink
//

/// Provides the data and accessors needed for an object to exist in a DoubleLinkList. An object should have a
/// member variable of type DoubleLink<OwnType>. An object can have multiple links for different purposes, and can
/// exist in multiple lists.
template <typename Type>
class DoubleLink {
public:
    DoubleLink() PRIME_NOEXCEPT : _next(NULL),
                                  _previous(NULL)
    {
    }

    /// Return the next element in our linked list.
    Type* getNext() const PRIME_NOEXCEPT { return _next; }

    /// Return the previous element in our linked list.
    Type* getPrevious() const PRIME_NOEXCEPT { return _previous; }

    /// Set the next element in the linked list.
    void setNext(Type* next) PRIME_NOEXCEPT { _next = next; }

    /// Set the previous element in the linked list.
    void setPrevious(Type* prev) PRIME_NOEXCEPT { _previous = prev; }

private:
    Type* _next;
    Type* _previous;

    PRIME_UNCOPYABLE(DoubleLink);
};

/// An invasive linked list where the elements have DoubleLink<> member variables. ElementManager should be
/// DeletingLinkListElementManager, DetachingLinkListElementManager, or RefCountingLinkListElementManager and determines whether and how
/// elements are freed when removed from the list. This is a non-circular linked list. Note that elements can
/// contain multiple DoubleLink<> members allowing them to appear simultaneously in multiple lists.
template <typename Type, typename ElementManager>
class DoubleLinkList {
public:
    typedef DoubleLink<Type> Type::*LinkPointer;

    /// e.g., DoubleLinkList<Object> list(&Object::link), where link is a member variable of type
    /// DoubleLink<Object>.
    explicit DoubleLinkList(LinkPointer linkPointer) PRIME_NOEXCEPT : _first(NULL),
                                                                      _last(NULL),
                                                                      _linkPointer(linkPointer),
                                                                      _count(0)
    {
    }

    /// Attach an existing linked list.
    DoubleLinkList(LinkPointer linkPointer, Type* attachFirst) PRIME_NOEXCEPT : _linkPointer(linkPointer)
    {
        _first = attachFirst;
        if (attachFirst) {
            Type* ptr = attachFirst;
            for (;;) {
                ++_count;
                Type* next = getNext(ptr);
                if (!next) {
                    break;
                }

                ptr = next;
            }

            _last = ptr;
        } else {
            _last = NULL;
            _count = 0;
        }
    }

    /// Attach an existing linked list.
    DoubleLinkList(Type* attachFirst, Type* attachLast, size_t attachCount) PRIME_NOEXCEPT
    {
        _first = attachLast;
        _last = attachFirst;
        _count = attachCount;
    }

    ~DoubleLinkList() PRIME_NOEXCEPT { clear(); }

    /// Returns true if the list is empty.
    bool empty() const PRIME_NOEXCEPT { return _first == NULL; }

    /// Get the first element in the list.
    const Type* getFirst() const PRIME_NOEXCEPT { return _first; }
    Type* getFirst() PRIME_NOEXCEPT { return _first; }

    /// Get the last element in the list.
    const Type* getLast() const PRIME_NOEXCEPT { return _last; }
    Type* getLast() PRIME_NOEXCEPT { return _last; }

    /// Return the next element in the list.
    const Type* getNext(const Type* element) const PRIME_NOEXCEPT { return getLink(element).getNext(); }
    Type* getNext(Type* element) PRIME_NOEXCEPT { return getLink(element).getNext(); }

    /// Return the previous element in the list.
    const Type* getPrevious(const Type* element) const PRIME_NOEXCEPT { return getLink(element).getPrevious(); }
    Type* getPrevious(Type* element) PRIME_NOEXCEPT { return getLink(element).getPrevious(); }

    /// Get a reference to the first element in the list.
    const Type& front() const PRIME_NOEXCEPT { return *_first; }
    Type& front() PRIME_NOEXCEPT { return *_first; }

    /// Get a reference to the last element in the list.
    const Type& back() const PRIME_NOEXCEPT { return *_last; }
    Type& back() PRIME_NOEXCEPT { return *_last; }

    /// Empty the list.
    void clear() PRIME_NOEXCEPT
    {
        while (_first) {
            erase(_first);
        }

        _last = NULL;
        _count = 0;
    }

    /// Add an element to the list.
    void push_back(Type* element) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!contains(element));

        pushBackWithoutNotifyingManager(element);
        _manager.added(element);
    }

    void pop_back() PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(_last);
        erase(_last);
    }

    /// Add an element to the front of the list.
    void push_front(Type* element) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!contains(element));

        pushFrontWithoutNotifyingManager(element);
        _manager.added(element);
    }

    void pop_front() PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(_first);
        erase(_first);
    }

    /// Insert an element in front of another element. If "before" is a null pointer, the element is inserted at
    /// the end of the list.
    void insertBefore(Type* element, Type* before) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!contains(element));

        insertBeforeWithoutNotifyingManager(element, before);
        _manager.added(element);
    }

    /// Insert an element behind another element. If "after" is a null pointer, the element is inserted at the
    /// end of the list.
    void insertAfter(Type* element, Type* after) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!contains(element));

        insertAfterWithoutNotifyingManager(element, after);
        _manager.added(element);
    }

    /// Remove an element from the list. Undefined behaviour if the element isn't in this list.
    void erase(Type* element) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(contains(element));
        removeWithoutNotifyingManager(element);
        _manager.removed(element);
    }

    /// Remove an element from the list without deleting it.
    Type* detach(Type* element) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(contains(element));
        removeWithoutNotifyingManager(element);

        setNext(element, NULL);
        setPrevious(element, NULL);

        return element;
    }

    /// Move the element to the end of the list.
    void moveToBack(Type* element) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(contains(element));
        removeWithoutNotifyingManager(element);
        pushBackWithoutNotifyingManager(element);
    }

    /// Move the element to the front of the list.
    void moveToFront(Type* element) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(contains(element));
        removeWithoutNotifyingManager(element);
        pushFrontWithoutNotifyingManager(element);
    }

    /// Move the element in front of another element.
    void moveBefore(Type* element, Type* before) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(contains(element));
        if (element == before) {
            return;
        }
        removeWithoutNotifyingManager(element);
        insertBeforeWithoutNotifyingManager(element, before);
    }

    /// Move the element behind another element.
    void moveAfter(Type* element, Type* after) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(contains(element));
        if (element == after) {
            return;
        }
        removeWithoutNotifyingManager(element);
        insertBeforeWithoutNotifyingManager(element, after);
    }

    /// Returns true if this list contains the specified element.
    bool contains(const Type* element) const PRIME_NOEXCEPT
    {
        if (_first) {
            for (const Type* test = element; test; test = getPrevious(test)) {
                if (test == _first) {
                    return true;
                }
            }
        }

        return false;
    }

    /// Returns true if element "a" comes after element "b". The test only works if a and b are both in the list.
    bool isAfter(const Type* a, const Type* b) const PRIME_NOEXCEPT
    {
        for (const Type* test = getNext(a); test; test = getNext(test)) {
            if (test == b) {
                return false;
            }
        }

        return true;
    }

    /// Returns the number of elements in the list.
    size_t size() const PRIME_NOEXCEPT { return _count; }

    const Type* at(size_t index) const PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(index < size());

        const Type* element;
        for (element = getFirst(); element && index; element = getNext(element), --index) { }

        return element;
    }

    Type* at(size_t index) PRIME_NOEXCEPT
    {
        return const_cast<Type*>(const_cast<const DoubleLinkList*>(this)->at(index));
    }

    void move(DoubleLinkList& other) PRIME_NOEXCEPT
    {
        clear();

        _first = other._first;
        _last = other._last;
        _count = other._count;

        other._first = other._last = NULL;
        other._count = 0;
    }

    // TODO: splice
    // TODO: remove
    // TODO: remove_if
    // TODO: unique
    // TODO: sort?

    // iterator and const_iterator could have been implemented as a single template,
    // but that caused an internal compiler error in Visual C++ 6.

    class iterator {
        Type* _element;
        DoubleLink<Type> Type::*_linkPointer;

    public:
        iterator(Type* element, DoubleLink<Type> Type::*linkPointer) PRIME_NOEXCEPT : _element(element),
                                                                                      _linkPointer(linkPointer)
        {
        }

        Type& operator*() const PRIME_NOEXCEPT { return *_element; }
        Type* operator->() const PRIME_NOEXCEPT { return _element; }

        iterator& operator++() PRIME_NOEXCEPT
        {
            _element = getLink(_element)->getNext();
            return *this;
        }

        iterator operator++(int) PRIME_NOEXCEPT
        {
            iterator result(*this);
            operator++();
            return result;
        }

        bool operator==(const iterator& rhs) const PRIME_NOEXCEPT
        {
            return _element == rhs._element && _linkPointer == rhs._linkPointer;
        }

        bool operator!=(const iterator& rhs) const PRIME_NOEXCEPT
        {
            return !operator==(rhs);
        }

        DoubleLink<Type>& getLink(Type* element) const PRIME_NOEXCEPT { return element->*_linkPointer; }
    };

    class const_iterator {
        const Type* _element;
        DoubleLink<Type> Type::*_linkPointer;

    public:
        const_iterator(const Type* element, DoubleLink<Type> Type::*linkPointer) PRIME_NOEXCEPT : _element(element),
                                                                                                  _linkPointer(linkPointer)
        {
        }

        const Type& operator*() const PRIME_NOEXCEPT { return *_element; }
        const Type* operator->() const PRIME_NOEXCEPT { return _element; }

        const_iterator& operator++() PRIME_NOEXCEPT
        {
            _element = getLink(_element).getNext();
            return *this;
        }

        const_iterator operator++(int) PRIME_NOEXCEPT
        {
            const_iterator result(*this);
            operator++();
            return result;
        }

        bool operator==(const const_iterator& rhs) const PRIME_NOEXCEPT
        {
            return _element == rhs._element && _linkPointer == rhs._linkPointer;
        }

        bool operator!=(const const_iterator& rhs) const PRIME_NOEXCEPT
        {
            return !operator==(rhs);
        }

        const DoubleLink<Type>& getLink(const Type* element) const PRIME_NOEXCEPT { return const_cast<Type*>(element)->*_linkPointer; }
    };

    const_iterator cbegin() const PRIME_NOEXCEPT { return const_iterator(_first, _linkPointer); }
    const_iterator cend() const PRIME_NOEXCEPT { return const_iterator(NULL, _linkPointer); }
    const_iterator begin() const PRIME_NOEXCEPT { return const_iterator(_first, _linkPointer); }
    const_iterator end() const PRIME_NOEXCEPT { return const_iterator(NULL, _linkPointer); }
    iterator begin() PRIME_NOEXCEPT { return iterator(_first, _linkPointer); }
    iterator end() PRIME_NOEXCEPT { return iterator(NULL, _linkPointer); }

private:
    /// Remove an element from the list without notifying the manager.
    void removeWithoutNotifyingManager(Type* element) PRIME_NOEXCEPT
    {
        Type* prev = getPrevious(element);
        Type* next = getNext(element);

        if (prev) {
            setNext(prev, next);
        } else {
            _first = next;
        }

        if (next) {
            setPrevious(next, prev);
        } else {
            _last = prev;
        }

        --_count;
    }

    /// Add an element to the end of the list without notifying the manager.
    void pushBackWithoutNotifyingManager(Type* element) PRIME_NOEXCEPT
    {
        if (_last) {
            setNext(_last, element);
            setPrevious(element, _last);
            setNext(element, NULL);
            _last = element;
        } else {
            _first = _last = element;
            setNext(element, NULL);
            setPrevious(element, NULL);
        }

        ++_count;
    }

    /// Add an element to the front of the list without notifying the manager.
    void pushFrontWithoutNotifyingManager(Type* element) PRIME_NOEXCEPT
    {
        setNext(element, _first);
        setPrevious(element, NULL);

        if (_first) {
            setPrevious(_first, element);
        } else {
            _last = element;
        }

        _first = element;

        ++_count;
    }

    /// Add an element in front of another element without notifying the manager.
    void insertBeforeWithoutNotifyingManager(Type* element, Type* before) PRIME_NOEXCEPT
    {
        if (!before) {
            pushBackWithoutNotifyingManager(element);
            return;
        }

        PRIME_DEBUG_ASSERT(contains(before));

        setPrevious(element, getPrevious(before));
        setNext(element, before);

        setPrevious(before, element);

        if (Type* prev = getPrevious(element)) {
            setNext(prev, element);
        } else {
            _first = element;
        }

        ++_count;
    }

    /// Add an element behind another element without notifying the manager.
    void insertAfterWithoutNotifyingManager(Type* element, Type* after) PRIME_NOEXCEPT
    {
        if (!after) {
            pushFrontWithoutNotifyingManager(element);
            return;
        }

        PRIME_DEBUG_ASSERT(contains(after));

        setPrevious(element, after);
        setNext(element, getNext(after));

        setNext(after, element);

        if (Type* next = getNext(element)) {
            setPrevious(next, element);
        } else {
            _last = element;
        }

        ++_count;
    }

    const DoubleLink<Type>& getLink(const Type* element) const PRIME_NOEXCEPT { return const_cast<Type*>(element)->*_linkPointer; }
    DoubleLink<Type>& getLink(const Type* element) PRIME_NOEXCEPT { return const_cast<Type*>(element)->*_linkPointer; }

    void setNext(Type* element, Type* next) PRIME_NOEXCEPT { getLink(element).setNext(next); }

    void setPrevious(Type* element, Type* prev) PRIME_NOEXCEPT { getLink(element).setPrevious(prev); }

    LinkPointer _linkPointer;
    ElementManager _manager;
    Type* _first;
    Type* _last;
    size_t _count;

    PRIME_UNCOPYABLE(DoubleLinkList);
};
}

#endif
