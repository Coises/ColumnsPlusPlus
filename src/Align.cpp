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
#include "resource.h"
#include "commctrl.h"
#include "Shlwapi.h"


void ColumnsPlusPlusData::alignLeft() {
    auto rs = getRectangularSelection();
    if (!rs.size()) return;
    sci.BeginUndoAction();
    for (auto row : rs) {
        std::string r;
        for (const auto& cell : row) {
            r += cell.trim();
            if (!settings.elasticEnabled || (cell.isLastInRow() && !cell.isEndOfLine())) {
                r += std::string(cell.leading() + cell.trailing(), ' ');
            }
            r += cell.terminator();
        }
        if (r != row.text()) row.replace(r);
    }
    sci.EndUndoAction();
    rs.refit();
}


void ColumnsPlusPlusData::alignRight() {
    auto rs = getRectangularSelection();
    if (!rs.size()) return;
    sci.BeginUndoAction();
    for (auto row : rs) {
        std::string r;
        for (const auto& cell : row) {
            if (cell.trimLength() == 0)  r += cell.text() + cell.terminator();
            else if (cell.isLastInRow()) r += std::string(row.vsMax() - row.vsMin() + cell.leading() + cell.trailing(), ' ') + cell.trim();
            else {
                int padWidth = sci.PointXFromPosition(cell.end() + 1) - sci.PointXFromPosition(cell.end());
                padWidth = rs.blankCount(padWidth) - (settings.elasticEnabled ? settings.minimumSpaceBetweenColumns : 1);
                if (padWidth < 0) padWidth = 0;
                r += std::string(padWidth + cell.leading() + cell.trailing(), ' ') + cell.trim() + '\t';
            }
        }
        if (r != row.text()) row.replace(r);
    }
    sci.EndUndoAction();
    rs.refit();
}


