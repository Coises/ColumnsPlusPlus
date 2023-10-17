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


class RegularExpressionBufferedInterface {
public:
    virtual ~RegularExpressionBufferedInterface() {}
    virtual bool         can_search(                                                                           ) const = 0;
    virtual std::wstring find      (const std::wstring& s, bool caseSensitive = false, bool literal = false    )       = 0;
    virtual std::string  format    (const std::string& replacement                                             ) const = 0;
    virtual void         invalidate(                                                                           )       = 0;
    virtual intptr_t     length    (int n = 0                                                                  ) const = 0;
    virtual size_t       mark_count(                                                                           ) const = 0;
    virtual intptr_t     position  (int n = 0                                                                  ) const = 0;
    virtual bool         search    (Scintilla::Position from, Scintilla::Position to, Scintilla::Position start)       = 0;
    virtual size_t       size      (                                                                           ) const = 0;
    virtual std::string  str       (size_t n                                                                   ) const = 0;
};


class RegularExpressionBufferedA : public RegularExpressionBufferedInterface {

    constexpr static intptr_t bufferSize = 32000;  // size of buffer
    constexpr static intptr_t bufferBack =  8000;  // backward space allotted when buffer is filled

    class BufferIterator {

        friend class RegularExpressionBufferedA;

        RegularExpressionBufferedA* reba;
        Scintilla::Position         position;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = char;
        using difference_type   = ptrdiff_t;
        using pointer           = char*;
        using reference         = char&;

        BufferIterator() : reba(0), position(0) {}
        BufferIterator(RegularExpressionBufferedA* reba, Scintilla::Position position = 0) : reba(reba), position(position) {}

        bool operator==(const BufferIterator& other) const { return position == other.position; }
        bool operator!=(const BufferIterator& other) const { return position != other.position; }

        BufferIterator& operator++() { ++position; return *this; }
        BufferIterator& operator--() { --position; return *this; }
        BufferIterator& operator=(const BufferIterator& other) { reba = other.reba; position = other.position; return *this; }

        char operator*() const {
            if (!reba || position < 0) return 0;
            if (reba->bufferPosition < 0 || position - reba->bufferPosition < 0 || position - reba->bufferPosition >= bufferSize) {
                Scintilla::Position end = reba->data.sci.Length();
                if (position >= end) return 0;
                reba->bufferPosition = position > bufferBack ? position - bufferBack : 0;
                Scintilla::TextRangeFull trf;
                trf.chrg.cpMin = reba->bufferPosition;
                trf.chrg.cpMax = std::min(end, reba->bufferPosition + bufferSize);
                trf.lpstrText  = reba->buffer;
                reba->data.sci.GetTextRangeFull(&trf);
            }
            return reba->buffer[position - reba->bufferPosition];
        }

    };

    friend class BufferIterator;

    char                buffer[bufferSize + 1];
    Scintilla::Position bufferPosition = -1;

    ColumnsPlusPlusData&                 data;
    boost::regex                         aFind;
    boost::match_results<BufferIterator> aMatch;
    bool                                 regexValid = false;

public:

    RegularExpressionBufferedA(ColumnsPlusPlusData& data) : data(data) {}

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

    void invalidate() override { bufferPosition = -1; }

    intptr_t length(int n = 0) const override {
        return aMatch.empty() || n < 0 || n >= aMatch.size() ? -1 : aMatch[n].second.position - aMatch[n].first.position;
    }

    size_t mark_count() const override { return !regexValid ? 0 : aFind.mark_count(); }

    intptr_t position(int n = 0) const override { return aMatch.empty() || n < 0 || n >= aMatch.size() ? -1 : aMatch[n].first.position; }

    bool search(Scintilla::Position from, Scintilla::Position to, Scintilla::Position start) override {
        if (!regexValid) return false;
        try {
            return boost::regex_search(BufferIterator(this, from), BufferIterator(this, to),
                                       aMatch, aFind, boost::match_not_dot_newline, BufferIterator(this, start));
        }
        catch (...) {}
        return false;
    }

