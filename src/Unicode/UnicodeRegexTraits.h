#pragma once

#include <windows.h>
#include <map>
#include <set>
#include <string>
#include "UnicodeCharacterData.h"

#pragma warning( push )
#pragma warning( disable : 4244 )
#include <boost/regex.hpp>
#pragma warning( pop )

namespace boost {
    namespace BOOST_REGEX_DETAIL_NS {
        template<> inline bool is_combining<char32_t>(char32_t c) {
            return ((1Ui64 << (unicodeGenCat(c) + 32)) & (CatMask_Mc | CatMask_Me | CatMask_Mn));
        }
        template<> inline bool is_separator<char32_t>(char32_t c) {
            return c == 0x0A || c == 0x0D;
        }
        template<> inline char32_t global_lower<char32_t>(char32_t c) {
            return unicodeLower(c);
        }
        template<> inline char32_t global_upper<char32_t>(char32_t c) {
            return unicodeUpper(c);
        }
    }
}


struct utf32_regex_traits {

    typedef char32_t                    char_type;
    typedef std::size_t                 size_type;
    typedef std::basic_string<char32_t> string_type;
    typedef std::wstring                locale_type;
    typedef uint64_t                    char_class_type;

    static constexpr char_class_type mask_upper      = 0x0001;  // upper case
    static constexpr char_class_type mask_lower      = 0x0002;  // lower case
    static constexpr char_class_type mask_digit      = 0x0004;  // decimal digits
    static constexpr char_class_type mask_punct      = 0x0008;  // punctuation characters
    static constexpr char_class_type mask_cntrl      = 0x0010;  // control characters
    static constexpr char_class_type mask_horizontal = 0x0020;  // horizontal space
    static constexpr char_class_type mask_vertical   = 0x0040;  // vertical space
    static constexpr char_class_type mask_xdigit     = 0x0080;  // hexadecimal digits
    static constexpr char_class_type mask_alpha      = 0x0100;  // any linguistic character
    static constexpr char_class_type mask_word       = 0x0200;  // word characters (alpha, number and underscore)
    static constexpr char_class_type mask_graph      = 0x0400;  // any visible character
    static constexpr char_class_type mask_ascii      = 0x0800;  // code points < 128
    static constexpr char_class_type mask_unicode    = 0x1000;  // code points > 255

    static constexpr char_class_type mask_blank = mask_horizontal;
    static constexpr char_class_type mask_space = mask_horizontal | mask_vertical;
    static constexpr char_class_type mask_alnum = mask_alpha | mask_digit;
    static constexpr char_class_type mask_print = mask_graph | mask_space;

    static const char_class_type categoryMasks[];
    static const char_class_type asciiMasks[];
    static const std::map<std::string, char_class_type> classnames;
    static const std::map<std::string, char_type> character_names;
    static const std::set<string_type> digraphs;

    locale_type locale;

    utf32_regex_traits() {
        locale.resize(LOCALE_NAME_MAX_LENGTH);
        int n = GetUserDefaultLocaleName(locale.data(), LOCALE_NAME_MAX_LENGTH);
        locale.resize(n - 1);
    }

    static size_type length(const char_type* p) { return std::char_traits<char_type>::length(p); }

    char_type translate(char_type c) const { return c; }

    char_type translate_nocase(char_type c) const { return unicodeFold(c); }

    string_type transform(const char_type* p1, const char_type* p2) const {
        if (p1 == p2) return string_type();
        std::wstring wc;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 0xffff) {
                wc += static_cast<wchar_t>(0xD800 | (*p >> 10));
                wc += static_cast<wchar_t>(0xDC00 | (*p & 0x3ff));
            }
            else wc += static_cast<wchar_t>(*p);
        }
        int n = LCMapStringEx(locale.data(), LCMAP_SORTKEY | NORM_LINGUISTIC_CASING,
            wc.data(), static_cast<int>(wc.length()), 0, 0, 0, 0, 0);
        std::wstring wt(n, 0);
        LCMapStringEx(locale.data(), LCMAP_SORTKEY | NORM_LINGUISTIC_CASING,
            wc.data(), static_cast<int>(wc.length()), wt.data(), n, 0, 0, 0);
        string_type st;
        for (wchar_t c : wt) {
            if (!c) break;
            st += c;
        }
        return st;
    }

    string_type transform_primary(const char_type* p1, const char_type* p2) const {
        if (p1 == p2) return string_type();
        std::wstring wc;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 0xffff) {
                wc += static_cast<wchar_t>(0xD800 | (*p >> 10));
                wc += static_cast<wchar_t>(0xDC00 | (*p & 0x3ff));
            }
            else wc += static_cast<wchar_t>(*p);
        }
        int n = LCMapStringEx(locale.data(),
            LCMAP_SORTKEY | LINGUISTIC_IGNOREDIACRITIC | NORM_IGNORECASE | NORM_IGNOREKANATYPE
            | NORM_IGNOREWIDTH | NORM_LINGUISTIC_CASING,
            wc.data(), static_cast<int>(wc.length()), 0, 0, 0, 0, 0);
        std::wstring wt(n, 0);
        LCMapStringEx(locale.data(),
            LCMAP_SORTKEY | LINGUISTIC_IGNOREDIACRITIC | NORM_IGNORECASE | NORM_IGNOREKANATYPE
            | NORM_IGNOREWIDTH | NORM_LINGUISTIC_CASING,
            wc.data(), static_cast<int>(wc.length()), wt.data(), n, 0, 0, 0);
        string_type st;
        for (wchar_t c : wt) {
            if (!c) break;
            st += c;
        }
        return st;
    }

    char_class_type lookup_classname(const char_type* p1, const char_type* p2) const {
        std::string name;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 127) return 0;
            name += static_cast<char>(std::tolower(static_cast<char>(*p)));
        }
        if (classnames.contains(name)) return classnames.at(name);
        return 0;
    }

    string_type lookup_collatename(const char_type* p1, const char_type* p2) const {
        if (p2 - p1 < 2) return string_type(p1, p2);
        std::string s;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 127) return string_type();
            s += static_cast<char>(std::tolower(static_cast<char>(*p)));
        }
        if (character_names.contains(s)) return string_type(1, character_names.at(s));
        if (p2 - p1 != 2) return string_type();
        string_type digraph(p1, p2);
        return digraphs.contains(digraph) ? digraph : string_type();
    }

    bool isctype(char_type c, char_class_type class_mask) const {
        if (c < 128) return class_mask & asciiMasks[c];
        if (c < 256) {
            if (c == 0x85) return (class_mask & (CatMask_Cc | mask_cntrl | mask_vertical));
        }
        else if (class_mask & mask_unicode) return true;
        return class_mask & categoryMasks[unicodeGenCat(c)];
    }

    int value(char_type c, int radix) const {
        int n = c <  '0' ? -1
              : c <= '9' ? c - '0'
              : c <  'A' ? -1
              : c <= 'F' ? c - ('A' - 10)
              : c <  'a' ? -1
              : c <= 'f' ? c - ('a' - 10)
              : -1;
        return n < radix ? n : -1;
    }

    locale_type imbue(locale_type l) {
        std::wstring last = locale;
        locale = l;
        return last;
    }

    locale_type getloc() const {
        return locale;
    }

};
