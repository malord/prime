// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_VALUE_H
#define PRIME_VALUE_H

#define PRIME_HAVE_VALUE

#include "Array.h"
#include "Convert.h"
#include "Data.h"
#include "DateTime.h"
#include "Dictionary.h"
#include "RefCounting.h"
#include "UIDCast.h"
#include "UnixTime.h"
#include <vector>
#ifdef PRIME_ENABLE_IOSTREAMS
#include <ostream>
#endif

namespace Prime {

//
// Value
//

/// Stores a null, boolean, integer, float, string, data, date, time, datetime (as a UnixTime), vector,
/// dictionary or pointer and provides methods to convert between them. There are get*() methods for reading the
/// value without conversion, to*() methods for returning a converted value and access*() methods for direct
/// access to this value, converting if necessary.
class PRIME_PUBLIC Value {
public:
    //
    // Types
    //

    enum Type {
        TypeUndefined,
        TypeNull,
        TypeBool,
        TypeInteger,
        TypeReal,
        TypeString,
        TypeData,
        TypeDate,
        TypeTime,
        TypeDateTime,
        TypeVector,
        TypeDictionary,
        TypeObject
    };

    typedef int64_t Integer;
#define PRIME_PRId_VALUE PRId64

    typedef uint64_t UInteger;

    typedef FloatMax Real;
#define PRIME_PRIg_VALUE PRIME_PRIg_FLOATMAX_AS_DOUBLE

    typedef std::vector<Value> Vector;
    typedef Prime::Dictionary<std::string, Value> Dictionary;
    typedef Prime::Dictionary<std::string, Value>::value_type Pair;

    template <typename Container>
    static Vector makeVector(const Container& container);

    /// An object capable of managing a pointer we can store.
    class PRIME_PUBLIC ObjectManager {
        PRIME_DECLARE_UID_CAST_BASE(0x54cecc72u, 0xd7d14edbu, 0xacc89ae3u, 0x3775d235u)

    public:
        ObjectManager() { }

        virtual ~ObjectManager();

        virtual void retain(const void* object) const = 0;

        virtual void release(const void* object) const = 0;

        virtual bool less(const void* lhs, const Value& rhs) const;

        virtual bool equal(const void* lhs, const Value& rhs) const;

        virtual Value toValue(const void* object) const;

        /// UIDObjectManager uses this directly, but COM could also use a UID and other non-Prime types could
        /// create their own UIDs for their types.
        virtual const void* cast(const UID& uid, const void* object) const = 0;

    private:
        PRIME_UNCOPYABLE(ObjectManager);
    };

    class PRIME_PUBLIC Object : public RefCounted {
        PRIME_DECLARE_UID_CAST_BASE(0x5bd29d12u, 0xaaee4dcau, 0x8e43a221u, 0x68854507u)

    public:
        virtual bool equal(const Value& other) const;

        virtual bool less(const Value& other) const;
    };

    /// A Value::ObjectManager for types which are UIDCast-able and implement retain/release reference counting.
    template <typename ObjectType>
    class UIDObjectManager;

    //
    // Constants
    //

    static const Value undefined;
    static const Value null;

    static const std::string emptyString;
    static const Date emptyDate;
    static const Time emptyTime;
    static const UnixTime emptyUnixTime;
    static const Data emptyData;
    static const Vector emptyVector;
    static const Dictionary emptyDictionary;

    //
    // Constructors and copy operators
    //

    Value() PRIME_NOEXCEPT : _type(TypeUndefined)
    {
    }

    Value(Undefined) PRIME_NOEXCEPT : _type(TypeUndefined)
    {
    }

    Value(Null) PRIME_NOEXCEPT : _type(TypeNull)
    {
    }

    Value(const Value& copy);

    Value(bool assign) PRIME_NOEXCEPT : _type(TypeBool)
    {
        _value.boolean = assign;
    }

    Value(char assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = assign;
    }

    Value(unsigned char assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = (Integer)assign;
    }

    Value(short assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = assign;
    }

    Value(unsigned short assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = (Integer)assign;
    }

    Value(int assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = assign;
    }

    Value(unsigned int assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = (Integer)assign;
    }

    Value(long assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = assign;
    }

    Value(unsigned long assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = (Integer)assign;
    }

#if defined(ULLONG_MAX)