    size_t size() const override { return aMatch.size(); }

    std::string str(size_t n) const override {
        std::string s;
        int ni = static_cast<int>(n);
        if (n > aMatch.size() || !aMatch[ni].matched) return s;
        for (const auto& c : aMatch[ni]) s += c;
        return s;
    }

};


class RegularExpressionBufferedW : public RegularExpressionBufferedInterface {

    constexpr static intptr_t bufferSize = 32000;  // size of buffer
    constexpr static intptr_t bufferBack =  8000;  // backward space allotted when buffer is filled

    class BufferIterator {

        friend class RegularExpressionBufferedW;

        RegularExpressionBufferedW* reba;
        Scintilla::Position         position;

        void rebuffer() const {
            Scintilla::Position end = reba->data.sci.Length();
            reba->bufferPosition = position > bufferBack ? position - bufferBack : 0;
            Scintilla::TextRangeFull trf;
            trf.chrg.cpMin = reba->bufferPosition;
            trf.chrg.cpMax = std::min(end, reba->bufferPosition + bufferSize);
            trf.lpstrText  = reba->buffer;
            reba->data.sci.GetTextRangeFull(&trf);
        }

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = wchar_t;
        using difference_type   = ptrdiff_t;
        using pointer           = wchar_t*;
        using reference         = wchar_t&;

        BufferIterator() : reba(0), position(0) {}
        BufferIterator(RegularExpressionBufferedW* reba, Scintilla::Position position = 0) : reba(reba), position(position) {}

        bool operator==(const BufferIterator& other) const { return position == other.position; }
        bool operator!=(const BufferIterator& other) const { return position != other.position; }

        BufferIterator& operator++() {
            if (!reba) return *this;
            if (position < 0) {
                position = 0;
                return *this;
            }
            Scintilla::Position end = reba->data.sci.Length();
            if (position >= end) {
                position = end;
                return *this;
            }
            if (reba->bufferPosition < 0 || position - reba->bufferPosition < 0 || position - reba->bufferPosition + 4 >= bufferSize) rebuffer();
            if ((reba->buffer[position - reba->bufferPosition] & 0xF8) == 0xF0) /* positioned on the first of two utf-16 characters */ {
                if (++position < end) ++position;  // position on third of four utf-8 characters; 10xxxxxx character will tell us to render the low surrogate
                return *this;
            }
            intptr_t p = position - reba->bufferPosition;
            while (++p < bufferSize && (reba->buffer[p] & 0xC0) == 0x80);  // find a first byte
            position = p + reba->bufferPosition;
            return *this;
        }

        BufferIterator& operator--() {
            if (!reba) return *this;
            if (position <= 0) {
                position = 0;
                return *this;
            }
            if (reba->bufferPosition < 0 || position - reba->bufferPosition < 4 || position - reba->bufferPosition >= bufferSize) rebuffer();
            intptr_t p = position - reba->bufferPosition;
            while (--p >= 0 && (reba->buffer[p] & 0xC0) == 0x80);  // find a first byte
            position = p < 0                                                                       ? reba->bufferPosition
                     : (reba->buffer[p] & 0xF8) == 0xF0 && p + reba->bufferPosition + 2 < position ? p + reba->bufferPosition + 2
                                                                                                   : p + reba->bufferPosition;
            return *this;
        }

        BufferIterator& operator=(const BufferIterator& other) { reba = other.reba; position = other.position; return *this; }

