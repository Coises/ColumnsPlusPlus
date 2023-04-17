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
#include <regex>
#include "resource.h"
#include "commctrl.h"
#include "Host\BoostRegexSearch.h"

#undef min
#undef max

INT_PTR CALLBACK searchDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ColumnsPlusPlusData* data;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        data = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else data = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    return data->searchDialogProc(hwndDlg, uMsg, wParam, lParam);
}

void ColumnsPlusPlusData::showSearchDialog() {
    if (searchData.customIndicator > 0) {
        sci.IndicSetStyle(searchData.customIndicator, Scintilla::IndicatorStyle::FullBox);
        sci.IndicSetFore (searchData.customIndicator, searchData.customColor);
        sci.IndicSetAlpha(searchData.customIndicator, static_cast<Scintilla::Alpha>(searchData.customAlpha));
        sci.IndicSetUnder(searchData.customIndicator, true);
    }
    if (searchData.dialog) if (SetFocus(searchData.dialog)) return;
    CreateDialogParam(dllInstance, MAKEINTRESOURCE(IDD_SEARCH), nppData._nppHandle,
                      ::searchDialogProc, reinterpret_cast<LPARAM>(this));
}

const std::regex rxExtended("([^\\\\]*)(\\\\([0nrt\\\\]|b[01]{1,8}|d\\d{1,3}|o[0-7]{1,3}|u[\\da-fA-F]{1,4}|x[\\da-fA-F]{1,2}|))(.*)", std::regex::optimize);

void expandExtendedSearchString(std::string& s, UINT codepage) {
    std::string r;
    while (s.length()) {
        std::smatch m;
        if (!std::regex_match(s, m, rxExtended)) {
            r += s;
            break;
        }
        r += m[1];
        std::string e = m[3];
        if (e.length() == 0) r += '\\';
        else switch (e[0]) {
        case '\\': r += '\\'; break;
        case '0': r += '\0'; break;
        case 'n': r += '\n'; break;
        case 'r': r += '\r'; break;
        case 't': r += '\t'; break;
        case 'b': r += static_cast<char>(std::stoi(e.substr(1), 0,  2)); break;
        case 'd': r += static_cast<char>(std::stoi(e.substr(1), 0, 10)); break;
        case 'o': r += static_cast<char>(std::stoi(e.substr(1), 0,  8)); break;
        case 'x': r += static_cast<char>(std::stoi(e.substr(1), 0, 16)); break;
        case 'u': {
            wchar_t u[1];
            char c[4];
            u[0] = static_cast<wchar_t>(std::stoi(e.substr(1), 0, 16));
            int cLen = WideCharToMultiByte(codepage, 0, u, 1, c, 4, 0, 0);
            r += std::string(c, cLen);
            break;
        }
        }
        s = m[4];
    }
    s = r;
}

