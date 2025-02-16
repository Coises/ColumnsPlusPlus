// This file is part of Columns++ for Notepad++.
// Copyright 2023 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "ColumnsPlusPlus.h"
#include "RegularExpression.h"

#include "Unicode\UnicodeCharacterData.h"

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
        template<> inline char32_t global_upper<char32_t>(char32_t c)  {
            return unicodeUpper(c);
        }
    }
}


namespace utf8byte {
    bool isASCII(char c) { return (c & 0x80) == 0x00; }
    bool isTrail(char c) { return (c & 0xC0) == 0x80; }
    bool isLead2(char c) { return (c & 0xE0) == 0xC0 && (c & 0xFE) != 0xC0; }
    bool isLead3(char c) { return (c & 0xF0) == 0xE0; }
    bool isLead4(char c) { return (c & 0xFC) == 0xF0 || c == 0xF4; }
    bool isTrash(char c) { return (c & 0xFE) == 0xC0 || ((c & 0xF0) == 0xF0 && (c & 0x0C) != 0x00 && c != 0xF4); }
    bool badPair(unsigned char c1, unsigned char c2) {
        // checks first two of 3 or 4 byte sequences; does not validate 1 or 2 byte sequences
        return ((c1 == 0xE0 && c2 < 0xA0) || (c1 == 0xED && c2 > 0x9F) || (c1 == 0xF0 && c2 < 0x90) || (c1 == 0xF4 && c2 > 0x8F));
    }
    size_t implicit_length(char c) {
        return isASCII(c) ? 1
             : isLead2(c) ? 2
             : isLead3(c) ? 3
             : isLead4(c) ? 4
                          : 0;
    }
    bool valid_trail(char c1, char c2, char c3)          { return !badPair(c1, c2) && isTrail(c2) && isTrail(c3); }
    bool valid_trail(char c1, char c2, char c3, char c4) { return !badPair(c1, c2) && isTrail(c2) && isTrail(c3) && isTrail(c4); }
    char32_t to32(char c1, char c2)                   { return (((c1 & 0x1F) <<  6) |  (c2 & 0x3F)); }
    char32_t to32(char c1, char c2, char c3)          { return (((c1 & 0x0F) << 12) | ((c2 & 0x3F) <<  6) |  (c3 & 0x3F)); }
    char32_t to32(char c1, char c2, char c3, char c4) { return (((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)); }
}


// Translation between utf-8, utf-16 and utf-32

std::basic_string<char32_t> utf16to32(const std::wstring_view w) {
    std::basic_string<char32_t> u;
    for (size_t i = 0; i < w.length(); ++i) {
        if (w[i] >= 0xD800 && w[i] < 0xDC00 && i + 1 < w.length() && w[i + 1] >= 0xDC00 && w[i + 1] <= 0xDFFF) {
            u += (static_cast<char32_t>(w[i] & 0x7FF) << 10 | (w[i + 1] & 0x03FF)) + 0x10000;
            ++i;
        }
        else u += w[i];
    }
    return u;
}

std::basic_string<char32_t> utf8to32(const std::string_view s) {
    std::basic_string<char32_t> u;
    for (size_t i = 0; i < s.length(); ++i) {
        switch (utf8byte::implicit_length(s[i])) {
        case 1:
            u += s[i];
            continue;
        case 2:
            if (i + 1 >= s.length() || !utf8byte::isTrail(s[i + 1])) break;
            u += utf8byte::to32(s[i], s[i + 1]);
            i += 1;
            continue;
        case 3:
            if (i + 2 >= s.length() || !utf8byte::valid_trail(s[i], s[i + 1], s[i + 2])) break;
            u += utf8byte::to32(s[i], s[i + 1], s[i + 2]);
            i += 2;
            continue;
        case 4:
            if (i + 3 >= s.length() || !utf8byte::valid_trail(s[i], s[i + 1], s[i + 2]), s[i + 3]) break;
            u += utf8byte::to32(s[i], s[i + 1], s[i + 2]);
            i += 3;
            continue;
        }
        u += 0xDC00 + s[i];  // Invalid Unicode code point: encode error byte in the same way as Python surrogateescape
    }
    return u;
}

std::string utf32to8(const std::basic_string_view<char32_t> u) {
    std::string s;
    for (auto c : u) {
        if (c < 0x80) s += static_cast<char>(c);
        else if (c < 0x800) {
            s += static_cast<char>((c >> 6) | 0xC0);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else if (c >= 0xD800 && c <= 0xDFFF) {
            if (c >= 0xDC80 && c <= 0xDCFF) s += static_cast<char>(0xFF & c);
            else s += "\xEF\xBF\xBD";
        }
        else if (c <= 0x10000) {
            s += static_cast<char>((c >> 12) | 0xE0);
            s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else if (c <= 0x110000){
            s += static_cast<char>((c >> 18) | 0xF0);
            s += static_cast<char>(((c >> 12) & 0x3F) | 0x80);
            s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else s += "\xEF\xBF\xBD";
    }
    return s;
}


struct utf32_regex_traits {

    typedef boost::regex_traits<wchar_t> WideTraits;
    WideTraits wideTraits;

    typedef char32_t                    char_type;
    typedef std::size_t                 size_type;
    typedef std::basic_string<char32_t> string_type;
    typedef WideTraits::locale_type     locale_type;
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

    static constexpr char_class_type categoryMasks[] = {
        CatMask_Cn,
        CatMask_Cc | mask_cntrl,
        CatMask_Cf | mask_cntrl,
        CatMask_Co,
        CatMask_Cs,
        CatMask_Ll | mask_graph | mask_word | mask_alpha | mask_lower,
        CatMask_Lm | mask_graph | mask_word | mask_alpha,
        CatMask_Lo | mask_graph | mask_word | mask_alpha,
        CatMask_Lt | mask_graph | mask_word | mask_alpha,
        CatMask_Lu | mask_graph | mask_word | mask_alpha | mask_upper,
        CatMask_Mc | mask_graph,
        CatMask_Me | mask_graph,
        CatMask_Mn | mask_graph,
        CatMask_Nd | mask_graph | mask_word | mask_digit,
        CatMask_Nl | mask_graph,
        CatMask_No | mask_graph,
        CatMask_Pc | mask_graph | mask_punct,
        CatMask_Pd | mask_graph | mask_punct,
        CatMask_Pe | mask_graph | mask_punct,
        CatMask_Pf | mask_graph | mask_punct,
        CatMask_Pi | mask_graph | mask_punct,
        CatMask_Po | mask_graph | mask_punct,
        CatMask_Ps | mask_graph | mask_punct,
        CatMask_Sc | mask_graph | mask_punct,
        CatMask_Sk | mask_graph | mask_punct,
        CatMask_Sm | mask_graph | mask_punct,
        CatMask_So | mask_graph | mask_punct,
        CatMask_Zl | mask_vertical,
        CatMask_Zp | mask_vertical,
        CatMask_Zs | mask_horizontal
    };

    static constexpr char_class_type asciiMasks[] = {
        /* 00 NUL   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 01 SOH   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 02 STX   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 03 ETX   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 04 EOT   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 05 ENQ   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 06 ACK   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 07 BEL   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 08 BS    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 09 HT    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_horizontal,
        /* 0A LF    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
        /* 0B VT    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
        /* 0C FF    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
        /* 0D CR    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
        /* 0E SO    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 0F SI    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 10 DLE   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 11 DC1   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 12 DC2   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 13 DC3   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 14 DC4   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 15 NAK   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 16 SYN   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 17 ETB   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 18 CAN   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 19 EM    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 1A SUB   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 1B ESC   */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 1C FS    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 1D GS    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 1E RS    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 1F US    */ CatMask_Cc | mask_ascii | mask_cntrl,
        /* 20 Space */ CatMask_Zs | mask_ascii | mask_horizontal,
        /* 21 !     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 22 "     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 23 #     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 24 $     */ CatMask_Sc | mask_ascii | mask_graph | mask_punct,
        /* 25 %     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 26 &     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 27 '     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 28 (     */ CatMask_Ps | mask_ascii | mask_graph | mask_punct,
        /* 29 )     */ CatMask_Pe | mask_ascii | mask_graph | mask_punct,
        /* 2A *     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 2B +     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
        /* 2C ,     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 2D -     */ CatMask_Pd | mask_ascii | mask_graph | mask_punct,
        /* 2E .     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 2F /     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 30 0     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 31 1     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 32 2     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 33 3     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 34 4     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 35 5     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 36 6     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 37 7     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 38 8     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 39 9     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
        /* 3A :     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 3B ;     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 3C <     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
        /* 3D =     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
        /* 3E >     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
        /* 3F ?     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 40 @     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 41 A     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
        /* 42 B     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
        /* 43 C     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
        /* 44 D     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
        /* 45 E     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
        /* 46 F     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
        /* 47 G     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 48 H     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 49 I     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 4A J     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 4B K     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 4C L     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 4D M     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 4E N     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 4F O     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 50 P     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 51 Q     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 52 R     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 53 S     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 54 T     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 55 U     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 56 V     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 57 W     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 58 X     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 59 Y     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 5A Z     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
        /* 5B [     */ CatMask_Ps | mask_ascii | mask_graph | mask_punct,
        /* 5C \     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
        /* 5D ]     */ CatMask_Pe | mask_ascii | mask_graph | mask_punct,
        /* 5E ^     */ CatMask_Sk | mask_ascii | mask_graph | mask_punct,
        /* 5F _     */ CatMask_Pc | mask_ascii | mask_graph | mask_punct | mask_word,
        /* 60 `     */ CatMask_Sk | mask_ascii | mask_graph | mask_punct,
        /* 61 a     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
        /* 62 b     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
        /* 63 c     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
        /* 64 d     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
        /* 65 e     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
        /* 66 f     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
        /* 67 g     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 68 h     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 69 i     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 6A j     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 6B k     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 6C l     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 6D m     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 6E n     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 6F o     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 70 p     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 71 q     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 72 r     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 73 s     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 74 t     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 75 u     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 76 v     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 77 w     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 78 x     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 79 y     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 7A z     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
        /* 7B {     */ CatMask_Ps | mask_ascii | mask_graph | mask_punct,
        /* 7C |     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
        /* 7D }     */ CatMask_Pe | mask_ascii | mask_graph | mask_punct,
        /* 7E ~     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
        /* 7F DEL   */ CatMask_Cc | mask_ascii | mask_cntrl,
    };


    utf32_regex_traits() {}

    static size_type length(const char_type* p) { return std::char_traits<char_type>::length(p); }

    char_type translate(char_type c) const { return c > 0xffff ? c : wideTraits.translate(static_cast<wchar_t>(c)); }

    char_type translate_nocase(char_type c) const { return c > 0xffff ? c : wideTraits.translate_nocase(static_cast<wchar_t>(c)); }

    string_type transform(const char_type* p1, const char_type* p2) const {
        string_type s;
        std::wstring w;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 0xffff) {
                if (!w.empty()) {
                    w = wideTraits.transform(w.data(), w.data() + w.length());
                    for (size_t i = 0; i < w.length(); ++i) s += w[i];
                    w.clear();
                }
                s += *p;
            }
            else w += static_cast<wchar_t>(*p);
        }
        if (!w.empty()) {
            w = wideTraits.transform(w.data(), w.data() + w.length());
            for (size_t i = 0; i < w.length(); ++i) s += w[i];
        }
        return s;
    }

    string_type transform_primary(const char_type* p1, const char_type* p2) const {
        string_type s;
        std::wstring w;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 0xffff) {
                if (!w.empty()) {
                    w = wideTraits.transform_primary(w.data(), w.data() + w.length());
                    for (size_t i = 0; i < w.length(); ++i) s += w[i];
                    w.clear();
                }
                s += *p;
            }
            else w += static_cast<wchar_t>(*p);
        }
        if (!w.empty()) {
            w = wideTraits.transform_primary(w.data(), w.data() + w.length());
            for (size_t i = 0; i < w.length(); ++i) s += w[i];
        }
        return s;
    }

