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


class RegularExpressionInterface {
public:
    virtual ~RegularExpressionInterface() {}
    virtual bool         can_search()                                            const = 0;
    virtual std::wstring find(const std::wstring& s, bool caseSensitive = false)       = 0;
    virtual std::string  format(const std::string& replacement)                  const = 0;
    virtual size_t       mark_count()                                            const = 0;
    virtual ptrdiff_t    position(int n = 0)                                     const = 0;
    virtual bool         search(std::string_view s, size_t from = 0)                   = 0;
    virtual size_t       size()                                                  const = 0;
    virtual std::string  str(size_t n)                                           const = 0;
};


class RegularExpressionA : public RegularExpressionInterface {

    boost::regex  aFind;
    boost::cmatch aMatch;
    bool regexValid = false;

public:

    bool can_search() const override { return regexValid; }

    std::wstring find(const std::wstring& s, bool caseSensitive = false) override {
        try { aFind.assign(fromWide(s, 0), caseSensitive ? boost::regex_constants::normal : boost::regex_constants::icase); }
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

    size_t mark_count() const override { return !regexValid ? 0 : aFind.mark_count(); }

    ptrdiff_t position(int n = 0) const override { return aMatch.empty() ? -1 : aMatch.position(n); }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        try {
            return boost::regex_search(s.data() + from, s.data() + s.length(), aMatch, aFind, boost::match_not_dot_newline, s.data());
        }
        catch (...) {}
        return false;
    }

    size_t size() const override { return aMatch.size(); }

    std::string str(size_t n) const override {
        int ni = static_cast<int>(n);
        if (n > aMatch.size() || !aMatch[ni].matched) return "";
        return aMatch[ni];
    }

};


class RegularExpressionW : public RegularExpressionInterface {

    boost::wregex  wFind;
    boost::wcmatch wMatch;
    std::wstring   wideString;
    bool regexValid = false;

public:

    bool can_search() const override { return regexValid; }

    std::wstring find(const std::wstring& s, bool caseSensitive = false) override {
        try { wFind.assign(s, caseSensitive ? boost::regex_constants::normal : boost::regex_constants::icase); }
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

    size_t mark_count() const override { return !regexValid ? 0 : wFind.mark_count(); }

    ptrdiff_t position(int n = 0) const override {
        if (wMatch.empty()) return -1;
        ptrdiff_t m = wMatch.position(n);
        if (m <= 0) return 0;
        std::string pfx = fromWide(std::wstring_view(wideString.data(), m), CP_UTF8);
        return pfx.length();
    }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        try {
            size_t wFrom = 0;
            if (from) {
                wideString = toWide(s.substr(0,from), CP_UTF8);
                wFrom = wideString.length();
                wideString += toWide(s.substr(from), CP_UTF8);
            }
            else wideString = toWide(s, CP_UTF8);
            return boost::regex_search(wideString.c_str() + wFrom, wideString.c_str() + wideString.length(), wMatch, wFind,
                                       boost::match_not_dot_newline, wideString.c_str());
        }
        catch (...) {}
        return false;
    }

    size_t size() const override { return wMatch.size(); }

    std::string str(size_t n) const override {
        int ni = static_cast<int>(n);
        if (n > wMatch.size() || !wMatch[ni].matched) return "";
        return fromWide(std::wstring_view(wMatch[ni].first, wMatch[ni].second), CP_UTF8);
    }

};


class RegularExpression {
    RegularExpressionInterface* rex = 0;
public:
    RegularExpression(ColumnsPlusPlusData& data) {
        if (data.sci.CodePage() == 0) rex = new RegularExpressionA;
                                 else rex = new RegularExpressionW;
    }
    ~RegularExpression() { if (rex) delete rex; }
    bool         can_search()                                                  const { return rex->can_search(); }
    std::wstring find      (const std::wstring& s, bool caseSensitive = false)       { return rex->find(s, caseSensitive); }
    std::string  format    (const std::string& replacement)                    const { return rex->format(replacement); }
    size_t       mark_count()                                                  const { return rex->mark_count(); }
    ptrdiff_t    position  (int n = 0)                                         const { return rex->position(n); }
    bool         search    (std::string_view s, size_t from = 0)                     { return rex->search(s, from); }
    size_t       size      ()                                                  const { return rex->size(); }
    std::string  str       (size_t n)                                          const { return rex->str(n); }
};