void searchLayout(const SearchData& sd, const RECT& rcDialog) {
    HWND ctrl;
    RECT rect;
    for (int i : {IDC_FIND_WHAT, IDC_REPLACE_WITH}) {
        ctrl = GetDlgItem(sd.dialog, i);
        GetWindowRect(ctrl, &rect);
        SetWindowPos(ctrl, 0, 0, 0, rcDialog.right - rcDialog.left - sd.dialogComboWidth, rect.bottom-rect.top, SWP_NOMOVE|SWP_NOZORDER);
    }
    for (int i : {IDOK, IDC_SEARCH_COUNT, IDC_SEARCH_REPLACE, IDC_SEARCH_REPLACE_ALL, IDCANCEL}) {
        ctrl = GetDlgItem(sd.dialog, i);
        GetWindowRect(ctrl, &rect);
        rect.left = rcDialog.right - sd.dialogButtonLeft;
        MapWindowPoints(0, sd.dialog, reinterpret_cast<LPPOINT>(&rect) , 1);
        SetWindowPos(ctrl, 0, rect.left, rect.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
    }
}

void updateSearchSettings(SearchData& searchData) {
    HWND h = GetDlgItem(searchData.dialog, IDC_FIND_WHAT);
    auto n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
    searchData.findWhat.resize(n);
    SendMessage(h, WM_GETTEXT, n + 1, (LPARAM) searchData.findWhat.data());
    h = GetDlgItem(searchData.dialog, IDC_REPLACE_WITH);
    n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
    searchData.replaceWith.resize(n);
    SendMessage(h, WM_GETTEXT, n + 1, (LPARAM) searchData.replaceWith.data());
    searchData.mode =
        SendDlgItemMessage(searchData.dialog, IDC_SEARCH_NORMAL  , BM_GETCHECK, 0, 0) == BST_CHECKED ? SearchData::Normal
      : SendDlgItemMessage(searchData.dialog, IDC_SEARCH_EXTENDED, BM_GETCHECK, 0, 0) == BST_CHECKED ? SearchData::Extended
                                                                                                     : SearchData::Regex;
    searchData.backward  = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_BACKWARD           , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.wholeWord = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_WHOLE_WORD         , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.matchCase = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_MATCH_CASE         , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.autoClear = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_INDICATOR_AUTOCLEAR, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void updateSearchHistory(HWND dialog, int control, const std::wstring& string, std::vector<std::wstring>& history) {
    if (string.length() && (history.empty() || history.back() != string)) {
        if (!history.empty()) {
            auto existing = std::find(history.begin(), history.end(), string);
            if (existing != history.end()) {
                SendDlgItemMessage(dialog, control, CB_DELETESTRING, history.end() - existing - 1, 0);
                SetDlgItemText(dialog, control, string.data());
                history.erase(existing);
            }
        }
        SendDlgItemMessage(dialog, control, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(string.data()));
        history.push_back(string);
    }
}

void updateFindHistory   (SearchData& searchData) { updateSearchHistory(searchData.dialog, IDC_FIND_WHAT   , searchData.findWhat   , searchData.findHistory   ); }
void updateReplaceHistory(SearchData& searchData) { updateSearchHistory(searchData.dialog, IDC_REPLACE_WITH, searchData.replaceWith, searchData.replaceHistory); }

void setSearchMessage(const ColumnsPlusPlusData& data, const std::wstring& text) {
    HWND msgHwnd = GetDlgItem(data.searchData.dialog, IDC_SEARCH_MESSAGE);
    SetWindowText(msgHwnd, text.data());
    InvalidateRect(data.searchData.dialog, 0, TRUE);
}


BOOL ColumnsPlusPlusData::searchDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
    
    case WM_DESTROY:
        return TRUE;
    
    case WM_INITDIALOG:
        {
            RECT rcDlg, rcFindButton, rcFindWhat;
            GetWindowRect(hwndDlg, &rcDlg);
            GetWindowRect(GetDlgItem(hwndDlg, IDOK), &rcFindButton);
            GetWindowRect(GetDlgItem(hwndDlg, IDC_FIND_WHAT), &rcFindWhat);
            searchData.dialog = hwndDlg;
            searchData.dialogHeight = rcDlg.bottom - rcDlg.top;
            searchData.dialogMinWidth = rcDlg.right - rcDlg.left;
            searchData.dialogButtonLeft = rcDlg.right - rcFindButton.left;
            searchData.dialogComboWidth = searchData.dialogMinWidth - (rcFindWhat.right - rcFindWhat.left);
            if (searchData.dialogLastPosition.top == searchData.dialogLastPosition.bottom) /* Center dialog on parent window */ {
                RECT rcNpp;
                GetWindowRect(nppData._nppHandle, &rcNpp);
                SetWindowPos(hwndDlg, HWND_TOP, (rcNpp.left + rcNpp.right + rcDlg.left - rcDlg.right) / 2,
                    (rcNpp.top + rcNpp.bottom + rcDlg.top - rcDlg.bottom) / 2, 0, 0, SWP_NOSIZE);
            }
            else /* restore last position */
                SetWindowPos(hwndDlg, HWND_TOP, searchData.dialogLastPosition.left, searchData.dialogLastPosition.top,
                    searchData.dialogLastPosition.right - searchData.dialogLastPosition.left, searchData.dialogHeight, 0);
        }
        switch (searchData.mode) {
        case SearchData::Normal:
            SendDlgItemMessage(hwndDlg, IDC_SEARCH_NORMAL, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD), TRUE);
            break;
        case SearchData::Extended:
            SendDlgItemMessage(hwndDlg, IDC_SEARCH_EXTENDED, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD), TRUE);
            break;
        case SearchData::Regex:
            SendDlgItemMessage(hwndDlg, IDC_SEARCH_REGEX, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD), FALSE);
            break;
        }
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_BACKWARD  , BM_SETCHECK, searchData.backward  ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_WHOLE_WORD, BM_SETCHECK, searchData.wholeWord ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_MATCH_CASE, BM_SETCHECK, searchData.matchCase ? BST_CHECKED : BST_UNCHECKED, 0);
        for (const auto& s : searchData.findHistory)
            SendDlgItemMessage(hwndDlg, IDC_FIND_WHAT, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        for (const auto& s : searchData.replaceHistory)
            SendDlgItemMessage(hwndDlg, IDC_REPLACE_WITH, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Find Mark Style"));
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 1"));
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 2"));
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 3"));
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 4"));
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 5"));
        if (searchData.customIndicator > 0)
            SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Custom Style"));
        switch (searchData.indicator) {
        case 31: SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_SETCURSEL, 0, 0); break;
        case 25: SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_SETCURSEL, 1, 0); break;
        case 24: SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_SETCURSEL, 2, 0); break;
        case 23: SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_SETCURSEL, 3, 0); break;
        case 22: SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_SETCURSEL, 4, 0); break;
        case 21: SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_SETCURSEL, 5, 0); break;
        default: SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_SETCURSEL, 6, 0);
        }
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR_AUTOCLEAR, BM_SETCHECK, searchData.autoClear ? BST_CHECKED : BST_UNCHECKED, 0);
        syncFindButton();
        SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, reinterpret_cast<LPARAM>(hwndDlg));
        return TRUE;
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            updateSearchSettings(searchData);
            GetWindowRect(searchData.dialog, &searchData.dialogLastPosition);
            searchData.dialog = 0;
            if (searchData.autoClear) {
                sci.SetIndicatorCurrent(searchData.indicator);
                sci.IndicatorClearRange(0, sci.Length());
            }
            SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(hwndDlg));
            DestroyWindow(hwndDlg);
            return TRUE;
        case IDOK:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            searchFind();
            syncFindButton();
            return TRUE;
        case IDC_SEARCH_COUNT:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            searchCount();
            syncFindButton();
            return TRUE;
        case IDC_SEARCH_REPLACE:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            updateReplaceHistory(searchData);
            searchReplace();
            syncFindButton();
            return TRUE;
        case IDC_SEARCH_REPLACE_ALL:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            updateReplaceHistory(searchData);
            searchReplaceAll();
            syncFindButton();
            return TRUE;
        case IDC_SEARCH_NORMAL:
        case IDC_SEARCH_EXTENDED:
        case IDC_SEARCH_REGEX:
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD),
                SendDlgItemMessage(hwndDlg, IDC_SEARCH_REGEX, BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE);
            break;
        case IDC_SEARCH_BACKWARD:
            updateSearchSettings(searchData);
            syncFindButton();
            break;
        case IDC_SEARCH_INDICATOR_CLEARNOW:
            sci.SetIndicatorCurrent(searchData.indicator);
            sci.IndicatorClearRange(0, sci.Length());
            break;
        case IDC_SEARCH_INDICATOR:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                auto i = SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR, CB_GETCURSEL, 0, 0);
                switch (i) {
                case 0 : searchData.indicator = 31; searchData.autoClear = false; break;
                case 1 : searchData.indicator = 25; searchData.autoClear = false; break;
                case 2 : searchData.indicator = 24; searchData.autoClear = false; break;
                case 3 : searchData.indicator = 23; searchData.autoClear = false; break;
                case 4 : searchData.indicator = 22; searchData.autoClear = false; break;
                case 5 : searchData.indicator = 21; searchData.autoClear = false; break;
                default: searchData.indicator = searchData.customIndicator; searchData.autoClear = true;
                }
                SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR_AUTOCLEAR, BM_SETCHECK, searchData.autoClear ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            break;
        }
        break;

    case WM_DRAWITEM:
        if (wParam == IDC_SEARCH_INDICATOR) {
            const DRAWITEMSTRUCT& dis = *reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            int indicator;
            switch (dis.itemID) {
            case 0 : indicator = 31; break;
            case 1 : indicator = 25; break;
            case 2 : indicator = 24; break;
            case 3 : indicator = 23; break;
            case 4 : indicator = 22; break;
            case 5 : indicator = 21; break;
            default: indicator = searchData.customIndicator;
            }
            RECT rect = dis.rcItem;
            int margin = (rect.bottom - rect.top) / 8;
            int side = rect.bottom - rect.top - 2 * margin;
            RECT square = {rect.left + margin,  rect.top + margin, rect.left + margin + side, rect.bottom - margin };
            Scintilla::Colour pageColor = sci.StyleGetBack(0);
            Scintilla::Colour indicatorColor = sci.IndicGetFore(indicator);
            int indicatorAlpha = static_cast<int>(sci.IndicGetAlpha(indicator));
            unsigned int ir = indicatorColor & 255;
            unsigned int ig = (indicatorColor >> 8) & 255;
            unsigned int ib = (indicatorColor >> 16) & 255;
            unsigned int pr = pageColor & 255;
            unsigned int pg = (pageColor >> 8) & 255;
            unsigned int pb = (pageColor >> 16) & 255;
            ir = (indicatorAlpha * ir + (255 - indicatorAlpha) * pr) / 255;
            ig = (indicatorAlpha * ig + (255 - indicatorAlpha) * pg) / 255;
            ib = (indicatorAlpha * ib + (255 - indicatorAlpha) * pb) / 255;
            unsigned int iColor = ir + (ig << 8) + (ib << 16);
            HBRUSH brush = CreateSolidBrush(iColor);
            FillRect(dis.hDC, &square, brush);
            DeleteObject(brush);
            rect.left += side + 3 * margin;
            DrawText(dis.hDC, reinterpret_cast<LPCTSTR>(dis.itemData), -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }
        break;

    case WM_GETMINMAXINFO :
    {
        MINMAXINFO& mmi = *reinterpret_cast<MINMAXINFO*>(lParam);
        mmi.ptMinTrackSize.x = searchData.dialogMinWidth;
        mmi.ptMinTrackSize.y = searchData.dialogHeight;
        mmi.ptMaxTrackSize.y = searchData.dialogHeight;
        return FALSE;
    }
    
    case WM_SIZING:
        switch (wParam) {
        case WMSZ_LEFT:
        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMLEFT:
        case WMSZ_RIGHT:
        case WMSZ_TOPRIGHT:
        case WMSZ_BOTTOMRIGHT:
            searchLayout(searchData, *reinterpret_cast<RECT*>(lParam));
            return TRUE;
        default:
            return FALSE;
        }

    case WM_SIZE:
    {
        RECT rcDialog;
        GetWindowRect(searchData.dialog, &rcDialog);
        searchLayout(searchData, rcDialog);
        return TRUE;
    }

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) searchData.wrap = false;
        break;

    default:
        break;
    
    }
    
    return FALSE;
}


