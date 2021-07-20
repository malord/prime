// Copyright 2000-2021 Mark H. P. Lord

// The bplist format looks as thought it was designed to be ready with mmap(), and reading from a stream would
// require a significant amount of seeking. To avoid this, I seek to the end to read the file footer, then seek back
// and read all the objects sequentially. Since arrays and dictionaries depend on other objects, both are initially
// loaded as raw data and are converted to arrays and dictionaries in a separate pass.

#include "BinaryPropertyListReader.h"
#include "ScopedPtr.h"
#include "TextEncoding.h"
#include <new>
#include <set>

namespace Prime {

namespace {

    const char header00[] = "bplist00";
    const char header01[] = "bplist01";

    const size_t headerSize = sizeof(header00) - 1;

    const size_t footerSize = sizeof(uint8_t) * 2 + sizeof(uint64_t) * 3;

    inline bool IsValidHeader(const void* header)
    {
        bool is00 = memcmp(header00, header, headerSize) == 0;
        bool is01 = memcmp(header01, header, headerSize) == 0;

        return is00 || is01;
    }

    inline bool IsFutureHeader(const void* header)
    {
        return memcmp(header, "bplist", 6) == 0;
    }

    inline bool IsVersion0Header(const void* header)
    {
        return memcmp(header, "bplist0", 7) == 0;
    }
}

bool BinaryPropertyListReader::readBigEndianUInt(StreamBuffer& buffer, uint64_t& out, size_t sizeInBytes, Log* log)
{
    uint64_t n = 0;
    uint8_t c = 0;

    while (sizeInBytes--) {
        if (!buffer.readByte(c, log)) {
            return false;
        }

        n = (n << 8) | c;
    }

    out = n;
    return true;
}

BinaryPropertyListReader::BinaryPropertyListReader()
{
}

BinaryPropertyListReader::~BinaryPropertyListReader()
{
}

Value BinaryPropertyListReader::read(Stream* stream, Log* log)
{
    if (StreamBuffer* existingStreamBuffer = UIDCast<StreamBuffer>(stream)) {
        return read(existingStreamBuffer, log);
    }

    // We have to seek all over the place to read these files, so a large buffer is actually detrimental as
    // after each seek the buffer gets re-filled.
    const size_t bufferSize = 512;
    StreamBuffer streamBuffer(stream, bufferSize);

    return read(&streamBuffer, log);
}

Value BinaryPropertyListReader::read(StreamBuffer* streamBuffer, Log* log)
{
    _log = log;
    _streamBuffer = streamBuffer;

    _streamSize = _streamBuffer->getSize(log);
    if (_streamSize < 0) {
        _log->error(PRIME_LOCALISE("Cannot read binary property list unless size is known."));
        return undefined;
    }

    // Apple's libraries write empty files for empty dictionaries.
    ptrdiff_t gotHeader = streamBuffer->requestNumberOfBytes(6, log);
    if (gotHeader == 0) {
        _log->verbose(PRIME_LOCALISE("Empty binary property list file."));
        return Value::Dictionary();
    }
    if (gotHeader <= 0) {
        return undefined;
    }

    char header[headerSize];
    if (!streamBuffer->setOffset(0, log) || !streamBuffer->readBytes(header, headerSize, log)) {
        return undefined;
    }

    if (!IsValidHeader(header)) {
        if (IsVersion0Header(header)) {
            _log->warning(PRIME_LOCALISE("Unsupported binary property list minor version (%c) - attempting to read."), header[5]);
        } else {
            if (IsFutureHeader(header)) {
                _log->error(PRIME_LOCALISE("Unsupported binary property list version."));
            } else {
                _log->error(PRIME_LOCALISE("Not a binary property list."));
            }

            return undefined;
        }
    }

    // FUTURE: v.2+ has an encoded int here specifying the size of the file but I've never seen it in use
    // FUTURE: v.2+ then has a CRC-32 encoded as: "0x12 0x__ 0x__ 0x__ 0x__", big-endian, may be 0 to indicate no CRC, never seen it in use
    // (in both the above cases, we'll skip them because the format gives us file offsets)

    // FUTURE: v1.5 binary property lists do not use object references but serialise objects in-place, and the first object after the header is the root
    // (never seen v1.5 in use)

    if (!readFooter()) {
        return undefined;
    }

    _referencedObjects.reset(new IncompleteValue[(size_t)(_footer.offsetTableSize)]);

    if (!readAllObjects()) {
        return undefined;
    }

    if (!buildContainers()) {
        return undefined;
    }

    return PRIME_MOVE(_referencedObjects[_footer.rootObjectIndex].value);
}

bool BinaryPropertyListReader::readFooter()
{
    if (_streamSize < (Stream::Offset)footerSize) {
        _log->error(PRIME_LOCALISE("Too small to be a binary property list."));
        return false;
    }

    const Stream::Offset footerOffset = _streamSize - footerSize;
    if (!_streamBuffer->setOffset(footerOffset, _log)) {
        return false;
    }

    if (!_streamBuffer->readByte(_footer.offsetTableEntrySize, _log)) {
        return false;
    }

    if (_footer.offsetTableEntrySize > 8) {
        _log->error(PRIME_LOCALISE("Offset table entry size is invalid."));
        return false;
    }

    if (!_streamBuffer->readByte(_footer.offsetTableIndexSize, _log)) {
        return false;
    }

    if (_footer.offsetTableIndexSize > 8) {
        _log->error(PRIME_LOCALISE("Offset table index size is invalid."));
        return false;
    }

    if (!readSizedInt(sizeof(uint64_t), _footer.offsetTableSize) || !readSizedInt(sizeof(uint64_t), _footer.rootObjectIndex) || !readSizedInt(sizeof(uint64_t), _footer.offsetTableOffset)) {

        return false;
    }

    int64_t offsetTableEndOffset = _footer.offsetTableOffset + _footer.offsetTableSize * _footer.offsetTableEntrySize;

    if (_footer.offsetTableOffset < 0 || offsetTableEndOffset > _streamSize) {
        _log->error(PRIME_LOCALISE("Offset table is corrupt."));
    }

    if (_footer.offsetTableSize < 1) {
        _log->error(PRIME_LOCALISE("No objects in binary property list."));
    }

    return readOffsetTable();
}

bool BinaryPropertyListReader::readOffsetTable()
{
    size_t offsetTableSize = (size_t)_footer.offsetTableSize;
    if ((int64_t)offsetTableSize != _footer.offsetTableSize) {
        _log->error(PRIME_LOCALISE("Offset table size exceeds addressable memory."));
        return false;
    }

    _offsetTable.resize(offsetTableSize);

    if (!_streamBuffer->setOffset(_footer.offsetTableOffset, _log)) {
        return false;
    }

    if (offsetTableSize) {
        uint32_t offsetTableEntrySize = _footer.offsetTableEntrySize;
        OffsetIndex* offsetTablePtr = &_offsetTable[0];

        for (size_t i = 0; i != offsetTableSize; ++i) {
            OffsetIndex offsetIndex;
            if (!readSizedInt(offsetTableEntrySize, offsetIndex.offset)) {
                return false;
            }

            offsetIndex.index = i;
            *offsetTablePtr++ = offsetIndex;
        }
    }

    return true;
}

bool BinaryPropertyListReader::readAllObjects()
{
    std::vector<OffsetIndex> sortedOffset(_offsetTable);
    std::sort(sortedOffset.begin(), sortedOffset.end(), OffsetIndex::lessByOffset);

    uint64_t currentOffset = (uint64_t)-1;
    bool firstSeek = true;

    for (std::vector<OffsetIndex>::iterator offsetPtr = sortedOffset.begin(); offsetPtr != sortedOffset.end(); ++offsetPtr) {
        if (offsetPtr->offset != currentOffset) {
            if (!firstSeek) {
                _log->trace("BinaryPropertyListReader: non-contiguous; having to seek.");
            }
            firstSeek = false;

            if (!_streamBuffer->setOffset((Stream::Offset)offsetPtr->offset, _log)) {
                return false;
            }

            currentOffset = (uint64_t)offsetPtr->offset;
        }

        uint64_t objectSize;
        if (!readObject(_referencedObjects[offsetPtr->index], objectSize)) {
            return false;
        }

        currentOffset += objectSize;
        PRIME_DEBUG_ASSERT(_streamBuffer->getOffset(_log) == (Stream::Offset)currentOffset);
    }

    return true;
}

bool BinaryPropertyListReader::buildContainers()
{
    return buildContainers(_referencedObjects.get(), _referencedObjects.get() + _footer.offsetTableSize);
}

bool BinaryPropertyListReader::buildContainers(IncompleteValue* begin, IncompleteValue* end)
{
    // Possible optimisation: linked list of IncompleteValues to avoid churning through all these ShouldBeAlreadyIs? Or does the cache alleviate this?
    for (IncompleteValue* incompleteValue = begin; incompleteValue != end; ++incompleteValue) {
        switch (incompleteValue->shouldBe) {
        case (unsigned char)ShouldBeAlreadyIs:
            break;

        case (unsigned char)ShouldBeArray:
            if (!buildArray(*incompleteValue)) {
                return false;
            }
            break;

        case (unsigned char)ShouldBeSet:
            if (!buildSet(*incompleteValue)) {
                return false;
            }
            break;

        case (unsigned char)ShouldBeDictionary:
            if (!buildDictionary(*incompleteValue)) {
                return false;
            }
            break;

        default:
            PRIME_ASSERT(0);
            break;
        }
    }

    return true;
}

uint64_t BinaryPropertyListReader::decodeSizedInt(const void* ptr, size_t sizeInBytes)
{
    uint64_t n = 0;
    const uint8_t* c = (const uint8_t*)ptr;

    while (sizeInBytes--) {
        n = (n << 8) | (*c++);
    }

    return n;
}

bool BinaryPropertyListReader::buildArray(IncompleteValue& incompleteValue)
{
    size_t count = incompleteValue.value.getData().size() / _footer.offsetTableIndexSize;

    Value::Vector array(count);

    if (count) {
        Value* arrayPtr = &array[0];

        const uint8_t* ptr = (const uint8_t*)&incompleteValue.value.getData()[0];

        for (; count--; ptr += _footer.offsetTableIndexSize, ++arrayPtr) {
            uint64_t n = decodeSizedInt(ptr, _footer.offsetTableIndexSize);
            size_t index = (size_t)n;

            IncompleteValue* other = &_referencedObjects[index];
            if (other->shouldBe != (unsigned char)ShouldBeAlreadyIs) {
                buildContainers(other, other + 1);
            }

            if (--other->refCount == 0) {
                new (arrayPtr) Value(PRIME_MOVE(other->value));
            } else {
                new (arrayPtr) Value(other->value);
            }

            PRIME_DEBUG_ASSERT(other->refCount >= 0);
        }
    }

    incompleteValue.value.accessVector().swap(array);
    incompleteValue.shouldBe = ShouldBeAlreadyIs;

    return true;
}

bool BinaryPropertyListReader::buildSet(IncompleteValue& incompleteValue)
{
    size_t count = incompleteValue.value.getData().size() / _footer.offsetTableIndexSize;

    std::set<Value> set;

    if (count) {
        const uint8_t* ptr = (const uint8_t*)&incompleteValue.value.getData()[0];

        for (; count--; ptr += _footer.offsetTableIndexSize) {
            uint64_t n = decodeSizedInt(ptr, _footer.offsetTableIndexSize);
            size_t index = (size_t)n;

            IncompleteValue* other = &_referencedObjects[index];
            if (other->shouldBe != (unsigned char)ShouldBeAlreadyIs) {
                buildContainers(other, other + 1);
            }

            if (--other->refCount == 0) {
                set.insert(PRIME_MOVE(other->value));
            } else {
                set.insert(other->value);
            }

            PRIME_DEBUG_ASSERT(other->refCount >= 0);
        }
    }

#if 1
    // Value doesn't support sets yet - so use a vector.
    _log->warning(PRIME_LOCALISE("Set converted to array."));
    Value::Vector vector(count);
    size_t vectorIndex = 0;
    for (std::set<Value>::iterator iter = set.begin(); iter != set.end(); ++iter, ++vectorIndex) {
        vector[vectorIndex] = *iter;
    }
    incompleteValue.value.accessVector().swap(vector);
#else
    incompleteValue->value.accessSet().swap(set);
#endif

    incompleteValue.shouldBe = ShouldBeAlreadyIs;

    return true;
}

bool BinaryPropertyListReader::buildDictionary(IncompleteValue& incompleteValue)
{
    size_t count = incompleteValue.value.getData().size() / _footer.offsetTableIndexSize / 2;

    Value::Dictionary dict;
    dict.reserve(count);

    if (count) {
        const uint8_t* keyPtr = (const uint8_t*)&incompleteValue.value.getData()[0];
        const uint8_t* valuePtr = keyPtr + count * _footer.offsetTableIndexSize;

        for (; count--; keyPtr += _footer.offsetTableIndexSize, valuePtr += _footer.offsetTableIndexSize) {
            uint64_t keyN = decodeSizedInt(keyPtr, _footer.offsetTableIndexSize);
            size_t keyIndex = (size_t)keyN;

            uint64_t valueN = decodeSizedInt(valuePtr, _footer.offsetTableIndexSize);
            size_t valueIndex = (size_t)valueN;

            IncompleteValue* key = &_referencedObjects[keyIndex];
            if (key->shouldBe != (unsigned char)ShouldBeAlreadyIs) {
                buildContainers(key, key + 1);
            }

            IncompleteValue* value = &_referencedObjects[valueIndex];
            if (value->shouldBe != (unsigned char)ShouldBeAlreadyIs) {
                buildContainers(value, value + 1);
            }

            if (!key->value.isString()) {
                std::string keyString = key->value.toString();
                _log->warning(PRIME_LOCALISE("Key not a string: %s"), keyString.c_str());
                --key->refCount;
                if (--value->refCount == 0) {
                    dict.push_back(Value::Dictionary::value_type(keyString, PRIME_MOVE(value->value)));
                } else {
                    dict.push_back(Value::Dictionary::value_type(keyString, value->value));
                }
            } else if (--key->refCount == 0) {
                if (--value->refCount == 0) {
                    dict.push_back(Value::Dictionary::value_type(PRIME_MOVE(key->value.accessString()), PRIME_MOVE(value->value)));
                } else {
                    dict.push_back(Value::Dictionary::value_type(PRIME_MOVE(key->value.accessString()), value->value));
                }
            } else {
                if (--value->refCount == 0) {
                    dict.push_back(Value::Dictionary::value_type(key->value.getString(), PRIME_MOVE(value->value)));
                } else {
                    dict.push_back(Value::Dictionary::value_type(key->value.getString(), value->value));
                }
            }

            PRIME_DEBUG_ASSERT(key->refCount >= 0);
            PRIME_DEBUG_ASSERT(value->refCount >= 0);
        }
    }

    incompleteValue.value.accessDictionary().swap(dict);
    incompleteValue.shouldBe = ShouldBeAlreadyIs;

    return true;
}

bool BinaryPropertyListReader::readObject(IncompleteValue& object, uint64_t& objectSize)
{
    int c;
    do {
        c = _streamBuffer->readByte(_log);
    } while (c == 0x0f); // fill byte

    if (c < 0) {
        return false;
    }

    unsigned int top4 = (unsigned int)c >> 4;
    unsigned int bottom4 = (unsigned int)c & 0x0f;

    objectSize = 1;
    object.shouldBe = ShouldBeAlreadyIs;

    uint64_t length = 0;

    switch (top4) {
    case 0x00:
        switch (bottom4) {
        case 0x00:
            object.value = null;
            return true;

        case 0x08:
            object.value = false;
            return true;

        case 0x09:
            object.value = true;
            return true;

        case 0x0c:
            // URL with no base URL
            PRIME_TODO();
            break;

        case 0x0d:
            // URL with base URL
            PRIME_TODO();
            break;

        case 0x0e:
            // 16-byte UUID
            PRIME_TODO();
            break;
        }
        break;

    case 0x01: {
        Value::Integer n;
        if (!readSizedInt(size_t(1) << bottom4, n)) {
            return false;
        }

        object.value = Value(n);
        objectSize += UINT64_C(1) << bottom4;
        return true;
    }

    case 0x02:
        Value::Real n;
        if (!readSizedFloat(size_t(1) << bottom4, n)) {
            return false;
        }

        object.value = Value(n);
        objectSize += UINT64_C(1) << bottom4;
        return true;

    case 0x03: {
        Value::Real timeInterval;
        if (!readSizedFloat(size_t(1) << bottom4, timeInterval)) {
            return false;
        }

        static const double secondsBetween1970And2001 = 978307200.0;
        object.value = Value(UnixTime((double)timeInterval + secondsBetween1970And2001));
        objectSize += UINT64_C(1) << bottom4;
        return true;
    }

    case 0x04:
        return readObjectLength(bottom4, length, objectSize) && readData(object.value, length, objectSize);

    case 0x05:
        return readObjectLength(bottom4, length, objectSize) && readASCII(object.value, length, objectSize);

    case 0x06:
        return readObjectLength(bottom4, length, objectSize) && readUnicode(object.value, length, objectSize);

    case 0x08:
        return readObjectLength(bottom4, length, objectSize) && readUID(object.value, length, objectSize);

    case 0x0a:
        object.shouldBe = ShouldBeArray;
        return readObjectLength(bottom4, length, objectSize) && readArrayOrSet(object.value, length, objectSize);

    case 0x0b:
        // ordset - which as of 2014/04/14, isn't actually implemented by Apple

    case 0x0c:
        object.shouldBe = ShouldBeSet;
        return readObjectLength(bottom4, length, objectSize) && readArrayOrSet(object.value, length, objectSize);
        break;

    case 0x0d:
        object.shouldBe = ShouldBeDictionary;
        return readObjectLength(bottom4, length, objectSize) && readDictionary(object.value, length, objectSize);
    }

    // Unknown object. Make it null.
    _log->warning(PRIME_LOCALISE("Invalid/unsupported object type 0x%02x - skipping."), top4);
    object.value = null;
    return true;
}

bool BinaryPropertyListReader::readObjectLength(unsigned int bottom4, uint64_t& length, uint64_t& objectSize)
{
    if (bottom4 == 0x0f) {
        IncompleteValue lengthValue;
        uint64_t lengthSize;
        if (!readObject(lengthValue, lengthSize)) {
            return false;
        }

        length = (uint64_t)lengthValue.value.toInteger();
        objectSize += lengthSize;
    } else {
        length = bottom4;
    }

    return true;
}

bool BinaryPropertyListReader::readSizedFloat(size_t sizeInBytes, Value::Real& value)
{
    if (sizeInBytes == 4) {
        Float32 n;
        if (!readFloat32(n)) {
            return false;
        }
        value = n;
        return true;
    } else if (sizeInBytes == 8) {
        Float64 n;
        if (!readFloat64(n)) {
            return false;
        }
        value = n;
        return true;
    }

    _log->error(PRIME_LOCALISE("Unsupported floating point number size."));
    return false;
}

bool BinaryPropertyListReader::readFloat32(Float32& value)
{
    union {
        uint32_t u;
        Float32 f;
    } u;

    if (!readSizedInt(4, u.u)) {
        return false;
    }

    value = u.f;
    return true;
}

bool BinaryPropertyListReader::readFloat64(Float64& value)
{
    union {
        uint64_t u;
        Float64 f;
    } u;

    if (!readSizedInt(8, u.u)) {
        return false;
    }

    value = u.f;
    return true;
}

bool BinaryPropertyListReader::readUID(Value& value, uint64_t length, uint64_t& objectSize)
{
    ++length;
    if (length > 8) {
        _log->error(PRIME_LOCALISE("UID too large."));
        return false;
    }

    uint64_t n;
    if (!readSizedInt((size_t)length, n)) {
        return false;
    }

    objectSize += (size_t)length;

#if 1
    Value::Dictionary dict(Value::Dictionary::value_type("CF$UID", (Value::Integer)n));
    value = Value(PRIME_MOVE(dict));
#else
    value = Value((Value::Integer)n);
#endif

    return true;
}

bool BinaryPropertyListReader::readData(Value& object, uint64_t length, uint64_t& objectSize)
{
    size_t size = (size_t)length;
    if (size != length) {
        _log->error(PRIME_LOCALISE("Data size exceeds addressable memory."));
        return false;
    }

    Data data(size);

    if (size && !_streamBuffer->readBytes(&data[0], size, _log)) {
        return false;
    }

    object = Value(PRIME_MOVE(data));
    objectSize += size;
    return true;
}

bool BinaryPropertyListReader::readASCII(Value& object, uint64_t length, uint64_t& objectSize)
{
    size_t size = (size_t)length;
    if (size != length) {
        _log->error(PRIME_LOCALISE("ASCII string size exceeds addressable memory."));
        return false;
    }

    std::string string(size, 0);

    if (size && !_streamBuffer->readBytes(&string[0], size, _log)) {
        return false;
    }

    object = Value(PRIME_MOVE(string));
    objectSize += size;
    return true;
}

bool BinaryPropertyListReader::readUnicode(Value& object, uint64_t length, uint64_t& objectSize)
{
    size_t size = (size_t)(length * 2);
    if (size != length * 2) {
        _log->error(PRIME_LOCALISE("Unicode string size exceeds addressable memory."));
        return false;
    }

    if (!size) {
        object = "";
        return true;
    }

    ScopedArrayPtr<uint16_t> raw(new uint16_t[(size_t)length]);

    if (size && !_streamBuffer->readBytes(raw.get(), size, _log)) {
        return false;
    }

#ifdef PRIME_LITTLE_ENDIAN
    UTF16ByteSwap(raw.get(), (size_t)length);
#endif

    // Pass null as the destination to compute the size without writing anything
    size_t utf8Size = UTF16ToUTF8(raw.get(), (size_t)length, 0, 0);

    std::vector<uint8_t> utf8(utf8Size + 1);

    UTF16ToUTF8(raw.get(), (size_t)length, &utf8[0], 0);

    object = Value(std::string((const char*)&utf8[0], utf8Size));
    objectSize += size;

    return true;
}

bool BinaryPropertyListReader::readArrayOrSet(Value& object, uint64_t length, uint64_t& objectSize)
{
    size_t size = (size_t)(length * _footer.offsetTableIndexSize);
    if (size != length * _footer.offsetTableIndexSize) {
        _log->error(PRIME_LOCALISE("Array/set larger than addressable memory."));
        return false;
    }

    Data data(size);

    if (size && !_streamBuffer->readExact(&data[0], size, _log)) {
        return false;
    }

    // Check valid indexes and increment refCounts of referenced objects. We'll later use the refCounts to
    // determine when we can std::move.
    size_t count = data.size() / _footer.offsetTableIndexSize;
    if (count) {
        const uint8_t* ptr = (const uint8_t*)&data[0];
        for (; count--; ptr += _footer.offsetTableIndexSize) {
            uint64_t n = decodeSizedInt(ptr, _footer.offsetTableIndexSize);

            size_t index = (size_t)n;

            if (index != n) {
                _log->error(PRIME_LOCALISE("Invalid object reference in array."));
                return false;
            }

            ++_referencedObjects[index].refCount;
        }
    }

    object = Value(PRIME_MOVE(data));
    objectSize += size;
    return true;
}

bool BinaryPropertyListReader::readDictionary(Value& object, uint64_t length, uint64_t& objectSize)
{
    size_t size = (size_t)(length * _footer.offsetTableIndexSize * 2);
    if (size != length * _footer.offsetTableIndexSize * 2) {
        _log->error(PRIME_LOCALISE("Dictionary size exceeds addressable memory."));
        return false;
    }

    Data data(size);

    if (size && !_streamBuffer->readExact(&data[0], size, _log)) {
        return false;
    }

    // Check valid indexes and increment refCounts of referenced objects. We'll later use the refCounts to
    // determine when we can std::move.
    size_t count = data.size() / _footer.offsetTableIndexSize / 2;
    if (count) {
        const uint8_t* keyPtr = (const uint8_t*)&data[0];
        const uint8_t* valuePtr = keyPtr + count * _footer.offsetTableIndexSize;

        for (; count--; keyPtr += _footer.offsetTableIndexSize, valuePtr += _footer.offsetTableIndexSize) {
            uint64_t keyN = decodeSizedInt(keyPtr, _footer.offsetTableIndexSize);
            size_t keyIndex = (size_t)keyN;

            uint64_t valueN = decodeSizedInt(valuePtr, _footer.offsetTableIndexSize);
            size_t valueIndex = (size_t)valueN;

            if (keyIndex != keyN || valueIndex != valueN) {
                _log->error(PRIME_LOCALISE("Invalid object reference in dictionary."));
                return false;
            }

            ++_referencedObjects[keyIndex].refCount;
            ++_referencedObjects[valueIndex].refCount;
        }
    }

    object = Value(PRIME_MOVE(data));
    objectSize += size;
    return true;
}

}
