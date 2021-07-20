// Copyright 2000-2021 Mark H. P. Lord

#include "Value.h"
#include "NumberUtils.h"
#include "ScopedPtr.h"
#include "StringUtils.h"
#include "TextEncoding.h"
#include <algorithm>
#include <math.h>
#include <memory>

namespace Prime {

//
// Value::ObjectManager
//

PRIME_DEFINE_UID_CAST_BASE(Value::ObjectManager)

Value::ObjectManager::~ObjectManager()
{
}

bool Value::ObjectManager::less(const void* lhs, const Value& rhs) const
{
    return lhs < rhs.getRawObjectPointer();
}

bool Value::ObjectManager::equal(const void* lhs, const Value& rhs) const
{
    return lhs == rhs.getRawObjectPointer();
}

Value Value::ObjectManager::toValue(const void*) const
{
    return null;
}

//
// Value::Object
//

PRIME_DEFINE_UID_CAST_BASE(Value::Object);

bool Value::Object::equal(const Value& other) const
{
    return UIDObjectManager<Object>::get()->equal(reinterpret_cast<const void*>(this), other);
}

bool Value::Object::less(const Value& other) const
{
    return UIDObjectManager<Object>::get()->less(reinterpret_cast<const void*>(this), other);
}

//
// Value
//

const std::string Value::emptyString;
const Date Value::emptyDate;
const Time Value::emptyTime;
const UnixTime Value::emptyUnixTime;
const Data Value::emptyData;
const Value::Vector Value::emptyVector;
const Value::Dictionary Value::emptyDictionary;
const Value Value::undefined(Prime::undefined);
const Value Value::null(Prime::null);

Value::Value(const Vector& assign)
    : _type(TypeVector)
{
    PRIME_COMPILE_TIME_ASSERT(sizeof(_value._vector) >= sizeof(Vector));
    new (rawVector()) Vector(assign);
}

#ifdef PRIME_COMPILER_RVALUEREF
Value::Value(Vector&& assign) PRIME_NOEXCEPT : _type(TypeVector)
{
    new (rawVector()) Vector(std::move(assign));
}
#endif

Value::Value(const Dictionary& assign)
    : _type(TypeDictionary)
{
    PRIME_COMPILE_TIME_ASSERT(sizeof(_value._dictionary) >= sizeof(Dictionary));
    new (rawDictionary()) Dictionary(assign);
}

#ifdef PRIME_COMPILER_RVALUEREF
Value::Value(Dictionary&& assign) PRIME_NOEXCEPT : _type(TypeDictionary)
{
    new (rawDictionary()) Dictionary(std::move(assign));
}
#endif

Value::Value(const std::vector<std::string>& strings)
    : _type(TypeUndefined)
{
    Vector& vector = resetVector();
    vector.reserve(strings.size());
    for (size_t i = 0; i != strings.size(); ++i) {
        vector.push_back(strings[i]);
    }
}

#ifdef PRIME_COMPILER_RVALUEREF
Value::Value(std::vector<std::string>&& strings)
    : _type(TypeUndefined)
{
    Vector& vector = resetVector();
    vector.reserve(strings.size());
    for (size_t i = 0; i != strings.size(); ++i) {
        vector.emplace_back(std::move(strings[i]));
    }
}
#endif

Value::Value(Object* object)
    : _type(TypeObject)
{
    new (rawObjectWrapper()) ObjectWrapper(UIDObjectManager<Object>::get(), object);
}

void Value::destructValueSlow() PRIME_NOEXCEPT
{
    switch (_type) {
    default:
        break;

    case TypeString:
        rawString()->~basic_string();
        break;

    case TypeData:
        rawData()->~Data();
        break;

    case TypeVector:
        rawVector()->~vector();
        break;

    case TypeDictionary:
        rawDictionary()->~Dictionary();
        break;

    case TypeObject:
        rawObjectWrapper()->~ObjectWrapper();
        break;
    }
}

Value::Value(const Value& copy)
    : _type(copy._type)
{
    switch (_type) {
    case TypeUndefined:
    case TypeNull:
        break;

    case TypeBool:
    case TypeInteger:
    case TypeReal:
        _value = copy._value;
        break;

    case TypeString:
        new (rawString()) std::string(*copy.rawString());
        break;

    case TypeData:
        new (rawData()) Data(*copy.rawData());
        break;

    case TypeDate:
        new (rawDate()) Date(*copy.rawDate());
        break;

    case TypeTime:
        new (rawTime()) Time(*copy.rawTime());
        break;

    case TypeDateTime:
        new (rawUnixTime()) UnixTime(*copy.rawUnixTime());
        break;

    case TypeVector:
        new (rawVector()) Vector(*copy.rawVector());
        break;

    case TypeDictionary:
        new (rawDictionary()) Dictionary(*copy.rawDictionary());
        break;

    case TypeObject: {
        new (rawObjectWrapper()) ObjectWrapper(*copy.rawObjectWrapper());
        break;
    }
    }
}

Value& Value::operator=(const Value& assign)
{
    if (this == &assign) {
        return *this;
    }

    destructValue();
    _type = assign._type;

    switch (_type) {
    case TypeUndefined:
    case TypeNull:
        break;

    case TypeBool:
    case TypeInteger:
    case TypeReal:
        _value = assign._value;
        break;

    case TypeString:
        new (rawString()) std::string(*assign.rawString());
        break;

    case TypeData:
        new (rawData()) Data(*assign.rawData());
        break;

    case TypeDate:
        new (rawDate()) Date(*assign.rawDate());
        break;

    case TypeTime:
        new (rawTime()) Time(*assign.rawTime());
        break;

    case TypeDateTime:
        new (rawUnixTime()) UnixTime(*assign.rawUnixTime());
        break;

    case TypeVector:
        new (rawVector()) Vector(*assign.rawVector());
        break;

    case TypeDictionary:
        new (rawDictionary()) Dictionary(*assign.rawDictionary());
        break;

    case TypeObject:
        new (rawObjectWrapper()) ObjectWrapper(*assign.rawObjectWrapper());
        break;
    }

    return *this;
}

void Value::move(Value& rhs)
{
    destructValue();
    constructMove(rhs);
}

void Value::constructMove(Value& move)
{
    _type = move._type;

    switch (_type) {
    case TypeUndefined:
    case TypeNull:
        break;

    case TypeBool:
    case TypeInteger:
    case TypeReal:
        _value = move._value;
        break;

    case TypeString:
        new (rawString()) std::string(PRIME_MOVE(*move.rawString()));
        move.rawString()->~basic_string();
        break;

    case TypeData:
        new (rawData()) Data(PRIME_MOVE(*move.rawData()));
        move.rawData()->~Data();
        break;

    case TypeDate:
        new (rawDate()) Date(PRIME_MOVE(*move.rawDate()));
        move.rawDate()->~Date();
        break;

    case TypeTime:
        new (rawTime()) Time(PRIME_MOVE(*move.rawTime()));
        move.rawTime()->~Time();
        break;

    case TypeDateTime:
        new (rawUnixTime()) UnixTime(PRIME_MOVE(*move.rawUnixTime()));
        move.rawUnixTime()->~UnixTime();
        break;

    case TypeVector:
        new (rawVector()) Vector(PRIME_MOVE(*move.rawVector()));
        move.rawVector()->~vector();
        break;

    case TypeDictionary:
        new (rawDictionary()) Dictionary(PRIME_MOVE(*move.rawDictionary()));
        move.rawDictionary()->~Dictionary();
        break;

    case TypeObject:
        new (rawObjectWrapper()) ObjectWrapper(PRIME_MOVE(*move.rawObjectWrapper()));
        move.rawObjectWrapper()->~ObjectWrapper();
        break;
    }

    move._type = TypeUndefined;
}

void Value::swap(Value& rhs)
{
    Value tmp;
    tmp.move(*this);
    this->move(rhs);
    rhs.move(tmp);
}

bool Value::isEmpty() const PRIME_NOEXCEPT
{
    switch (_type) {
    case TypeUndefined:
    case TypeNull:
        return true;

    case TypeString:
        return rawString()->empty();

    case TypeVector:
        return rawVector()->empty();

    case TypeDictionary:
        return rawDictionary()->empty();

    default:
        return false;
    }
}

const Value& Value::getVectorIndex(const Value& key) const
{
    if (_type == TypeVector) {
        Integer index = key.toInteger(-1);
        if (index >= 0 && index < (Integer)rawVector()->size()) {
            return (*rawVector())[(size_t)index];
        }
    }

    return undefined;
}

bool Value::getVectorIndex(const Value& key, Value& output) const
{
    if (_type == TypeVector) {
        Integer index = key.toInteger(-1);
        if (index >= 0 && index < (Integer)rawVector()->size()) {
            output = (*rawVector())[(size_t)index];
            return true;
        }
    }

    return false;
}

int Value::toInt(int defaultValue) const
{
    int result;
    if (!UnsafeConvertToInteger(result, *this, -1)) {
        result = defaultValue;
    }
    return result;
}

unsigned int Value::toUInt(unsigned int defaultValue) const
{
    unsigned int result;
    if (!UnsafeConvertToInteger(result, *this, -1)) {
        result = defaultValue;
    }
    return result;
}

int64_t Value::toInt64(int64_t defaultValue) const
{
    int64_t result;
    if (!UnsafeConvertToInteger(result, *this, -1)) {
        result = defaultValue;
    }
    return result;
}

long Value::toLong(long defaultValue) const
{
    long result;
    if (!UnsafeConvertToInteger(result, *this, -1)) {
        result = defaultValue;
    }
    return result;
}

float Value::toFloat(float defaultValue) const
{
    Real real = toReal(defaultValue);
    if (real <= -std::numeric_limits<float>::max() || real >= std::numeric_limits<float>::max()) {
        return defaultValue;
    }
    return static_cast<float>(real);
}

double Value::toDouble(double defaultValue) const
{
    Real real = toReal(defaultValue);
    if (real <= -std::numeric_limits<double>::max() || real >= std::numeric_limits<double>::max()) {
        return defaultValue;
    }
    return static_cast<double>(real);
}

Value::Vector Value::toVector(const Vector& valueIfUndefined) const
{
    Vector converted;
    if (!UnsafeConvert(converted, *this)) {
        converted = valueIfUndefined;
    }
    return converted;
}

Value::Dictionary Value::toDictionary(const Dictionary& valueIfUndefined) const
{
    Dictionary converted;
    if (!UnsafeConvert(converted, *this)) {
        converted = valueIfUndefined;
    }
    return converted;
}

Value Value::toValue() const
{
    if (_type == TypeObject) {
        const ObjectWrapper* objectWrapper = rawObjectWrapper();
        return objectWrapper->toValue();
    }

    return *this;
}

bool Value::equal(const Value& lhs, const Value& rhs)
{
    // We can't use equal(result, lhs, rhs) because it will fail if an Object can't be compared. This
    // version never fails, which is important for using Values as keys.

    Type maxType = Max(lhs._type, rhs._type);

    switch (maxType) {
    case TypeUndefined:
        return lhs._type == rhs._type;

    case TypeNull:
        return lhs._type == rhs._type;

    case TypeBool:
        return lhs.toBool() == rhs.toBool();

    case TypeInteger:
        return lhs.toInteger() == rhs.toInteger();

    case TypeReal:
        return lhs.toReal() == rhs.toReal();

    case TypeString:
        if (lhs._type == rhs._type) {
            return *lhs.rawString() == *rhs.rawString();
        }

        return lhs.toString() == rhs.toString();

    case TypeData:
        if (lhs._type == rhs._type) {
            return *lhs.rawData() == *rhs.rawData();
        }

        return lhs.toData() == rhs.toData();

    case TypeDate:
        if (lhs._type == rhs._type) {
            return *lhs.rawDate() == *rhs.rawDate();
        }

        return lhs.toDate() == rhs.toDate();

    case TypeTime:
        if (lhs._type == rhs._type) {
            return *lhs.rawTime() == *rhs.rawTime();
        }

        return lhs.toTime() == rhs.toTime();

    case TypeDateTime:
        if (lhs._type == rhs._type) {
            return *lhs.rawUnixTime() == *rhs.rawUnixTime();
        }

        return lhs.toUnixTime() == rhs.toUnixTime();

    case TypeVector:
        if (lhs._type == rhs._type) {
            return *lhs.rawVector() == *rhs.rawVector();
        }

        return lhs.toVector() == rhs.toVector();

    case TypeDictionary:
        if (lhs._type == rhs._type) {
            return *lhs.rawDictionary() == *rhs.rawDictionary();
        }

        return lhs.toDictionary() == rhs.toDictionary();

    case TypeObject:
        if (lhs.isObject()) {
            return lhs.rawObjectWrapper()->equal(rhs);
        }

        return rhs.rawObjectWrapper()->equal(lhs);
    }

    return false;
}

bool Value::less(const Value& lhs, const Value& rhs)
{
    // We can't use less(result, lhs, rhs) because it will fail if an Object can't be compared. This
    // version never fails, which is important for using Values as keys.

    Type maxType = Max(lhs._type, rhs._type);

    switch (maxType) {
    case TypeUndefined:
        return lhs._type < rhs._type;

    case TypeNull:
        return lhs._type < rhs._type;

    case TypeBool:
        return (lhs.toBool() ? 1 : 0) < (rhs.toBool() ? 1 : 0);

    case TypeInteger:
        return lhs.toInteger() < rhs.toInteger();

    case TypeReal:
        return lhs.toReal() < rhs.toReal();

    case TypeString:
        if (lhs._type == rhs._type) {
            return *lhs.rawString() < *rhs.rawString();
        }

        return lhs.toString() < rhs.toString();

    case TypeData:
        if (lhs._type == rhs._type) {
            return *lhs.rawData() < *rhs.rawData();
        }

        return lhs.toData() < rhs.toData();

    case TypeDate:
        if (lhs._type == rhs._type) {
            return *lhs.rawDate() < *rhs.rawDate();
        }

        return lhs.toDate() < rhs.toDate();

    case TypeTime:
        if (lhs._type == rhs._type) {
            return *lhs.rawTime() < *rhs.rawTime();
        }

        return lhs.toTime() < rhs.toTime();

    case TypeDateTime:
        if (lhs._type == rhs._type) {
            return *lhs.rawUnixTime() < *rhs.rawUnixTime();
        }

        return lhs.toUnixTime() < rhs.toUnixTime();

    case TypeVector:
        if (lhs._type == rhs._type) {
            return *lhs.rawVector() < *rhs.rawVector();
        }

        return lhs.toVector() < rhs.toVector();

    case TypeDictionary:
        if (lhs._type == rhs._type) {
            return *lhs.rawDictionary() < *rhs.rawDictionary();
        }

        return lhs.toDictionary() < rhs.toDictionary();

    case TypeObject:
        if (lhs.isObject()) {
            return lhs.rawObjectWrapper()->less(rhs);
        }
        return !(rhs.rawObjectWrapper()->equal(lhs) || rhs.rawObjectWrapper()->less(lhs));
    }

    return false;
}

bool& Value::convertToBool()
{
    Value temp;
    if (!convert(TypeBool, temp)) {
        resetBool();
    } else {
        swap(temp);
    }
    return _value.boolean;
}

Value::Integer& Value::convertToInteger()
{
    Value temp;
    if (!convert(TypeInteger, temp)) {
        resetInteger();
    } else {
        swap(temp);
    }
    return _value.integer;
}

Value::Real& Value::convertToReal()
{
    Value temp;
    if (!convert(TypeReal, temp)) {
        resetReal();
    } else {
        swap(temp);
    }
    return _value.real;
}

std::string& Value::convertToString()
{
    Value temp;
    if (!convert(TypeString, temp)) {
        resetString();
    } else {
        swap(temp);
    }
    return *rawString();
}

Data& Value::convertToData()
{
    Value temp;
    if (!convert(TypeData, temp)) {
        resetData();
    } else {
        swap(temp);
    }
    return *rawData();
}

Date& Value::convertToDate()
{
    Value temp;
    if (!convert(TypeDate, temp)) {
        resetDate();
    } else {
        swap(temp);
    }
    return *rawDate();
}

Time& Value::convertToTime()
{
    Value temp;
    if (!convert(TypeTime, temp)) {
        resetTime();
    } else {
        swap(temp);
    }
    return *rawTime();
}

UnixTime& Value::convertToUnixTime()
{
    Value temp;
    if (!convert(TypeDateTime, temp)) {
        resetUnixTime();
    } else {
        swap(temp);
    }
    return *rawUnixTime();
}

Value::Vector& Value::convertToVector()
{
    Value temp;
    if (!convert(TypeVector, temp)) {
        resetVector();
    } else {
        swap(temp);
    }
    return *rawVector();
}

Value::Dictionary& Value::convertToDictionary()
{
    Value temp;
    if (!convert(TypeDictionary, temp)) {
        resetDictionary();
    } else {
        swap(temp);
    }
    return *rawDictionary();
}

bool Value::convert(Type type, Value& result) const
{
    switch (type) {
    case TypeUndefined:
    case TypeNull:
    case TypeDictionary: // Could convert a vector (alternate key/value)?
        return false;

    case TypeBool: {
        bool converted;
        if (UnsafeConvert(converted, *this)) {
            result = converted;
            return true;
        }
        return false;
    }

    case TypeInteger: {
        Integer converted;
        if (UnsafeConvertToInteger(converted, *this, -1)) {
            result = converted;
            return true;
        }
        return false;
    }

    case TypeReal: {
        Real converted;
        if (UnsafeConvert(converted, *this)) {
            result = converted;
            return true;
        }
        return false;
    }

    case TypeDate: {
        Date converted;
        if (UnsafeConvert(converted, *this)) {
            result = converted;
            return true;
        }
        return false;
    }

    case TypeTime: {
        Time converted;
        if (UnsafeConvert(converted, *this)) {
            result = converted;
            return true;
        }
        return false;
    }

    case TypeDateTime: {
        UnixTime converted;
        if (UnsafeConvert(converted, *this)) {
            result = converted;
            return true;
        }
        return false;
    }

    case TypeString: {
        StringAppend(result.resetString(), *this);
        return false;
    }

    case TypeData: {
        Data converted;
        if (UnsafeConvert(converted, *this)) {
            result.resetData().swap(converted);
            return true;
        }
        return false;
    }

    case TypeVector: {
        Vector converted;
        if (UnsafeConvert(converted, *this)) {
            result.resetVector().swap(converted);
            return true;
        }
        return false;
    }

    case TypeObject:
        return toValue().convert(type, result);
    }

    return false;
}

Value::Dictionary Value::dictionaryDiff(const Dictionary& old, const Dictionary& now, const std::string& missingKeysKey)
{
    Dictionary diffs;
    size_t i;

    // Check for keys removed from old or with changed values between old and now

    for (i = 0; i != old.size(); ++i) {
        const Value::Dictionary::value_type& oldPair = old.pair(i);
        Value nowValue = now.get(oldPair.first);

        // TODO: diffing vectors? The patch would be a dictionary with the missinsKeysKey used to store the new size?

        if (nowValue.isDictionary() && oldPair.second.isDictionary()) {
            Dictionary subDiffs = dictionaryDiff(*oldPair.second.rawDictionary(), *nowValue.rawDictionary(), missingKeysKey);
            if (!subDiffs.empty()) {
                diffs.set(oldPair.first, subDiffs);
            }
        } else if (nowValue.isUndefined()) {
            if (missingKeysKey.empty()) {
                diffs.set(oldPair.first, undefined);
            } else {
                Vector vector = diffs.get(missingKeysKey).toVector();
                vector.push_back(oldPair.first);
                diffs.set(missingKeysKey, vector);
            }
        } else if (nowValue._type != oldPair.second._type || nowValue != oldPair.second) {
            diffs.set(oldPair.first, nowValue);
        }
    }

    // Now check for keys in now that are not in old

    for (i = 0; i != now.size(); ++i) {
        const Value::Dictionary::value_type& nowPair = now.pair(i);

        if (!old.has(nowPair.first)) {
            diffs.set(nowPair.first, nowPair.second);
        }
    }

    return Dictionary(diffs);
}

Value::Dictionary Value::dictionaryPatch(const Dictionary& old, const Dictionary& diff, const std::string& missingKeysKey)
{
    Value::Dictionary patch(old);

    for (size_t i = 0; i != diff.size(); ++i) {
        const Value::Dictionary::value_type& diffPair = diff.pair(i);

        if (!missingKeysKey.empty() && diffPair.first == missingKeysKey) {
            continue;
        }

        const Value oldValue = old.get(diffPair.first);

        if (oldValue.isDictionary() && diffPair.second.isDictionary()) {
            patch.set(diffPair.first, dictionaryPatch(*oldValue.rawDictionary(), *diffPair.second.rawDictionary(), missingKeysKey));
        } else if (diffPair.second.isUndefined() && missingKeysKey.empty()) {
            patch.erase(diffPair.first);
        } else {
            patch.set(diffPair.first, diffPair.second);
        }
    }

    return Dictionary(patch);
}

bool Value::merge(Dictionary& target, const Dictionary& source, MergeMode mode)
{
    bool changed = false;

    for (size_t i = 0; i != source.size(); ++i) {
        const Value::Dictionary::value_type& mergePair = source.pair(i);

        Value targetValue = target.get(mergePair.first);

        if (merge(targetValue, mergePair.second, mode)) {
            changed = true;
            target.set(mergePair.first, targetValue);
        }
    }

    // If we want the result to contain only the equal values from both dictionaries, we need to check for extra
    // keys in the target.
    if (mode == MergeModeEqual) {
        std::vector<std::string> missingKeys;
        size_t i;

        for (i = 0; i != target.size(); ++i) {
            const Value::Dictionary::value_type& targetPair = target.pair(i);

            if (!source.has(targetPair.first)) {
                missingKeys.push_back(targetPair.first);
            }
        }

        for (i = 0; i != missingKeys.size(); ++i) {
            target.set(missingKeys[i], null);
            changed = true;
        }
    }

    return changed;
}

bool Value::merge(Value& target, const Value& source, MergeMode mode)
{
    if (target._type == source._type && target == source) {
        return false;
    }

    if (mode == MergeModeMissing) {
        if (target.isUndefined()) {
            target = source;
            return true;
        }
    }

    if (target.isDictionary() && source.isDictionary()) {
        merge(target.accessDictionary(), source.getDictionary(), mode);
        return true;
    }

    if (mode == MergeModeVector) {
        if (!target.isUndefined()) {
            if (target.isVector()) {
                target.accessVector().push_back(source);
                return true;
            }
            target = Vector(1, target);
            target.accessVector().push_back(source);
            return true;
        }

        target = source;
        return true;
    }

    if (mode == MergeModeMissing) {
        return false;
    }

    if (mode == MergeModeOverwrite) {
        target = source;
        return true;
    }

    PRIME_ASSERT(mode == MergeModeEqual);

    target = null;
    return true;
}

bool& Value::resetBool()
{
    destructValue();
    _type = TypeBool;
    return _value.boolean;
}

Value::Integer& Value::resetInteger()
{
    destructValue();
    _type = TypeInteger;
    return _value.integer;
}

Value::Real& Value::resetReal()
{
    destructValue();
    _type = TypeReal;
    return _value.real;
}

std::string& Value::resetString()
{
    destructValue();
    _type = TypeString;
    new (rawString()) std::string;
    return *rawString();
}

Data& Value::resetData()
{
    destructValue();
    _type = TypeData;
    new (rawData()) Data;
    return *rawData();
}

Date& Value::resetDate()
{
    destructValue();
    _type = TypeDate;
    new (rawDate()) Date;
    return *rawDate();
}

Time& Value::resetTime()
{
    destructValue();
    _type = TypeTime;
    new (rawTime()) Time;
    return *rawTime();
}

UnixTime& Value::resetUnixTime()
{
    destructValue();
    _type = TypeDateTime;
    new (rawUnixTime()) UnixTime;
    return *rawUnixTime();
}

Value::Vector& Value::resetVector()
{
    destructValue();
    _type = TypeVector;
    new (rawVector()) Vector;
    return *rawVector();
}

Value::Dictionary& Value::resetDictionary()
{
    destructValue();
    _type = TypeDictionary;
    new (rawDictionary()) Dictionary;
    return *rawDictionary();
}

void Value::setDictionaryPath(Dictionary& rootDictionary, const char* path, Value value)
{
    Dictionary* dictionary = &rootDictionary;
    for (;;) {
        const char* ptr = strchr(path, '.');

        char buffer[128];
        if (!ptr) {
            StringCopy(buffer, sizeof(buffer), path);
        } else {
            StringCopy(buffer, sizeof(buffer), path, ptr - path);
            path = ptr + 1;
        }

        bool more = ptr != NULL;

        if (more) {
            dictionary = &dictionary->access(buffer).accessDictionary();
        } else {
            dictionary->access(buffer).move(value);
            break;
        }
    }
}

const Value& Value::operator[](StringView string) const
{
    return getDictionary()[string];
}

const Value& Value::operator[](const char* string) const
{
    return getDictionary()[string];
}

const Value& Value::operator[](const std::string& string) const
{
    return getDictionary()[string];
}

const Value& Value::operator[](size_t key) const
{
    // So, value[0] will return *this if we're not a Value, emulating value.toVector()[key]
    return (_type == TypeVector) ? (*rawVector())[key]
                                 : (key == 0) ? *this
                                              : undefined;
}

//
// Conversions
//

bool UnsafeConvert(bool& out, const Value& value)
{
    switch (value.getType()) {
    case Value::TypeUndefined:
        break;

    case Value::TypeNull:
        out = false;
        return true;

    case Value::TypeBool:
        out = value.getBool();
        return true;

    case Value::TypeInteger:
        out = value.getInteger() != 0;
        return true;

    case Value::TypeReal:
        out = fabs(static_cast<double>(value.getReal())) > 0.001;
        return true;

    case Value::TypeString:
        return UnsafeConvert(out, value.getString());

    case Value::TypeData:
        out = !value.getData().empty();
        return true;

    case Value::TypeDate:
        out = value.getDate() != Date();
        return true;

    case Value::TypeTime:
        out = value.getTime() != Time();
        return true;

    case Value::TypeDateTime:
        out = value.getUnixTime() != UnixTime();
        return true;

    case Value::TypeVector:
        out = !value.getVector().empty();
        return true;

    case Value::TypeDictionary:
        out = !value.getDictionary().empty();
        return true;

    case Value::TypeObject:
        return UnsafeConvert(out, value.toValue());
    }

    return false;
}

bool UnsafeConvert(Data& out, const Value& value)
{
    if (value.isData()) {
        out = value.getData();
        return true;
    }

    if (value.getType() == Value::TypeObject) {
        return UnsafeConvert(out, value.toValue());
    }

    if (!value.isString()) {
        return false;
    }

    return UnsafeConvert(out, value.getString());
}

bool UnsafeConvert(Date& out, const Value& value)
{
    switch (value.getType()) {
    case Value::TypeDate:
        out = value.getDate();
        return true;

    case Value::TypeString:
        return UnsafeConvert(out, value.getString());

    case Value::TypeDateTime:
        return UnsafeConvert(out, value.getUnixTime());

    case Value::TypeObject:
        return UnsafeConvert(out, value.toValue());

    default:
        break;
    }

    return false;
}

bool UnsafeConvert(Time& out, const Value& value)
{
    switch (value.getType()) {
    case Value::TypeTime:
        out = value.getTime();
        return true;

    case Value::TypeString:
        return UnsafeConvert(out, value.getString());

    case Value::TypeDateTime:
        return UnsafeConvert(out, value.getUnixTime());

    case Value::TypeObject:
        return UnsafeConvert(out, value.toValue());

    default:
        break;
    }

    return false;
}

bool UnsafeConvert(UnixTime& out, const Value& value)
{
    switch (value.getType()) {
    case Value::TypeDateTime:
        out = value.getUnixTime();
        return true;

    case Value::TypeString:
        return UnsafeConvert(out, value.getString());

    case Value::TypeInteger:
        out = UnixTime(value.getInteger(), 0);
        return true;

    case Value::TypeReal:
        out = UnixTime(static_cast<double>(value.getReal()));
        return true;

    case Value::TypeObject:
        return UnsafeConvert(out, value.toValue());

    default:
        break;
    }

    return false;
}

bool UnsafeConvert(DateTime& out, const Value& value)
{
    UnixTime unixTime;
    if (!UnsafeConvert(unixTime, value)) {
        return false;
    }

    out = DateTime(unixTime);
    return true;
}

bool UnsafeConvert(Value::Vector& out, const Value& value)
{
    if (value.getType() == Value::TypeVector) {
        out = value.getVector();
        return true;
    }

    if (value.getType() == Value::TypeUndefined) {
        return false;
    }

    if (value.getType() == Value::TypeObject) {
        return UnsafeConvert(out, value.toValue());
    }

    out = Value::Vector(1, value);
    return true;
}

bool UnsafeConvert(Value::Dictionary& out, const Value& value)
{
    if (value.getType() == Value::TypeDictionary) {
        out = value.getDictionary();
        return true;
    }

    if (value.getType() == Value::TypeObject) {
        return UnsafeConvert(out, value.toValue());
    }

    return false;
}

bool UnsafeConvert(Value::Integer& out, const Value& value, int base)
{
    switch (value.getType()) {
    case Value::TypeUndefined:
    case Value::TypeData:
    case Value::TypeVector:
    case Value::TypeDictionary:
        break;

    case Value::TypeNull:
        out = 0;
        return true;

    case Value::TypeBool:
        out = value.getBool() ? 1 : 0;
        return true;

    case Value::TypeInteger:
        out = value.getInteger();
        return true;

    case Value::TypeReal:
        out = Value::Integer(value.getReal());
        return true;

    case Value::TypeDate:
    case Value::TypeTime:
        break;

    case Value::TypeDateTime:
        out = value.getUnixTime().getSeconds();
        return true;

    case Value::TypeString: {
        Value::Integer n;
        if (StringToInt<Value::Integer>(value.getString(), n, base)) {
            out = n;
            return true;
        }

        return false;
    }

    case Value::TypeObject:
        return UnsafeConvert(out, value.toValue(), base);
    }

    return false;
}

bool UnsafeConvert(Value::Real& out, const Value& value)
{
    switch (value.getType()) {
    case Value::TypeUndefined:
    case Value::TypeData:
    case Value::TypeVector:
    case Value::TypeDictionary:
        break;

    case Value::TypeNull:
        out = 0;
        return true;

    case Value::TypeBool:
        out = value.getBool() ? 1.0 : 0.0;
        return true;

    case Value::TypeInteger:
        out = Value::Real(value.getInteger());
        return true;

    case Value::TypeReal:
        out = value.getReal();
        // TODO: check for NaN and +-INF?
        return true;

    case Value::TypeDate:
    case Value::TypeTime:
        break;

    case Value::TypeDateTime:
        out = value.getUnixTime().toDouble();
        return true;

    case Value::TypeString: {
        Value::Real n;
        if (StringToReal<Value::Real>(value.getString(), n)) {
            out = n;
            return true;
        }

        return false;
    }

    case Value::TypeObject:
        return UnsafeConvert(out, value.toValue());
    }

    return false;
}

bool UnsafeConvert(std::vector<std::string>& output, const Value& input, StringView separator, unsigned int flags)
{
    if (input.isUndefined()) {
        return false;
    }

    if (input.isString() && !separator.empty()) {
        return UnsafeConvert(output, input.getString(), separator, flags);
    }

    if (input.isVector()) {
        const Value::Vector& vector = input.getVector();

        output.resize(vector.size());
        for (size_t i = 0; i != vector.size(); ++i) {
            output[i] = ToString(vector[i]);
        }

        return true;
    }

    Value::Vector converted;
    if (!UnsafeConvert(converted, input)) {
        return false;
    }

    output.resize(converted.size());
    for (size_t i = 0; i != converted.size(); ++i) {
        output[i] = converted[i].toString();
    }

    return true;
}

bool UnsafeConvert(Value& output, const std::vector<std::string>& vector)
{
    Value::Vector& vvector = output.resetVector();
    vvector.resize(vector.size());

    for (size_t i = 0; i != vector.size(); ++i) {
        vvector[i].resetString() = vector[i];
    }

    return true;
}

#ifdef PRIME_COMPILER_RVALUEREF
bool UnsafeConvert(Value& output, std::vector<std::string>&& vector)
{
    Value::Vector& vvector = output.resetVector();
    vvector.resize(vector.size());

    for (size_t i = 0; i != vector.size(); ++i) {
        vvector[i].resetString() = std::move(vector[i]);
    }

    return true;
}
#endif

bool StringAppend(std::string& output, const Value& value)
{
    switch (value.getType()) {
    case Value::TypeUndefined:
        return false;

    case Value::TypeNull:
        output += "null";
        break;

    case Value::TypeBool:
        StringAppend(output, value.getBool());
        break;

    case Value::TypeInteger:
        StringAppend(output, value.getInteger());
        break;

    case Value::TypeReal:
        StringAppend(output, value.getReal());
        break;

    case Value::TypeString:
        StringAppend(output, value.getString());
        break;

    case Value::TypeData:
        StringAppend(output, value.getData());
        break;

    case Value::TypeDate:
        StringAppend(output, value.getDate());
        break;

    case Value::TypeTime:
        StringAppend(output, value.getTime());
        break;

    case Value::TypeDateTime:
        StringAppend(output, value.getUnixTime());
        break;

    case Value::TypeVector:
        StringAppend(output, value.getVector());
        break;

    case Value::TypeDictionary:
        StringAppend(output, value.getDictionary());
        break;

    case Value::TypeObject:
        StringAppend(output, value.toValue());
        break;
    }

    return true;
}

bool StringAppend(std::string& output, const Value::Vector& vector)
{
    bool result = true;

    for (size_t i = 0; i != vector.size(); ++i) {
        if (i != 0) {
            output += ", ";
        }

        size_t sizeWas = output.size();
        result = StringAppend(output, vector[i]) && result;
        QuoteIfNecessary(output, sizeWas);
    }

    return result;
}

bool StringAppend(std::string& output, const Value::Dictionary& dictionary)
{
    bool result = true;

    for (size_t i = 0; i != dictionary.size(); ++i) {
        if (i != 0) {
            output += ", ";
        }

        const Value::Dictionary::value_type& pair = dictionary.pair(i);

        size_t sizeWas = output.size();
        output += pair.first;
        QuoteIfNecessary(output, sizeWas);

        output += ": ";

        sizeWas = output.size();
        result = StringAppend(output, pair.second) && result;
        QuoteIfNecessary(output, sizeWas);
    }

    return result;
}
}
