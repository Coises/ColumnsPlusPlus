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
#include <algorithm>

enum class SortType { Binary, Locale, Numeric };

struct SortSelectionLine {
    std::string key;
    std::vector<long double> numKeys;
    char* line;
    size_t lineLength;
    int selection;
    bool atEndOfDocument = false;
};

struct SortSelection : std::vector<SortSelectionLine> {
    Scintilla::Line textLine;
    Scintilla::Position textStart, textEnd;
    std::string text;
};

void getNumericSortKey(const RectangularSelection_Row& row, std::vector<long double>& keys) {
    for (const auto& cell : row) {
        long double value;
        size_t decimalPlaces;
        int timeSegments;
        if (row.rs.data.getNumber(cell.trim(), value, decimalPlaces, timeSegments)) keys.push_back(value);
        else keys.push_back(std::numeric_limits<long double>::quiet_NaN());
    }
}

std::string getSortKey(const char* text, size_t length, UINT codepage, DWORD options) {
    if (length < 1) return "";
    std::wstring wideText = toWide(std::string(text, length), codepage);
    int wideTextLength = clamp_cast<int>(wideText.length());
    int m = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, options, wideText.data(), wideTextLength, 0, 0, 0, 0, 0);
    std::string key(m, 0);
    LCMapStringEx(LOCALE_NAME_USER_DEFAULT, options, wideText.data(), wideTextLength, (LPWSTR) key.data(), m, 0, 0, 0);
    return key;
}

bool getSortSelection(ColumnsPlusPlusData& data, SortSelection& ss, SortType sortType, DWORD options = 0) {
    auto rs = data.getRectangularSelection();
    int lines = rs.size();
    if (lines < 2) return false;
    ss.resize(lines);
    UINT codepage = data.sci.CodePage();
    bool forward = rs.topToBottom();
    ss.textLine  = rs.top().ln;
    ss.textStart = rs.top().st;
    if (ss.textLine + lines == data.sci.LineCount()) {
        ss.textEnd = data.sci.Length();
        ss[lines - 1].atEndOfDocument = true;
    }
    else ss.textEnd = data.sci.PositionFromLine(ss.textLine + lines);
    ss.text = data.sci.StringOfRange(Scintilla::Span(ss.textStart, ss.textEnd));
    char* textPointer = ss.text.data();
    Scintilla::Position cpNextLine = ss.textEnd;
    for (int n = lines - 1; n >= 0; --n) {
        ss[n].selection = forward ? n : lines - 1 - n;
        auto row = rs[ss[n].selection];
        Scintilla::Position cpLine = data.sci.PositionFromLine(row.line());
        Scintilla::Position cpMin  = row.cpMin();
        Scintilla::Position cpMax  = row.cpMax();
        ss[n].line = cpLine - ss.textStart + textPointer;
        if (sortType == SortType::Numeric) getNumericSortKey(row, ss[n].numKeys);
        else ss[n].key = sortType == SortType::Locale ? getSortKey(cpMin - ss.textStart + textPointer, cpMax - cpMin, codepage, options)
                                                      : std::string(cpMin - ss.textStart + textPointer, cpMax - cpMin);
        ss[n].lineLength = cpNextLine - cpLine;
        cpNextLine = cpLine;
    }
    return true;
}

void replaceSortSelection(ColumnsPlusPlusData& data, SortSelection& ss) {

    std::string r;
    r.resize(ss.textEnd - ss.textStart);
    char* p = r.data();
    Scintilla::Line lines = ss.size();
    for (int i = 0; i < lines; ++i) {
        SortSelectionLine& s = ss[i];
        memcpy(p, s.line, s.lineLength);
        p += s.lineLength;
        if (s.atEndOfDocument && i != lines - 1) {
            SortSelectionLine& last = ss[lines - 1];
            if (last.lineLength >= 1) {
                if (last.line[last.lineLength - 1] == '\n') {
                    if (last.lineLength >= 2 && last.line[last.lineLength - 2] == '\r') {
                        p[0] = '\r';
                        p[1] = '\n';
                        p += 2;
                        last.lineLength -= 2;
                    }
                    else {
                        p[0] = '\n';
                        ++p;
                        --last.lineLength;
                    }
                }
                else if (last.line[last.lineLength - 1] == '\r') {
                    p[0] = '\r';
                    ++p;
                    --last.lineLength;
                }
            }
        }
    }

    Scintilla::Position anchor = data.sci.RectangularSelectionAnchor();
    Scintilla::Position caret = data.sci.RectangularSelectionCaret();
    bool forward = anchor < caret;
    int selAnchor = ss[ forward ? 0 : lines - 1].selection;
    int selCaret  = ss[!forward ? 0 : lines - 1].selection;
    Scintilla::Position cpAnchor = data.sci.SelectionNAnchor            (selAnchor);
    Scintilla::Position vsAnchor = data.sci.SelectionNAnchorVirtualSpace(selAnchor);
    Scintilla::Position cpCaret  = data.sci.SelectionNCaret             (selCaret );
    Scintilla::Position vsCaret  = data.sci.SelectionNCaretVirtualSpace (selCaret );
    cpAnchor -= data.sci.PositionFromLine(ss.textLine + (forward ? selAnchor : lines - selAnchor - 1));
    cpCaret  -= data.sci.PositionFromLine(ss.textLine + (forward ? selCaret  : lines - selCaret  - 1));

    data.sci.SetTargetRange(ss.textStart, ss.textEnd);
    data.sci.ReplaceTarget(r);
    if (data.settings.elasticEnabled) {
        DocumentData* ddp = data.getDocument();
        data.analyzeTabstops(*ddp);
        Scintilla::Line firstVisible = data.sci.FirstVisibleLine();
        Scintilla::Line lastVisible = firstVisible + data.sci.LinesOnScreen();
        data.setTabstops(*ddp, std::min(ss.textLine, firstVisible), std::max(ss.textLine + lines - 1, lastVisible));
    }

    cpAnchor += data.sci.PositionFromLine(ss.textLine + ( forward ? 0 : lines - 1));
    cpCaret  += data.sci.PositionFromLine(ss.textLine + (!forward ? 0 : lines - 1));
    data.sci.SetRectangularSelectionAnchor            (cpAnchor);
    data.sci.SetRectangularSelectionCaret             (cpCaret );
    data.sci.SetRectangularSelectionAnchorVirtualSpace(vsAnchor);
    data.sci.SetRectangularSelectionCaretVirtualSpace (vsCaret );

}

