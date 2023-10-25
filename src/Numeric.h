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
#include "commctrl.h"
#include "resource.h"

#pragma warning (push)
#pragma warning (disable: 4702)
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#include "exprtk/exprtk.hpp"
#pragma warning (pop)

class CalculateHistory {
public:

    class LineCache {
    public:
        std::vector<double> col;
        std::vector<double> reg;
        std::vector<double> tab;
        std::string text;
        double result;
        bool colValid    = false;
        bool regValid    = false;
        bool tabValid    = false;
        bool textValid   = false;
        void invalidate() {
            colValid    = false;
            regValid    = false;
            tabValid    = false;
            textValid   = false;
        }
    };

    ColumnsPlusPlusData& data;
    const int origin;

    RegularExpression rx;
    std::vector<LineCache> cache;
    std::vector<double>    results;
    std::vector<size_t>    skipMap;
    std::vector<bool>      skipFlag;
    double lastFiniteResult = 0;
    size_t currentEntry     = 0;

    CalculateHistory(ColumnsPlusPlusData& data)
        : data(data), rx(data), origin(data.sci.Anchor() > data.sci.CurrentPos() ? data.sci.Selections() - 1 : 0) {}


    // previous(n) - get the index of the entry in the cache corresponding to the line n lines previous to the current line,
    //               taking into account that skipped lines should not be counted
    //               if there is not a cache entry for the requested line, enlarge the cache to create it and all more recent lines

    size_t previous(size_t n) {
        if (n >= skipMap.size()) return std::string::npos;
        n = results.size() - skipMap[skipMap.size() - n - 1] - 1;
        if (n < cache.size()) return n <= currentEntry ? currentEntry - n : cache.size() + currentEntry - n;
        cache.insert(cache.begin() + currentEntry + 1, n + 1 - cache.size(), LineCache());
        return currentEntry + 1;
    }


    // selectionIndex(cacheIndex) - given the index of an entry in the cache, find the selection index to use in Scintilla calls

    int selectionIndex(size_t cacheIndex) {
        size_t n = cacheIndex <= currentEntry ? currentEntry - cacheIndex : cache.size() + currentEntry - cacheIndex;
        return origin ? origin - static_cast<int>(results.size() - n - 1) : static_cast<int>(results.size() - n - 1);
    }


    // text(cacheIndex) given the index of an entry in the cache, return a reference to the text in the cache

    const std::string& text(size_t lcn) {
        static const std::string nullString = "";
        if (lcn == std::string::npos) return nullString;
        LineCache& lc = cache[lcn];
        if (!lc.textValid) {
            lc.textValid = true;
            const int index = selectionIndex(lcn);
            const Scintilla::Position cpMin = data.sci.SelectionNStart(index);
            const Scintilla::Position cpMax = data.sci.SelectionNEnd(index);
            if (cpMin == cpMax) lc.text = "";
            else if (!data.settings.elasticEnabled || !data.settings.leadingTabsIndent)
                lc.text = data.sci.StringOfRange(Scintilla::Span(cpMin, cpMax));
            else {
                const Scintilla::Line     lineNumber     = data.sci.LineFromPosition(cpMin);
                const Scintilla::Position lineStart      = data.sci.PositionFromLine(lineNumber);
                const std::string         line           = data.sci.StringOfRange(Scintilla::Span(lineStart, cpMax));
                const size_t              selectionStart = cpMin - lineStart;
                const size_t              firstNonTab    = line.find_first_not_of('\t');
                if (firstNonTab == std::string::npos) lc.text = "";
                else lc.text = line.substr(std::max(selectionStart, firstNonTab));
            }
        }
        return lc.text;
    }


    void push() {
        if (cache.size() < 3) {
            cache.emplace_back();
            currentEntry = cache.size() - 1;
        }
        else {
            ++currentEntry;
            if (currentEntry == cache.size()) currentEntry = 0;
            cache[currentEntry].invalidate();
        }
        if (results.size() && std::isfinite(results.back())) lastFiniteResult = results.back();
        skipFlag.push_back(false);
        skipMap.push_back(results.size());
        results.push_back(std::numeric_limits<double>::quiet_NaN());
    }

    void skip() {
        skipFlag.back() = true;
        skipMap.pop_back();
    }

    bool skipped(size_t n) { return skipFlag[n]; }