bool searchPrepare(ColumnsPlusPlusData& data, std::string* sciFind, std::string* sciRepl = 0) {
    if (data.searchData.findWhat.length() == 0) {
        setSearchMessage(data, L"No string to find.");
        SetFocus(GetDlgItem(data.searchData.dialog, IDC_FIND_WHAT));
        return false;
    }
    UINT codepage = data.sci.CodePage();
    if (sciFind) {
        *sciFind = fromWide(data.searchData.findWhat, codepage);
        if (data.searchData.mode == SearchData::Extended) expandExtendedSearchString(*sciFind, codepage);
    }
    if (sciRepl) {
        *sciRepl = fromWide(data.searchData.replaceWith, codepage);
        if (data.searchData.mode == SearchData::Extended) expandExtendedSearchString(*sciRepl, codepage);
    }
    Scintilla::FindOption searchFlags = Scintilla::FindOption::None;
    if (data.searchData.wholeWord && data.searchData.mode != SearchData::Regex) searchFlags |= Scintilla::FindOption::WholeWord;
    if (data.searchData.matchCase) searchFlags |= Scintilla::FindOption::MatchCase;
    if (data.searchData.mode == SearchData::Regex) searchFlags |= Scintilla::FindOption::RegExp | Scintilla::FindOption::Posix
                                                               | static_cast<Scintilla::FindOption>(SCFIND_REGEXP_EMPTYMATCH_ALL);
    data.sci.SetSearchFlags(searchFlags);
    return true;
}

