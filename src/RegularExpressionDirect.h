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


class RegularExpressionDirectInterface {
public:
    virtual ~RegularExpressionDirectInterface() {}
    virtual bool                can_search(                                                                           ) const = 0;
    virtual std::wstring        find      (const std::wstring& s, bool caseSensitive = false, bool literal = false    )       = 0;
    virtual std::string         format    (const std::string& replacement                                             ) const = 0;
    virtual void                invalidate(                                                                           )       = 0;
    virtual intptr_t            length    (int n = 0                                                                  ) const = 0;
    virtual size_t              mark_count(                                                                           ) const = 0;
    virtual Scintilla::Position position  (int n = 0                                                                  ) const = 0;
    virtual bool                search    (Scintilla::Position from, Scintilla::Position to, Scintilla::Position start)       = 0;
    virtual size_t              size      (                                                                           ) const = 0;
    virtual std::string         str       (int n = 0                                                                  ) const = 0;
};


class RegularExpressionDirectA : public RegularExpressionDirectInterface {

    class DocumentIterator {

        Scintilla::Position pos;
        Scintilla::Position gap;
        const char*         pt1;
        const char*         pt2;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = char;
        using difference_type   = ptrdiff_t;
        using pointer           = char*;
        using reference         = char&;

        DocumentIterator() : pos(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionDirectA* reba, Scintilla::Position pos) : pos(pos), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) {}
        DocumentIterator(const DocumentIterator& di    , Scintilla::Position pos) : pos(pos), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) {}

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        DocumentIterator& operator++() { ++pos; return *this; }
        DocumentIterator& operator--() { --pos; return *this; }

        char operator*() const { return pos < gap ? pt1[pos] : pt2[pos]; }

        DocumentIterator operator+(const ptrdiff_t difference)    const { return DocumentIterator(*this, pos + difference); }
        DocumentIterator operator-(const ptrdiff_t difference)    const { return DocumentIterator(*this, pos - difference); }
        ptrdiff_t        operator-(const DocumentIterator& other) const { return pos - other.pos; }

        Scintilla::Position position(                      ) const { return pos; }
        DocumentIterator&   position(Scintilla::Position at)       { pos = at; return *this; }

    };

    friend class DocumentIterator;

    Scintilla::Position end = 0;
    Scintilla::Position gap = 0;
    const char*         pt1 = 0;
    const char*         pt2 = 0;

    ColumnsPlusPlusData&                   data;
    boost::regex                           aFind;
    boost::match_results<DocumentIterator> aMatch;
    bool                                   regexValid = false;

public:

    RegularExpressionDirectA(ColumnsPlusPlusData& data) : data(data) {}

    bool can_search() const override { return regexValid; }

    std::wstring find(const std::wstring& s, bool caseSensitive = false, bool literal = false) override {
        try {
            aFind.assign(fromWide(s, 0), (caseSensitive ? boost::regex_constants::normal  : boost::regex_constants::icase )
                                       | (literal       ? boost::regex_constants::literal : boost::regex_constants::normal));
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
        return aMatch.empty() || n < 0 || n >= aMatch.size() ? -1 : aMatch[n].second - aMatch[n].first;
    }

    size_t mark_count() const override { return !regexValid ? 0 : aFind.mark_count(); }

    intptr_t position(int n = 0) const override { return aMatch.empty() || n < 0 || n >= aMatch.size() ? -1 : aMatch[n].first.position(); }

    bool search(Scintilla::Position from, Scintilla::Position to, Scintilla::Position start) override {
        if (!regexValid) return false;
        if (pt1 == 0 && pt2 == 0) {
            end = data.sci.Length();
            gap = data.sci.GapPosition();
            pt1 = gap > 0   ? reinterpret_cast<const char*>(data.sci.RangePointer(0  , gap      ))       : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(data.sci.RangePointer(gap, end - gap)) - gap : 0;
        }
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to),
                                       aMatch, aFind, boost::match_not_dot_newline, DocumentIterator(this, start));
        }
        catch (...) {}
        return false;
    }

    size_t size() const override { return aMatch.size(); }

    std::string str(int n) const override {
        if (aMatch.empty() || n < 0 || n > aMatch.size() || !aMatch[n].matched) return "";
        Scintilla::Position s1 = aMatch[n].first.position();
        Scintilla::Position s2 = aMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

};


class RegularExpressionDirectW : public RegularExpressionDirectInterface {

    class DocumentIterator {

        friend class RegularExpressionDirectW;

        Scintilla::Position pos;
        Scintilla::Position end;
        Scintilla::Position gap;
        const char* pt1;
        const char* pt2;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = wchar_t;
        using difference_type   = ptrdiff_t;
        using pointer           = wchar_t*;
        using reference         = wchar_t&;

