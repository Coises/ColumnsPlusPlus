// This file is part of Columns++ for Notepad++.
// Copyright 2023 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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
    searchData.backward  = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_BACKWARD   , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.wholeWord = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_WHOLE_WORD , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.matchCase = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_MATCH_CASE , BM_GETCHECK, 0, 0) == BST_CHECKED;
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
    RECT msgRect;
    GetWindowRect(msgHwnd, &msgRect);
    MapWindowPoints(0, data.searchData.dialog, reinterpret_cast<LPPOINT>(&msgRect), 1);
    InvalidateRect(data.searchData.dialog, &msgRect, TRUE);
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
                RECT rc, rcOwner;
                HWND owner = GetParent(hwndDlg);
                GetWindowRect(owner ? owner : GetDesktopWindow(), &rcOwner);
                CopyRect(&rc, &rcOwner);
                OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
                OffsetRect(&rc, -rc.left, -rc.top);
                OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
                SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + rc.right / 2, rcOwner.top + rc.bottom / 2, 0, 0, SWP_NOSIZE);
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
        syncFindButton(*getDocument());
        SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, reinterpret_cast<LPARAM>(hwndDlg));
        return TRUE;
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            updateSearchSettings(searchData);
            GetWindowRect(searchData.dialog, &searchData.dialogLastPosition);
            searchData.dialog = 0;
            SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<LPARAM>(hwndDlg));
            DestroyWindow(hwndDlg);
            return TRUE;
        case IDOK:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            searchFind();
            syncFindButton(*getDocument());
            return TRUE;
        case IDC_SEARCH_COUNT:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            searchCount();
            syncFindButton(*getDocument());
            return TRUE;
        case IDC_SEARCH_REPLACE:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            updateReplaceHistory(searchData);
            searchReplace();
            syncFindButton(*getDocument());
            return TRUE;
        case IDC_SEARCH_REPLACE_ALL:
            updateSearchSettings(searchData);
            updateFindHistory(searchData);
            updateReplaceHistory(searchData);
            searchReplaceAll();
            syncFindButton(*getDocument());
            return TRUE;
        case IDC_SEARCH_NORMAL:
        case IDC_SEARCH_EXTENDED:
        case IDC_SEARCH_REGEX:
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD),
                SendDlgItemMessage(hwndDlg, IDC_SEARCH_REGEX, BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE);
            break;
        case IDC_SEARCH_BACKWARD:
            updateSearchSettings(searchData);
            syncFindButton(*getDocument());
            break;
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

RectangularSelection getSearchSelection(ColumnsPlusPlusData& data, DocumentData& dd) {
    RectangularSelection rs(data);
    if (dd.search.selectionValid && rs.anchor().cp == rs.caret().cp && rs.anchor().vs == rs.caret().vs) {
        dd.search.lastMatch = dd.search.None;
        dd.search.lastAnchor = dd.search.lastCaret = rs.anchor().cp;
        rs.loadBounds(dd.search.selection);
    }
    else if (dd.search.selectionValid && dd.search.lastMatch != dd.search.None
             && rs.anchor().cp == dd.search.lastAnchor && rs.caret().cp == dd.search.lastCaret
             && rs.anchor().vs == 0 && rs.caret().vs == 0) {
        rs.loadBounds(dd.search.selection);
    }
    else {
        rs.extend();
        if (rs.size()) {
            dd.search.selection = rs.getBounds();
            dd.search.selectionValid = true;
            dd.search.lastMatch = dd.search.None;
            dd.search.lastAnchor = dd.search.lastCaret = data.searchData.backward ? std::numeric_limits<Scintilla::Position>::max() : 0;
        }
        else {
            dd.search.selectionValid = false;
            dd.search.lastMatch = dd.search.None;
            setSearchMessage(data, L"No rectangular selection in which to search.");
        }
    }
    return rs;
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
    DocumentData& dd = *getDocument();
    std::string sciFind;
    if (!searchPrepare(*this, &sciFind)) return;
    RectangularSelection rs = getSearchSelection(*this, dd);
    if (!dd.search.selectionValid) return;
    int count = 0;
    for (auto row : rs) {
        Scintilla::Position cpMin = row.cpMin();
        Scintilla::Position cpMax = row.cpMax();
        for (;;) {
            sci.SetTargetRange(cpMin, cpMax);
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
            cpMin = cpMin == targetEnd ? cpMin + 1 : targetEnd;
            if (cpMin >= cpMax) break;
        }
    }
    setSearchMessage(*this, count == 0 ? L"No matches found in selection."
                          : count == 1 ? L"One match found in selection."
                                       : std::to_wstring(count) + L" matches found in selection.");
}

