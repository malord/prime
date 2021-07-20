// Copyright 2000-2021 Mark H. P. Lord

//
// Removing undesirable characters from HTML:
//
//     Regex("[\r\n]").replaceAllInPlace(plain, "");
//     Regex("< */? *(h1|h2|h3|h4|p|li|div)\\b(?:.|\\n)*?>", Regex::I).replaceAllInPlace(plain, "&newline;");
//     Regex("< */? *(br) *>", Regex::I).replaceAllInPlace(plain, "&newline;");
//     Regex("(&newline;)+", Regex::I).replaceAllInPlace(plain, "\r\n");
//     Regex("<(?:.|\n)*?>").replaceAllInPlace(plain, "");
//     Regex("&nbsp;", Regex::I).replaceAllInPlace(plain, " ");
//     Regex("&gt;", Regex::I).replaceAllInPlace(plain, ">");
//     Regex("&lt;", Regex::I).replaceAllInPlace(plain, "<");
//     Regex("&apos;", Regex::I).replaceAllInPlace(plain, "'");
//     Regex("&quot;", Regex::I).replaceAllInPlace(plain, "\"");
//     Regex("&amp;", Regex::I).replaceAllInPlace(plain, "&");
//     Regex("(\r\n)+$").replaceAllInPlace(plain, "");
//     Regex("^(\r\n)+").replaceAllInPlace(plain, "");
//
// Parsing a date/time:
//
//     Optional<DateTime> ParseLocalDateTime(const std::string& string, bool* hasTime)
//     {
//         static const Regex dateTimeFormat("(\\d+)[-/.](\\d+)[-/.](\\d+)(?: *(\\d+)(?:[:](\\d+)(?:[:](\\d+))?)?)?");
//
//         Regex::Match match;
//         if (! dateTimeFormat.search(match, string)) {
//             return nullopt;
//         }
//
//         int year = ToInt(match.getGroupView(string, 3), -1);
//         int month = ToInt(match.getGroupView(string, 2), -1);
//         int day = ToInt(match.getGroupView(string, 1), -1);
//
//         if (hasTime) {
//             *hasTime = match.getCount() > 4;
//         }
//
//         int hour = match.getCount() > 4 ? ToInt(match.getGroupView(string, 4), 12) : 12;
//         int minute = match.getCount() > 5 ? ToInt(match.getGroupView(string, 5), 0) : 0;
//         int second = match.getCount() > 6 ? ToInt(match.getGroupView(string, 6), 0) : 0;
//
//         return DateTime(year, month, day, hour, minute, second);
//     }

#ifndef PRIME_REGEX_H
#define PRIME_REGEX_H

#include "Oniguruma.h"

#define PRIME_HAVE_REGEX

#ifdef PRIME_HAVE_ONIGURUMA

namespace Prime {
/// Typedef to a class wich can implement regular expressions.
typedef Oniguruma Regex;
}

#else

#undef PRIME_HAVE_REGEX

#endif

#endif