    Value(long long assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = assign;
    }

#elif defined(UINT64_MAX)

    Value(int64_t assign) PRIME_NOEXCEPT : _type(TypeInteger)
    {
        _value.integer = assign;
    }

#endif

    Value(Real assign) PRIME_NOEXCEPT : _type(TypeReal)
    {
        _value.real = assign;
    }

#ifndef PRIME_LONG_DOUBLE_IS_DOUBLE
    Value(double assign) PRIME_NOEXCEPT : _type(TypeReal)
    {
        _value.real = assign;
    }

#endif

    Value(float assign) PRIME_NOEXCEPT : _type(TypeReal)
    {
        _value.real = assign;
    }

    Value(const std::string& assign)
        : _type(TypeString)
    {
        new (rawString()) std::string(assign);
    }

    Value(const char* string)
        : _type(TypeString)
    {
        new (rawString()) std::string(string);
    }

    Value(StringView string)
        : _type(TypeString)
    {
        new (rawString()) std::string(string.begin(), string.end());
    }

    Value(const Data& assign)
        : _type(TypeData)
    {
        new (rawData()) Data(assign);
    }

    Value(const Vector& assign);

    Value(const Dictionary& assign);

    Value(const Date& assign)
        : _type(TypeDate)
    {
        new (rawDate()) Date(assign);
    }

    Value(const Time& assign)
        : _type(TypeTime)
    {
        new (rawTime()) Time(assign);
    }

    Value(const UnixTime& assign)
        : _type(TypeDateTime)
    {
        new (rawUnixTime()) UnixTime(assign);
    }

    Value(const DateTime& assign) PRIME_NOEXCEPT : _type(TypeDateTime)
    {
        new (rawUnixTime()) UnixTime(assign.toUnixTime());
    }

    Value(ObjectManager* manager, void* object)
        : _type(TypeObject)
    {
        new (rawObjectWrapper()) ObjectWrapper(manager, object);
    }

    Value(Object* object);

    Value(const std::vector<std::string>& strings);

#ifdef PRIME_COMPILER_RVALUEREF

    Value(std::string&& assign) PRIME_NOEXCEPT : _type(TypeString)
    {
        new (rawString()) std::string(std::move(assign));
    }

    Value(Data&& assign) PRIME_NOEXCEPT : _type(TypeData)
    {
        new (rawData()) Data(std::move(assign));
    }

    Value(Vector&& assign) PRIME_NOEXCEPT;

    Value(Dictionary&& assign) PRIME_NOEXCEPT;

    Value(std::vector<std::string>&& strings);

    Value(Value&& move) PRIME_NOEXCEPT
    {
        constructMove(move);
    }

#endif

#ifdef PRIME_COMPILER_INITLIST
    Value(std::initializer_list<Value> list)
    {
        new (rawVector()) Vector(list);
    }
#endif

    ~Value() PRIME_NOEXCEPT
    {
        destructValue();
    }

    Value& operator=(const Value& assign);

#ifdef PRIME_COMPILER_RVALUEREF
    Value& operator=(Value&& rhs) PRIME_NOEXCEPT
    {
        if (this == &rhs) {
            return *this;
        }

        move(rhs);
        return *this;
    }
#endif

    void move(Value& rhs);

    void swap(Value& rhs);

    //
    // Querying the type
    //

    Type getType() const PRIME_NOEXCEPT { return _type; }

    /// Truthiness. Only undefined is considered false. This is to allow, `if (Value value = dict["key"]) { ... }`
    bool operator!() const PRIME_NOEXCEPT { return _type == TypeUndefined; }

#ifdef PRIME_COMPILER_EXPLICIT_CONVERSION
    explicit operator bool() const PRIME_NOEXCEPT
    {
        return _type != TypeUndefined;
    }
#endif

    bool isUndefined() const PRIME_NOEXCEPT
    {
        return _type == TypeUndefined;
    }
    bool isNull() const PRIME_NOEXCEPT { return _type == TypeNull; }
    bool isBool() const PRIME_NOEXCEPT { return _type == TypeBool; }
    bool isInteger() const PRIME_NOEXCEPT { return _type == TypeInteger; }
    bool isReal() const PRIME_NOEXCEPT { return _type == TypeReal; }
    bool isString() const PRIME_NOEXCEPT { return _type == TypeString; }
    bool isData() const PRIME_NOEXCEPT { return _type == TypeData; }
    bool isDate() const PRIME_NOEXCEPT { return _type == TypeDate; }
    bool isTime() const PRIME_NOEXCEPT { return _type == TypeTime; }
    bool isDateTime() const PRIME_NOEXCEPT { return _type == TypeDateTime; }
    bool isVector() const PRIME_NOEXCEPT { return _type == TypeVector; }
    bool isDictionary() const PRIME_NOEXCEPT { return _type == TypeDictionary; }
    bool isObject() const PRIME_NOEXCEPT { return _type == TypeObject; }