void ColumnsPlusPlusData::searchFind() {
    DocumentData& dd = *getDocument();
    std::string sciFind;
    if (!searchPrepare(*this, &sciFind)) return;
    RectangularSelection rs = getSearchSelection(*this, dd);
    if (!dd.search.selectionValid) return;
    Scintilla::Position searchFrom = searchData.backward && searchData.mode != SearchData::Regex ? dd.search.lastAnchor : dd.search.lastCaret;
    rs.reverse(searchData.backward);
    for (auto row : rs) {
        Scintilla::Position cpMin = row.cpMin();
        Scintilla::Position cpMax = row.cpMax();
        if (!searchData.backward) {
            if (cpMax <= searchFrom) continue;
            sci.SetTargetRange(std::max(searchFrom, cpMin), cpMax);
        }
        else if (searchData.mode == SearchData::Regex) {
            if (cpMin > searchFrom || cpMax == searchFrom) continue;
            sci.SetTargetRange(cpMax < searchFrom ? cpMin : std::max(searchFrom, cpMin), cpMax);
        }
        else {
            if (cpMin >= searchFrom) continue;
            sci.SetTargetRange(std::min(searchFrom, cpMax), cpMin);
        }
        Scintilla::Position found = sci.SearchInTarget(sciFind);
        if (found >= 0) {
            dd.search.lastAnchor = sci.TargetStart();
            dd.search.lastCaret  = sci.TargetEnd();
            dd.search.lastMatch  = dd.search.Find;
            showRange(*this, dd.search.lastAnchor, dd.search.lastCaret);
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
    dd.search.lastMatch = dd.search.None;
    setSearchMessage(*this, searchFrom == 0 || searchFrom == std::numeric_limits<Scintilla::Position>::max()
                            ? L"No matches found in selection." : L"No more matches found in selection.");
}

void ColumnsPlusPlusData::searchReplace() {
    DocumentData& dd = *getDocument();
    if (!dd.search.selectionValid || dd.search.lastMatch != dd.search.Find) return searchFind();
    RectangularSelection rs(*this);
    if (rs.anchor().cp != dd.search.lastAnchor || rs.caret().cp != dd.search.lastCaret
        || rs.anchor().vs != 0 || rs.caret().vs != 0) return searchFind();
    std::string sciFind, sciRepl;
    if (!searchPrepare(*this, &sciFind, &sciRepl)) return;
    sci.SetTargetRange(dd.search.lastAnchor, dd.search.lastCaret);
    Scintilla::Position found = sci.SearchInTarget(sciFind);
    if (found == dd.search.lastAnchor && sci.TargetEnd() == dd.search.lastCaret) {
        dd.search.lastCaret = ( searchData.mode == SearchData::Regex ? sci.ReplaceTargetRE(sciRepl)
                                                                     : sci.ReplaceTarget(sciRepl) )
                            + dd.search.lastAnchor;
        dd.search.lastMatch = dd.search.Replace;
        showRange(*this, dd.search.lastAnchor, dd.search.lastCaret);
        setSearchMessage(*this, L"Match replaced.");
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
    DocumentData& dd = *getDocument();
    std::string sciFind, sciRepl;
    if (!searchPrepare(*this, &sciFind, &sciRepl)) return;
    auto rs = getSearchSelection(*this, dd);
    if (!dd.search.selectionValid) return;
    int count = 0;
    sci.BeginUndoAction();
    for (auto row : rs) {
        Scintilla::Position cpMin = row.cpMin();
        Scintilla::Position cpMax = row.cpMax();
        for (;;) {
            sci.SetTargetRange(cpMin, cpMax);
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
            Scintilla::Position oldLength = sci.TargetEnd() - found;
            Scintilla::Position newLength = searchData.mode == SearchData::Regex ? sci.ReplaceTargetRE(sciRepl)
                                                                                 : sci.ReplaceTarget(sciRepl);
            ++count;
            cpMin = found + newLength;
            cpMax = cpMax + newLength - oldLength;
            if (cpMin >= cpMax) break;
        }
    }
    sci.EndUndoAction();
    if (count > 0) {
        if (settings.elasticEnabled) {
            analyzeTabstops(dd);
            setTabstops(dd);
        }
        rs.loadBounds(dd.search.selection);
    }
    setSearchMessage(*this, count == 0 ? L"No matches found in selection."
                          : count == 1 ? L"One replacement made in selection."
                                       : std::to_wstring(count) + L" replacements made in selection.");
}
