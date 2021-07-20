// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_BINARYPROPERTYLISTREADER_H
#define PRIME_BINARYPROPERTYLISTREADER_H

#include "Log.h"
#include "ScopedPtr.h"
#include "StreamBuffer.h"
#include "Value.h"
#include <vector>

namespace Prime {

/// Reads a property list from an Apple binary format property list file. Requires that the Stream be seekable.
class PRIME_PUBLIC BinaryPropertyListReader {
public:
    explicit BinaryPropertyListReader();

    ~BinaryPropertyListReader();

    /// Read the root object from the stream. Returns an invalid Value on error.
    Value read(Stream* stream, Log* log);

    /// Read the root object from the stream. Returns an invalid Value on error. This value of read() avoids
    /// the need for a new StreamBuffer (and buffer) to be created.
    Value read(StreamBuffer* stream, Log* log);

private:
    struct Footer {
        uint8_t offsetTableEntrySize;
        uint8_t offsetTableIndexSize;
        int64_t offsetTableSize;
        int64_t rootObjectIndex;
        int64_t offsetTableOffset;
    };

    bool readFooter();
    bool readOffsetTable();

    static bool readBigEndianUInt(StreamBuffer& buffer, uint64_t& out, size_t sizeInBytes, Log* log);

    static uint64_t decodeSizedInt(const void* ptr, size_t size);

    /// Sets error on error.
    template <typename Integer>
    bool readSizedInt(size_t sizeInBytes, Integer& value)
    {
        uint64_t n;

        if (!readBigEndianUInt(*_streamBuffer, n, sizeInBytes, _log)) {
            return false;
        }

        value = static_cast<Integer>(n);
        return true;
    }

    bool readSizedFloat(size_t sizeInBytes, Value::Real& value);
    bool readFloat32(Float32& value);
    bool readFloat64(Float64& value);

    enum IncompleteValueShouldBe {
        ShouldBeAlreadyIs,
        ShouldBeArray,
        ShouldBeSet,
        ShouldBeDictionary
    };

    struct IncompleteValue {
        IncompleteValue()
            : refCount(0)
        {
        }

        Value value;
        uint32_t refCount;
        IncompleteValueShouldBe shouldBe;

        PRIME_UNCOPYABLE(IncompleteValue);
    };

    /// objectSize must not be NULL.
    bool readObject(IncompleteValue& object, uint64_t& objectSize);
    bool readObjectLength(unsigned int bottom4, uint64_t& length, uint64_t& objectSize);
    bool readUID(Value& object, uint64_t length, uint64_t& objectSize);
    bool readData(Value& object, uint64_t length, uint64_t& objectSize);
    bool readASCII(Value& object, uint64_t length, uint64_t& objectSize);
    bool readUnicode(Value& object, uint64_t length, uint64_t& objectSize);
    bool readArrayOrSet(Value& object, uint64_t length, uint64_t& objectSize);
    bool readSet(Value& object, uint64_t length, uint64_t& objectSize);
    bool readDictionary(Value& object, uint64_t length, uint64_t& objectSize);

    bool readAllObjects();
    bool buildContainers();
    bool buildContainers(IncompleteValue* begin, IncompleteValue* end);
    bool buildArray(IncompleteValue& incompleteValue);
    bool buildSet(IncompleteValue& incompleteValue);
    bool buildDictionary(IncompleteValue& incompleteValue);

    StreamBuffer* _streamBuffer;
    Log* _log; // Only used during a read(), so not retained.
    Stream::Offset _streamSize;

    Footer _footer;

    ScopedArrayPtr<IncompleteValue> _referencedObjects;

    struct OffsetIndex {
        uint64_t offset;
        uint64_t index;

        static bool lessByOffset(const OffsetIndex& a, const OffsetIndex& b)
        {
            return a.offset < b.offset;
        }
    };

    std::vector<OffsetIndex> _offsetTable;

    PRIME_UNCOPYABLE(BinaryPropertyListReader);
};
}

#endif