void ColumnsPlusPlusData::sortAscendingBinary() {
    SortSelection ss;
    if (!getSortSelection(*this, ss, SortType::Binary)) return;
    std::stable_sort(ss.begin(), ss.end(),
        [](const SortSelectionLine& a, const SortSelectionLine& b) {return a.key < b.key;});
    replaceSortSelection(*this, ss);
}

void ColumnsPlusPlusData::sortDescendingBinary() {
    SortSelection ss;
    if (!getSortSelection(*this, ss, SortType::Binary)) return;
    std::stable_sort(ss.begin(), ss.end(),
        [](const SortSelectionLine& a, const SortSelectionLine& b) {return a.key > b.key;});
    replaceSortSelection(*this, ss);
}

void ColumnsPlusPlusData::sortAscendingLocale() {
    SortSelection ss;
    if (!getSortSelection(*this, ss, SortType::Locale, LCMAP_SORTKEY | NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE | SORT_DIGITSASNUMBERS)) return;
    std::stable_sort(ss.begin(), ss.end(),
        [](const SortSelectionLine& a, const SortSelectionLine& b) {return a.key < b.key;});
    replaceSortSelection(*this, ss);
}

void ColumnsPlusPlusData::sortDescendingLocale() {
    SortSelection ss;
    if (!getSortSelection(*this, ss, SortType::Locale, LCMAP_SORTKEY | NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE | SORT_DIGITSASNUMBERS)) return;
    std::stable_sort(ss.begin(), ss.end(),
        [](const SortSelectionLine& a, const SortSelectionLine& b) {return a.key > b.key;});
    replaceSortSelection(*this, ss);
}

void ColumnsPlusPlusData::sortAscendingNumeric() {
    SortSelection ss;
    if (!getSortSelection(*this, ss, SortType::Numeric)) return;
    std::stable_sort(ss.begin(), ss.end(),
        [](const SortSelectionLine& a, const SortSelectionLine& b) {
            for (size_t i = 0; i < std::min(a.numKeys.size(), b.numKeys.size()); ++i) {
                if (std::_Is_nan(a.numKeys[i]))
                    if (std::_Is_nan(b.numKeys[i])) continue; else return true;
                if (std::_Is_nan(b.numKeys[i])) return false;
                if (a.numKeys[i] == b.numKeys[i]) continue;
                return a.numKeys[i] < b.numKeys[i];
            }
            return a.numKeys.size() < b.numKeys.size();
        });
    replaceSortSelection(*this, ss);
}

void ColumnsPlusPlusData::sortDescendingNumeric() {
    SortSelection ss;
    if (!getSortSelection(*this, ss, SortType::Numeric)) return;
    std::stable_sort(ss.begin(), ss.end(),
        [](const SortSelectionLine& a, const SortSelectionLine& b) {
            for (size_t i = 0; i < std::min(a.numKeys.size(), b.numKeys.size()); ++i) {
                if (std::_Is_nan(a.numKeys[i]))
                    if (std::_Is_nan(b.numKeys[i])) continue; else return true;
                if (std::_Is_nan(b.numKeys[i])) return false;
                if (a.numKeys[i] == b.numKeys[i]) continue;
                return a.numKeys[i] > b.numKeys[i];
            }
            return a.numKeys.size() > b.numKeys.size();
        });
    replaceSortSelection(*this, ss);
}
