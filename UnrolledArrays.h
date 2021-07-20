// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_UNROLLEDARRAYS_H
#define PRIME_UNROLLEDARRAYS_H

#include "Config.h"

namespace Prime {

template <typename Type>
inline void ArraySet(Type* a, Type a0, Type a1)
{
    a[0] = a0;
    a[1] = a1;
}

template <typename Type>
inline void ArraySet(Type* a, Type a0, Type a1, Type a2)
{
    a[0] = a0;
    a[1] = a1;
    a[2] = a2;
}

template <typename Type>
inline void ArraySet(Type* a, Type a0, Type a1, Type a2, Type a3)
{
    a[0] = a0;
    a[1] = a1;
    a[2] = a2;
    a[3] = a3;
}

template <typename Type>
inline void ArraySet(Type* a, Type a0, Type a1, Type a2, Type a3, Type a4)
{
    a[0] = a0;
    a[1] = a1;
    a[2] = a2;
    a[3] = a3;
    a[4] = a4;
}

#ifndef PRIME_COMPILER_NO_PARTIAL_TEMPLATE_SPECIALISATION

//
// Unrolled arrays
//

namespace Private {

    template <size_t ArraySize, typename Type>
    struct UnrolledArray {

        static void copy(Type* out, const Type* in)
        {
            *out = *in;
            UnrolledArray<ArraySize - 1, Type>::copy(out + 1, in + 1);
        }

        static bool less(const Type* a, const Type* b)
        {
            if (*a < *b) {
                return true;
            }

            if (*a > *b) {
                return false;
            }

            return UnrolledArray<ArraySize - 1, Type>::less(a + 1, b + 1);
        }

        static int compare(const Type* a, const Type* b)
        {
            if (*a != *b) {
                return *a < *b ? -1 : 1;
            }

            return UnrolledArray<ArraySize - 1, Type>::compare(a + 1, b + 1);
        }

        static bool equal(const Type* a, const Type* b)
        {
            if (*a != *b) {
                return false;
            }

            return UnrolledArray<ArraySize - 1, Type>::equal(a + 1, b + 1);
        }

        static void fill(Type* array, const Type& value)
        {
            *array = value;
            UnrolledArray<ArraySize - 1, Type>::fill(array + 1, value);
        }
    };

    template <typename Type>
    struct UnrolledArray<0, Type> {

        static void copy(Type*, const Type*) { }

        static bool less(const Type*, const Type*) { return false; }

        static int compare(const Type*, const Type*) { return 0; }

        static bool equal(const Type*, const Type*) { return true; }

        static void fill(Type*, const Type&) { }
    };
}

template <size_t ArraySize, typename Type>
inline void ArrayCopy(Type* out, const Type* in)
{
    Private::UnrolledArray<ArraySize, Type>::copy(out, in);
}

template <size_t ArraySize, typename Type>
inline bool ArraysEqual(const Type* a, const Type* b)
{
    return Private::UnrolledArray<ArraySize, Type>::equal(a, b);
}

template <size_t ArraySize, typename Type>
inline bool ArrayLess(const Type* a, const Type* b)
{
    return Private::UnrolledArray<ArraySize, Type>::less(a, b);
}

template <size_t ArraySize, typename Type>
inline int ArrayCompare(const Type* a, const Type* b)
{
    return Private::UnrolledArray<ArraySize, Type>::compare(a, b);
}

template <size_t ArraySize, typename Type>
inline void ArrayFill(Type* a, const Type& v)
{
    return Private::UnrolledArray<ArraySize, Type>::fill(a, v);
}

#endif
}

#endif