bool convertSelectionToSearchRegion(ColumnsPlusPlusData& data) {
    if (data.sci.Selections() < 2 || data.sci.SelectionMode() != Scintilla::SelectionMode::Stream) {
        RectangularSelection rs(data);
        rs.extend();
        if (!rs.size()) {
            setSearchMessage(data, L"No rectangular or multiple selection in which to search.");
            return false;
        }
    }
    data.sci.SetIndicatorCurrent(data.searchData.indicator);
    if (data.searchData.autoClear) data.sci.IndicatorClearRange(0, data.sci.Length());
    data.sci.SetIndicatorValue(1);
    int n = data.sci.Selections();
    for (int i = 0; i < n; ++i) data.sci.IndicatorFillRange(data.sci.SelectionNStart(i), data.sci.SelectionNEnd(i) - data.sci.SelectionNStart(i));
    return true;
}

void showRange(ColumnsPlusPlusData& data, Scintilla::Position foundStart, Scintilla::Position foundEnd) {
    Scintilla::Line foundLine = data.sci.LineFromPosition(foundStart);
    data.sci.SetVisiblePolicy(Scintilla::VisiblePolicy::Slop, 3 * static_cast<int>(data.sci.LinesOnScreen()) / 4);
    data.sci.EnsureVisibleEnforcePolicy(foundLine);
    int pxStart    = data.sci.PointXFromPosition(foundStart);
    int pxEnd      = data.sci.PointXFromPosition(foundEnd);
    int pxLine     = data.sci.PointXFromPosition(data.sci.PositionFromLine(foundLine));
    int pxOffset   = data.sci.XOffset();
    int marginLeft = pxOffset + pxLine;
    RECT scintillaRect;
    GetClientRect(data.activeScintilla, &scintillaRect);
    int scintillaTextWidth = scintillaRect.right - marginLeft - data.sci.MarginRight();
    if (pxStart < marginLeft || pxEnd > scintillaTextWidth + marginLeft) {
        if (pxEnd - pxLine <= scintillaTextWidth) data.sci.SetXOffset(0);
        else {
            int available = scintillaTextWidth - pxEnd + pxStart;
            if (available <= 0) data.sci.ScrollRange(foundStart, foundEnd);
            else data.sci.SetXOffset(pxStart - pxLine - (data.searchData.backward && data.searchData.mode != SearchData::Regex
                ? std::max(available / 2, available - 5 * data.sci.TextWidth(0, " "))
                : std::min(available / 2, 5 * data.sci.TextWidth(0, " "))));
        }
    }
    data.sci.SetSel(foundStart, foundEnd);
    data.sci.ChooseCaretX();
}

