// Copyright 2000-2021 Mark H. P. Lord

#include "BinaryPropertyListWriter.h"
#include "ScopedPtr.h"
#include "StreamBuffer.h"
#include "TextEncoding.h"

namespace Prime {

namespace {
    // 1 byte token + up to uint64_t.
    const size_t maxEncodedSizeOfIntegerWithToken = 9;

    // 1 byte token + possibly 1 byte int size token + up to uint64_t.
    const size_t maxEncodedSizeOfLengthWithToken = 10;

    const size_t bplistHeaderSize = 8;
    const size_t bplistFooterSize = 6 + 2 + sizeof(uint64_t) * 3;
}

BinaryPropertyListWriter::BinaryPropertyListWriter()
{
}

BinaryPropertyListWriter::~BinaryPropertyListWriter()
{
    freeDatas();
}

void BinaryPropertyListWriter::freeDatas()
{
    for (size_t i = 0; i != _tempValues.size(); ++i) {
        delete _tempValues[i];
    }

    _tempValues.clear();
}

bool BinaryPropertyListWriter::write(Stream* stream, Log* log, const Value& value, const Options& options,
    size_t bufferSize, void* buffer)
{
    freeDatas();

    _options = options;

    StreamBuffer tempStreamBuffer(stream, bufferSize, buffer);
    _streamBuffer = &tempStreamBuffer;

    _log = log;

    _nextIndex = 0;
    _error = false;
    uint64_t fileSize = bplistHeaderSize;
    uint64_t referencesCount = 0;

    // Visit every object. Build a list of unique objects and get a count.
    uint64_t rootIndex = visitValue(&value);

    if (_error) {
        return false;
    }

    std::vector<ObjectOffset> sortedObjects;
    sortedObjects.reserve(_objects.size());
    for (std::set<Object>::iterator iter = _objects.begin(); iter != _objects.end(); ++iter) {
        ObjectOffset oo;
        oo.object = &(*iter);
        fileSize += oo.object->encodedSize;
        sortedObjects.push_back(oo);

        // Vectors and dictionaries are encoded temporarily as lists of uint64_t object indexes stored as Data.
        if (oo.object->type == Value::TypeVector || oo.object->type == Value::TypeDictionary) {
            referencesCount += oo.object->value.value->getData().size() / sizeof(uint64_t);
        }
    }

    std::sort(sortedObjects.begin(), sortedObjects.end(), ObjectOffset::lessByIndex);

    // Now we know how many objects we have, we know how big our references (file offsets) will be.
    _referenceSize = getRequiredSizeOfIntegerInBytes(sortedObjects.size());

    // Add the size of all the references to the total.
    fileSize += _referenceSize * referencesCount;

    int offsetTableEntrySize = getRequiredSizeOfIntegerInBytes(fileSize);

    // Write the header.

    if (!_streamBuffer->writeExact("bplist00", bplistHeaderSize, log)) {
        return false;
    }

    uint64_t offset = bplistHeaderSize;

#ifdef PRIME_DEBUG
    uint64_t actualReferencesCount = 0;
#endif

    std::vector<ObjectOffset>::iterator oo;

    for (oo = sortedObjects.begin(); oo != sortedObjects.end(); ++oo) {
        oo->offset = offset;
        uint64_t encodedSize;
        if (!writeObject(*oo->object, encodedSize)) {
            return false;
        }

#ifdef PRIME_DEBUG
        if (oo->object->type == Value::TypeVector || oo->object->type == Value::TypeDictionary) {
            size_t count = oo->object->value.value->getData().size() / sizeof(uint64_t);
            actualReferencesCount += count;
            uint64_t ooEncodedSize = oo->object->encodedSize;
            uint64_t estimatedSize = count * _referenceSize + ooEncodedSize;
            if (!PRIME_GUARD(encodedSize == estimatedSize)) {
                writeObject(*oo->object, encodedSize);
            }
        } else {
            PRIME_ASSERT(encodedSize == oo->object->encodedSize);
        }
#endif

        offset += encodedSize;
        PRIME_DEBUG_ASSERT(_streamBuffer->getOffset(_log) == (Stream::Offset)offset);
    }

#ifdef PRIME_DEBUG
    PRIME_ASSERT(actualReferencesCount == referencesCount);
#endif

    PRIME_ASSERT(offset == fileSize);

    for (oo = sortedObjects.begin(); oo != sortedObjects.end(); ++oo) {
        uint8_t encoded[maxEncodedSizeOfIntegerWithToken];
        encodeSizedUnsignedInteger(oo->offset, offsetTableEntrySize, encoded);
        if (!_streamBuffer->writeBytes(encoded, offsetTableEntrySize, _log)) {
            return false;
        }
    }

    uint8_t footer[bplistFooterSize];

    memset(footer, 0, 6); // Reserved bytes.
    footer[6] = (uint8_t)offsetTableEntrySize;
    footer[7] = (uint8_t)_referenceSize;
    encodeSizedUnsignedInteger(sortedObjects.size(), 8, footer + 8);
    encodeSizedUnsignedInteger(rootIndex, 8, footer + 16); // index of root object.
    encodeSizedUnsignedInteger((uint64_t)offset, 8, footer + 24);

    if (!_streamBuffer->writeBytes(footer, bplistFooterSize, log)) {
        return false;
    }

    freeDatas();

    return _streamBuffer->flushWrites(_log);
}

uint64_t BinaryPropertyListWriter::visitValue(const Value* value)
{
    switch (value->getType()) {
    case Value::TypeVector:
        return visitArray(&value->getVector());

    case Value::TypeDictionary:
        return visitDictionary(&value->getDictionary());

    case Value::TypeString:
        return visitString(&value->getString());

    case Value::TypeObject: {
        Value serialised = value->toValue();
        if (serialised.isUndefined() || serialised.isObject()) {
            _log->error(PRIME_LOCALISE("Unserialisable object cannot be written to binary property list."));
            _error = true;
            return 0;
        }

        _tempValues.push_back(new Value);
        *_tempValues.back() = PRIME_MOVE(serialised);

        return visitValue(_tempValues.back());
    }

    default:
        return visitPrimitive(value);
    }
}

uint64_t BinaryPropertyListWriter::visitString(const std::string* string)
{
    Object object;
    object.value.string = string;
    object.type = Value::TypeString;
    object.encodedSize = estimateStringSize(*string);

    return insertObject(PRIME_MOVE(object));
}

uint64_t BinaryPropertyListWriter::visitArray(const Value::Vector* array)
{
    Data data(array->size() * sizeof(uint64_t));

    if (!data.empty()) {
        uint64_t* indexPtr = (uint64_t*)&data[0];
        for (size_t i = 0; i != array->size(); ++i, ++indexPtr) {
            *indexPtr = visitValue(&(*array)[i]);
        }
    }

    ScopedPtr<Value> value(new Value(PRIME_MOVE(data)));
    _tempValues.push_back(value.get());

    Object object;
    object.value.value = value.detach();
    object.type = Value::TypeVector;

    uint8_t buffer[maxEncodedSizeOfLengthWithToken];
    int len = encodeLength(array->size(), buffer); // don't really need to encode when visiting
    object.encodedSize = len;

    return insertObject(PRIME_MOVE(object));
}

uint64_t BinaryPropertyListWriter::visitDictionary(const Value::Dictionary* dict)
{
    Data data(dict->size() * sizeof(uint64_t) * 2);

    if (!data.empty()) {
        uint64_t* keyIndexPtr = (uint64_t*)&data[0];
        uint64_t* valueIndexPtr = keyIndexPtr + dict->size();
        for (size_t i = 0; i != dict->size(); ++i, ++keyIndexPtr, ++valueIndexPtr) {
            const Value::Dictionary::value_type* pair = &dict->pair(i);
            *keyIndexPtr = visitString(&pair->first);
            *valueIndexPtr = visitValue(&pair->second);
        }
    }

    ScopedPtr<Value> value(new Value(PRIME_MOVE(data)));
    _tempValues.push_back(value.get());

    Object object;
    object.value.value = value.detach();
    object.type = Value::TypeDictionary;

    uint8_t buffer[maxEncodedSizeOfLengthWithToken];
    int len = encodeLength(dict->size(), buffer); // don't really need to encode when visiting
    object.encodedSize = len;

    return insertObject(PRIME_MOVE(object));
}

void BinaryPropertyListWriter::visitUnixTime(Object& object, const UnixTime& unixTime)
{
    uint8_t buffer[maxEncodedSizeOfIntegerWithToken];
    static const Float64 secondsBetween1970And2001 = 978307200.0;
    int len = encodeDouble(unixTime.toDouble() - secondsBetween1970And2001, buffer);
    buffer[0] |= 0x30;
    object.encodedSize = len;
}

uint64_t BinaryPropertyListWriter::visitPrimitive(const Value* value)
{
    Object object;
    object.value.value = value;
    object.type = value->getType();

    switch (value->getType()) {
    case Value::TypeNull:
        object.encodedSize = 1;
        break;

    case Value::TypeBool:
        object.encodedSize = 1;
        break;

    case Value::TypeInteger:
        object.encodedSize = getRequiredSizeOfIntegerInBytes(value->getInteger()) + 1;
        break;

    case Value::TypeReal:
        object.encodedSize = getRequiredSizeOfFloatOrDoubleInBytes((Float64)value->getReal()) + 1;
        break;

    case Value::TypeData: {
        uint8_t buffer[maxEncodedSizeOfLengthWithToken];
        int len = encodeLength(value->getData().size(), buffer);
        object.encodedSize = len + value->getData().size();
        break;
    }

    case Value::TypeDate: {
        visitUnixTime(object, DateTime(object.value.value->getDate(), Time()).toUnixTime());
        break;
    }

    case Value::TypeTime: {
        visitUnixTime(object, DateTime(Date(2001, 0, 0), object.value.value->getTime()).toUnixTime());
        break;
    }

    case Value::TypeDateTime: {
        visitUnixTime(object, object.value.value->getUnixTime());
        break;
    }

    case Value::TypeUndefined:
    case Value::TypeVector:
    case Value::TypeDictionary:
    case Value::TypeObject:
    case Value::TypeString:
        PRIME_ASSERT(0);
        break;
    }

    return insertObject(PRIME_MOVE(object));
}

#ifdef PRIME_COMPILER_RVALUEREF
uint64_t BinaryPropertyListWriter::insertObject(Object&& object)
#else
uint64_t BinaryPropertyListWriter::insertObject(Object& object)
#endif
{
    std::set<Object>::iterator iter = _objects.lower_bound(object);
    if (iter != _objects.end() && *iter == object) {
        return iter->index;
    }

    uint64_t index = _nextIndex;
    object.index = _nextIndex;
    ++_nextIndex;
#if defined(PRIME_COMPILER_RVALUEREF) && !PRIME_GCC_AND_OLDER(4, 8, 0)
    _objects.emplace_hint(iter, PRIME_MOVE(object));
#else
    _objects.insert(iter, PRIME_MOVE(object));
#endif

    return index;
}

int BinaryPropertyListWriter::getRequiredSizeOfIntegerInBytes(uint64_t n)
{
    if (n <= 0xff) {
        return 1;
    } else if (n <= 0xffff) {
        return 2;
    } else if ((uint64_t)n <= UINT64_C(0xffffffff)) {
        return 4;
    } else if ((uint64_t)n <= UINT64_C(0xffffffffffffffff)) {
        return 8;
    }

    return 16;
}

bool BinaryPropertyListWriter::writeUnixTime(const UnixTime& unixTime, uint64_t& encodedSize)
{
    uint8_t buffer[maxEncodedSizeOfIntegerWithToken];
    // Turns out CoreFoundation doesn't let you encode a date as a float32.
    static const Float64 secondsBetween1970And2001 = 978307200.0;
    int len = encodeDouble(unixTime.toDouble() - secondsBetween1970And2001, buffer);
    buffer[0] |= 0x30;
    encodedSize = len;
    return _streamBuffer->writeBytes(buffer, len, _log);
}

bool BinaryPropertyListWriter::writeObject(const Object& object, uint64_t& encodedSize)
{
    switch (object.type) {
    case Value::TypeNull: {
        encodedSize = 1;
        return _streamBuffer->writeByte(0, _log);
    }

    case Value::TypeBool:
        if (object.value.value->getBool()) {
            encodedSize = 1;
            return _streamBuffer->writeByte(0x09, _log);
        } else {
            encodedSize = 1;
            return _streamBuffer->writeByte(0x08, _log);
        }

    case Value::TypeInteger: {
        uint8_t buffer[maxEncodedSizeOfIntegerWithToken];
        int len = encodeSizedInteger(object.value.value->getInteger(), buffer);
        buffer[0] |= 0x10;
        encodedSize = len;
        return _streamBuffer->writeBytes(buffer, len, _log);
    }

    case Value::TypeReal: {
        uint8_t buffer[maxEncodedSizeOfIntegerWithToken];
        int len = encodeFloatOrDouble((Float64)object.value.value->getReal(), buffer);
        buffer[0] |= 0x20;
        encodedSize = len;
        return _streamBuffer->writeBytes(buffer, len, _log);
    }

    case Value::TypeDate: {
        return writeUnixTime(DateTime(object.value.value->getDate(), Time()).toUnixTime(), encodedSize);
    }

    case Value::TypeTime: {
        return writeUnixTime(DateTime(Date(2001, 0, 0), object.value.value->getTime()).toUnixTime(), encodedSize);
    }

    case Value::TypeDateTime: {
        return writeUnixTime(object.value.value->getUnixTime(), encodedSize);
    }

    case Value::TypeData: {
        const Data& data = object.value.value->getData();
        uint8_t buffer[maxEncodedSizeOfLengthWithToken];
        int len = encodeLength(data.size(), buffer);
        buffer[0] |= 0x40;
        encodedSize = len + data.size();
        return _streamBuffer->writeBytes(buffer, len, _log) && data.size() && _streamBuffer->writeBytes(&data[0], data.size(), _log);
    }

    case Value::TypeString:
        return writeString(*object.value.string, encodedSize);

    case Value::TypeVector:
        return writeArray((const uint64_t*)&object.value.value->getData()[0], object.value.value->getData().size() / sizeof(uint64_t), encodedSize);

    case Value::TypeDictionary:
        return writeDictionary((const uint64_t*)&object.value.value->getData()[0], object.value.value->getData().size() / sizeof(uint64_t), encodedSize);

    default:
        _log->error(PRIME_LOCALISE("Attempting to write undefined value to binary property list."));
        return false;
    }
}

int BinaryPropertyListWriter::encodeSizedInteger(int64_t n, uint8_t* buffer)
{
    return encodeSizedUnsignedInteger((uint64_t)n, buffer);
}

int BinaryPropertyListWriter::encodeSizedUnsignedInteger(uint64_t n, uint8_t* buffer)
{
    if (n & UINT64_C(0xffffffff00000000)) {
        buffer[0] = 3;
        encodeSizedUnsignedInteger(n, 8, buffer + 1);
        return 9;
    } else if (n & UINT64_C(0xffff0000)) {
        buffer[0] = 2;
        encodeSizedUnsignedInteger(n, 4, buffer + 1);
        return 5;
    } else if (n & UINT64_C(0xff00)) {
        buffer[0] = 1;
        encodeSizedUnsignedInteger(n, 2, buffer + 1);
        return 3;
    } else {
        buffer[0] = 0;
        buffer[1] = (uint8_t)n;
        return 2;
    }
}

void BinaryPropertyListWriter::encodeSizedUnsignedInteger(uint64_t n, int sizeInBytes, uint8_t* buffer)
{
    switch (sizeInBytes) {
    case 8:
        buffer[0] = (uint8_t)(n >> 56);
        buffer[1] = (uint8_t)(n >> 48);
        buffer[2] = (uint8_t)(n >> 40);
        buffer[3] = (uint8_t)(n >> 32);
        buffer[4] = (uint8_t)(n >> 24);
        buffer[5] = (uint8_t)(n >> 16);
        buffer[6] = (uint8_t)(n >> 8);
        buffer[7] = (uint8_t)n;
        break;

    case 4: {
        uint32_t u = (uint32_t)n;
        buffer[0] = (uint8_t)(u >> 24);
        buffer[1] = (uint8_t)(u >> 16);
        buffer[2] = (uint8_t)(u >> 8);
        buffer[3] = (uint8_t)u;
        break;
    }

    case 2: {
        unsigned int u = (unsigned int)n;
        buffer[0] = (uint8_t)(u >> 8);
        buffer[1] = (uint8_t)u;
        break;
    }

    case 1:
        buffer[0] = (uint8_t)n;
        break;

    default:
        PRIME_ASSERT(0);
        return;
    }
}

int BinaryPropertyListWriter::getRequiredSizeOfFloatOrDoubleInBytes(Float64 d) const
{
    volatile Float32 f = Float32(d);

    if (fabs(d - Float64(f)) == 0.0) {
        return 4;
    }

    return 8;
}

int BinaryPropertyListWriter::encodeFloatOrDouble(Float64 d, uint8_t* buffer)
{
    size_t encodedSize = getRequiredSizeOfFloatOrDoubleInBytes(d);

    if (encodedSize == 4) {
        union {
            Float32 f;
            uint32_t u;
        } u;

        u.f = (Float32)d;
        buffer[0] = 0x02;
        encodeSizedUnsignedInteger(u.u, 4, buffer + 1);
        return 5;
    } else {
        union {
            Float64 d;
            uint64_t u;
        } u;

        u.d = d;
        buffer[0] = 0x03;
        encodeSizedUnsignedInteger(u.u, 8, buffer + 1);
        return 9;
    }
}

int BinaryPropertyListWriter::encodeDouble(Float64 d, uint8_t* buffer)
{
    union {
        Float64 d;
        uint64_t u;
    } u;

    u.d = d;
    buffer[0] = 0x03;
    encodeSizedUnsignedInteger(u.u, 8, buffer + 1);
    return 9;
}

uint64_t BinaryPropertyListWriter::estimateStringSize(const std::string& string) const
{
    if (isASCII(string)) {
        uint8_t buffer[maxEncodedSizeOfLengthWithToken];
        int len = encodeLength(string.size(), buffer); // Note: don't actually need to encode it
        return len + string.size();
    }

    // Pass null as the destination to compute the size without writing anything
    size_t utf16Length = UTF8ToUTF16(string.data(), string.size(), 0, 0);

    uint8_t buffer[maxEncodedSizeOfLengthWithToken];
    int len = encodeLength(utf16Length, buffer); // Note: don't actually need to encode it
    return len + utf16Length * 2;
}

bool BinaryPropertyListWriter::writeString(const std::string& string, uint64_t& encodedSize)
{
    if (isASCII(string)) {
        uint8_t buffer[maxEncodedSizeOfLengthWithToken];
        int len = encodeLength(string.size(), buffer);
        buffer[0] |= 0x50;
        encodedSize = len + string.size();
        return _streamBuffer->writeBytes(buffer, len, _log) && _streamBuffer->writeString(string, _log);
    }

    // Pass null as the destination to compute the size without writing anything
    size_t utf16Length = UTF8ToUTF16(string.data(), string.size(), 0, 0);

    ScopedArrayPtr<uint16_t> utf16(new uint16_t[utf16Length + 1]);

    UTF8ToUTF16(string.data(), string.size(), utf16.get(), 0);

#ifdef PRIME_LITTLE_ENDIAN
    UTF16ByteSwap(utf16.get(), utf16Length);
#endif

    uint8_t buffer[maxEncodedSizeOfLengthWithToken];
    int len = encodeLength(utf16Length, buffer);
    buffer[0] |= 0x60;
    encodedSize = len + utf16Length * 2;
    return _streamBuffer->writeBytes(buffer, len, _log) && _streamBuffer->writeBytes(utf16.get(), utf16Length * 2, _log);
}

bool BinaryPropertyListWriter::isASCII(const std::string& string)
{
    const uint8_t* ptr = (const uint8_t*)string.data();
    const uint8_t* end = ptr + string.size();

    for (; ptr != end; ++ptr) {
        if (*ptr & 0x80) {
            return false;
        }
    }

    return true;
}

int BinaryPropertyListWriter::encodeLength(uint64_t length, uint8_t* buffer)
{
    if (length < 15) {
        buffer[0] = (uint8_t)length;
        return 1;
    }

    buffer[0] = 0x0f;
    int len = encodeSizedUnsignedInteger(length, buffer + 1);
    buffer[1] |= 0x10;

    return len + 1;
}

bool BinaryPropertyListWriter::writeArray(const uint64_t* references, size_t count, uint64_t& encodedSize)
{
    return writeArrayOrDictionary(0xa0, references, count, count, encodedSize);
}

bool BinaryPropertyListWriter::writeDictionary(const uint64_t* references, size_t count, uint64_t& encodedSize)
{
    return writeArrayOrDictionary(0xd0, references, count, count / 2, encodedSize);
}

bool BinaryPropertyListWriter::writeArrayOrDictionary(uint8_t top4, const uint64_t* references, size_t count,
    uint64_t length, uint64_t& encodedSize)
{
    uint8_t buffer[maxEncodedSizeOfLengthWithToken];
    int len = encodeLength(length, buffer);
    buffer[0] |= top4;
    if (!_streamBuffer->writeBytes(buffer, len, _log)) {
        return false;
    }
    encodedSize = len + _referenceSize * count;

    return writeReferences(references, count);
}

bool BinaryPropertyListWriter::writeReferences(const uint64_t* references, size_t count)
{
    while (count--) {
        uint8_t buffer[maxEncodedSizeOfIntegerWithToken];
        encodeSizedUnsignedInteger(*references++, _referenceSize, buffer);
        if (!_streamBuffer->writeBytes(buffer, _referenceSize, _log)) {
            return false;
        }
    }

    return true;
}

}