    char_class_type lookup_classname(const char_type* p1, const char_type* p2) const {
        std::string name;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 127) return 0;
            name += static_cast<char>(std::tolower(static_cast<char>(*p)));
        }
        static const std::map<std::string, char_class_type> classnames = {
            {"c*", CatMask_Cc | CatMask_Cf | CatMask_Cn | CatMask_Co},
            {"l*", CatMask_Ll | CatMask_Lm | CatMask_Lo | CatMask_Lt | CatMask_Lu},
            {"m*", CatMask_Mc | CatMask_Me | CatMask_Mn},
            {"n*", CatMask_Nd | CatMask_Nl | CatMask_No},
            {"p*", CatMask_Pc | CatMask_Pd | CatMask_Pe | CatMask_Pf | CatMask_Pi | CatMask_Po | CatMask_Ps},
            {"s*", CatMask_Sc | CatMask_Sk | CatMask_Sm | CatMask_So},
            {"z*", CatMask_Zl | CatMask_Zp | CatMask_Zs},
            {"cc", CatMask_Cc},
            {"cf", CatMask_Cf},
            {"cn", CatMask_Cn},
            {"co", CatMask_Co},
            {"ll", CatMask_Ll},
            {"lm", CatMask_Lm},
            {"lo", CatMask_Lo},
            {"lt", CatMask_Lt},
            {"lu", CatMask_Lu},
            {"mc", CatMask_Mc},
            {"me", CatMask_Me},
            {"mn", CatMask_Mn},
            {"nd", CatMask_Nd},
            {"nl", CatMask_Nl},
            {"no", CatMask_No},
            {"pc", CatMask_Pc},
            {"pd", CatMask_Pd},
            {"pe", CatMask_Pe},
            {"pf", CatMask_Pf},
            {"pi", CatMask_Pi},
            {"po", CatMask_Po},
            {"ps", CatMask_Ps},
            {"sc", CatMask_Sc},
            {"sk", CatMask_Sk},
            {"sm", CatMask_Sm},
            {"so", CatMask_So},
            {"zl", CatMask_Zl},
            {"zp", CatMask_Zp},
            {"zs", CatMask_Zs},
            {"ascii"                   , mask_ascii},
            {"any"                     , 0x3fffffff00000000U},
            {"assigned"                , 0x3fffffee00000000U},
            {"other"                   , CatMask_Cc | CatMask_Cf | CatMask_Cn | CatMask_Co},
            {"letter"                  , CatMask_Ll | CatMask_Lm | CatMask_Lo | CatMask_Lt | CatMask_Lu},
            {"mark"                    , CatMask_Mc | CatMask_Me | CatMask_Mn},
            {"number"                  , CatMask_Nd | CatMask_Nl | CatMask_No},
            {"punctuation"             , CatMask_Pc | CatMask_Pd | CatMask_Pe | CatMask_Pf | CatMask_Pi | CatMask_Po | CatMask_Ps},
            {"symbol"                  , CatMask_Sc | CatMask_Sk | CatMask_Sm | CatMask_So},
            {"separator"               , CatMask_Zl | CatMask_Zp | CatMask_Zs},
            {"control"                 , CatMask_Cc},
            {"format"                  , CatMask_Cf},
            {"not assigned"            , CatMask_Cn},
            {"private use"             , CatMask_Co},
            {"invalid"                 , CatMask_Cs},  // No surrogates in UTF-32, but we use some to hold invalid UTF-8 bytes
            {"lowercase letter"        , CatMask_Ll},
            {"modifier letter"         , CatMask_Lm},
            {"other letter"            , CatMask_Lo},
            {"titlecase"               , CatMask_Lt},
            {"uppercase letter"        , CatMask_Lu},
            {"spacing combining mark"  , CatMask_Mc},
            {"enclosing mark"          , CatMask_Me},
            {"non-spacing mark"        , CatMask_Mn},
            {"decimal digit number"    , CatMask_Nd},
            {"letter number"           , CatMask_Nl},
            {"other number"            , CatMask_No},
            {"connector punctuation"   , CatMask_Pc},
            {"dash punctuation"        , CatMask_Pd},
            {"close punctuation"       , CatMask_Pe},
            {"final punctuation"       , CatMask_Pf},
            {"initial punctuation"     , CatMask_Pi},
            {"other punctuation"       , CatMask_Po},
            {"open punctuation"        , CatMask_Ps},
            {"currency symbol"         , CatMask_Sc},
            {"modifier symbol"         , CatMask_Sk},
            {"math symbol"             , CatMask_Sm},
            {"other symbol"            , CatMask_So},
            {"line separator"          , CatMask_Zl},
            {"paragraph separator"     , CatMask_Zp},
            {"space separator"         , CatMask_Zs},
            {"alnum"   , mask_alnum     },
            {"alpha"   , mask_alpha     },
            {"blank"   , mask_blank     },
            {"cntrl"   , mask_cntrl     },
            {"d"       , mask_digit     },
            {"digit"   , mask_digit     },
            {"graph"   , mask_graph     },
            {"h"       , mask_horizontal},
            {"i"       , CatMask_Cs     },
            {"l"       , mask_lower     },
            {"lower"   , mask_lower     },
            {"m"       , CatMask_Mc | CatMask_Me | CatMask_Mn},
            {"o"       , mask_ascii     },
            {"print"   , mask_print     },
            {"punct"   , mask_punct     },
            {"s"       , mask_space     },
            {"space"   , mask_space     },
            {"u"       , mask_upper     },
            {"unicode" , mask_unicode   },
            {"upper"   , mask_upper     },
            {"v"       , mask_vertical  },
            {"w"       , mask_word      },
            {"word"    , mask_word      },
            {"xdigit"  , mask_xdigit    },
            {"y"       , 0x3fffffe600000000U}
        };
        if (classnames.contains(name)) return classnames.at(name);
        return 0;
    }

    string_type lookup_collatename(const char_type* p1, const char_type* p2) const {
        std::wstring w;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 0xffff) return string_type();
            w += static_cast<wchar_t>(*p);
        }
        return utf16to32(wideTraits.lookup_collatename(w.data(), w.data() + w.length()));
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
        return c > 0xffff ? -1 : wideTraits.value(static_cast<wchar_t>(c), radix);
    }

    locale_type imbue(locale_type l) { return wideTraits.imbue(l); }

    locale_type getloc() const { return wideTraits.getloc(); }

};