void ColumnsPlusPlusData::searchCount() {
    std::string sciFind;
    if (!searchPrepare(*this, &sciFind)) return;
    if (!searchRegionReady()) {
        if (!convertSelectionToSearchRegion(*this)) return;
        searchData.wrap = false;
    }
    Scintilla::Position documentLength = sci.Length();
    int count = 0;
    for (Scintilla::Position cpFrom = 0, cpTo; cpFrom < documentLength; cpFrom = cpTo) {
        cpTo = sci.IndicatorEnd(searchData.indicator, cpFrom);
        if (sci.IndicatorValueAt(searchData.indicator, cpFrom)) {
            while (cpFrom < cpTo) {
                sci.SetTargetRange(cpFrom, cpTo);
                Scintilla::Position found = sci.SearchInTarget(sciFind);
                if (found == -1) break;
                if (found < -1) {
                    if (found == -2) {
                        setSearchMessage(*this, L"Invalid regular expression.");
                        SetFocus(GetDlgItem(searchData.dialog, IDC_FIND_WHAT));
                    }
                    else setSearchMessage(*this, L"An unidentified error occurred (SEARCHINTARGET returned "
                                                 + std::to_wstring(found) + L").");
                    return;
                }
                ++count;
                Scintilla::Position targetEnd = sci.TargetEnd();
                cpFrom = cpFrom == targetEnd ? cpFrom + 1 : targetEnd;
            }
        }
    }
    setSearchMessage(*this, count == 0 ? L"No matches found in selection."
                          : count == 1 ? L"One match found in selection."
                                       : std::to_wstring(count) + L" matches found in selection.");
}

void ColumnsPlusPlusData::searchFind() {
    std::string sciFind;
    if (!searchPrepare(*this, &sciFind)) return;
    bool fullSearch = searchData.wrap;
    Scintilla::Position documentLength = sci.Length();
    if (!searchRegionReady()) {
        if (!convertSelectionToSearchRegion(*this)) return;
        fullSearch = true;
    }
    searchData.wrap = false;
    Scintilla::Position cpFrom = fullSearch ? (searchData.backward ? documentLength : 0) 
                               : searchData.backward ? std::min(sci.Anchor(), sci.CurrentPos())
                               : std::max(sci.Anchor(), sci.CurrentPos());
    Scintilla::Position cpTo;
    for (;;) {
        cpTo = searchData.backward ? sci.IndicatorStart(searchData.indicator, cpFrom - 1) : sci.IndicatorEnd(searchData.indicator, cpFrom);
        if (sci.IndicatorValueAt(searchData.indicator, searchData.backward ? cpTo : cpFrom)) {
            sci.SetTargetRange(cpFrom, cpTo);
            Scintilla::Position found = sci.SearchInTarget(sciFind);
            if (found >= 0) {
                showRange(*this, sci.TargetStart(), sci.TargetEnd());
                setSearchMessage(*this, L"");
                return;
            }
            if (found < -1) {
                if (found == -2) {
                    setSearchMessage(*this, L"Invalid regular expression.");
                    SetFocus(GetDlgItem(searchData.dialog, IDC_FIND_WHAT));
                }
                else setSearchMessage(*this, L"An unidentified error occurred (SEARCHINTARGET returned "
                    + std::to_wstring(found) + L").");
                return;
            }
        }
        if (searchData.backward ? cpTo == 0 : cpTo == documentLength) break;
        cpFrom = cpTo;
    }
    setSearchMessage(*this, fullSearch ? L"No matches found in selection." : L"No more matches found in selection.");
    searchData.wrap = true;
}

