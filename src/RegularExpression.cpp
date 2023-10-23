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
#include <boost/regex.hpp>
#include "RegularExpression.h"


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

        DocumentIterator operator+(const ptrdiff_t difference   ) const { return DocumentIterator(*this, pos + difference); }
        DocumentIterator operator-(const ptrdiff_t difference   ) const { return DocumentIterator(*this, pos - difference); }
        ptrdiff_t        operator-(const DocumentIterator& other) const { return pos - other.pos; }

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
        return aMatch.empty() || n < 0 || n >= static_cast<int>(aMatch.size()) ? -1 : aMatch[n].second - aMatch[n].first;
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

};


class RegularExpressionW : public RegularExpressionInterface {

    class DocumentIterator {

        friend class RegularExpressionW;

        intptr_t    pos;
        intptr_t    end;
        intptr_t    gap;
        const char* pt1;
        const char* pt2;

        char at(intptr_t cp) const { return cp < gap ? pt1[cp] : pt2[cp]; }
        bool isASCII(char c) const { return (c & 0x80) == 0x00; }
        bool isTrail(char c) const { return (c & 0xC0) == 0x80; }
        bool isLead2(char c) const { return (c & 0xE0) == 0xC0 && (c & 0xFE) != 0xC0; }
        bool isLead3(char c) const { return (c & 0xF0) == 0xE0; }
        bool isLead4(char c) const { return (c & 0xFC) == 0xF0 || c == 0xF4; }
        bool isTrash(char c) const { return (c & 0xFE) == 0xC0 || ((c & 0xF0) == 0xF0 && (c & 0x0C) != 0x00 && c != 0xF4); }

        bool badPair(unsigned char c1, unsigned char c2) const /* checks first two of 3 or 4 byte sequences; does not validate 1 or 2 byte sequences */ {
            return ((c1 == 0xE0 && c2 < 0xA0) || (c1 == 0xED && c2 > 0x9F) || (c1 == 0xF0 && c2 < 0x90) || (c1 == 0xF4 && c2 > 0x8F));
        }

        // Note: Scintilla shows an invalid byte graphic -- an "x" and two hexadecimal digits on an inverted background -- 
        // for each byte in a sequence that cannot be decoded.  Accordingly, instead of following the standard of counting
        // the longest valid prefix as a single error, this iterator regards each byte of an undecodable sequence as an error.
        // The replacement character, 0xFFFD, results from dereferencing an iterator to an error byte.

        // Four-byte sequences encompass two iterator positions.  The first position in on the lead byte; dereferencing it
        // returns the high surrogate.  The second position is on the last byte of the sequence; dereferencing it returns
        // the low surrogate.  Iterators to other valid sequences are always positioned on the lead byte; an iterator position
        // on a continuation byte other than the last byte of a valid four-byte sequence is an error byte.

        // length(p) returns the length of the sequence indexed by p if it is a valid sequence, or 1 if it is not a valid sequence
        // p must not be less than 0 nor greater than or equal to end

        int length(intptr_t p) const {
            unsigned char c1 = at(p);
            if (isASCII(c1)) return 1;
            if (pos + 1 >= end || isTrash(c1)) return 1;
            unsigned char c2 = at(pos + 1);
            if (!isTrail(c2)) return 1;
            if (isLead2(c1)) return 2;
            if (badPair(c1, c2)) return 1;
            if (pos + 2 >= end) return 1;
            unsigned char c3 = at(pos + 2);
            if (!isTrail(c3)) return 1;
            if (isLead3(c1)) return 3;
            if (isLead4(c1)) {
                if (pos + 3 >= end) return 1;
                unsigned char c4 = at(pos + 3);
                if (!isTrail(c4)) return 1;
                return 4;
            }
            return 1;
        }

        // fix_position advances the iterator position if it is on a continuation byte within a valid character
        // so that it points to the start of a valid character, or to an error byte.
        // If this were not done, we could create an iterator that could never return to the same value
        // after being incremented and decremented.

        void fix_position() {
            if (pos > 0 && pos < end && isTrail(at(pos)) && !isASCII(at(pos - 1))) {
                int n = length(pos - 1);
                if (n > 1) pos += n - 1;
                else if (pos > 1) {
                    n = length(pos - 2);
                    if (n > 2) pos += n - 2;
                    else if (pos > 3) {
                        n = length(pos - 3);
                        if (n > 3) ++pos;
                    }
                }
            }
        }

