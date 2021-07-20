// Copyright 2000-2021 Mark H. P. Lord

#include "ValueNSObject.h"
#include "../StringUtils.h"
#include <Foundation/Foundation.h>

namespace Prime {

//
// UnsafeConvert(Value, id)
//

bool UnsafeConvert(Value& output, id object)
{
    if ([object isKindOfClass:[NSString class]]) {
        NSString* string = (NSString*)object;
        output.resetString() = [string UTF8String];
        return true;
    }

    if ([object isKindOfClass:[NSDictionary class]]) {
        NSDictionary* dictionary = (NSDictionary*)object;

        NSArray* keys = [dictionary allKeys];
        NSUInteger count = [keys count];

        Value::Dictionary& outputDictionary = output.resetDictionary();
        outputDictionary.reserve(count);

        for (NSUInteger i = 0; i != count; ++i) {
            id keyObject = [keys objectAtIndex:i];
            id valueObject = [dictionary objectForKey:keyObject];

            Value key = ToValue(keyObject);
            if (!key.isString()) {
                key = key.toString();
                DeveloperWarning("NSDictionary key not string: %s", key.c_str());
            }
            Value value = ToValue(valueObject);

            outputDictionary.push_back(Value::Dictionary::value_type(key.getString(), value));
        }

        return true;
    }

    if ([object isKindOfClass:[NSNumber class]]) {
        NSNumber* number = (NSNumber*)object;
        const char* type = [number objCType];

        if (StringsEqual(type, @encode(BOOL))) {
            output = [number boolValue] ? true : false;
            return true;
        }

        if (StringsEqual(type, @encode(double)) || StringsEqual(type, @encode(float)) || StringsEqual(type, @encode(long double))) {
            // If we can losslessly convert a number to an integer and back to a double then store it as an integer.
            Value::Real realValue = [number doubleValue];
            Value::Integer integerValue = (Value::Integer)realValue;
            if ((double)integerValue == realValue) {
                output = integerValue;
                return true;
            } else {
                output = realValue;
                return true;
            }
        }

        output = [number longLongValue];
        return true;
    }

    if ([object isKindOfClass:[NSArray class]]) {
        NSArray* array = (NSArray*)object;

        NSUInteger count = [array count];

        Value::Vector& outputVector = output.resetVector();
        outputVector.resize(count);

        for (NSUInteger i = 0; i != count; ++i) {
            UnsafeConvert(outputVector[i], [array objectAtIndex:i]);
        }

        return true;
    }

    if ([object isKindOfClass:[NSData class]]) {
        NSData* data = (NSData*)object;
        const char* begin = (const char*)[data bytes];
        const char* end = begin + [data length];
        output.resetData().assign(begin, end);
        return true;
    }

    if ([object isKindOfClass:[NSDate class]]) {
        NSDate* date = (NSDate*)object;
        NSTimeInterval interval = [date timeIntervalSince1970];

        output = UnixTime(interval);
        return true;
    }

    return false;
}

//
// ToNSObject
//

id ToNSObject(const Value& value)
{
    if (value.isBool()) {
        return [NSNumber numberWithBool:(value.getBool() ? YES : NO)];
    }

    if (value.isInteger()) {
        return [NSNumber numberWithLongLong:value.getInteger()];
    }

    if (value.isReal()) {
        return [NSNumber numberWithDouble:(double)value.getReal()];
    }

    if (value.isDateTime()) {
        const UnixTime& unixTime = value.getUnixTime();

        NSTimeInterval interval = unixTime.toDouble();
        return [NSDate dateWithTimeIntervalSince1970:interval];
    }

    if (value.isData()) {
        const Data& data = value.getData();
        return [NSData dataWithBytes:data.data() length:data.size()];
    }

    if (value.isString()) {
        return [NSString stringWithUTF8String:value.c_str()];
    }

    if (value.isVector()) {
        const Value::Vector& array = value.getVector();
        size_t count = array.size();

        NSMutableArray* newArray = [[[NSMutableArray alloc] initWithCapacity:array.size()] autorelease];

        for (size_t i = 0; i != count; ++i) {
            id object = ToNSObject(array[i]);

            if (!PRIME_GUARD(object)) {
                continue;
            }

            [newArray addObject:object];
        }

        return newArray;
    }

    if (value.isDictionary()) {
        const Value::Dictionary& dict = value.getDictionary();

        NSMutableDictionary* newDictionary = [[[NSMutableDictionary alloc] initWithCapacity:dict.size()] autorelease];

        size_t count = dict.size();
        for (size_t i = 0; i != count; ++i) {
            const Value::Dictionary::value_type& pair = dict.pair(i);
            id valueObject = ToNSObject(pair.second);

            if (!PRIME_GUARD(valueObject)) {
                continue;
            }

            id keyObject = ToNSObject(pair.first);

            if (PRIME_GUARD(keyObject)) {
                [newDictionary setObject:valueObject forKey:keyObject];
            }
        }

        return newDictionary;
    }

    PRIME_ASSERTMSG(0, "unknown value type");

    return nil;
}
}
