// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_DATETIME_H
#define PRIME_DATETIME_H

#include "Convert.h"
#include "UnixTime.h"
#ifdef PRIME_ENABLE_IOSTREAMS
#include <ostream>
#endif

namespace Prime {

enum DateOrder {
    DateOrderISO8601,
    DateOrderYMD,
    DateOrderYMDOrBestGuess,
    DateOrderDMY,
    DateOrderDMYOrBestGuess,
    DateOrderMDY,
    DateOrderMDYOrBestGuess
};

enum TimeFormat {
    TimeFormatUnknown,
    TimeFormatISO8601,
    TimeFormat12HourAMPM,
    TimeFormat24Hour
};

enum DateTimeLocale {
    DateTimeLocaleUK,
};

//
// Date
//

/// A date on the Gregorian calendar.
class PRIME_PUBLIC Date {
public:

    static const char* shortMonthNames[12];

    static const char* longMonthNames[12];

    static const int daysInMonth[12];

    /// A "UNIX day" is a UNIX time divided by secondsPerDay.
    static Date fromUnixDay(int64_t unixDay);

    static Date fromUnixTime(int64_t unixTime)
    {
        return fromUnixDay(unixTime / secondsPerDay);
    }

    static int parseRFC1123MonthName(StringView name) PRIME_NOEXCEPT;

    static bool parseEnglishMonthName(StringView string, const char*& newBegin, int& month) PRIME_NOEXCEPT;

    static Optional<Date> parseISO8601(StringView iso8601) PRIME_NOEXCEPT;

    static Optional<Date> parseRFC1123(StringView rfc1123) PRIME_NOEXCEPT;

    static Optional<Date> parse(StringView string, DateOrder dateOrder = DateOrderYMD,
        TimeFormat timeFormat = TimeFormatUnknown, int allowShortYears = -1) PRIME_NOEXCEPT;

    static bool isLeapYear(int year);

    static const char* getShortMonthName(int month)
    {
        return month < 1 || month > 12 ? "" : shortMonthNames[month-1];
    }

    static const char* getLongMonthName(int month)
    {
        return month < 1 || month > 12 ? "" : longMonthNames[month-1];
    }

    Date(int year = 0, int month = 0, int day = 0) PRIME_NOEXCEPT : _year(year), _month(month), _day(day)
    {
    }

    Date(const Date& copy) PRIME_NOEXCEPT : _year(copy._year), _month(copy._month), _day(copy._day)
    {
    }

    bool isZero() const { return _year == 0 && _month == 0 && _day == 0; }

    /// 78 is 78 AD, 1978 is 1978.
    int getYear() const PRIME_NOEXCEPT { return _year; }

    /// 1 is January.
    int getMonth() const PRIME_NOEXCEPT { return _month; }

    /// 1 is the 1st.
    int getDay() const PRIME_NOEXCEPT { return _day; }

    /// 78 is 78 AD, 1978 is 1978.
    void setYear(int value) PRIME_NOEXCEPT { _year = value; }

    /// 1 is January.
    void setMonth(int value) PRIME_NOEXCEPT { _month = value; }

    /// 1 is the 1st.
    void setDay(int value) PRIME_NOEXCEPT { _day = value; }

    /// Return the same date a month from now, normalised.
    Date getNextMonth(int preferredDay = -1) const PRIME_NOEXCEPT;

    /// Return N days from now.
    Date getAddDays(int delta) const PRIME_NOEXCEPT;

    /// A UNIX day is a UNIX time divided by secondsPerDay.
    int64_t toUnixDay() const PRIME_NOEXCEPT { return dateToUnixDay(_year, _month, _day); }

    UnixTime toUnixTime() const PRIME_NOEXCEPT;

    enum Weekday {
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday
    };

    Weekday getWeekday() const PRIME_NOEXCEPT;

    // Future: getISO8601WeekNumber() // The first week of the year is the first week that contains Thursday.
    // Future: getUSWeekNumber() // The first week of the year is the week containing Jan 1.
    // Future: getWeekNumber(firstDayOfWeek, containJanTheNth) // e.g., Wednesday is the first day of the week, week containing Jan 1st.