        DocumentIterator() : pos(0), end(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionDirectW* reba, Scintilla::Position pos) : pos(pos), end(reba->end), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) {}
        DocumentIterator(const DocumentIterator& di    , Scintilla::Position pos) : pos(pos), end(di.end   ), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) {}

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        DocumentIterator operator+(const ptrdiff_t difference)    const { return DocumentIterator(*this, pos + difference); }
        DocumentIterator operator-(const ptrdiff_t difference)    const { return DocumentIterator(*this, pos - difference); }
        ptrdiff_t        operator-(const DocumentIterator& other) const { return pos - other.pos; }

        Scintilla::Position position(                      ) const { return pos; }
        DocumentIterator&   position(Scintilla::Position at)       { pos = at; return *this; }

        DocumentIterator& operator++() {
            if (((pos < gap ? pt1[pos] : pt2[pos]) & 0xF0) == 0xF0) pos += 2;
            else {
                ++pos;
                while (pos < end && ((pos < gap ? pt1[pos] : pt2[pos]) & 0xC0) == 0x80) ++pos;
            }
            return *this;
        }

        DocumentIterator& operator--() {
            Scintilla::Position p = pos - 1;
            while (p > 0 && ((p < gap ? pt1[p] : pt2[p]) & 0xC0) == 0x80) --pos;
            pos = ((p < gap ? pt1[p] : pt2[p]) & 0xF0) == 0xF0 && ((pos < gap ? pt1[pos] : pt2[pos]) & 0xC0) != 0x80 ? p + 2 : p;
            return *this;
        }

        wchar_t operator*() const {
            char c1 = pos < gap ? pt1[pos] : pt2[pos];
            if ((c1 & 0x80) == 0x00 || pos + 1 >= end) return c1;
            char c2 = pos + 1 < gap ? pt1[pos + 1] : pt2[pos + 1];
            if ((c1 & 0xC0) == 0x80) /* either low surrogate or bad utf-8 */ return 0xDC00 | ((c1 & 0x0F) << 6) | (c2 & 0x3F);
            else {
                if ((c1 & 0xE0) == 0xC0 || pos + 2 >= end) return (((c1 & 0x1F) << 6) | (c2 & 0x3F));
                char c3 = pos + 2 < gap ? pt1[pos + 2] : pt2[pos + 2];
                if ((c1 & 0xF0) == 0xE0 || pos + 3 >= end) return (((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
                return 0xD800 | ((c1 & 0x03) << 8) | ((c2 & 0x3F) << 2) | ((c3 & 0x30) >> 4);
            }
        }
    };

    friend class DocumentIterator;

    Scintilla::Position end = 0;
    Scintilla::Position gap = 0;
    const char*         pt1 = 0;
    const char*         pt2 = 0;

    ColumnsPlusPlusData&                   data;
    boost::wregex                          wFind;
    boost::match_results<DocumentIterator> wMatch;
    bool                                   regexValid = false;

public:

    RegularExpressionDirectW(ColumnsPlusPlusData& data) : data(data) {}

    bool can_search() const override { return regexValid; }

    std::wstring find(const std::wstring& s, bool caseSensitive = false, bool literal = false) override {
        try {
            wFind.assign(s, (caseSensitive ? boost::regex_constants::normal  : boost::regex_constants::icase )
                          | (literal       ? boost::regex_constants::literal : boost::regex_constants::normal));
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
        return wMatch.empty() || n < 0 || n >= wMatch.size() ? -1 : wMatch[n].second.position() - wMatch[n].first.position();
    }

    size_t mark_count() const override { return !regexValid ? 0 : wFind.mark_count(); }

    intptr_t position(int n = 0) const override { return wMatch.empty() || n < 0 || n >= wMatch.size() ? -1 : wMatch[n].first.position(); }

    bool search(Scintilla::Position from, Scintilla::Position to, Scintilla::Position start) override {
        if (!regexValid) return false;
        if (pt1 == 0 && pt2 == 0) {
            end = data.sci.Length();
            gap = data.sci.GapPosition();
            pt1 = gap > 0   ? reinterpret_cast<const char*>(data.sci.RangePointer(0  , gap      ))       : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(data.sci.RangePointer(gap, end - gap)) - gap : 0;
        }
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to),
                                       wMatch, wFind, boost::match_not_dot_newline, DocumentIterator(this, start));
        }
        catch (...) {}
        return false;
    }

    size_t size() const override { return wMatch.size(); }

    std::string str(int n) const override {
        if (wMatch.empty() || n < 0 || n > wMatch.size() || !wMatch[n].matched) return "";
        Scintilla::Position s1 = wMatch[n].first.position();
        Scintilla::Position s2 = wMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

};


class RegularExpressionDirect {
    RegularExpressionDirectInterface* rex = 0;
public:
    RegularExpressionDirect(ColumnsPlusPlusData& data) {
        if (data.sci.CodePage() == 0) rex = new RegularExpressionDirectA(data);
                                 else rex = new RegularExpressionDirectW(data);
    }
    ~RegularExpressionDirect() { if (rex) delete rex; }
    bool                can_search(                                                                           ) const { return rex->can_search()                   ; }
    std::wstring        find      (const std::wstring& s, bool caseSensitive = false, bool literal = false    )       { return rex->find(s, caseSensitive, literal); }
    std::string         format    (const std::string& replacement                                             ) const { return rex->format(replacement)            ; }
    void                invalidate(                                                                           )       {        rex->invalidate()                   ; }
    intptr_t            length    (int n = 0                                                                  ) const { return rex->length(n)                      ; }
    size_t              mark_count(                                                                           ) const { return rex->mark_count()                   ; }
    Scintilla::Position position  (int n = 0                                                                  ) const { return rex->position(n)                    ; }
    bool                search    (Scintilla::Position from, Scintilla::Position to, Scintilla::Position start)       { return rex->search(from, to, start)        ; }
    size_t              size      (                                                                           ) const { return rex->size()                         ; }
    std::string         str       (int n                                                                      ) const { return rex->str(n)                         ; }
};