    /// Returns true for undefined, null or an empty string/vector/dictionary.
    bool isEmpty() const PRIME_NOEXCEPT;

    //
    // Access to our data with no conversion. These methods are guaranteed to succeed, but will return a null
    // value if we're not of the correct type. Use the getType() or isTypeName() methods to check the type, or the
    // toTypeName() methods if you want conversion.
    //

    bool getBool() const PRIME_NOEXCEPT { return (_type == TypeBool) ? _value.boolean : false; }
    Integer getInteger() const PRIME_NOEXCEPT { return (_type == TypeInteger) ? _value.integer : 0; }
    Real getReal() const PRIME_NOEXCEPT { return (_type == TypeReal) ? _value.real : 0; }
    const std::string& getString() const PRIME_NOEXCEPT { return (_type == TypeString) ? *rawString() : emptyString; }
    const Data& getData() const PRIME_NOEXCEPT { return (_type == TypeData) ? *rawData() : emptyData; }
    const Date& getDate() const PRIME_NOEXCEPT { return (_type == TypeDate) ? *rawDate() : emptyDate; }
    const Time& getTime() const PRIME_NOEXCEPT { return (_type == TypeTime) ? *rawTime() : emptyTime; }
    const UnixTime& getUnixTime() const PRIME_NOEXCEPT { return (_type == TypeDateTime) ? *rawUnixTime() : emptyUnixTime; }
    DateTime getDateTime() const PRIME_NOEXCEPT { return (_type == TypeDateTime) ? DateTime(*rawUnixTime()) : DateTime(); }
    const Vector& getVector() const PRIME_NOEXCEPT { return (_type == TypeVector) ? *rawVector() : emptyVector; }
    const Dictionary& getDictionary() const PRIME_NOEXCEPT { return (_type == TypeDictionary) ? *rawDictionary() : emptyDictionary; }

    const char* c_str() const PRIME_NOEXCEPT { return (_type == TypeString) ? rawString()->c_str() : ""; }

    template <typename ObjectType>
    const ObjectType* getObject() const PRIME_NOEXCEPT
    {
        if (_type != TypeObject) {
            return NULL;
        }

        const ObjectWrapper* objectWrapper = rawObjectWrapper();
        return reinterpret_cast<const ObjectType*>(objectWrapper->cast(ObjectType::classGetUID()));
    }

    template <typename ObjectType>
    ObjectType* getObject() PRIME_NOEXCEPT
    {
        if (_type != TypeObject) {
            return NULL;
        }

        const ObjectWrapper* objectWrapper = rawObjectWrapper();
        return const_cast<ObjectType*>(reinterpret_cast<const ObjectType*>(objectWrapper->cast(ObjectType::classGetUID())));
    }

    const void* getRawObjectPointer() const PRIME_NOEXCEPT { return (_type != TypeObject) ? NULL : rawObjectWrapper()->getPointer(); }
    void* getRawObjectPointer() PRIME_NOEXCEPT { return (_type != TypeObject) ? NULL : rawObjectWrapper()->getPointer(); }

    //
    // Direct access to the underlying value. If you call accessString() on the integer 7, this Value will become
    // a string and you'll receive a reference to "7". If you call accessVector() on the integer 7, this Value
    // will become a Vector containing the Value 7.
    //

    bool& accessBool() { return (_type == TypeBool) ? _value.boolean : convertToBool(); }
    Integer& accessInteger() { return (_type == TypeInteger) ? _value.integer : convertToInteger(); }
    Real& accessReal() { return (_type == TypeReal) ? _value.real : convertToReal(); }
    std::string& accessString() { return (_type == TypeString) ? *rawString() : convertToString(); }
    Data& accessData() { return (_type == TypeData) ? *rawData() : convertToData(); }
    Date& accessDate() { return (_type == TypeDate) ? *rawDate() : convertToDate(); }
    Time& accessTime() { return (_type == TypeTime) ? *rawTime() : convertToTime(); }
    UnixTime& accessUnixTime() { return (_type == TypeDateTime) ? *rawUnixTime() : convertToUnixTime(); }
    Vector& accessVector() { return (_type == TypeVector) ? *rawVector() : convertToVector(); }
    Dictionary& accessDictionary() { return (_type == TypeDictionary) ? *rawDictionary() : convertToDictionary(); }

