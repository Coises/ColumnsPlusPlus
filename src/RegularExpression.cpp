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

#pragma warning( push )
#pragma warning( disable : 4244 )
#include <boost/regex.hpp>
#pragma warning( pop )


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
    typedef WideTraits::char_class_type char_class_type;
    struct boost_extensions_tag {};

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
        std::wstring w;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 0xffff) return 0;
            w += static_cast<wchar_t>(*p);
        }
        return wideTraits.lookup_classname(w.data(), w.data() + w.length());
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
        if ((c >= 0xd000 && c <= 0xdfff) || c > 0x10FFFF) return false;  // error bytes
        if (c > 0xffff) return class_mask & boost::BOOST_REGEX_DETAIL_NS::char_class_unicode;
        return wideTraits.isctype(static_cast<wchar_t>(c), class_mask);
    }

    int value(char_type c, int radix) const {
        return c > 0xffff ? -1 : wideTraits.value(static_cast<wchar_t>(c), radix);
    }

    locale_type imbue(locale_type l) { return wideTraits.imbue(l); }

    locale_type getloc() const { return wideTraits.getloc(); }

    boost::regex_constants::syntax_type syntax_type(char_type c) const {
        return c > 0xffff ? boost::regex_constants::syntax_char : wideTraits.syntax_type(static_cast<wchar_t>(c));
    }

    boost::regex_constants::escape_syntax_type escape_syntax_type(char_type c) const {
        return c > 0xffff ? boost::regex_constants::escape_type_identity : wideTraits.escape_syntax_type(static_cast<wchar_t>(c));
    }

    char_type translate(char_type c, bool ignoreCase) const {
        if (c > 0xffff) return c;
        return wideTraits.translate(static_cast<wchar_t>(c), ignoreCase);
    }

    intmax_t toi(const char_type*& p1, const char_type* p2, int radix) const {
        std::intmax_t limit = (std::numeric_limits<std::intmax_t>::max)() / radix;
        std::intmax_t next_value = value(*p1, radix);
        if ((p1 == p2) || (next_value < 0) || (next_value >= radix)) return -1;
        std::intmax_t result = 0;
        while (p1 != p2)  {
            next_value = value(*p1, radix);
            if ((next_value < 0) || (next_value >= radix)) break;
            result *= radix;
            result += next_value;
            ++p1;
            if (result > limit) return -1;
        }
        return result;
    }

    std::string error_string(boost::regex_constants::error_type n) const { return wideTraits.error_string(n); }

    char_type tolower(char_type c) const {
        if (c > 0xffff) return c;
        return wideTraits.tolower(static_cast<wchar_t>(c));
    }

    char_type toupper(char_type c) const {
        if (c > 0xffff) return c;
        return wideTraits.toupper(static_cast<wchar_t>(c));
    }

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