void ColumnsPlusPlusData::alignNumeric() {

    struct Item {
        std::string text;
        intptr_t colonExtentLeft;
        intptr_t colonExtentRight;
        intptr_t decimalExtentLeft;
        intptr_t decimalExtentRight;
        bool     align;
    };

    struct Line : std::vector<Item> {
        std::string line;
        bool eol;
    };

    struct Column {
        intptr_t colonExtentLeft    = 0;
        intptr_t colonExtentRight   = 0;
        intptr_t decimalExtentLeft  = 0;
        intptr_t decimalExtentRight = 0;
        intptr_t width              = 0;
        intptr_t pad                = 0;
    };

    auto rs = getRectangularSelection();
    if (rs.size() < 2) return;

    std::vector<Line> item(rs.size());
    std::vector<Column> column;

    for (auto row : rs) {
        Line& cLine = item[row.index];
        cLine.line  = row.text();
        cLine.eol   = row.isEndOfLine();
        for (const auto& cell : row) {
            cLine.emplace_back();
            Item& cItem = cLine.back();
            if (cLine.size() > column.size()) column.emplace_back();
            Column& cColumn = column[cLine.size() - 1];
            intptr_t width = sci.PointXFromPosition(cell.end()) - sci.PointXFromPosition(cell.start());
            if (cell.isLastInRow()) width += rs.blankWidth(row.vsMax() - row.vsMin());
            if (width > cColumn.width) cColumn.width = width;
            size_t cp = 0;
            size_t dp = 0;
            if (cell.trimLength() == 0 || !getNumericAlignment(cell.trim(), cp, dp)) {
                cItem.text = cell.text();
                cItem.align = false;
                continue;
            }
            if (cp == std::string::npos) {
                auto dpX = sci.PointXFromPosition(cell.left() + dp);
                cItem.decimalExtentLeft  = dpX - sci.PointXFromPosition(cell.left());
                cItem.decimalExtentRight = sci.PointXFromPosition(cell.right()) - dpX;
                if (cItem.decimalExtentLeft  > cColumn.decimalExtentLeft ) cColumn.decimalExtentLeft  = cItem.decimalExtentLeft;
                if (cItem.decimalExtentRight > cColumn.decimalExtentRight) cColumn.decimalExtentRight = cItem.decimalExtentRight;
                cItem.colonExtentLeft = cItem.colonExtentRight = 0;
            }
            else {
                auto cpX = sci.PointXFromPosition(cell.left() + cp);
                cItem.colonExtentLeft  = cpX - sci.PointXFromPosition(cell.left());
                cItem.colonExtentRight = sci.PointXFromPosition(cell.right()) - cpX;
                if (cItem.colonExtentLeft  > cColumn.colonExtentLeft ) cColumn.colonExtentLeft  = cItem.colonExtentLeft;
                if (cItem.colonExtentRight > cColumn.colonExtentRight) cColumn.colonExtentRight = cItem.colonExtentRight;
                cItem.decimalExtentLeft = cItem.decimalExtentRight = 0;
            }
            cItem.text = cell.trim();
            cItem.align = true;
        }
    }

    // At this point, the selection is represented as a vector of rows, each containing a vector
    //     of items which are separated by tabs in the source.
    // item.align is true if the item can be numeric aligned, false if it is to be left as is.
    // Each item.text to be aligned has been trimmed of leading and trailing blanks.
    // column.width is the maximum number of pixels in any item in the column before trimming,
    //     including virtual space.
    // item.colonExtentLeft is the number of pixels left of the colon that will be used for alignment,
    //     if there is a colon; else it is zero.
    // item.colonExtentRight is the number of pixels right of the colon that will be used for alignment,
    //     including the colon itself, if there is a colon; else it is zero.
    // item.decimalExtentLeft is the number of pixels left of the (possibly implicit) decimal separator
    //     when there is no colon; else it is zero.
    // item.decimalExtentRight is the number of pixels right of the (possibly implicit) ones digit,
    //     including the decimal separator if there is one, when there is no colon; else it is zero.
    // column.*ExtentLeft and .*ExtentRight are the maximums of the *ExtentLeft and *ExtentRight values
    //     of all items in the column.

    // colonDecimalOffset is the number of pixels the alignment colon for time format numbers should be left of the decimal in non-time format numbers
    
    intptr_t colonDecimalOffset = (timeScalarUnit - (timePartialRule == 0 ? 0 : timePartialRule == 3 ? 2 : 1)) * sci.TextWidth(STYLE_DEFAULT, ":00");

    for (Column& col : column) {
        if (col.colonExtentRight == 0) /* no time format numbers in this column */ {
            col.pad = rs.blankCount(col.width - col.decimalExtentLeft - col.decimalExtentRight);
        }
        else if (col.decimalExtentLeft == 0 && col.decimalExtentRight == 0) /* only time format numbers in this column */ {
            col.pad = rs.blankCount(col.width - col.colonExtentLeft - col.colonExtentRight);
        }
        else /* mixed time and non-time formats in this column */ {
            col.pad = rs.blankCount( col.width - std::max(col.colonExtentLeft , col.decimalExtentLeft  - colonDecimalOffset)
                                               - std::max(col.colonExtentRight, col.decimalExtentRight + colonDecimalOffset) );
        } 
    }

    // A positive pad value means once leading and trailing blanks are removed from all items in a column
    //     and the text is numerically aligned, the result is not as wide as the original column,
    //     so extra padding will be added to the left of aligned items to avoid changing the column width.
    // A negative pad value means the original column isn't wide enough to hold the column
    //     after numeric alignment, so the width of all items in the column will be increased.

    sci.BeginUndoAction();

    for (auto row : rs) {
        const auto& cells = item[row.index];
        bool notEOL = !cells.eol;
        intptr_t lastCell = cells.size() - 1;
        std::string r;
        for (int j = 0;;++j) {
            const Item& cell = cells[j];
            const Column& col = column[j];
            if (cell.align) {
                if (col.pad > 0) r += std::string(col.pad, ' ');
                intptr_t colLeft, colRight, cellLeft, cellRight;
                if (col.colonExtentRight == 0) /* no time format numbers in this column */ {
                    colLeft   = col .decimalExtentLeft;
                    colRight  = col .decimalExtentRight;
                    cellLeft  = cell.decimalExtentLeft;
                    cellRight = cell.decimalExtentRight;
                }
                else if (col.decimalExtentLeft == 0 && col.decimalExtentRight == 0) /* only time format numbers in this column */ {
                    colLeft   = col .colonExtentLeft;
                    colRight  = col .colonExtentRight;
                    cellLeft  = cell.colonExtentLeft;
                    cellRight = cell.colonExtentRight;
                }
                else /* mixed time and non-time formats in this column */ {
                    colLeft  = std::max(col.colonExtentLeft , col.decimalExtentLeft  - colonDecimalOffset);
                    colRight = std::max(col.colonExtentRight, col.decimalExtentRight + colonDecimalOffset);
                    if (cell.colonExtentRight == 0) {
                        cellLeft  = cell.decimalExtentLeft  - colonDecimalOffset;
                        cellRight = cell.decimalExtentRight + colonDecimalOffset;
                    }
                    else {
                        cellLeft  = cell.colonExtentLeft;
                        cellRight = cell.colonExtentRight;
                    }
                }
                r += std::string(rs.blankCount(colLeft - cellLeft), ' ');
                r += cell.text;
                if (j == lastCell ? notEOL : !settings.elasticEnabled)
                    r += std::string(rs.blankCount(colRight - cellRight), ' ');
            }
            else {
                r += cell.text;
                if (col.pad < 0 && (j == lastCell ? notEOL : !settings.elasticEnabled)) r += std::string(-col.pad, ' ');
            }
            if (j == lastCell) break;
            r += '\t';
        }
        if (r != cells.line) row.replace(r);
    }

    sci.EndUndoAction();
    rs.refit();

}


