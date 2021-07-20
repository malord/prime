// Copyright 2000-2021 Mark H. P. Lord

#include "DateTime.h"
#include "Clocks.h"
#include "NumberParsing.h"
#include "NumberUtils.h"
#include "StringUtils.h"

namespace Prime {

//
// Date
//

const char* Date::shortMonthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

const char* Date::longMonthNames[] = { "January", "February", "March", "April",
    "May", "June", "July", "August",
    "September", "October", "November", "December" };

const int Date::daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int Date::parseRFC1123MonthName(StringView name) PRIME_NOEXCEPT
{
    for (int i = 0; i != 12; ++i) {
        if (ASCIIEqualIgnoringCase(name, shortMonthNames[i])) {
            return i + 1;
        }
    }

    return -1;
}

bool Date::parseEnglishMonthName(StringView string, const char*& newBegin, int& month) PRIME_NOEXCEPT
{
    for (int i = 0; i != 12; ++i) {
        if ((string.size() == 3 || (string.size() > 3 && !ASCIIIsAlpha(string[3]))) && ASCIIEqualIgnoringCase(string.begin(), string.begin() + 3, shortMonthNames[i])) {
            newBegin = string.begin() + 3;
            month = i + 1;
            return true;
        }

        size_t len = strlen(longMonthNames[i]);

        if ((string.size() == len || (string.size() > len && !ASCIIIsAlpha(string[len]))) && ASCIIEqualIgnoringCase(string.begin(), string.begin() + len, longMonthNames[i])) {
            newBegin = string.begin() + len;
            month = i + 1;
            return true;
        }
    }

    return false;
}

Optional<Date> Date::parseISO8601(StringView iso8601) PRIME_NOEXCEPT
{
    Date date;
    Time time;
    int tzoffset;
    int32_t nanoseconds;
    if (!DateTime::parseISO8601(iso8601, date, time, tzoffset, nanoseconds, false)) {
        return nullopt;
    }

    return date;
}

Optional<Date> Date::parseRFC1123(StringView rfc1123) PRIME_NOEXCEPT
{
    Date date;
    Time time;
    int tzoffset;
    if (!DateTime::parseRFC1123(rfc1123, date, time, tzoffset, false)) {
        return nullopt;
    }

    return date;
}

Optional<Date> Date::parse(StringView string, DateOrder dateOrder, TimeFormat timeFormat, int allowShortYears) PRIME_NOEXCEPT
{
    Date date;
    Time time;
    int tzoffset;
    int32_t nanoseconds;
    if (DateTime::parseInternational(string, dateOrder, allowShortYears, date, timeFormat, time, tzoffset, nanoseconds, false)) {
        return date;
    }

    if (Optional<Date> iso8601 = parseISO8601(string)) {
        return iso8601;
    }

    if (Optional<Date> rfc1123 = parseRFC1123(string)) {
        return rfc1123;
    }

    return nullopt;
}

int64_t Date::dateToUnixDay(int year, int month, int dayOfMonth) PRIME_NOEXCEPT
{
    int64_t a = (14 - month) / 12;
    int64_t y = year + 4800 - a;
    int64_t m = month + 12 * a - 3;

    return dayOfMonth + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - (32045 + 2440588);
}

Date::Weekday Date::getWeekday() const PRIME_NOEXCEPT
{
    return (Weekday)PositiveModulus<int64_t>(toUnixDay() - Date(2012, 12, 10).toUnixDay(), 7);
}

const char* Date::getRFC1123WeekdayName(Weekday weekday) PRIME_NOEXCEPT
{
    PRIME_ASSERT((size_t)weekday >= 0 && (size_t)weekday < 7);

    static const char* names[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

    return names[weekday];
}

const char* Date::getRFC1123MonthName(int month) PRIME_NOEXCEPT
{
    PRIME_ASSERT(month >= 1 && month <= 12);

    return shortMonthNames[month - 1];
}

void Date::unixDayToDate(int64_t unixDay, int& year, int& month, int& dayOfMonth) PRIME_NOEXCEPT
{
    // This is a Julian Day calculation modified so that 0 is at the Unix epoch.

    const int64_t y = 4716;
    const int64_t j = 1401;
    const int64_t m = 3;
    const int64_t n = 12;
    const int64_t r = 4;
    const int64_t p = 1461;
    const int64_t v = 3;
    const int64_t u = 5;
    const int64_t s = 153;
    const int64_t w = 2;
    const int64_t B = 274277;
    const int64_t G = -38;

    int64_t julianDay = unixDay + 2440588;

    int64_t g = ((3 * (((4 * julianDay + B) / 146097)) / 4)) + G;
    int64_t J_ = julianDay + j + g;
    int64_t Y_ = ((r * J_ + v) / p);
    int64_t T_ = (((r * J_ + v) % p) / r);
    int64_t M_ = ((u * T_ + w) / s);
    int64_t D_ = (((u * T_ + w) % s) / u);
    dayOfMonth = Narrow<int>(D_ + 1);
    month = Narrow<int>(((M_ + m - 1) % n) + 1);
    year = Narrow<int>(Y_ - y + (((n + m - 1 - month) / n)));
}

Date Date::fromUnixDay(int64_t unixDay)
{
    int year, month, dayOfMonth;
    unixDayToDate(unixDay, year, month, dayOfMonth);

    return Date(year, month, dayOfMonth);
}

bool Date::toISO8601(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT
{
    return StringFormat(buffer, bufferSize, "%04d-%02d-%02d", getYear(), getMonth(), getDay());
}

std::string Date::toISO8601() const PRIME_NOEXCEPT
{
    return Format("%04d-%02d-%02d", getYear(), getMonth(), getDay());
}

bool Date::toPacked(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT
{
    return StringFormat(buffer, bufferSize, "%04d%02d%02d", getYear(), getMonth(), getDay());
}

bool Date::toRFC1123(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT
{
    // Wed, 23 Sep 2015
    return StringFormat(buffer, bufferSize, "%s, %02d %s %04d", getRFC1123WeekdayName(), getDay(), getRFC1123MonthName(), getYear());
}

UnixTime Date::toUnixTime() const PRIME_NOEXCEPT
{
    return UnixTime(toUnixDay() * secondsPerDay, 0);
}

Date Date::getNextMonth(int preferredDay) const PRIME_NOEXCEPT
{
    Date newDate(*this);
    ++newDate._month;
    if (newDate._month == 13) {
        ++newDate._year;
        newDate._month = 1;
    }

    if (preferredDay >= 28) {
        // so 31 Jan -> 28 Feb -> 31 Mar -> 30 Apr
        newDate._day = preferredDay;
    }

    if (newDate._month == 2) {
        int length = isLeapYear(newDate._year) ? 29 : 28;
        if (newDate._day > length) {
            newDate._day = length;
        }
    } else {
        if (newDate._day > daysInMonth[newDate._month - 1]) {
            newDate._day = daysInMonth[newDate._month - 1];
        }
    }

    return newDate;
}

Date Date::getAddDays(int delta) const PRIME_NOEXCEPT
{
    return Date::fromUnixDay(toUnixDay() + delta);
}

bool Date::isLeapYear(int year)
{
    return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

bool Date::isValid() const PRIME_NOEXCEPT
{
    if (_month < 1 || _month > 12 || _day < 1) {
        return false;
    }

    if (_month == 2) {
        if (_day > (isLeapYear(_year) ? 29 : 28)) {
            return false;
        }
    } else if (_day > daysInMonth[_month - 1]) {
        return false;
    }

    return true;
}

std::string Date::toText(DateTimeLocale locale) const
{
    switch (locale) {
    case DateTimeLocaleUK:
    default:
        return Format("%02d/%02d/%04d", getDay(), getMonth(), getYear());
    }
}

//
// Time
//

Optional<Time> Time::parse(StringView string) PRIME_NOEXCEPT
{
    Time time;
    if (!parse(string, time)) {
        return nullopt;
    }

    return time;
}

bool Time::parse(StringView string, Time& time) PRIME_NOEXCEPT
{
    int value[3] = { 0, 0, 0 };
    int nvalues = 0;

    for (;;) {
        string = StringViewLeftTrim(string);
        if (string.empty()) {
            break;
        }

        if (nvalues == Narrow<int>(PRIME_COUNTOF(value))) {
            return false;
        }

        const char* newBegin;
        if (!ParseInt(string.substr(0, 2), newBegin, value[nvalues])) {
            return false;
        }

        ++nvalues;

        string = StringViewLeftTrim(StringView(newBegin, string.end()));

        if (!string.empty() && strchr(":-.", string[0])) {
            string.remove_prefix();
        } else if (nvalues >= 1) {
            if (ASCIIStartsWithIgnoringCase(string, "am")) {
                if (value[0] < 1 || value[0] > 12) {
                    return false;
                }
                if (value[0] == 12) {
                    value[0] = 0;
                }
                string.remove_prefix(2);
            } else if (ASCIIStartsWithIgnoringCase(string, "pm")) {
                if (value[0] < 1 || value[0] > 12) {
                    return false;
                }
                if (value[0] != 12) {
                    value[0] += 12;
                }
                string.remove_prefix(2);
            } else {
                continue;
            }

            if (StringIsWhitespace(string)) {
                break;
            }

            return false;
        }
    }

    if (nvalues < 1) {
        return false;
    }

    time = Time(value[0], value[1], value[2]);

    return true;
}

int Time::toSecondWithinDay() const PRIME_NOEXCEPT
{
    return _hour * 60 * 60 + _minute * 60 + _second;
}

bool Time::toISO8601(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT
{
    return StringFormat(buffer, bufferSize, "%02d:%02d:%02d", getHour(), getMinute(), getSecond());
}

int Time::get12HourHour() const PRIME_NOEXCEPT
{
    int hour = getHour();
    if (hour == 0) {
        return 12;
    }
    if (hour < 13) {
        return hour;
    }

    return hour - 12;
}

std::string Time::toText(DateTimeLocale locale) const
{
    switch (locale) {
    case DateTimeLocaleUK:
    default:
        return Format("%0d:%02d%s", get12HourHour(), getMinute(), getHour() >= 12 ? "pm" : "am");
    }
}

//
// DateTime
//

bool DateTime::parseISO8601(StringView iso8601, Date& date, Time& time, int& tzoffset, int32_t& nanoseconds, bool withTime) PRIME_NOEXCEPT
{
    return parseInternational(iso8601, DateOrderISO8601, false, date, TimeFormatISO8601, time, tzoffset, nanoseconds, withTime);
}

bool DateTime::parseInternational(StringView string,
    DateOrder dateOrder, int allowShortYears, Date& date,
    TimeFormat timeFormat, Time& time,
    int& tzoffset, int32_t& nanoseconds,
    bool withTime) PRIME_NOEXCEPT
{
    // TODO: support weekday form (YYYY-Www or YYYY-Www-D)
    // TODO: support year form?
    // TODO: support [-+]YYYYY

    // Supporting : as a date separator for historical reasons
    const char* const dateSeparators = dateOrder == DateOrderISO8601 ? "-:" : "-:./";
    const char* const timeSeparators = timeFormat == TimeFormatISO8601 ? ":" : ":.";

    const int* ndigits;
    static const int ndigitsISO8601[8] = { 4, 2, 2, 2, 2, 2, 2, 2 };
    static const int ndigitsYMD[8] = { 4, 2, 2, 2, 2, 2, 2, 2 };
    static const int ndigitsDMYOrMDY[8] = { 2, 2, 4, 2, 2, 2, 2, 2 };
    static const int ndigitsDMYOrMDYOrBestGuess[8] = { 4, 2, 4, 2, 2, 2, 2, 2 };
    switch (dateOrder) {
    case DateOrderDMY:
    case DateOrderMDY:
        ndigits = ndigitsDMYOrMDY;
        break;
    case DateOrderYMD:
        ndigits = ndigitsYMD;
        break;
    case DateOrderISO8601:
    default:
        ndigits = ndigitsISO8601;
        break;
    case DateOrderDMYOrBestGuess:
    case DateOrderMDYOrBestGuess:
    case DateOrderYMDOrBestGuess:
        ndigits = ndigitsDMYOrMDYOrBestGuess;
        break;
    }

    int value[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    const int maxValues = withTime ? 8 : 3;
    int32_t tempNanoseconds = 0;
    int nvalues = 0;
    StringView view = string;
    char tzsign = 0;

    enum { Neither,
        AM,
        PM } ampm
        = Neither;
    int parsedEnglishMonth = -1;

    for (;;) {
        const char* newBegin;

        view = StringViewLeftTrim(view);
        if (view.empty()) {
            break;
        }

        if (nvalues == maxValues) {
            return false;
        }

        if (nvalues == 5) {
            // We're parsing the seconds - allow a fractional part
            double seconds;
            if (!ParseReal(view, newBegin, seconds)) {
                return false;
            }
            value[nvalues] = Narrow<int>(floor(seconds));
            tempNanoseconds = (int32_t)((seconds - floor(seconds)) * 1e9);

        } else {
            if (!ParseInt(view.substr(0, ndigits[nvalues]), newBegin, value[nvalues])) {
                if (parsedEnglishMonth >= 0 || nvalues >= 2 || !Date::parseEnglishMonthName(view, newBegin, value[nvalues])) {
                    return false;
                }

                parsedEnglishMonth = nvalues;
            }
        }

        ++nvalues;

        view = StringViewLeftTrim(StringView(newBegin, view.end()));

        if (nvalues < 3) {
            if (!view.empty() && strchr(dateSeparators, view[0])) {
                view.remove_prefix();
                continue;
            }

            continue;
        }

        if (nvalues == 3) {
            if (view.at(0) == 'T' || view.at(0) == 't') {
                view.remove_prefix();
            }

            continue;
        }

        if (nvalues < 6) {
            if (view.empty()) {
                continue;
            }

            if (strchr(timeSeparators, view[0])) {
                view.remove_prefix();
                continue;
            }

            if (!strchr("+-zZaApP", view[0])) {
                continue;
            }

            // minutes and/or seconds have been skipped
            nvalues = 6;
        }

        if (nvalues == 6) {
            if (view.empty()) {
                continue;
            }

            if (view[0] == 'z' || view[0] == 'Z') {
                view.remove_prefix();
                if (!StringViewLeftTrim(view).empty()) {
                    return false;
                }

                break;
            }

            if (view[0] == 'a' || view[0] == 'A' || view[0] == 'p' || view[0] == 'P') {
                if (view[0] == 'p' || view[0] == 'P') {
                    ampm = PM;
                } else {
                    ampm = AM;
                }

                //                    if (timeFormat == TimeFormatISO8601) {
                //                        return false;
                //                    }
                if (view.size() > 1) {
                    if (view[1] != 'm' && view[1] != 'M') {
                        return false;
                    }
                    view.remove_prefix(2);
                } else {
                    view.remove_prefix();
                }
            }

            if (view[0] == '+' || view[0] == '-') {
                tzsign = view[0];
                view.remove_prefix();
            }

            continue;
        }

        /*if (nvalues < 8)*/ {
            if (!view.empty() && (view[0] == ':' || view[0] == '-')) {
                view.remove_prefix();
            }

            continue;
        }
    }

    if (nvalues < 3) {
        return false;
    }

    Date tempDate;
    bool determinedDateOrder = false;
    bool allowGuess = dateOrder == DateOrderYMDOrBestGuess || dateOrder == DateOrderMDYOrBestGuess || dateOrder == DateOrderDMYOrBestGuess;

    if (allowGuess && parsedEnglishMonth >= 0 && parsedEnglishMonth <= 2) {
        int yearIndex = -1;
        int dayIndex = -1;

        for (int i = 0; i != 3; ++i) {
            if (parsedEnglishMonth != i && value[i] > 31) {
                yearIndex = i;
            }
        }
        if (yearIndex >= 0) {
            for (int i = 0; i != 3; ++i) {
                if (parsedEnglishMonth != i && yearIndex != i) {
                    dayIndex = i;
                }
            }

            if (dayIndex >= 0) {
                tempDate = Date(value[yearIndex], value[parsedEnglishMonth], value[dayIndex]);
                determinedDateOrder = true;
            }
        }
    }

    if (!determinedDateOrder) {
        if (parsedEnglishMonth >= 0) {
            switch (dateOrder) {
            case DateOrderISO8601:
            case DateOrderYMD:
            case DateOrderYMDOrBestGuess:
                if (parsedEnglishMonth != 1) {
                    return false;
                }
                break;
            case DateOrderDMY:
            case DateOrderDMYOrBestGuess:
                if (parsedEnglishMonth != 1) {
                    return false;
                }
                break;
            case DateOrderMDY:
            case DateOrderMDYOrBestGuess:
                if (parsedEnglishMonth != 0) {
                    return false;
                }
                break;
            }
        }

        switch (dateOrder) {
        case DateOrderYMD:
        case DateOrderYMDOrBestGuess:
        case DateOrderISO8601:
            tempDate = Date(value[0], value[1], value[2]);
            break;

        case DateOrderMDYOrBestGuess:
            if (value[0] > 12) {
                tempDate = Date(value[0], value[1], value[2]);
                break;
            }
            // Fall through
        case DateOrderMDY:
            tempDate = Date(value[2], value[0], value[1]);
            break;

        case DateOrderDMYOrBestGuess:
            if (value[0] > 31) {
                tempDate = Date(value[0], value[1], value[2]);
                break;
            }
            // Fall through
        case DateOrderDMY:
            tempDate = Date(value[2], value[1], value[0]);
            break;
        }
    }

    if (allowShortYears >= 0 && tempDate.getYear() < 100) {
        int endYear = DateTime(Clock::getCurrentTime()).getYear() + allowShortYears;
        if (tempDate.getYear() > endYear % 100) {
            // So if end year is 2020, 21 will mean 1921
            tempDate.setYear((endYear / 100 - 1) * 100 + tempDate.getYear());
        } else {
            // So if end year is 2020, 19 will mean 2019
            tempDate.setYear(endYear / 100 * 100 + tempDate.getYear());
        }
    }

    if (!tempDate.isValid()) {
        return false;
    }

    switch (ampm) {
    case Neither:
        if (value[3] < 0 || value[3] > 23) {
            return false;
        }
        break;
    case AM:
        if (value[3] < 1 || value[3] > 12) {
            return false;
        }
        if (value[3] == 12) {
            value[3] = 0;
        }
        break;
    case PM:
        if (value[3] < 1 || value[3] > 12) {
            return false;
        }
        if (value[3] < 12) {
            value[3] += 12;
        }
        break;
    }

    if (value[4] < 0 || value[4] > 59 || value[5] < 0 || value[5] > 59) {
        return false;
    }

    date = tempDate;
    time = Time(value[3], value[4], value[5]);
    nanoseconds = tempNanoseconds;

    tzoffset = (value[6] * 60 + value[7]) * 60;
    if (tzsign == '-') {
        tzoffset = -tzoffset;
    }

    return true;
}

Optional<DateTime> DateTime::parseISO8601(StringView iso8601) PRIME_NOEXCEPT
{
    if (Optional<UnixTime> unixTime = parseISO8601UnixTime(iso8601)) {
        return DateTime(*unixTime);
    }

    return nullopt;
}

Optional<UnixTime> DateTime::parseISO8601UnixTime(StringView iso8601) PRIME_NOEXCEPT
{
    Date date;
    Time time;
    int tzoffset;
    int32_t nanoseconds;

    if (!parseISO8601(iso8601, date, time, tzoffset, nanoseconds, true)) {
        return nullopt;
    }

    int64_t unixDay = date.toUnixDay();
    int64_t secondWithinDay = time.toSecondWithinDay();
    int64_t unixTimeWithoutZone = unixDay * secondsPerDay + secondWithinDay;

    return UnixTime(unixTimeWithoutZone - tzoffset, nanoseconds);
}

DateTime DateTime::fromUnixDay(int64_t unixDay, int hour, int minute, int second, int32_t nanosecond) PRIME_NOEXCEPT
{
    return DateTime(Date::fromUnixDay(unixDay), Time(hour, minute, second, nanosecond));
}

DateTime DateTime::fromUnixTime(int64_t unixTime, int32_t nanosecond) PRIME_NOEXCEPT
{
    int32_t secondWithinDay = (int32_t)PositiveModulus<int64_t>(unixTime, secondsPerDay);

    // Always round to negative
    int64_t unixDay = (unixTime - secondWithinDay) / secondsPerDay;

    int32_t hour = secondWithinDay / (60 * 60);
    int32_t minute = secondWithinDay / 60 % 60;
    int32_t second = secondWithinDay % 60;

    return fromUnixDay(unixDay, hour, minute, second, nanosecond);
}

bool DateTime::parseRFC1123(StringView inputView, Date& date, Time& time, int& tzoffset, bool withTime) PRIME_NOEXCEPT
{
    static const char separators[] = ",:";

    TokenParser parser(inputView);

    StringView unusedToken;

    if (!parser.parse(unusedToken, separators)) {
        return false;
    }

    if (parser.getSeparator() == ',') {
        // We have a weekday. Ignore it.
    } else {
        // Put the first token back.
        parser.putBack();
    }

    char token[64];

    // Parse the day.
    if (!parser.parse(token, separators)) {
        return false;
    }

    int day = -1;
    if (!StringToInt(token, day)) {
        return false;
    }

    // Parse the month.
    if (!parser.parse(token, separators)) {
        return false;
    }

    int month = parseRFC1123MonthName(token);
    if (month < 0) {
        return false;
    }

    // Parse the year.
    if (!parser.parse(token, separators)) {
        return false;
    }

    int year = -1;
    if (!StringToInt(token, year)) {
        return false;
    }

    if (year < 1000) {
        year += 1900;
    }

    date = Date(year, month, day);

    if (!withTime) {
        time = Time();
        tzoffset = 0;
        return StringIsWhitespace(parser.getRemainingString());
    }

    // Parse the hour.
    if (!parser.parse(token, separators)) {
        return false;
    }

    int hour = -1;
    if (!StringToInt(token, hour)) {
        return false;
    }

    if (hour < 0 || hour > 23) {
        return false;
    }

    // Parse the minutes.
    if (!parser.parse(token, separators)) {
        return false;
    }

    int minute = -1;
    if (!StringToInt(token, minute)) {
        return false;
    }

    if (minute < 0 || minute > 59) {
        return false;
    }

    // Parse seconds, if we have them.
    int second = 0;
    if (parser.getSeparator() == ':') {
        if (!parser.parse(token, separators)) {
            return false;
        }

        if (!StringToInt(token, second)) {
            return false;
        }

        if (second < 0 || second > 59) {
            return false;
        }
    }

    time = Time(hour, minute, second);

    // Parse the time zone.
    tzoffset = 0;
    if (parser.parse(token, separators)) {
        if (token[0] == '+' || token[1] == '-') {
            char sign = token[0];

            int hhmm = 0;
            if (StringToInt(token + 1, hhmm)) {
                int hh = hhmm / 100;
                int mm = hhmm % 100;

                tzoffset = hh * 60 + mm;

                if (sign == '-') {
                    tzoffset = -tzoffset;
                }
            }

        } else {
            ASCIIToLowerInPlace(token);

            static const struct {
                const char* name;
                int tzoffset;
            } OFFSETS[] = {
                { "ut", 0 },
                { "gmt", 0 },
                { "est", -5 * 50 },
                { "edt", -4 * 60 },
                { "cst", -6 * 60 },
                { "cdt", -5 * 60 },
                { "mst", -7 * 60 },
                { "mdt", -6 * 60 },
                { "pst", -8 * 60 },
                { "pdt", -7 * 60 },
                { "z", 0 },
                { "a", -1 * 60 },
                { "m", -12 * 60 },
                { "n", 1 * 60 },
                { "y", 12 * 60 },
            };

            for (size_t i = 0; i != COUNTOF(OFFSETS); ++i) {
                if (ASCIIEqualIgnoringCase(OFFSETS[i].name, token)) {
                    tzoffset = OFFSETS[i].tzoffset;
                }
            }
        }
    }

    return StringIsWhitespace(parser.getRemainingString());
}

Optional<DateTime> DateTime::parseRFC1123(StringView rfc1123) PRIME_NOEXCEPT
{
    if (Optional<UnixTime> unixTime = parseRFC1123UnixTime(rfc1123)) {
        return DateTime(*unixTime);
    }

    return nullopt;
}

Optional<UnixTime> DateTime::parseRFC1123UnixTime(StringView rfc1123) PRIME_NOEXCEPT
{
    Date date;
    Time time;
    int tzoffset;

    if (!parseRFC1123(rfc1123, date, time, tzoffset, true)) {
        return nullopt;
    }

    int64_t unixDay = date.toUnixDay();
    int64_t secondWithinDay = time.toSecondWithinDay();
    int64_t unixTimeWithoutZone = unixDay * secondsPerDay + secondWithinDay;

    return UnixTime(unixTimeWithoutZone - tzoffset * 60, 0);
}

Optional<DateTime> DateTime::parse(StringView string, DateOrder dateOrder, TimeFormat timeFormat,
    int allowShortYears) PRIME_NOEXCEPT
{
    if (Optional<UnixTime> unixTime = parseUnixTime(string, dateOrder, timeFormat, allowShortYears)) {
        return DateTime(*unixTime);
    }

    return nullopt;
}

Optional<UnixTime> DateTime::parseUnixTime(StringView string, DateOrder dateOrder, TimeFormat timeFormat,
    int allowShortYears) PRIME_NOEXCEPT
{
    Date date;
    Time time;
    int tzoffset;
    int32_t nanoseconds;

    if (parseInternational(string, dateOrder, allowShortYears, date, timeFormat, time, tzoffset, nanoseconds, true)) {
        int64_t unixDay = date.toUnixDay();
        int64_t secondWithinDay = time.toSecondWithinDay();
        int64_t unixTimeWithoutZone = unixDay * secondsPerDay + secondWithinDay;

        return UnixTime(unixTimeWithoutZone - tzoffset, nanoseconds);
    }

    if (Optional<UnixTime> unixTime = parseISO8601UnixTime(string)) {
        return unixTime;
    }

    if (Optional<UnixTime> unixTime = parseRFC1123UnixTime(string)) {
        return unixTime;
    }

    return nullopt;
}

UnixTime DateTime::toUnixTime() const PRIME_NOEXCEPT
{
    UnixTime::Seconds seconds = toUnixDay() * secondsPerDay + toSecondWithinDay();
    return UnixTime(seconds, getNanosecond());
}

bool DateTime::toISO8601(char* buffer, size_t bufferSize, StringView separator, StringView zone) const PRIME_NOEXCEPT
{
    bool fit = StringFormat(buffer, bufferSize, "%04d-%02d-%02d",
        getYear(), getMonth(), getDay());
    fit = fit && StringAppend(buffer, bufferSize, separator);
    fit = fit && StringAppendFormat(buffer, bufferSize, "%02d:%02d:%02d", getHour(), getMinute(), getSecond());
    fit = fit && StringAppend(buffer, bufferSize, zone);
    return fit;
}

std::string DateTime::toISO8601(StringView separator, StringView zone) const PRIME_NOEXCEPT
{
    char buffer[128];
    if (!toISO8601(buffer, sizeof(buffer), separator, zone)) {
        return "";
    }
    return buffer;
}

bool DateTime::toRFC1123(char* buffer, size_t bufferSize) const PRIME_NOEXCEPT
{
    // Wed, 23 Sep 2015 16:45:52 +0000
    return StringFormat(buffer, bufferSize, "%s, %02d %s %04d %02d:%02d:%02d GMT",
        getRFC1123WeekdayName(), getDay(), getRFC1123MonthName(), getYear(),
        getHour(), getMinute(), getSecond());
}

std::string DateTime::toText(DateTimeLocale locale) const
{
    PRIME_UNUSED(locale);
    return Date::toText(locale) + " at " + Time::toText(locale);
}

//
// Conversions
//

bool StringAppend(std::string& output, const Date& date)
{
    char buffer[128];

    date.toISO8601(buffer, sizeof(buffer));

    output += buffer;
    return true;
}

bool StringAppend(std::string& output, const Time& time)
{
    char buffer[128];

    time.toISO8601(buffer, sizeof(buffer));

    output += buffer;
    return true;
}

bool StringAppend(std::string& output, const UnixTime& unixTime)
{
    char buffer[128];

    DateTime(unixTime).toISO8601(buffer, sizeof(buffer), " ", "");

    output += buffer;
    return true;
}

bool StringAppend(std::string& output, const DateTime& value)
{
    return StringAppend(output, value.toUnixTime());
}

bool UnsafeConvert(Date& output, StringView input) PRIME_NOEXCEPT
{
    if (Optional<Date> date = Date::parse(input)) {
        output = *date;
        return true;
    }

    return false;
}

bool UnsafeConvert(Date& output, const UnixTime& input) PRIME_NOEXCEPT
{
    output = DateTime(input).getDate();
    return true;
}

bool UnsafeConvert(Date& output, const DateTime& input) PRIME_NOEXCEPT
{
    output = input.getDate();
    return true;
}

bool UnsafeConvert(Time& output, const UnixTime& input) PRIME_NOEXCEPT
{
    output = DateTime(input).getTime();
    return true;
}

bool UnsafeConvert(Time& output, const DateTime& input) PRIME_NOEXCEPT
{
    output = input.getTime();
    return true;
}

bool UnsafeConvert(Time& output, StringView input) PRIME_NOEXCEPT
{
    Time time;
    if (!Time::parse(input, time)) {
        return false;
    }

    output = time;
    return true;
}

bool UnsafeConvert(UnixTime& output, StringView input) PRIME_NOEXCEPT
{
    if (Optional<UnixTime> unixTime = DateTime::parseUnixTime(input)) {
        output = *unixTime;
        return true;
    }

    return false;
}

bool UnsafeConvert(DateTime& output, StringView input) PRIME_NOEXCEPT
{
    UnixTime temp;
    if (!UnsafeConvert(temp, input)) {
        return false;
    }

    output = temp;
    return true;
}
}
