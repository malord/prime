// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_BINARYPROPERTYLISTWRITER_H
#define PRIME_BINARYPROPERTYLISTWRITER_H

#include "Log.h"
#include "Stream.h"
#include "StreamBuffer.h"
#include "Value.h"
#include <set>
#include <vector>

namespace Prime {

/// Writes a property list in a version of Apple's binary property list format.
class PRIME_PUBLIC BinaryPropertyListWriter {
public:
    BinaryPropertyListWriter();

    ~BinaryPropertyListWriter();

    struct PRIME_PUBLIC Options {

        Options()
        {
        }

        // All removed
    };

    /// Write the property list to a Stream.
    bool write(Stream* stream, Log* log, const Value& value, const Options& options,
        size_t bufferSize, void* buffer = NULL);

private:
    struct Object {
        union {
            const Value* value;
            const std::string* string;
        } value;

        Value::Type type;
        uint64_t index;
        uint64_t encodedSize;

        Object() { }

#ifdef PRIME_COMPILER_RVALUEREF
    private:
#endif
        Object(const Object& copy)
            : value(copy.value)
            , type(copy.type)
            , index(copy.index)
            , encodedSize(copy.encodedSize)
        {
        }

        Object& operator=(const Object& copy)
        {
            value = copy.value;
            type = copy.type;
            index = copy.index;
            encodedSize = copy.encodedSize;
            return *this;
        }

#ifdef PRIME_COMPILER_RVALUEREF
    public:
#endif

#ifdef PRIME_COMPILER_RVALUEREF
        Object(Object&& move) PRIME_NOEXCEPT : value(move.value),
                                               type(move.type),
                                               index(move.index),
                                               encodedSize(move.encodedSize)
        {
        }

        Object& operator=(Object&& copy) PRIME_NOEXCEPT
        {
            value = copy.value;
            type = copy.type;
            index = copy.index;
            encodedSize = copy.encodedSize;
            return *this;
        }
#endif

        bool operator==(const Object& rhs) const
        {
            if (type != rhs.type) {
                return false;
            }

            return type == Value::TypeString ? (*value.string == *rhs.value.string)
                                             : (*value.value == *rhs.value.value);
        }

        bool operator<(const Object& rhs) const
        {
            if (type < rhs.type) {
                return true;
            }
            if (type > rhs.type) {
                return false;
            }

            return type == Value::TypeString ? (*value.string < *rhs.value.string)
                                             : (*value.value < *rhs.value.value);
        }

        PRIME_IMPLIED_COMPARISONS_OPERATORS(const Object&)
    };

    struct ObjectOffset {
        const Object* object;
        uint64_t offset;

        static bool lessByIndex(const ObjectOffset& a, const ObjectOffset& b)
        {
            return a.object->index < b.object->index;
        }
    };

    static bool isASCII(const std::string& string);

    static int getRequiredSizeOfIntegerInBytes(uint64_t n);

    int getRequiredSizeOfFloatOrDoubleInBytes(Float64 d) const;

    uint64_t visitValue(const Value* value);

    uint64_t visitString(const std::string* value);

    uint64_t visitArray(const Value::Vector* array);

    uint64_t visitDictionary(const Value::Dictionary* dict);

    uint64_t visitPrimitive(const Value* value);

    void visitUnixTime(Object& object, const UnixTime& unixTime);

#ifdef PRIME_COMPILER_RVALUEREF
    uint64_t insertObject(Object&& object);
#else
    uint64_t insertObject(Object& object);
#endif

    uint64_t estimateStringSize(const std::string& string) const;

    bool writeObject(const Object& object, uint64_t& encodedSize);

    bool writeString(const std::string& string, uint64_t& encodedSize);

    bool writeArray(const uint64_t* references, size_t count, uint64_t& encodedSize);

    bool writeDictionary(const uint64_t* references, size_t count, uint64_t& encodedSize);

    bool writeArrayOrDictionary(uint8_t top4, const uint64_t* references, size_t count, uint64_t length, uint64_t& encodedSize);

    bool writeReferences(const uint64_t* references, size_t count);

    bool writeUnixTime(const UnixTime& unixTime, uint64_t& encodedSize);

    static int encodeSizedInteger(int64_t n, uint8_t* buffer);

    static int encodeSizedUnsignedInteger(uint64_t n, uint8_t* buffer);

    static void encodeSizedUnsignedInteger(uint64_t n, int sizeInBytes, uint8_t* buffer);

    int encodeFloatOrDouble(Float64 d, uint8_t* buffer);

    int encodeDouble(Float64 d, uint8_t* buffer);

    static int encodeLength(uint64_t length, uint8_t* buffer);

    void freeDatas();

    Log* _log; // Only used during a write(), so not retained.
    bool _error;

    std::set<Object> _objects;
    uint64_t _nextIndex;
    int _referenceSize;

    StreamBuffer* _streamBuffer;

    Options _options;

    std::vector<Value*> _tempValues;

    PRIME_UNCOPYABLE(BinaryPropertyListWriter);
};
}

#endif