    double col(size_t n, size_t i) {
        const size_t lcn = previous(n);
        if (lcn == std::string::npos) return std::numeric_limits<double>::quiet_NaN();
        LineCache& lc = cache[lcn];
        if (!lc.colValid) {
            lc.colValid = true;
            const std::string& s = text(lcn);
            lc.col.clear();
            lc.col.push_back(data.parseNumber(s));
            for (size_t j = 0; j = s.find_first_not_of("\t ", j), j != std::string::npos;) {
                size_t k = s.find_first_of("\t ", j);
                lc.col.push_back(data.parseNumber(s.substr(j, k - j)));
                if (k == std::string::npos) break;
                j = k + 1;
            }
        }
        if (i >= lc.col.size()) return std::numeric_limits<double>::quiet_NaN();
        return lc.col[i];
    }

    double reg(size_t n, size_t i, bool* response = 0) {
        if (response) *response = false;
        if (!rx.can_search() || i > rx.mark_count()) return std::numeric_limits<double>::quiet_NaN();
        const size_t lcn = previous(n);
        if (lcn == std::string::npos) return std::numeric_limits<double>::quiet_NaN();
        LineCache& lc = cache[lcn];
        if (!lc.regValid) {
            lc.regValid = true;
            const std::string& s = text(lcn);
            lc.reg.clear();
            if (rx.search(s)) {
                for (int j = 0; j < static_cast<int>(rx.size()); ++j) lc.reg.push_back(data.parseNumber(rx.str(j)));
                if (response) *response = true;
            }
        }
        if (i >= lc.reg.size()) return std::numeric_limits<double>::quiet_NaN();
        return lc.reg[i];
    }

    double tab(size_t n, size_t i) {
        const size_t lcn = previous(n);
        if (lcn == std::string::npos) return std::numeric_limits<double>::quiet_NaN();
        LineCache& lc = cache[lcn];
        if (!lc.tabValid) {
            lc.tabValid = true;
            const std::string& s = text(lcn);
            lc.tab.clear();
            lc.tab.push_back(data.parseNumber(s));
            for (size_t j = 0; j < s.length();) {
                size_t k = s.find_first_of("\t", j);
                lc.tab.push_back(data.parseNumber(s.substr(j, k - j)));
                if (k == std::string::npos) break;
                j = k + 1;
            }
        }
        if (i >= lc.tab.size()) return std::numeric_limits<double>::quiet_NaN();
        return lc.tab[i];
    }
};


// The following struct definitions implement functions that can be registered with ExprTk

struct ExCol : public exprtk::ivararg_function <double> {
    CalculateHistory& history;
    ExCol(CalculateHistory& history) : history(history) {}
    double operator()(const std::vector<double>& arglist) {
        double n = history.col(
            arglist.size() > 1 ? static_cast<size_t>(arglist[1]) : 0,
            arglist.size() > 0 ? static_cast<size_t>(arglist[0]) : 0
        );
        if (arglist.size() > 2 && !std::isfinite(n)) n = arglist[2];
        return n;
    }
};

struct ExReg : public exprtk::ivararg_function <double> {
    CalculateHistory& history;
    ExReg(CalculateHistory& history) : history(history) {}
    double operator()(const std::vector<double>& arglist) {
        double n = history.reg(
            arglist.size() > 1 ? static_cast<size_t>(arglist[1]) : 0,
            arglist.size() > 0 ? static_cast<size_t>(arglist[0]) : 0
        );
        if (arglist.size() > 2 && !std::isfinite(n)) n = arglist[2];
        return n;
    }
};

struct ExTab : public exprtk::ivararg_function <double> {
    CalculateHistory& history;
    ExTab(CalculateHistory& history) : history(history) {}
    double operator()(const std::vector<double>& arglist) {
        double n = history.tab(
            arglist.size() > 1 ? static_cast<size_t>(arglist[1]) : 0,
            arglist.size() > 0 ? static_cast<size_t>(arglist[0]) : 0
        );
        if (arglist.size() > 2 && !std::isfinite(n)) n = arglist[2];
        return n;
    }
};

struct ExLast : public exprtk::ivararg_function <double> {
    CalculateHistory& history;
    ExLast(CalculateHistory& history) : history(history) {exprtk::enable_zero_parameters(*this);}
    double operator()(const std::vector<double>& arglist) {
        if (!arglist.size()) return history.lastFiniteResult;
        double n = std::numeric_limits<double>::quiet_NaN();
        size_t i = static_cast<size_t>(arglist[0]);
        if (i > 0 && i < history.results.size()) n = history.results[history.results.size() - i - 1];
        if (arglist.size() > 1 && !std::isfinite(n)) n = arglist[1];
        return n;
    }
};