class RegularExpressionA : public RegularExpressionInterface {

    class DocumentIterator {

        intptr_t    pos;
        intptr_t    gap;
        const char* pt1;
        const char* pt2;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = char;
        using difference_type = ptrdiff_t;
        using pointer = char*;
        using reference = char&;

        DocumentIterator() : pos(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionA*     reba, intptr_t pos) : pos(pos), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) {}
        DocumentIterator(const DocumentIterator& di  , intptr_t pos) : pos(pos), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) {}

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        DocumentIterator& operator++() { ++pos; return *this; }
        DocumentIterator& operator--() { --pos; return *this; }

        char operator*() const { return pos < gap ? pt1[pos] : pt2[pos]; }

        intptr_t          position(           ) const { return pos; }
        DocumentIterator& position(intptr_t at)       { pos = at; return *this; }

    };

    friend class DocumentIterator;

    intptr_t    end = 0;
    intptr_t    gap = 0;
    const char* pt1 = 0;
    const char* pt2 = 0;

    ColumnsPlusPlusData&                   data;
    boost::regex                           aFind;
    boost::match_results<DocumentIterator> aMatch;
    bool                                   regexValid = false;

public:

    RegularExpressionA(ColumnsPlusPlusData& data) : data(data) {}

    bool can_search() const override { return regexValid; }

    std::wstring find(const std::wstring& s, bool caseSensitive = false) override {
        try {
            aFind.assign(fromWide(s, 0), (caseSensitive ? boost::regex_constants::normal  : boost::regex_constants::icase));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            return toWide(e.what(), 0);
        }
        catch (...) {
            regexValid = false;
            return L"Undetermined error processing this regular expression.";
        }
        regexValid = true;
        return L"";
    }

    std::string format(const std::string& replacement) const override { return aMatch.format(replacement, boost::format_all); }

    void invalidate() override {
        end = gap = 0;
        pt1 = pt2 = 0;
    }

    intptr_t length(int n = 0) const override {
        return aMatch.empty() || n < 0 || n >= static_cast<int>(aMatch.size()) ? -1 : aMatch[n].second.position() - aMatch[n].first.position();
    }

    size_t mark_count() const override { return !regexValid ? 0 : aFind.mark_count(); }

    intptr_t position(int n = 0) const override { return aMatch.empty() || n < 0 || n >= static_cast<int>(aMatch.size()) ? -1 : aMatch[n].first.position(); }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        end = gap = s.length();
        pt1 = s.data();
        pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, s.length()), aMatch, aFind,
                boost::match_not_dot_newline, DocumentIterator(this, 0));
        }
        catch (const boost::regex_error& e) {
            MessageBox(0, toWide(e.what(), 0).data(), L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    bool search(intptr_t from, intptr_t to, intptr_t start) override {
        if (!regexValid) return false;
        if (pt1 == 0 && pt2 == 0) {
            end = data.sci.Length();
            gap = data.sci.GapPosition();
            pt1 = gap > 0 ? reinterpret_cast<const char*>(data.sci.RangePointer(0, gap)) : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(data.sci.RangePointer(gap, end - gap)) - gap : 0;
        }
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to), aMatch, aFind,
                boost::match_not_dot_newline, DocumentIterator(this, start));
        }
        catch (const boost::regex_error& e) {
            MessageBox(0, toWide(e.what(), 0).data(), L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    size_t size() const override { return aMatch.size(); }

    std::string str(int n) const override {
        if (aMatch.empty() || n < 0 || n >= static_cast<int>(aMatch.size()) || !aMatch[n].matched) return "";
        Scintilla::Position s1 = aMatch[n].first.position();
        Scintilla::Position s2 = aMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::string str(std::string_view n) const override {
        if (aMatch.empty() || n.empty()) return "";
        auto x = aMatch[n.data()];
        if (!x.matched) return "";
        Scintilla::Position s1 = x.first.position();
        Scintilla::Position s2 = x.second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

};


class RegularExpressionU : public RegularExpressionInterface {

    class DocumentIterator {

        intptr_t    pos;
        intptr_t    end;
        intptr_t    gap;
        const char* pt1;
        const char* pt2;

        char at(intptr_t cp) const { return cp < gap ? pt1[cp] : pt2[cp]; }

        // Note: Scintilla shows an invalid byte graphic -- an "x" and two hexadecimal digits on an inverted background -- 
        // for each byte in a sequence that cannot be decoded.  Accordingly, instead of following the standard of counting
        // the longest valid prefix as a single error, this iterator regards each byte of an undecodable sequence as an error.
        // An invalid Unicode code point, 0xDC00 + the error byte (which must be between 0x80 and 0xFF), results
        // from dereferencing an iterator to an error byte. This is the same encoding as Python surrogateescape.

        // length(p) returns the length of the sequence indexed by p if it is a valid sequence, or 1 if it is not a valid sequence
        // p must not be less than 0 nor greater than or equal to end

        int length(intptr_t p) const {
            const unsigned char c1 = at(p);
            intptr_t n = utf8byte::implicit_length(c1);
            if (n < 2 || p + n > end) return 1;
            const unsigned char c2 = at(p + 1);
            if (!utf8byte::isTrail(c2)) return 1;
            if (n == 2) return 2;
            if (utf8byte::badPair(c1, c2)) return 1;
            if (!utf8byte::isTrail(at(p + 2))) return 1;
            if (n == 3) return 3;
            return utf8byte::isTrail(at(p + 3)) ? 4 : 1;
        }

        // fix_position advances the iterator position if it is on a continuation byte within a valid character
        // so that it points to the start of a valid character or to an error byte.
        // If this were not done, we could create an iterator that could never return to the same value
        // after being incremented and decremented, which can break the regular expression algorithm.

        void fix_position() {
            if (pos <= 0 || pos >= end || !utf8byte::isTrail(at(pos)) || utf8byte::isASCII(at(pos - 1))) return;
            int n = length(pos - 1);
            if (n > 1) pos += n - 1;
            else if (pos > 1) {
                n = length(pos - 2);
                if (n > 2) pos += n - 2;
                else if (pos > 2) {
                    n = length(pos - 3);
                    if (n > 3) pos += n - 3;
                }
            }
        }

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = char32_t;
        using difference_type   = ptrdiff_t;
        using pointer           = char32_t*;
        using reference         = char32_t&;

        DocumentIterator() : pos(0), end(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionU*     reba, intptr_t pos) : pos(pos), end(reba->end), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) { fix_position(); }
        DocumentIterator(const DocumentIterator& di  , intptr_t pos) : pos(pos), end(di.end   ), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) { fix_position(); }

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        intptr_t          position() const { return pos; }
        DocumentIterator& position(Scintilla::Position at) { pos = at; return *this; }

        DocumentIterator& operator++() {
            pos += length(pos);
            return *this;
        }

        DocumentIterator& operator--() {
            if      (!utf8byte::isTrail(at(pos - 1))) --pos;
            else if (pos < 2              ) --pos;
            else if (length(pos - 2) == 2 ) pos -= 2;
            else if (pos < 3              ) --pos;
            else if (length(pos - 3) == 3 ) pos -= 3;
            else if (pos < 4              ) --pos;
            else if (length(pos - 4) == 4 ) pos -= 4;
            else --pos;
            return *this;
        }

        char32_t operator*() const {
            unsigned char c1 = at(pos);
            if (utf8byte::isASCII(c1)) return c1;
            int n = length(pos);
            return n == 2 ? utf8byte::to32(c1, at(pos + 1))
                 : n == 3 ? utf8byte::to32(c1, at(pos + 1), at(pos + 2))
                 : n == 4 ? utf8byte::to32(c1, at(pos + 1), at(pos + 2), at(pos + 3))
                 : 0xDC00 + c1;  /* Invalid Unicode code point: encode error byte in the same way as Python surrogateescape */
        }

    };

    friend class DocumentIterator;

    intptr_t    end = 0;
    intptr_t    gap = 0;
    const char* pt1 = 0;
    const char* pt2 = 0;

    ColumnsPlusPlusData&                             data;
    boost::basic_regex<char32_t, utf32_regex_traits> uFind;
    boost::match_results<DocumentIterator>           uMatch;
    bool                                             regexValid = false;

public:

    RegularExpressionU(ColumnsPlusPlusData& data) : data(data) {}

    bool can_search() const override { return regexValid; }

    std::wstring find(const std::wstring& s, bool caseSensitive = false) override {
        try {
            uFind.assign(utf16to32(s), (caseSensitive ? boost::regex_constants::normal : boost::regex_constants::icase));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            return toWide(e.what(), CP_UTF8);
        }
        catch (...) {
            regexValid = false;
            return L"Undetermined error processing this regular expression.";
        }
        regexValid = true;
        return L"";
    }

    std::string format(const std::string& replacement) const override {
        return utf32to8(uMatch.format(utf8to32(replacement), boost::format_all));
    }

    void invalidate() override {
        end = gap = 0;
        pt1 = pt2 = 0;
    }

    intptr_t length(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].second.position() - uMatch[n].first.position();
    }

    size_t mark_count() const override { return !regexValid ? 0 : uFind.mark_count(); }

    intptr_t position(int n = 0) const override { return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].first.position(); }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        end = gap = s.length();
        pt1 = s.data();
        pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, s.length()), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, 0));
        }
        catch (const boost::regex_error& e) {
            MessageBox(0, toWide(e.what(), 0).data(), L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    bool search(intptr_t from, intptr_t to, intptr_t start) override {
        if (!regexValid) return false;
        if (pt1 == 0 && pt2 == 0) {
            end = data.sci.Length();
            gap = data.sci.GapPosition();
            pt1 = gap > 0   ? reinterpret_cast<const char*>(data.sci.RangePointer(0  , gap      ))       : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(data.sci.RangePointer(gap, end - gap)) - gap : 0;
        }
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, start));
        }
        catch (const boost::regex_error& e) {
            MessageBox(0, toWide(e.what(), 0).data(), L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Columns++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    size_t size() const override { return uMatch.size(); }

    std::string str(int n) const override {
        if (uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) || !uMatch[n].matched) return "";
        Scintilla::Position s1 = uMatch[n].first.position();
        Scintilla::Position s2 = uMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::string str(std::string_view n) const override {
        if (uMatch.empty() || n.empty()) return "";
        auto x = uMatch[n.data()];
        if (!x.matched) return "";
        Scintilla::Position s1 = uMatch[n.data()].first.position();
        Scintilla::Position s2 = uMatch[n.data()].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

};

RegularExpression::RegularExpression(ColumnsPlusPlusData& data) {
    if (data.sci.CodePage() == 0) rex = new RegularExpressionA(data);
                             else rex = new RegularExpressionU(data);
    }