void ColumnsPlusPlusData::searchReplace() {
    if (!searchRegionReady()) return searchFind();
    std::string sciFind, sciRepl;
    if (!searchPrepare(*this, &sciFind, &sciRepl)) return;
    Scintilla::Position anchor = sci.Anchor();
    Scintilla::Position caret = sci.CurrentPos();
    sci.SetTargetRange(anchor, caret);
    Scintilla::Position found = sci.SearchInTarget(sciFind);
    if (found == anchor && sci.TargetEnd() == caret) {
        Scintilla::Position replacementLength = searchData.mode == SearchData::Regex ? sci.ReplaceTargetRE(sciRepl)
                                                                                     : sci.ReplaceTarget(sciRepl);
        sci.SetIndicatorCurrent(searchData.indicator);
        sci.SetIndicatorValue(1);
        sci.IndicatorFillRange(anchor, replacementLength);
        caret = anchor + replacementLength;
        showRange(*this, anchor, caret);
        setSearchMessage(*this, L"Match replaced.");
        searchData.wrap = false;
        return;
    }
    if (found >= -1) return searchFind();
    if (found == -2) {
        setSearchMessage(*this, L"Invalid regular expression.");
        SetFocus(GetDlgItem(searchData.dialog, IDC_FIND_WHAT));
        return;
    }
    setSearchMessage(*this, L"An unidentified error occurred (SEARCHINTARGET returned "
                             + std::to_wstring(found) + L").");
}

void ColumnsPlusPlusData::searchReplaceAll() {
    std::string sciFind, sciRepl;
    if (!searchPrepare(*this, &sciFind, &sciRepl)) return;
    if (!searchRegionReady()) {
        if (!convertSelectionToSearchRegion(*this)) return;
        searchData.wrap = false;
    }
    int count = 0;
    sci.BeginUndoAction();
    for (Scintilla::Position cpFrom = 0, cpTo; cpFrom < sci.Length(); cpFrom = cpTo) {
        cpTo = sci.IndicatorEnd(searchData.indicator, cpFrom);
        if (sci.IndicatorValueAt(searchData.indicator, cpFrom)) {
            while (cpFrom < cpTo) {
                sci.SetTargetRange(cpFrom, cpTo);
                Scintilla::Position found = sci.SearchInTarget(sciFind);
                if (found == -1) break;
                if (found < -1) {
                    if (found == -2) {
                        setSearchMessage(*this, L"Invalid regular expression.");
                        SetFocus(GetDlgItem(searchData.dialog, IDC_FIND_WHAT));
                    }
                    else setSearchMessage(*this, L"An unidentified error occurred (SEARCHINTARGET returned "
                        + std::to_wstring(found) + L").");
                    return;
                }
                ++count;
                Scintilla::Position oldLength = sci.TargetEnd() - found;
                Scintilla::Position newLength = searchData.mode == SearchData::Regex ? sci.ReplaceTargetRE(sciRepl)
                                                                                     : sci.ReplaceTarget(sciRepl);
                cpFrom = found + newLength;
                cpTo += newLength - oldLength;
                sci.SetIndicatorCurrent(searchData.indicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorFillRange(found, newLength);
            }
        }
    }
    sci.EndUndoAction();
    if (count > 0) {
        if (settings.elasticEnabled) {
            DocumentData& dd = *getDocument();
            analyzeTabstops(dd);
            setTabstops(dd);
        }
    }
    setSearchMessage(*this, count == 0 ? L"No matches found in selection."
                          : count == 1 ? L"One replacement made in selection."
                                       : std::to_wstring(count) + L" replacements made in selection.");
}
