// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_STRINGSEARCHING_H
#define PRIME_STRINGSEARCHING_H

#include "Config.h"
#include <algorithm>

namespace Prime {

/// Implements comparison and searching methods matching those provided by std::basic_string for any string type
/// that has data(), size() and empty() methods and an npos constant.
template <typename Char, typename String, typename SizeType, typename Traits>
class StringSearching {
public:
    typedef SizeType size_type;
    typedef Char value_type;
    typedef Traits traits_type;

    /// Clamp *offset and *count to the size of the string.
    static void fix_offset_count(const String& string, size_type& offset, size_type& count) PRIME_NOEXCEPT
    {
        size_type size = string.size();

        if (offset > size) {
            offset = size;
        }

        if (count > size - offset) {
            count = size - offset;
        }
    }

    int compare(const String& other) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.compare(0, self.size(), other.data(), other.size());
    }

    int compare(size_type offset, size_type count, const String& other) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.compare(offset, count, other, 0, other.size());
    }

    // Ambiguous.
    // int compare(size_type offset, size_type count, const Char* other) const PRIME_NOEXCEPT
    // {
    //  const String& self = static_cast<const String&>(*this);
    //  return self.compare(offset, count, other, traits_type::length(other));
    // }

    int compare(size_type offset, size_type count, const String& other, size_type otherOffset, size_type otherSize) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        fix_offset_count(self, offset, count);
        fix_offset_count(other, otherOffset, otherSize);
        return compare(self.data() + offset, self.data() + offset + count, other.data() + otherOffset, other.data() + otherOffset + otherSize);
    }

    int compare(const value_type* string) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.compare(0, self.size(), string, traits_type::length(string));
    }

    int compare(size_type offset, size_type count, const value_type* array, size_type arraySize = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        fix_offset_count(self, offset, count);
        arraySize = std::min(traits_type::length(array), arraySize);
        return compare(self.data() + offset, self.data() + offset + count, array, array + arraySize);
    }

    size_type find(const String& other, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find(other.data(), start, other.size());
    }

    size_type find(const value_type* array, size_type start, size_type arraySize) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (start >= self.size()) {
            // This bonkersness brought to you by the C++ standard
            if (arraySize == 0 && start == self.size()) {
                return self.size();
            } else {
                return String::npos;
            }
        }

        if (arraySize > self.size() - start) {
            return String::npos;
        }

        if (arraySize == 0) {
            return start;
        }

        const value_type* e = self.data() + self.size();
        for (const value_type* p = self.data() + start; p != e; ++p) {
            if (std::equal(p, p + arraySize, array)) {
                return static_cast<size_type>(p - self.data());
            }
        }

        return String::npos;
    }

    size_type find(const value_type* string, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find(string, start, traits_type::length(string));
    }

    size_type find(const value_type& needle, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (start < self.size()) {
            const value_type* e = self.data() + self.size();
            for (const value_type* p = self.data() + start; p != e; ++p) {
                if (*p == needle) {
                    return static_cast<size_type>(p - self.data());
                }
            }
        }

        return String::npos;
    }

    size_type rfind(const String& other, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.rfind(other.data(), start, other.size());
    }

    size_type rfind(const value_type* array, size_type start, size_type arraySize) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (arraySize > self.size()) {
            return String::npos;
        }

        size_type max_start = self.size() - arraySize;
        if (start > max_start) {
            start = max_start;
        }

        if (!arraySize) {
            return start;
        }

        do {
            if (std::equal(array, array + arraySize, self.data() + start)) {
                return start;
            }
        } while (start--);

        return String::npos;
    }

    size_type rfind(const value_type* string, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.rfind(string, start, traits_type::length(string));
    }

    size_type rfind(const value_type& needle, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (!self.empty()) {
            if (start > self.size() - 1) {
                start = self.size() - 1;
            }

            const value_type* self_chars = self.data();

            do {
                if (self_chars[start] == needle) {
                    return start;
                }
            } while (start--);
        }

        return String::npos;
    }

    size_type find_first_of(const String& other, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_first_of(other.data(), start, other.size());
    }

    size_type find_first_of(const value_type* array, size_type start, size_type arraySize) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (start >= self.size()) {
            return String::npos;
        }

        const value_type* array_end = array + arraySize;

        const value_type* e = self.data() + self.size();
        for (const value_type* p = self.data() + start; p != e; ++p) {
            if (std::find(array, array_end, *p) != array_end) {
                return static_cast<size_type>(p - self.data());
            }
        }

        return String::npos;
    }

    size_type find_first_of(const value_type* string, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_first_of(string, start, traits_type::length(string));
    }

    size_type find_first_of(const value_type& needle, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find(needle, start);
    }

    size_type find_last_of(const String& other, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_last_of(other.data(), start, other.size());
    }

    size_type find_last_of(const value_type* array, size_type start, size_type arraySize) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (self.empty()) {
            return String::npos;
        }

        size_type max_start = self.size() - 1;
        if (start > max_start) {
            start = max_start;
        }

        const value_type* array_end = array + arraySize;
        const value_type* self_chars = self.data();

        do {
            if (std::find(array, array_end, self_chars[start]) != array_end) {
                return start;
            }
        } while (start--);

        return String::npos;
    }

    size_type find_last_of(const value_type* string, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_last_of(string, start, traits_type::length(string));
    }

    size_type find_last_of(const value_type& needle, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.rfind(needle, start);
    }

    size_type find_first_not_of(const String& other, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_first_not_of(other.data(), start, other.size());
    }

    size_type find_first_not_of(const value_type* array, size_type start, size_type arraySize) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (start >= self.size()) {
            return String::npos;
        }

        const value_type* array_end = array + arraySize;

        const value_type* e = self.data() + self.size();
        for (const value_type* p = self.data() + start; p != e; ++p) {
            if (std::find(array, array_end, *p) == array_end) {
                return static_cast<size_type>(p - self.data());
            }
        }

        return String::npos;
    }

    size_type find_first_not_of(const value_type* string, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_first_not_of(string, start, traits_type::length(string));
    }

    size_type find_first_not_of(const value_type& needle, size_type start = 0) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (start < self.size()) {
            const value_type* e = self.data() + self.size();
            for (const value_type* p = self.data() + start; p != e; ++p) {
                if (*p != needle) {
                    return static_cast<size_type>(p - self.data());
                }
            }
        }

        return String::npos;
    }

    size_type find_last_not_of(const String& other, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_last_not_of(other.data(), start, other.size());
    }

    size_type find_last_not_of(const value_type* array, size_type start, size_type arraySize) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (self.empty()) {
            return String::npos;
        }

        size_type max_start = self.size() - 1;
        if (start > max_start) {
            start = max_start;
        }

        const value_type* array_end = array + arraySize;
        const value_type* self_chars = self.data();

        do {
            if (std::find(array, array_end, self_chars[start]) == array_end) {
                return start;
            }
        } while (start--);

        return String::npos;
    }

    size_type find_last_not_of(const value_type* string, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        return self.find_last_not_of(string, start, traits_type::length(string));
    }

    size_type find_last_not_of(const value_type& needle, size_type start = String::npos) const PRIME_NOEXCEPT
    {
        const String& self = static_cast<const String&>(*this);
        if (!self.empty()) {
            if (start > self.size() - 1) {
                start = self.size() - 1;
            }

            const value_type* self_chars = self.data();

            do {
                if (self_chars[start] != needle) {
                    return start;
                }
            } while (start--);
        }

        return String::npos;
    }

protected:
    /// 3-way lexicographical compare.
    static int compare(const Char* begin1, size_t length1, const Char* begin2, size_t length2) PRIME_NOEXCEPT
    {
        size_t minLength = length1 < length2 ? length1 : length2;
        if (minLength == 0) {
            return 0;
        }

        int n = traits_type::compare(begin1, begin2, minLength);
        if (n != 0) {
            return n;
        }

        if (length1 < length2) {
            return -1;
        } else if (length1 > length2) {
            return 1;
        }

        return 0;
    }
};

}

#endif
