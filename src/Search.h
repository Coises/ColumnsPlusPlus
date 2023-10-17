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

#include "RegularExpressionBuffered.h"
#include <regex>
#include <string.h>
#include "commctrl.h"
#include "resource.h"

#pragma warning (push)
#pragma warning (disable: 4702)
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#include "exprtk/exprtk.hpp"
#pragma warning (pop)

class RegexCalcHistory {
public:
    std::vector<std::vector<double>> values;      // Values of regex capture groups for each match
    std::vector<std::vector<double>> results;     // Results of each expression for each match
    std::vector<double> lastFiniteResult;         // Last finite result of each expression
    size_t expressionIndex;                       // index of expression currently being processed
};

// The following struct definitions implement functions that can be registered with ExprTk

struct RCReg : public exprtk::ivararg_function <double> {
    RegexCalcHistory& history;
    RCReg(RegexCalcHistory& history) : history(history) {}
    double operator()(const std::vector<double>& arglist) {
        size_t capture  = arglist.size() > 0 ? static_cast<size_t>(arglist[0]) : 0;
        size_t previous = arglist.size() > 1 ? static_cast<size_t>(arglist[1]) : 0;
        double n = std::numeric_limits<double>::quiet_NaN();
        if (previous >= 0 && capture >= 0 && previous < history.values.size() && capture < history.values[previous].size())
            n = history.values[history.values.size() - previous - 1][capture];
        if (arglist.size() > 2 && !std::isfinite(n)) n = arglist[2];
        return n;
    }
};

struct RCSub : public exprtk::ivararg_function <double> {
    RegexCalcHistory& history;
    RCSub(RegexCalcHistory& history) : history(history) {}
    double operator()(const std::vector<double>& arglist) {
        size_t substitution = arglist.size() > 0 ? static_cast<size_t>(arglist[0]) : 0;
        size_t previous = arglist.size() > 1 ? static_cast<size_t>(arglist[1]) : 0;
        double n = std::numeric_limits<double>::quiet_NaN();
        if (previous >= 0 && substitution >= 0 && previous < history.results.size() && substitution <= history.results[previous].size()) {
            if (substitution == 0) substitution = history.expressionIndex + 1;
            if (arglist.size() < 2)
                n = history.lastFiniteResult[substitution - 1];
            else if (previous > 0 || substitution <= history.expressionIndex)
                n = history.results[history.results.size() - previous - 1][substitution - 1];
        }
        if (arglist.size() > 2 && !std::isfinite(n)) n = arglist[2];
        return n;
    }
};

struct RCLast : public exprtk::ivararg_function <double> {
    RegexCalcHistory& history;
    RCLast(RegexCalcHistory& history) : history(history) {exprtk::enable_zero_parameters(*this);}
    double operator()(const std::vector<double>& arglist) {
        if (!arglist.size()) return history.lastFiniteResult[history.expressionIndex];
        double n = std::numeric_limits<double>::quiet_NaN();
        size_t previous = static_cast<size_t>(arglist[0]);
        if (previous > 0 && previous < history.results.size())
            n = history.results[history.results.size() - previous - 1][history.expressionIndex];
        if (arglist.size() > 1 && !std::isfinite(n)) n = arglist[1];
        return n;
    }
};