    //
    // Dictionary/Vector lookup
    //

    const Value& operator[](StringView key) const;
    const Value& operator[](const char* key) const;
    const Value& operator[](const std::string& key) const;

    template <typename Key>
    const Value& operator[](Key key) const { return operator[](static_cast<size_t>(key)); }

    const Value& operator[](size_t key) const;

    //
    // Set the type of this Value and clear any existing value.
    //

    bool& resetBool();
    Integer& resetInteger();
    Real& resetReal();
    std::string& resetString();
    Data& resetData();
    Date& resetDate();
    Time& resetTime();
    UnixTime& resetUnixTime();
    Vector& resetVector();
    Dictionary& resetDictionary();

    //
    // Read the value in the desired type, converting if necessary.
    // For vectors and dictionaries, this requires deep-copying.
    //

    bool toBool(bool defaultValue = false) const
    {
        return (_type == TypeBool) ? _value.boolean : ToBool(*this, defaultValue);
    }

    Integer toInteger(Integer defaultValue = 0) const
    {
        return (_type == TypeInteger) ? _value.integer : ToInteger<Integer>(*this, defaultValue);
    }

    int toInt(int defaultValue = 0) const;

    unsigned int toUInt(unsigned int defaultValue) const;

    int64_t toInt64(int64_t defaultValue = 0) const;

    long toLong(long defaultValue = 0) const;

    Real toReal(Real defaultValue = 0) const
    {
        return (_type == TypeReal) ? _value.real : ToReal<Real>(*this, defaultValue);
    }

    float toFloat(float defaultValue = 0) const;

    double toDouble(double defaultValue = 0) const;

    /// Returns the value as a string, without quotes.
    std::string toString(StringView defaultValue = "") const
    {
        return (_type == TypeString) ? *rawString() : ToString(*this, defaultValue);
    }

    Data toData() const
    {
        return (_type == TypeData) ? *rawData() : ToData(*this);
    }

    Date toDate(const Date& defaultValue = Date()) const
    {
        return (_type == TypeDate) ? *rawDate() : ToDate(*this, defaultValue);
    }

    Time toTime(const Time& defaultValue = Time()) const
    {
        return (_type == TypeTime) ? *rawTime() : ToTime(*this, defaultValue);
    }

    UnixTime toUnixTime(const UnixTime& defaultValue = UnixTime()) const
    {
        return (_type == TypeDateTime) ? *rawUnixTime() : ToUnixTime(*this, defaultValue);
    }

    DateTime toDateTime(const DateTime& defaultValue = DateTime()) const
    {
        return (_type == TypeDateTime) ? DateTime(*rawUnixTime()) : ToDateTime(*this, defaultValue);
    }

    /// If we're not a Vector, returns a Vector with this as its one element. If we're undefined, returns
    /// valueIfUndefined.
    Vector toVector(const Vector& valueIfUndefined = emptyVector) const;

    /// This is rarely necessary, but it can convert an object to a Dictionary.
    Dictionary toDictionary(const Dictionary& valueIfUndefined = emptyDictionary) const;

    /// Converts an object to a non-object, otherwise returns a copy of this.
    Value toValue() const;

    //
    // Conversions with error returns
    //

    bool convert(Type type, Value& result) const;

    //
    // Returns *this if this is defined, otherwise returns other
    // e.g., properties["markup"].otherwise(0)
    //

    const Value& otherwise(const Value& other) const
    {
        return isUndefined() ? other : *this;
    }

    Value& otherwise(Value& other)
    {
        return isUndefined() ? other : *this;
    }

    //
    // Comparison helpers
    //

    static bool equal(const Value& lhs, const Value& rhs);

    static bool less(const Value& lhs, const Value& rhs);

    //
    // Dictionary helpers
    //