        constexpr static wchar_t replacement_character = 0xFFFD;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = wchar_t;
        using difference_type = ptrdiff_t;
        using pointer = wchar_t*;
        using reference = wchar_t&;

        DocumentIterator() : pos(0), end(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionW*     reba, intptr_t pos) : pos(pos), end(reba->end), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) { fix_position(); }
        DocumentIterator(const DocumentIterator& di  , intptr_t pos) : pos(pos), end(di.end   ), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) { fix_position(); }

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        intptr_t          position() const { return pos; }
        DocumentIterator& position(Scintilla::Position at) { pos = at; return *this; }

        DocumentIterator& operator++() {
            pos += std::min(3, length(pos));
            return *this;
        }

        DocumentIterator& operator--() {
            if      (!isTrail(at(pos - 1))) --pos;
            else if (pos < 2              ) --pos;
            else if (length(pos - 2) == 2 ) pos -= 2;
            else if (pos < 3              ) --pos;
            else if (length(pos - 3) >= 3 ) pos -= 3;
            else --pos;
            return *this;
        }

        wchar_t operator*() const {
            unsigned char c1 = at(pos);
            if (isASCII(c1)) return c1;
            int n = length(pos);
            return n == 2 ? (((c1 & 0x1F) << 6) | (at(pos + 1) & 0x3F))
                 : n == 3 ? (((c1 & 0x0F) << 12) | ((at(pos + 1) & 0x3F) << 6) | (at(pos + 2) & 0x3F))
                 : n == 4 ? 0xD7C0 + (((c1 & 0x07) << 8) | ((at(pos + 1) & 0x3F) << 2) | ((at(pos + 2) & 0x30) >> 4))
                 : pos >= 3 && length(pos - 3) == 4 ? 0xDC00 | ((at(pos - 1) & 0x0F) << 6) | (c1 & 0x3F)
                 : replacement_character;
        }

    };

    friend class DocumentIterator;

    intptr_t    end = 0;
    intptr_t    gap = 0;
    const char* pt1 = 0;
    const char* pt2 = 0;

    ColumnsPlusPlusData&                   data;
    boost::wregex                          wFind;
    boost::match_results<DocumentIterator> wMatch;
    bool                                   regexValid = false;

public:

    RegularExpressionW(ColumnsPlusPlusData& data) : data(data) {}

    bool can_search() const override { return regexValid; }

    std::wstring find(const std::wstring& s, bool caseSensitive = false) override {
        try {
            wFind.assign(s, (caseSensitive ? boost::regex_constants::normal : boost::regex_constants::icase));
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
        return fromWide(wMatch.format(toWide(replacement, CP_UTF8), boost::format_all), CP_UTF8);
    }

    void invalidate() override {
        end = gap = 0;
        pt1 = pt2 = 0;
    }

    intptr_t length(int n = 0) const override {
        return wMatch.empty() || n < 0 || n >= static_cast<int>(wMatch.size()) ? -1 : wMatch[n].second.position() - wMatch[n].first.position();
    }

    size_t mark_count() const override { return !regexValid ? 0 : wFind.mark_count(); }

    intptr_t position(int n = 0) const override { return wMatch.empty() || n < 0 || n >= static_cast<int>(wMatch.size()) ? -1 : wMatch[n].first.position(); }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        end = gap = s.length();
        pt1 = s.data();
        pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, s.length()), wMatch, wFind,
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
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to), wMatch, wFind,
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

    size_t size() const override { return wMatch.size(); }

    std::string str(int n) const override {
        if (wMatch.empty() || n < 0 || n >= static_cast<int>(wMatch.size()) || !wMatch[n].matched) return "";
        Scintilla::Position s1 = wMatch[n].first.position();
        Scintilla::Position s2 = wMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

};


RegularExpression::RegularExpression(ColumnsPlusPlusData& data) {
    if (data.sci.CodePage() == 0) rex = new RegularExpressionA(data);
                             else rex = new RegularExpressionW(data);
    }