INT_PTR CALLBACK alignDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    ColumnsPlusPlusData* cdp = 0;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        cdp = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else cdp = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!cdp) return TRUE;
    ColumnsPlusPlusData& data = *cdp;

    switch (uMsg) {

    case WM_DESTROY:
        return TRUE;

    case WM_INITDIALOG:
    {
        RECT rcNpp, rcDlg;
        GetWindowRect(data.nppData._nppHandle, &rcNpp);
        GetWindowRect(hwndDlg, &rcDlg);
        SetWindowPos(hwndDlg, HWND_TOP, (rcNpp.left + rcNpp.right + rcDlg.left - rcDlg.right) / 2,
            (rcNpp.top + rcNpp.bottom + rcDlg.top - rcDlg.bottom) / 2, 0, 0, SWP_NOSIZE);
        for (const auto& s : data.align.history)
            SendDlgItemMessage(hwndDlg, IDC_ALIGN_BY, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        SendDlgItemMessage(hwndDlg, IDC_ALIGN_BY, CB_SETCURSEL, 0, 0);
        switch (data.align.alignOn) {
        case AlignSettings::Last : CheckRadioButton(hwndDlg, IDC_ALIGN_FIRST, IDC_ALIGN_REGEX, IDC_ALIGN_LAST ); break;
        case AlignSettings::Regex: CheckRadioButton(hwndDlg, IDC_ALIGN_FIRST, IDC_ALIGN_REGEX, IDC_ALIGN_REGEX); break;
        default                  : CheckRadioButton(hwndDlg, IDC_ALIGN_FIRST, IDC_ALIGN_REGEX, IDC_ALIGN_FIRST);
        }
        CheckRadioButton(hwndDlg, IDC_ALIGN_MARGIN_LEFT, IDC_ALIGN_MARGIN_RIGHT, data.align.marginRight ? IDC_ALIGN_MARGIN_RIGHT : IDC_ALIGN_MARGIN_LEFT);
        SendDlgItemMessage(hwndDlg, IDC_ALIGN_MATCH_CASE, BM_SETCHECK, data.align.matchCase ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_ALIGN_MARGIN_SPIN, UDM_SETRANGE, 0, MAKELPARAM(99, 0));
        SendDlgItemMessage(hwndDlg, IDC_ALIGN_MARGIN_SPIN, UDM_SETPOS, 0, data.align.margin);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
        {
            HWND h = GetDlgItem(hwndDlg, IDC_ALIGN_BY);
            auto n = GetWindowTextLength(h);
            std::wstring s(n, 0);
            s.resize(GetWindowText(h, s.data(), n + 1));
            bool matchCase = IsDlgButtonChecked(hwndDlg, IDC_ALIGN_MATCH_CASE) == BST_CHECKED;
            std::wstring error = s.empty() ? L"Enter something to align." : L"";
            if (!s.empty() && IsDlgButtonChecked(hwndDlg, IDC_ALIGN_REGEX) == BST_CHECKED) {
                RegularExpression rx(data.sci);
                error = rx.find(s, matchCase);
            }
            if (!error.empty()) {
                COMBOBOXINFO cbi;
                cbi.cbSize = sizeof(COMBOBOXINFO);
                GetComboBoxInfo(h, &cbi);
                EDITBALLOONTIP ebt;
                ebt.cbStruct = sizeof(EDITBALLOONTIP);
                ebt.pszTitle = L"";
                ebt.ttiIcon = TTI_NONE;
                ebt.pszText = error.data();
                SendMessage(cbi.hwndItem, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(cbi.hwndItem), TRUE);
                return TRUE;
            }
            if (!validateSpin(data.align.margin, hwndDlg, IDC_ALIGN_MARGIN_SPIN, L"Indent must be a value between 0 and 99.")) return TRUE;
            updateComboHistory(hwndDlg, IDC_ALIGN_BY, data.align.history);
            data.align.marginRight = IsDlgButtonChecked(hwndDlg, IDC_ALIGN_MARGIN_RIGHT) == BST_CHECKED;
            data.align.matchCase   = matchCase;
            data.align.alignOn     = IsDlgButtonChecked(hwndDlg, IDC_ALIGN_LAST ) == BST_CHECKED ? AlignSettings::Last
                                   : IsDlgButtonChecked(hwndDlg, IDC_ALIGN_REGEX) == BST_CHECKED ? AlignSettings::Regex
                                                                                                 : AlignSettings::First;
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        }
        break;

    }
    return FALSE;
}


void ColumnsPlusPlusData::alignCustom() {

    auto rs = getRectangularSelection();
    if (rs.size() < 2) return;

    if (DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_ALIGN), nppData._nppHandle, alignDialogProc, reinterpret_cast<LPARAM>(this))) return;

    std::string find = fromWide(align.history.back(), sci.CodePage());
    Scintilla::FindOption searchFlags = Scintilla::FindOption::None;
    if (align.matchCase) searchFlags |= Scintilla::FindOption::MatchCase;

    RegularExpression rx(sci);
    if (align.alignOn == AlignSettings::Regex) rx.find(align.history.back(), align.matchCase);

    struct Item {
        std::string text;
        intptr_t    extentLeft;
        intptr_t    extentRight;
        bool        align;
    };

    struct Line : std::vector<Item> {
        std::string line;
        bool eol;
    };

    struct Column {
        intptr_t extentLeft  = 0;
        intptr_t extentRight = 0;
        intptr_t width       = 0;
        intptr_t pad         = 0;
    };

    std::vector<Line> item(rs.size());
    std::vector<Column> column;

    for (auto row : rs) {
        Line& cLine = item[row.index];
        cLine.line  = row.text();
        cLine.eol   = row.isEndOfLine();
        for (const auto& cell : row) {
            cLine.emplace_back();
            Item& cItem = cLine.back();
            if (cLine.size() > column.size()) column.emplace_back();
            Column& cColumn = column[cLine.size() - 1];
            intptr_t width = sci.PointXFromPosition(cell.end()) - sci.PointXFromPosition(cell.start());
            if (cell.isLastInRow()) width += rs.blankWidth(row.vsMax() - row.vsMin());
            if (width > cColumn.width) cColumn.width = width;
            Scintilla::Position found = -1;
            if (cell.trimLength() > 0) {
                if (align.alignOn == AlignSettings::Regex) {
                    if (rx.search(cell.trim())) found = cell.left() + rx.position(0);
                }
                else {
                    if (align.alignOn == AlignSettings::Last) sci.SetTargetRange(cell.right(), cell.left());
                                                         else sci.SetTargetRange(cell.left(), cell.right());
                    found = sci.SearchInTarget(find);
                }
            }
            if (found < 0) {
                cItem.text = cell.text();
                cItem.align = false;
                continue;
            }
            auto cpX = sci.PointXFromPosition(found);
            cItem.extentLeft  = cpX - sci.PointXFromPosition(cell.left());
            cItem.extentRight = sci.PointXFromPosition(cell.right()) - cpX;
            cItem.text = cell.trim();
            if (align.margin > 0) {
                if (align.marginRight) cItem.extentRight +=  rs.blankWidth(align.margin);
                else {
                    cItem.text.insert(0, std::string(align.margin, ' '));
                    cItem.extentLeft += rs.blankWidth(align.margin);
                }
            }
            if (cItem.extentLeft  > cColumn.extentLeft ) cColumn.extentLeft  = cItem.extentLeft;
            if (cItem.extentRight > cColumn.extentRight) cColumn.extentRight = cItem.extentRight;
            cItem.align = true;
        }
    }

    // At this point, the selection is represented as a vector of rows, each containing a vector
    //     of items which are separated by tabs in the source.
    // item.align is true if the item can be aligned, false if it is to be left as is.
    // Each item.text to be aligned has been trimmed of leading and trailing blanks, and left indent (if any) has been affixed.
    // column.width is the maximum number of pixels in any item in the column before trimming,
    //     including virtual space.
    // item.extentLeft is the number of pixels left of the alignment point.
    // item.extentRight is the number of pixels right of the alignment point.
    // column.extentLeft and .extentRight are the maximums of the extentLeft and extentRight values
    //     of all items in the column.

    for (Column& col : column) col.pad = rs.blankCount(col.width - col.extentLeft - col.extentRight);

    // A positive pad value means once leading and trailing blanks are removed from all items in a column
    //     and the text is aligned, the result is not as wide as the original column,
    //     so extra padding will be added to aligned items to avoid changing the column width.
    // A negative pad value means the original column isn't wide enough to hold the column
    //     after alignment, so the width of all items in the column will be increased.

    sci.BeginUndoAction();

    for (auto row : rs) {
        const auto& cells = item[row.index];
        bool notEOL = !cells.eol;
        intptr_t lastCell = cells.size() - 1;
        std::string r;
        for (int j = 0;;++j) {
            const Item& cell = cells[j];
            const Column& col = column[j];
            if (cell.align) {
                if (col.pad > 0 && align.marginRight) r += std::string(col.pad, ' ');
                intptr_t colLeft, colRight, cellLeft, cellRight;
                colLeft   = col .extentLeft;
                colRight  = col .extentRight;
                cellLeft  = cell.extentLeft;
                cellRight = cell.extentRight;
                r += std::string(rs.blankCount(colLeft - cellLeft), ' ');
                r += cell.text;
                if (j == lastCell ? notEOL : !settings.elasticEnabled) {
                    intptr_t rightPad = rs.blankCount(colRight - cellRight);
                    if (align.marginRight) rightPad += align.margin;
                    else if (col.pad > 0) rightPad += col.pad;
                    r += std::string(rightPad, ' ');
                }
            }
            else {
                r += cell.text;
                if (col.pad < 0 && (j == lastCell ? notEOL : !settings.elasticEnabled)) r += std::string(-col.pad, ' ');
            }
            if (j == lastCell) break;
            r += '\t';
        }
        if (r != cells.line) row.replace(r);
    }

    sci.EndUndoAction();
    rs.refit();

 }