    /// e.g., setDictionaryPath(&dictionary, "MainWindow.size.x", 1000);
    static void setDictionaryPath(Dictionary& dictionary, const char* path, Value value);

    //
    // Diffing/patching
    //

    /// Creates a dictionary that contains the difference between two dictionaries. Any dictionaries within
    /// dictionaries are recursively diff'd as well. Keys removed from dictionaries are listed as a vector with
    /// the key specified by missingKeysKey. If missingKeysKey is empty, absent keys are given an undefined
    /// value in the diff.
    static Dictionary dictionaryDiff(const Dictionary& old, const Dictionary& now, const std::string& missingKeysKey);

    /// Given a diff computed by dictionaryDiff(), modifies an existing dictionary by patching it, producing
    /// the new dictionary.
    static Dictionary dictionaryPatch(const Dictionary& old, const Dictionary& diff, const std::string& missingKeysKey);

    enum MergeMode {
        MergeModeOverwrite,
        MergeModeVector,
        MergeModeEqual,
        MergeModeMissing,
    };

    /// Merge source on to target, returning true if target was modified, false if target and source are equal.
    static bool merge(Value& target, const Value& source, MergeMode mode);

    /// Merge source on to target, returning true if target was modified, false if target and source are equal.
    static bool merge(Dictionary& target, const Dictionary& source, MergeMode mode);

private:
    operator void*() const { return NULL; }

    void constructMove(Value& assign);

    Value(const void*)
        : _type(TypeUndefined)
    {
    }

    void destructValue() PRIME_NOEXCEPT
    {
        if (_type >= TypeString) {
            destructValueSlow();
        }
    }

    void destructValueSlow() PRIME_NOEXCEPT;

    const Value& getVectorIndex(const Value& key) const;

    bool getVectorIndex(const Value& key, Value& output) const;

    // This operator used to exist but was a bad idea. Now private to uncover old code.
    template <typename RHS>
    const Value& operator||(const RHS&) const { return *this; }

    // This operator used to exist but was a bad idea. Now private to uncover old code.
    template <typename RHS>
    const Value& operator&&(const RHS&) const { return *this; }

    class ObjectWrapper {
    private:
        ObjectManager* _manager;
        void* _object;

    public:
        ObjectWrapper(ObjectManager* manager, void* object) PRIME_NOEXCEPT : _manager(manager),
                                                                             _object(object)
        {
            _manager->retain(_object);
        }

        ObjectWrapper(const ObjectWrapper& copy) PRIME_NOEXCEPT : _manager(copy._manager),
                                                                  _object(copy._object)
        {
            _manager->retain(_object);
        }

        ObjectWrapper& operator=(const ObjectWrapper& copy) PRIME_NOEXCEPT
        {
            if (_object != copy._object) {
                if (copy._object) {
                    copy._manager->retain(copy._object);
                }

                release();

                _manager = copy._manager;
                _object = copy._object;
            }

            return *this;
        }

#ifdef PRIME_COMPILER_RVALUEREF
        ObjectWrapper(ObjectWrapper&& move) PRIME_NOEXCEPT : _manager(move._manager),
                                                             _object(move._object)
        {
            move._manager = NULL;
            move._object = NULL;
        }

        ObjectWrapper& operator=(ObjectWrapper&& move) PRIME_NOEXCEPT
        {
            if (_object != move._object) {
                release();

                _manager = move._manager;
                _object = move._object;

                move._manager = NULL;
                move._object = NULL;
            }

            return *this;
        }
#endif

        ~ObjectWrapper() PRIME_NOEXCEPT
        {
            release();
        }

        void release() PRIME_NOEXCEPT
        {
            if (_object) {
                _manager->release(_object);
            }
        }

        const void* cast(const UID& uid) const
        {
            return _manager->cast(uid, const_cast<const void*>(_object));
        }

        Value toValue() const PRIME_NOEXCEPT
        {
            return _manager->toValue(_object);
        }

        bool less(const Value& rhs) const PRIME_NOEXCEPT
        {
            return _manager->less(_object, rhs);
        }

        bool equal(const Value& rhs) const PRIME_NOEXCEPT
        {
            return _manager->equal(_object, rhs);
        }

        const void* getPointer() const PRIME_NOEXCEPT { return _object; }
        void* getPointer() PRIME_NOEXCEPT { return _object; }
    };