    static const char* getRFC1123WeekdayName(Weekday weekDay) PRIME_NOEXCEPT;

    const char* getRFC1123WeekdayName() const PRIME_NOEXCEPT { return getRFC1123WeekdayName(getWeekday()); }

    static const char* getRFC1123MonthName(int month) PRIME_NOEXCEPT;

    const char* getRFC1123MonthName() const PRIME_NOEXCEPT { return getRFC1123MonthName(_month); }

    /// Returns false if the buffer is too small.
    bool toISO8601(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT;

    std::string toISO8601() const PRIME_NOEXCEPT;

    /// Builds a YYYYMMDD formatted string. Returns false if the buffer is too small.
    bool toPacked(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT;

    /// Builds a string in RFC-1123 format. Returns false if the buffer is too small.
    bool toRFC1123(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT;

    bool isValid() const PRIME_NOEXCEPT;

    bool operator==(const Date& rhs) const PRIME_NOEXCEPT
    {
        return _year == rhs._year && _month == rhs._month && _day == rhs._day;
    }

    bool operator<(const Date& rhs) const PRIME_NOEXCEPT
    {
        if (_year < rhs._year)
            return true;
        if (_year > rhs._year)
            return false;

        if (_month < rhs._month)
            return true;
        if (_month > rhs._month)
            return false;

        if (_day < rhs._day)
            return true;
        //if (_day > rhs._day) return false;

        return false;
    }

    std::string toText(DateTimeLocale locale) const;

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Date&)

private:
    static int64_t dateToUnixDay(int year, int month, int dayOfMonth) PRIME_NOEXCEPT;

    static void unixDayToDate(int64_t unixDay, int& year, int& month, int& dayOfMonth) PRIME_NOEXCEPT;

    int _year;
    int _month;
    int _day;
};

//
// Time
//

class PRIME_PUBLIC Time {
public:
    static bool parse(StringView string, Time& time) PRIME_NOEXCEPT;

    static Optional<Time> parse(StringView string) PRIME_NOEXCEPT;

    Time(int hour = 0, int minute = 0, int second = 0, int32_t nanosecond = 0) PRIME_NOEXCEPT : _hour(hour), _minute(minute), _second(second), _nanosecond(nanosecond)
    {
    }

    Time(const Time& copy) PRIME_NOEXCEPT : _hour(copy._hour), _minute(copy._minute), _second(copy._second), _nanosecond(copy._nanosecond)
    {
    }

    int getHour() const PRIME_NOEXCEPT { return _hour; }
    int getMinute() const PRIME_NOEXCEPT { return _minute; }
    int getSecond() const PRIME_NOEXCEPT { return _second; }
    int32_t getNanosecond() const PRIME_NOEXCEPT { return _nanosecond; }

    int get12HourHour() const PRIME_NOEXCEPT;

    void setHour(int value) PRIME_NOEXCEPT { _hour = value; }
    void setMinute(int value) PRIME_NOEXCEPT { _minute = value; }
    void setSecond(int value) PRIME_NOEXCEPT { _second = value; }
    void setNanosecond(int32_t value) PRIME_NOEXCEPT { _nanosecond = value; }

    bool isMidnight() const PRIME_NOEXCEPT { return _hour == 0 && _minute == 0 && _second == 0; }

    bool isMidday() const PRIME_NOEXCEPT { return _hour == 12 && _minute == 0 && _second == 0; }

    /// 0 through 60*60*24-1
    int toSecondWithinDay() const PRIME_NOEXCEPT;

    bool operator==(const Time& rhs) const PRIME_NOEXCEPT
    {
        return _hour == rhs._hour && _minute == rhs._minute && _second == rhs._second && _nanosecond == rhs._nanosecond;
    }

    bool operator<(const Time& rhs) const PRIME_NOEXCEPT
    {
        if (_hour < rhs._hour)
            return true;
        if (_hour > rhs._hour)
            return false;

        if (_minute < rhs._minute)
            return true;
        if (_minute > rhs._minute)
            return false;

        if (_second < rhs._second)
            return true;
        if (_second > rhs._second)
            return false;

        if (_nanosecond < rhs._nanosecond)
            return true;
        // if (_nanosecond > rhs._nanosecond) return false;

        return false;
    }

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const Time&)

    /// Returns false if the buffer is too small.
    bool toISO8601(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT;

    std::string toText(DateTimeLocale locale) const;

private:
    int _hour;
    int _minute;
    int _second;
    int32_t _nanosecond;
};

//
// DateTime
//

/// A date/time on the Gregorian calendar.
class PRIME_PUBLIC DateTime : public Date, public Time {
public:
    static DateTime fromUnixTime(int64_t unixTime, int32_t nanosecond = 0) PRIME_NOEXCEPT;

    /// A "UNIX day" is a UNIX time divided by secondsPerDay.
    static DateTime fromUnixDay(int64_t unixDay, int hour = 0, int minute = 0, int second = 0,
        int32_t nanosecond = 0) PRIME_NOEXCEPT;

    static DateTime fromDouble(double time) PRIME_NOEXCEPT
    {
        return DateTime(UnixTime(time));
    }

    static Optional<DateTime> parseISO8601(StringView iso8601) PRIME_NOEXCEPT;
    static Optional<UnixTime> parseISO8601UnixTime(StringView iso8601) PRIME_NOEXCEPT;

    static Optional<DateTime> parseRFC1123(StringView rfc1123) PRIME_NOEXCEPT;
    static Optional<UnixTime> parseRFC1123UnixTime(StringView rfc1123) PRIME_NOEXCEPT;

    static Optional<DateTime> parse(StringView string, DateOrder dateOrder = DateOrderYMD,
        TimeFormat timeFormat = TimeFormatUnknown, int allowShortYears = -1) PRIME_NOEXCEPT;
    static Optional<UnixTime> parseUnixTime(StringView string, DateOrder dateOrder = DateOrderYMD,
        TimeFormat timeFormat = TimeFormatUnknown, int allowShortYears = -1) PRIME_NOEXCEPT;

    static bool parseRFC1123(StringView inputView, Date& date, Time& time, int& tzoffset, bool withTime) PRIME_NOEXCEPT;
    static bool parseISO8601(StringView iso8601, Date& date, Time& time, int& tzoffset, int32_t& nanoseconds, bool withTime) PRIME_NOEXCEPT;
    static bool parseInternational(StringView string, DateOrder dateOrder, int allowShortYears, Date& date,
        TimeFormat timeFormat, Time& time, int& tzoffset, int32_t& nanoseconds,
        bool withTime) PRIME_NOEXCEPT;

    DateTime(const DateTime& copy) PRIME_NOEXCEPT : Date(copy),
                                                    Time(copy)
    {
    }

    // Intellisense needs this.
    DateTime() PRIME_NOEXCEPT { }

    explicit DateTime(int year, int month = 0, int day = 0, int hour = 0, int minute = 0, int second = 0,
        int32_t nanosecond = 0) PRIME_NOEXCEPT : Date(year, month, day),
                                                 Time(hour, minute, second, nanosecond)
    {
    }

    explicit DateTime(const Date& date, const Time& time = Time()) PRIME_NOEXCEPT : Date(date),
                                                                                    Time(time)
    {
    }

    DateTime(const UnixTime& unixTime) PRIME_NOEXCEPT
    {
        operator=(fromUnixTime(unixTime.getSeconds(), unixTime.getFractionNanoseconds()));
    }

    const Date& getDate() const PRIME_NOEXCEPT { return *this; }
    void setDate(const Date& date) { static_cast<Date&>(*this) = date; }

    const Time& getTime() const PRIME_NOEXCEPT { return *this; }
    void setTime(const Time& time) { static_cast<Time&>(*this) = time; }

    const DateTime getMidnight() const
    {
        DateTime copy(*this);
        copy.setTime(Time(0, 0));
        return copy;
    }

    UnixTime toUnixTime() const PRIME_NOEXCEPT;

    double toDouble() const PRIME_NOEXCEPT { return toUnixTime().toDouble(); }

    /// Returns false if the buffer is too small.
    bool toISO8601(char* buffer, size_t bufferSize, StringView separator = "T", StringView zone = "Z") const PRIME_NOEXCEPT;

    std::string toISO8601(StringView separator = "T", StringView zone = "Z") const PRIME_NOEXCEPT;

    /// Returns a date in RFC-1123 format (as updated by RFC-1123). Returns false if the buffer is too small.
    bool toRFC1123(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT;

    bool operator==(const DateTime& rhs) const PRIME_NOEXCEPT
    {
        return getDate() == rhs.getDate() && getTime() == rhs.getTime();
    }

    bool operator<(const DateTime& rhs) const PRIME_NOEXCEPT
    {
        if (getDate() < rhs.getDate())
            return true;
        if (getDate() > rhs.getDate())
            return false;

        if (getTime() < rhs.getTime())
            return true;
        // if (getTime() > rhs.getTime()) return false;

        return false;
    }

    std::string toText(DateTimeLocale locale) const;

    PRIME_IMPLIED_COMPARISONS_OPERATORS(const DateTime&)
};

//
// Conversions
//

PRIME_PUBLIC bool StringAppend(std::string& output, const Date& value);
PRIME_PUBLIC bool StringAppend(std::string& output, const Time& value);
PRIME_PUBLIC bool StringAppend(std::string& output, const UnixTime& value);
PRIME_PUBLIC bool StringAppend(std::string& output, const DateTime& value);

PRIME_PUBLIC bool UnsafeConvert(Date& output, StringView input) PRIME_NOEXCEPT;

inline bool UnsafeConvert(Date& output, const char* input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

inline bool UnsafeConvert(Date& output, const std::string& input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

PRIME_PUBLIC bool UnsafeConvert(Date& output, const UnixTime& input) PRIME_NOEXCEPT;
PRIME_PUBLIC bool UnsafeConvert(Date& output, const DateTime& input) PRIME_NOEXCEPT;

PRIME_PUBLIC bool UnsafeConvert(Time& output, StringView input) PRIME_NOEXCEPT;

inline bool UnsafeConvert(Time& output, const char* input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

inline bool UnsafeConvert(Time& output, const std::string& input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

PRIME_PUBLIC bool UnsafeConvert(Time& output, const UnixTime& input) PRIME_NOEXCEPT;
PRIME_PUBLIC bool UnsafeConvert(Time& output, const DateTime& input) PRIME_NOEXCEPT;

PRIME_PUBLIC bool UnsafeConvert(UnixTime& output, StringView input) PRIME_NOEXCEPT;

inline bool UnsafeConvert(UnixTime& output, const char* input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

inline bool UnsafeConvert(UnixTime& output, const std::string& input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

PRIME_PUBLIC bool UnsafeConvert(DateTime& output, StringView input) PRIME_NOEXCEPT;

inline bool UnsafeConvert(DateTime& output, const char* input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

inline bool UnsafeConvert(DateTime& output, const std::string& input) PRIME_NOEXCEPT
{
    return UnsafeConvert(output, StringView(input));
}

template <typename Input>
Date ToDate(const Input& input, const Date& defaultValue = Date())
{
    Date temp;
    return UnsafeConvert(temp, input) ? temp : defaultValue;
}

template <typename Input>
Time ToTime(const Input& input, const Time& defaultValue = Time())
{
    Time temp;
    return UnsafeConvert(temp, input) ? temp : defaultValue;
}

template <typename Input>
UnixTime ToUnixTime(const Input& input, const UnixTime& defaultValue = UnixTime())
{
    UnixTime temp;
    return UnsafeConvert(temp, input) ? temp : defaultValue;
}

template <typename Input>
DateTime ToDateTime(const Input& input, const DateTime& defaultValue = DateTime())
{
    UnixTime temp;
    return UnsafeConvert(temp, input) ? DateTime(temp) : defaultValue;
}

//
// ostream
//

#ifdef PRIME_ENABLE_IOSTREAMS

inline std::ostream& operator<<(std::ostream& os, const Date& value)
{
    os << ToString(value);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Time& value)
{
    os << ToString(value);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const UnixTime& value)
{
    os << ToString(value);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const DateTime& value)
{
    os << value.toUnixTime();
    return os;
}

#endif
}

#endif