        wchar_t operator*() const {
            if (!reba || position < 0) return 0;
            if (reba->bufferPosition < 0 || position - reba->bufferPosition < 0 || position - reba->bufferPosition >= bufferSize) rebuffer();
            else {
                char c = reba->buffer[position - reba->bufferPosition];
                if ( ((c & 0xC0) == 0x80 && position - reba->bufferPosition >= bufferSize - 1)
                  || ((c & 0xE0) == 0xC0 && position - reba->bufferPosition >= bufferSize - 1)
                  || ((c & 0xF0) == 0xE0 && position - reba->bufferPosition >= bufferSize - 2)
                  || ((c & 0xF0) == 0xF0 && position - reba->bufferPosition >= bufferSize - 3)) rebuffer();
            }
            intptr_t i = position - reba->bufferPosition;
            wchar_t c = reba->buffer[i];
            if (!(c & 0x80)) return c;
            if ((c & 0xE0) == 0xC0) return (((c & 0x1F) << 6) | (reba->buffer[i + 1] & 0x3F));
            if ((c & 0xF0) == 0xE0) return (((c & 0x0F) << 12) | ((reba->buffer[i + 1] & 0x3F) << 6) | (reba->buffer[i + 2] & 0x3F));
            // Surrogate pair (or invalid utf-8)
            if ((c & 0xC0) == 0x80) return 0xDC00 | ((c & 0x0F) << 6) | (reba->buffer[i + 1] & 0x3F);
            return 0xD800 | ((c & 0x03) << 8) | ((reba->buffer[i + 1] & 0x3F) << 2) | ((reba->buffer[i + 2] & 0x30) >> 4);
        }

    };

    friend class BufferIterator;

    char                buffer[bufferSize + 1];
    Scintilla::Position bufferPosition = -1;

    ColumnsPlusPlusData&                 data;
    boost::wregex                        wFind;
    boost::match_results<BufferIterator> wMatch;
    bool                                 regexValid = false;

public:

    RegularExpressionBufferedW(ColumnsPlusPlusData& data) : data(data) {}

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
    
    void invalidate() override { bufferPosition = -1; }

    intptr_t length(int n = 0) const override {
        return wMatch.empty() || n < 0 || n >= wMatch.size() ? -1 : wMatch[n].second.position - wMatch[n].first.position;
    }

    size_t mark_count() const override { return !regexValid ? 0 : wFind.mark_count(); }

    intptr_t position(int n = 0) const override { return wMatch.empty() || n < 0 || n >= wMatch.size() ? -1 : wMatch[n].first.position; }

    bool search(Scintilla::Position from, Scintilla::Position to, Scintilla::Position start) override {
        if (!regexValid) return false;
        try {
            return boost::regex_search(BufferIterator(this, from), BufferIterator(this, to),
                                       wMatch, wFind, boost::match_not_dot_newline, BufferIterator(this, start));
        }
        catch (...) {}
        return false;
    }

    size_t size() const override { return wMatch.size(); }

    std::string str(size_t n) const override {
        std::wstring s;
        int ni = static_cast<int>(n);
        if (n > wMatch.size() || !wMatch[ni].matched) return "";
        for (const auto& c : wMatch[ni]) s += c;
        return fromWide(s, CP_UTF8);
    }

};


class RegularExpressionBuffered {
    RegularExpressionBufferedInterface* rex = 0;
public:
    RegularExpressionBuffered(ColumnsPlusPlusData& data) {
        if (data.sci.CodePage() == 0) rex = new RegularExpressionBufferedA(data);
                                 else rex = new RegularExpressionBufferedW(data);
    }
    ~RegularExpressionBuffered() { if (rex) delete rex; }
    bool         can_search(                                                                           ) const { return rex->can_search()                   ; }
    std::wstring find      (const std::wstring& s, bool caseSensitive = false, bool literal = false    )       { return rex->find(s, caseSensitive, literal); }
    std::string  format    (const std::string& replacement                                             ) const { return rex->format(replacement)            ; }
    void         invalidate(                                                                           )       {        rex->invalidate()                   ; }
    intptr_t     length    (int n = 0                                                                  ) const { return rex->length(n)                      ; }
    size_t       mark_count(                                                                           ) const { return rex->mark_count()                   ; }
    intptr_t     position  (int n = 0                                                                  ) const { return rex->position(n)                    ; }
    bool         search    (Scintilla::Position from, Scintilla::Position to, Scintilla::Position start)       { return rex->search(from, to, start)        ; }
    size_t       size      (                                                                           ) const { return rex->size()                         ; }
    std::string  str       (size_t n                                                                   ) const { return rex->str(n)                         ; }
};