    bool& convertToBool();
    Integer& convertToInteger();
    Real& convertToReal();
    std::string& convertToString();
    Data& convertToData();
    Date& convertToDate();
    Time& convertToTime();
    UnixTime& convertToUnixTime();
    Vector& convertToVector();
    Dictionary& convertToDictionary();

    union Storage {
        bool boolean;
        Integer integer;
        Real real;
        char _date[sizeof(Date)];
        char _time[sizeof(Time)];
        char _unixTime[sizeof(UnixTime)];
        char _string[sizeof(std::string)];
        char _data[sizeof(Data)];
        char _vector[sizeof(std::vector<Prime::Dictionary<std::string, std::string>>)];
        char _dictionary[sizeof(Prime::Dictionary<Prime::Dictionary<std::string, std::string>, std::string>)];
        char _objectWrapper[sizeof(ObjectWrapper)];
    };

    Storage _value;
    Type _type;

    Date* rawDate() PRIME_NOEXCEPT { return (Date*)_value._date; }
    const Date* rawDate() const PRIME_NOEXCEPT { return (Date*)_value._date; }

    Time* rawTime() PRIME_NOEXCEPT { return (Time*)_value._time; }
    const Time* rawTime() const PRIME_NOEXCEPT { return (Time*)_value._time; }

    UnixTime* rawUnixTime() PRIME_NOEXCEPT { return (UnixTime*)_value._unixTime; }
    const UnixTime* rawUnixTime() const PRIME_NOEXCEPT { return (const UnixTime*)_value._unixTime; }

    Data* rawData() PRIME_NOEXCEPT { return (Data*)_value._data; }
    const Data* rawData() const PRIME_NOEXCEPT { return (const Data*)_value._data; }

    std::string* rawString() PRIME_NOEXCEPT { return (std::string*)_value._string; }
    const std::string* rawString() const PRIME_NOEXCEPT { return (const std::string*)_value._string; }

    Vector* rawVector() PRIME_NOEXCEPT { return (Vector*)_value._vector; }
    const Vector* rawVector() const PRIME_NOEXCEPT { return (const Vector*)_value._vector; }

    Dictionary* rawDictionary() PRIME_NOEXCEPT { return (Dictionary*)_value._dictionary; }
    const Dictionary* rawDictionary() const PRIME_NOEXCEPT { return (const Dictionary*)_value._dictionary; }

    ObjectWrapper* rawObjectWrapper() PRIME_NOEXCEPT { return (ObjectWrapper*)_value._objectWrapper; }
    const ObjectWrapper* rawObjectWrapper() const PRIME_NOEXCEPT { return (const ObjectWrapper*)_value._objectWrapper; }
};

template <typename Container>
Value::Vector Value::makeVector(const Container& container)
{
    Value::Vector v;
    std::copy(container.begin(), container.end(), std::back_inserter(v));
    return v;
}

//
// Value::UIDObjectManager
//

template <typename ObjectType>
class Value::UIDObjectManager : public Value::ObjectManager {
public:
    virtual void retain(const void* object) const PRIME_NOEXCEPT PRIME_OVERRIDE
    {
        reinterpret_cast<const ObjectType*>(object)->retain();
    }

    virtual void release(const void* object) const PRIME_NOEXCEPT PRIME_OVERRIDE
    {
        reinterpret_cast<const ObjectType*>(object)->release();
    }

    virtual const void* cast(const UID& uid, const void* object) const PRIME_NOEXCEPT PRIME_OVERRIDE
    {
        return reinterpret_cast<const ObjectType*>(object)->castUID(uid);
    }

    /// e.g., Value::UIDObjectManager<User>::wrap(user)
    static Value wrap(ObjectType* object) PRIME_NOEXCEPT
    {
        return Value(get(), reinterpret_cast<void*>(object));
    }

    /// e.g., Value::UIDObjectManager<User>::get()
    static UIDObjectManager* get() PRIME_NOEXCEPT
    {
        static UIDObjectManager singleton;
        return &singleton;
    }

private:
    UIDObjectManager() PRIME_NOEXCEPT { }
};

//
// Value comparisons (with specialisations for strings)
//

inline bool operator==(const Value& lhs, const Value& rhs)
{
    return Value::equal(lhs, rhs);
}

inline bool operator<(const Value& lhs, const Value& rhs)
{
    return Value::less(lhs, rhs);
}

inline bool operator==(const Value& lhs, const std::string& rhs)
{
    if (lhs.isString()) {
        return lhs.getString() == rhs;
    }

    return lhs.toString() == rhs;
}

inline bool operator<(const Value& lhs, const std::string& rhs)
{
    if (lhs.isString()) {
        return lhs.getString() < rhs;
    }

    return lhs.toString() < rhs;
}

inline bool operator==(const std::string& lhs, const Value& rhs)
{
    if (rhs.isString()) {
        return rhs.getString() == lhs;
    }

    return rhs.toString() == lhs;
}

inline bool operator<(const std::string& lhs, const Value& rhs)
{
    if (rhs.isString()) {
        return lhs < rhs.getString();
    }

    return lhs < rhs.toString();
}

inline bool operator==(const Value& lhs, const char* rhs)
{
    if (lhs.isString()) {
        return lhs.getString() == rhs;
    }

    return lhs.toString() == rhs;
}

inline bool operator<(const Value& lhs, const char* rhs)
{
    if (lhs.isString()) {
        return lhs.getString() < rhs;
    }

    return lhs.toString() < rhs;
}

inline bool operator==(const char* lhs, const Value& rhs)
{
    if (rhs.isString()) {
        return rhs.getString() == lhs;
    }

    return rhs.toString() == lhs;
}

inline bool operator<(const char* lhs, const Value& rhs)
{
    if (rhs.isString()) {
        return lhs < rhs.getString();
    }

    return lhs < rhs.toString();
}

inline bool operator!=(const Value& lhs, const Value& rhs) { return !operator==(lhs, rhs); }
inline bool operator>(const Value& lhs, const Value& rhs) { return operator<(rhs, lhs); }
inline bool operator<=(const Value& lhs, const Value& rhs) { return !operator>(lhs, rhs); }
inline bool operator>=(const Value& lhs, const Value& rhs) { return !operator<(lhs, rhs); }

inline bool operator!=(const Value& lhs, const char* rhs) { return !operator==(lhs, rhs); }
inline bool operator>(const Value& lhs, const char* rhs) { return !operator<(rhs, lhs); }
inline bool operator<=(const Value& lhs, const char* rhs) { return !operator>(lhs, rhs); }
inline bool operator>=(const Value& lhs, const char* rhs) { return !operator<(lhs, rhs); }

inline bool operator!=(const char* lhs, const Value& rhs) { return !operator==(lhs, rhs); }
inline bool operator>(const char* lhs, const Value& rhs) { return !operator<(rhs, lhs); }
inline bool operator<=(const char* lhs, const Value& rhs) { return !operator>(lhs, rhs); }
inline bool operator>=(const char* lhs, const Value& rhs) { return !operator<(lhs, rhs); }

inline bool operator!=(const Value& lhs, const std::string& rhs) { return !operator==(lhs, rhs); }
inline bool operator>(const Value& lhs, const std::string& rhs) { return !operator<(rhs, lhs); }
inline bool operator<=(const Value& lhs, const std::string& rhs) { return !operator>(lhs, rhs); }
inline bool operator>=(const Value& lhs, const std::string& rhs) { return !operator<(lhs, rhs); }

inline bool operator!=(const std::string& lhs, const Value& rhs) { return !operator==(lhs, rhs); }
inline bool operator>(const std::string& lhs, const Value& rhs) { return !operator<(rhs, lhs); }
inline bool operator<=(const std::string& lhs, const Value& rhs) { return !operator>(lhs, rhs); }
inline bool operator>=(const std::string& lhs, const Value& rhs) { return !operator<(lhs, rhs); }

//
// UnsafeConvert implementations for the standard types
// Use Convert, which is a wrapper around UnsafeConvert, rather than using UnsafeConvert directly
//

PRIME_PUBLIC bool UnsafeConvert(bool& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(Value::Integer& output, const Value& input, int base);
PRIME_PUBLIC bool UnsafeConvert(Value::Real& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(Data& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(Date& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(Time& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(UnixTime& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(DateTime& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(Value::Vector& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(Value::Dictionary& output, const Value& input);
PRIME_PUBLIC bool UnsafeConvert(std::vector<std::string>& output, const Value& input,
    StringView separator, unsigned int flags);

template <typename Output>
bool UnsafeConvertToInteger(Output& output, const Value& input, int base)
{
    Value::Integer whole;
    if (UnsafeConvert(whole, input, base)) {
        if (static_cast<Value::Integer>(static_cast<Output>(whole)) == whole) {
            output = static_cast<Output>(whole);
            return true;
        }
    }

    return false;
}

template <typename Output>
bool UnsafeConvertToIntegerArray(ArrayView<Output> array, size_t minCount, const Value& input, size_t* count, int base)
{
    if (input.isVector()) {
        const Value::Vector& vector = input.getVector();
        size_t size = vector.size();
        if (size < minCount || size > array.size()) {
            return false;
        }

        for (size_t i = 0; i != size; ++i) {
            Value::Integer value;
            if (!UnsafeConvertToInteger(value, vector[i], base)) {
                return false;
            }
            array[i] = Output(value);
        }

        if (count) {
            *count = size;
        }

        return true;
    }

    // TODO: UnsafeConvertToInteger the value of the Value, if minCount <= 1

    return UnsafeConvertToIntegerArray(array, minCount, input.c_str(), count, base);
}

template <typename Output>
bool UnsafeConvertToReal(Output& output, const Value& input)
{
    Value::Real real;
    if (!UnsafeConvert(real, input)) {
        return false;
    }

    output = static_cast<Output>(real);

    return true;
}

template <typename Output>
bool UnsafeConvertToRealArray(ArrayView<Output> array, size_t minCount, const Value& input, size_t* count)
{
    if (input.isVector()) {
        const Value::Vector& vector = input.getVector();
        size_t size = vector.size();

        if (size < minCount || size > array.size()) {
            return false;
        }

        for (size_t i = 0; i != size; ++i) {
            Value::Real value;
            if (!UnsafeConvertToReal(value, vector[i])) {
                return false;
            }
            array[i] = Output(value);
        }

        if (count) {
            *count = size;
        }

        return true;
    }

    // TODO: UnsafeConvertToInteger the value of the Value, if minCount <= 1

    return UnsafeConvertToRealArray(array, minCount, input.c_str(), count);
}

//
// UnsafeConvert to Value implementations for the standard types
//

inline bool UnsafeConvert(Value& output, bool value)
{
    output.resetBool() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, int value)
{
    output.resetInteger() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, Value::Integer value)
{
    output.resetInteger() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, Value::Real value)
{
    output.resetReal() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, const std::string& value)
{
    output.resetString() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, const char*& value)
{
    output.resetString() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, StringView value)
{
    output.resetString().assign(value.begin(), value.end());
    return true;
}

inline bool UnsafeConvert(Value& output, const Data& value)
{
    output.resetData() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, const Date& value)
{
    output.resetDate() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, const Time& value)
{
    output.resetTime() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, const UnixTime& value)
{
    output.resetUnixTime() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, const DateTime& value)
{
    output.resetUnixTime() = value.toUnixTime();
    return true;
}

inline bool UnsafeConvert(Value& output, const Value::Vector& value)
{
    output.resetVector() = value;
    return true;
}

inline bool UnsafeConvert(Value& output, const Value::Dictionary& value)
{
    output.resetDictionary() = value;
    return true;
}

PRIME_PUBLIC bool UnsafeConvert(Value& output, const std::vector<std::string>& vector);

#ifdef PRIME_COMPILER_RVALUEREF
PRIME_PUBLIC bool UnsafeConvert(Value& output, std::vector<std::string>&& vector);
#endif

//
// ToValue
//

/// Convert any type T that has a UnsafeConvert(Value&, T) to a Value.
template <typename Input>
inline Value ToValue(const Input& input, Value defaultValue = Value())
{
    Value temp;
    if (!UnsafeConvert(temp, input)) {
        temp.swap(defaultValue);
    }
    return temp;
}

//
// StringAppend for Value, to support MakeString/ToString
//

PRIME_PUBLIC bool StringAppend(std::string& output, const Value& value);
PRIME_PUBLIC bool StringAppend(std::string& output, const Value::Vector& value);
PRIME_PUBLIC bool StringAppend(std::string& output, const Value::Dictionary& value);

//
// Optional ostream support
//

#ifdef PRIME_ENABLE_IOSTREAMS

inline std::ostream& operator<<(std::ostream& os, const Value& value)
{
    os << value.toString();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Value::Vector& value)
{
    os << ToString(value);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Value::Dictionary& value)
{
    os << ToString(value);
    return os;
}

#endif
}

#endif
