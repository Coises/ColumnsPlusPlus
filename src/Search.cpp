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

#include "Search.h"
#include "Host\BoostRegexSearch.h"

std::regex rxFormat("\\s*(\\d{1,2}[t]?|t)?(?:([.,])(?:((\\d{1,2})?-)?(\\d{1,2}))?)?\\s*:(.*)", std::regex::optimize);

class RegexCalc {
public:

    class Formula {
    public:
        exprtk::expression<double> expression;
        NumericFormat format;
    };

    RegexCalcHistory history;
    exprtk::symbol_table<double> symbol_table;
    exprtk::parser<double>       parser;
    std::vector<Formula>         formula;
    std::vector<std::string>     replacement;
    double rcMatch = 0;
    double rcLine  = 0;
    double rcThis  = 0;
    RCReg  rcReg;
    RCSub  rcSub;
    RCLast rcLast;
    size_t maxCapture = 0;  // maximum number of capture groups possible (there could be fewer)

    RegexCalc() : rcReg(history), rcSub(history), rcLast(history) {
        symbol_table.add_variable("match", rcMatch);
        symbol_table.add_variable("line" , rcLine );
        symbol_table.add_variable("this" , rcThis );
        symbol_table.add_function("reg"  , rcReg  );
        symbol_table.add_function("sub"  , rcSub  );
        symbol_table.add_function("last" , rcLast );
    }

    void clear() {
        formula.clear();
        replacement.clear();
        history.values.clear();
        history.results.clear();
        history.lastFiniteResult.clear();
        rcMatch = 0;
        maxCapture = 0;
    }

};

SearchData::SearchData() { regexCalc = std::make_unique<RegexCalc>(); }
SearchData::~SearchData() {}

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
    if (searchData.enableCustomIndicator) {
        sci.IndicSetStyle(searchData.customIndicator, Scintilla::IndicatorStyle::FullBox);
        sci.IndicSetFore (searchData.customIndicator, searchData.customColor);
        sci.IndicSetAlpha(searchData.customIndicator, static_cast<Scintilla::Alpha>(searchData.customAlpha));
        sci.IndicSetUnder(searchData.customIndicator, true);
    }
    searchData.regexCalc->clear();
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

bool updateSearchRegion(ColumnsPlusPlusData& data, bool modify = false, bool remove = false) {
    bool postClear = data.searchData.autoClearSelection
                  || ( modify
                    && data.searchData.autoSetSelection
                    && ( data.sci.SelectionMode() != Scintilla::SelectionMode::Stream || data.sci.Selections() > 1 )
                    && ( data.sci.IndicatorValueAt(data.searchData.indicator, 0)
                      || ( data.sci.IndicatorEnd(data.searchData.indicator, 0) != 0
                        && data.sci.IndicatorEnd(data.searchData.indicator, 0) != data.sci.Length() ) ) );
    data.sci.SetIndicatorCurrent(data.searchData.indicator);
    if (!modify) data.sci.IndicatorClearRange(0, data.sci.Length());
    data.sci.SetIndicatorValue(1);
    int n = data.sci.Selections();
    if (remove)
         for (int i = 0; i < n; ++i) data.sci.IndicatorClearRange(data.sci.SelectionNStart(i), data.sci.SelectionNEnd(i) - data.sci.SelectionNStart(i));
    else for (int i = 0; i < n; ++i) data.sci.IndicatorFillRange (data.sci.SelectionNStart(i), data.sci.SelectionNEnd(i) - data.sci.SelectionNStart(i));
    if (postClear) data.sci.SetEmptySelection(data.sci.CurrentPos());
    return true;
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
    searchData.backward           = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_BACKWARD           , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.wholeWord          = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_WHOLE_WORD         , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.matchCase          = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_MATCH_CASE         , BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.autoClear          = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_INDICATOR_AUTOCLEAR, BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.autoClearSelection = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_SELECTION_AUTOCLEAR, BM_GETCHECK, 0, 0) == BST_CHECKED;
    searchData.autoSetSelection   = SendDlgItemMessage(searchData.dialog, IDC_SEARCH_SELECTION_AUTOSET  , BM_GETCHECK, 0, 0) == BST_CHECKED;
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

void showSearchError(ColumnsPlusPlusData& data, Scintilla::Position found) {
    if (found > -2) return;
    if (found < -2) {
        setSearchMessage(data, L"An unidentified error occurred (SEARCHINTARGET returned " + std::to_wstring(found) + L").");
        return;
    }
    setSearchMessage(data, L"Invalid regular expression.");
    HWND h = GetDlgItem(data.searchData.dialog, IDC_FIND_WHAT);
    SendMessage(data.searchData.dialog, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(h), TRUE);
    size_t n = data.sci.Call(static_cast<Scintilla::Message>(SCI_GETBOOSTREGEXERRMSG), 0, 0);
    if (n) {
        std::string msg(n, 0);
        data.sci.Call(static_cast<Scintilla::Message>(SCI_GETBOOSTREGEXERRMSG), n, reinterpret_cast<LPARAM>(msg.data()));
        std::wstring wmsg = toWide(msg, CP_UTF8);
        COMBOBOXINFO cbi;
        cbi.cbSize = sizeof(COMBOBOXINFO);
        GetComboBoxInfo(GetDlgItem(data.searchData.dialog, IDC_FIND_WHAT), &cbi);
        EDITBALLOONTIP ebt;
        ebt.cbStruct = sizeof(EDITBALLOONTIP);
        ebt.pszTitle = L"";
        ebt.ttiIcon = TTI_NONE;
        ebt.pszText = wmsg.data();
        SendMessage(cbi.hwndItem, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
    }
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
            CheckRadioButton(hwndDlg, IDC_SEARCH_NORMAL, IDC_SEARCH_REGEX, IDC_SEARCH_NORMAL);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_BACKWARD  ), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD), TRUE);
            break;
        case SearchData::Extended:
            CheckRadioButton(hwndDlg, IDC_SEARCH_NORMAL, IDC_SEARCH_REGEX, IDC_SEARCH_EXTENDED);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_BACKWARD  ), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD), TRUE);
            break;
        case SearchData::Regex:
            CheckRadioButton(hwndDlg, IDC_SEARCH_NORMAL, IDC_SEARCH_REGEX, IDC_SEARCH_REGEX);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_BACKWARD  ), FALSE);
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
        if (searchData.enableCustomIndicator)
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
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_INDICATOR_AUTOCLEAR, BM_SETCHECK, searchData.autoClear          ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_SELECTION_AUTOCLEAR, BM_SETCHECK, searchData.autoClearSelection ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SEARCH_SELECTION_AUTOSET  , BM_SETCHECK, searchData.autoSetSelection   ? BST_CHECKED : BST_UNCHECKED, 0);
        syncFindButton();
        SendMessage(nppData._nppHandle, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, reinterpret_cast<LPARAM>(hwndDlg));
        return TRUE;
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            sci.CallTipCancel();
            updateSearchSettings(searchData);
            GetWindowRect(searchData.dialog, &searchData.dialogLastPosition);
            searchData.dialog = 0;
            if (searchData.autoClear) {
                sci.SetIndicatorCurrent(searchData.indicator);
                sci.IndicatorClearRange(0, sci.Length());
            }
            searchData.regexCalc->clear();
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
        case IDC_SEARCH_SELECTION_SET:
            updateSearchSettings(searchData);
            updateSearchRegion(*this);
            return TRUE;
        case IDC_SEARCH_SELECTION_ADD:
            updateSearchSettings(searchData);
            updateSearchRegion(*this, true);
            return TRUE;
        case IDC_SEARCH_SELECTION_REMOVE:
            updateSearchSettings(searchData);
            updateSearchRegion(*this, true, true);
            return TRUE;
        case IDC_SEARCH_NORMAL:
        case IDC_SEARCH_EXTENDED:
        case IDC_SEARCH_REGEX:
        {
            bool enable = SendDlgItemMessage(hwndDlg, IDC_SEARCH_REGEX, BM_GETCHECK, 0, 0) == BST_CHECKED ? FALSE : TRUE;
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_BACKWARD  ), enable);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLE_WORD), enable);
            syncFindButton();
        }
            break;
        case IDC_SEARCH_BACKWARD:
            updateSearchSettings(searchData);
            syncFindButton();
            break;
        case IDC_SEARCH_INDICATOR_CLEARNOW:
            sci.SetIndicatorCurrent(searchData.indicator);
            sci.IndicatorClearRange(0, sci.Length());
            syncFindButton();
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


bool searchPrepare(ColumnsPlusPlusData& data, std::string* sciFind, std::vector<std::string>* sciRepl = 0, bool isStepwiseReplace = false) {
    if (data.searchData.findWhat.length() == 0) {
        setSearchMessage(data, L"No string to find.");
        SetFocus(GetDlgItem(data.searchData.dialog, IDC_FIND_WHAT));
        return false;
    }
    UINT codepage = data.sci.CodePage();
    if (sciFind) {
        sciFind[0] = fromWide(data.searchData.findWhat, codepage);
        if (data.searchData.mode == SearchData::Extended) expandExtendedSearchString(*sciFind, codepage);
        if (data.searchData.mode != SearchData::Regex) sciFind[1] = sciFind[0];
        else /* build alternate search expression replacing ^ with \G to be used as first search in a row or selection */ {
            std::string& s = sciFind[0];
            std::string& r = sciFind[1];
            bool charClass = false;
            bool quoted    = false;
            bool escaped   = false;
            for (size_t i = 0; i < s.length(); ++i) {
                if (!charClass && !quoted && !escaped && s[i] == '^') {
                    r += "\\G";
                    continue;
                }
                r += s[i];
                if      (quoted                 ) { if (s.substr(i, 2) == "\\E") quoted = false; }
                else if (escaped                ) escaped = false;
                else if (s.substr(i, 2) == "\\Q") quoted  = true;
                else if (s[i] == '\\'           ) escaped = true;
                else if (charClass              ) { if (s[i] == ']') charClass = false; }
                else if (s[i] == '['            ) charClass = true;
            }
        }
    }
    if (sciRepl) {
        std::string r = fromWide(data.searchData.replaceWith, codepage);
        switch (data.searchData.mode) {
        case SearchData::Extended:
            expandExtendedSearchString(r, codepage);
        case SearchData::Normal:
            sciRepl->push_back(r);
            break;
        default:
        {
            bool insideQuotes = false;
            int  depth = 0;
            sciRepl->emplace_back();
            for (size_t i = 0; i < r.length(); ++i)
                if (insideQuotes) {
                    if (r[i] == '\'') insideQuotes = false;
                    sciRepl->back() += r[i];
                }
                else if (depth) {
                    switch (r[i]) {
                    case '\'' :
                        insideQuotes = true;
                        break;
                    case '(' :
                        ++depth;
                        break;
                    case ')' :
                        if (!--depth) sciRepl->emplace_back();
                        break;
                    }
                    sciRepl->back() += r[i];
                }
                else {
                    sciRepl->back() += r[i];
                    switch (r[i]) {
                    case '(':
                        if (i < r.length() - 2 && r.substr(i, 3) == "(?=") {
                            i += 2;
                            sciRepl->emplace_back();
                            depth = 1;
                        }
                        break;
                    case '\\':
                        if (i < r.length() - 1) sciRepl->back() += r[++i];
                        break;
                    }
                }
        }
        }
    }
    Scintilla::FindOption searchFlags = Scintilla::FindOption::None;
    if (data.searchData.wholeWord && data.searchData.mode != SearchData::Regex) searchFlags |= Scintilla::FindOption::WholeWord;
    if (data.searchData.matchCase) searchFlags |= Scintilla::FindOption::MatchCase;
    if (data.searchData.mode == SearchData::Regex) {
        searchFlags |= Scintilla::FindOption::RegExp | Scintilla::FindOption::Posix | static_cast<Scintilla::FindOption>(SCFIND_REGEXP_EMPTYMATCH_ALL);
        if (isStepwiseReplace) searchFlags |= static_cast<Scintilla::FindOption>(SCFIND_REGEXP_EMPTYMATCH_ALLOWATSTART);
    }
    data.sci.SetSearchFlags(searchFlags);
    return true;
}

bool prepareSubstitutions(ColumnsPlusPlusData& data, const std::string& sciFind, const std::vector<std::string>& sciRepl) {
    auto& rc = *data.searchData.regexCalc;
    if (sciRepl == rc.replacement) return true;
    rc.clear();
    if (sciRepl.size() == 1) return true;
    for (size_t i = 1; i < sciRepl.size(); i += 2) {
        auto& formula = rc.formula.emplace_back();
        formula.expression.register_symbol_table(rc.symbol_table);
        std::string s = sciRepl[i];
        std::smatch m;
        if (std::regex_match(s, m, rxFormat)) {
            if (m[1].matched) {
                if (std::isdigit(m[1].str()[0])) formula.format.leftPad = std::stoi(m[1]);
                if (!(std::isdigit(m[1].str().back()))) formula.format.timeEnable = data.timeFormatEnable;
                formula.format.minDec = -1;
                formula.format.maxDec = 0;
            }
            if (m[2].matched) {
                formula.format.maxDec = 6;
                if (m[5].matched) {
                    formula.format.minDec = formula.format.maxDec = std::stoi(m[5]);
                    if (m[3].matched) formula.format.minDec = m[4].matched ? std::stoi(m[4]) : -1;
                }
            }
            s = m[6];
        }
        if (!rc.parser.compile(s, formula.expression)) {
            auto error = rc.parser.get_error(0);
            std::wstring msg = L"Formula " + std::to_wstring((i + 1) / 2) + L": " + toWide(error.diagnostic, CP_UTF8);
            COMBOBOXINFO cbi;
            cbi.cbSize = sizeof(COMBOBOXINFO);
            GetComboBoxInfo(GetDlgItem(data.searchData.dialog, IDC_REPLACE_WITH), &cbi);
            EDITBALLOONTIP ebt;
            ebt.cbStruct = sizeof(EDITBALLOONTIP);
            ebt.pszTitle = L"";
            ebt.ttiIcon = TTI_NONE;
            ebt.pszText = msg.data();
            SendMessage(cbi.hwndItem, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
            SendMessage(data.searchData.dialog, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(cbi.hwndItem), TRUE);
            return false;
        }
    }
    rc.history.lastFiniteResult.insert(rc.history.lastFiniteResult.end(), rc.formula.size(), 0);
    rc.maxCapture = std::count(sciFind.begin(), sciFind.end(), '(');
    rc.replacement = sciRepl;
    return true;
}

std::string calculateSubstitutions(ColumnsPlusPlusData& data, Scintilla::Position found) {
    std::string r;
    auto& rc = *data.searchData.regexCalc;
    ++rc.rcMatch;
    rc.rcLine     = static_cast<double>(data.sci.LineFromPosition(found) + 1);
    auto& values  = rc.history.values.emplace_back();
    auto& results = rc.history.results.emplace_back();
    values.push_back(rc.rcThis = data.parseNumber(data.sci.TargetText()));
    for (size_t j = 1; j <= rc.maxCapture; ++j) {
        std::string t = data.sci.Tag(static_cast<int>(j));
        if (t.empty()) continue;
        NumericParse np = data.parseNumber(t);
        if (!np) continue;
        if (values.size() < j) values.insert(values.end(), j - values.size(), std::numeric_limits<double>::quiet_NaN());
        values.push_back(np.value);
    }
    for (size_t i = 0; i < rc.replacement.size(); ++i) {
        if (i & 1) {
            rc.history.expressionIndex = (i - 1) / 2;
            auto& formula = rc.formula[(i - 1) / 2];
            double result = formula.expression.value();
            results.push_back(result);
            if (std::isfinite(result)) {
                r += data.formatNumber(result, formula.format);
            }
            else if (formula.expression.results().count()) {
                const auto& exResults = formula.expression.results();
                bool lastWasNumeric = false;
                for (size_t j = 0; j < exResults.count(); ++j) {
                    auto& rj = exResults[j];
                    if (rj.type == rj.e_string) {
                        exprtk::igeneric_function<double>::generic_type::string_view sv(rj);
                        r += std::string_view(sv.data_, sv.size());
                        lastWasNumeric = false;
                    }
                    else if (rj.type == exprtk::type_store<double>::store_type::e_scalar) {
                        if (lastWasNumeric) r += ' ';
                        exprtk::igeneric_function<double>::generic_type::scalar_view sv(rj);
                        r += data.formatNumber(sv(), formula.format);
                        lastWasNumeric = true;
                    }
                }
            }
        }
        else r += rc.replacement[i];
    }
    for (size_t i = 0; i < results.size(); ++i) if (std::isfinite(results[i])) rc.history.lastFiniteResult[i] = results[i];
    return r;
}

bool convertSelectionToSearchRegion(ColumnsPlusPlusData& data) {
    RectangularSelection rs(data);
    if (rs.size() || (data.sci.Selections() < 2 && rs.anchor().ln == rs.caret().ln)) {
        rs.extend();
        if (!rs.size()) return false;
    }
    int n = data.sci.Selections();
    for (int i = 0;; ++i) {
        if (i >= n) {
            MessageBox(data.searchData.dialog, L"Could not construct a search region from this selection.", L"Search in indicated region", MB_ICONERROR);
            return false;
        }
        if (data.sci.SelectionNStart(i) < data.sci.SelectionNEnd(i)) break;
    }
    data.sci.SetIndicatorCurrent(data.searchData.indicator);
    data.sci.IndicatorClearRange(0, data.sci.Length());
    data.sci.SetIndicatorValue(1);
    for (int i = 0; i < n; ++i) data.sci.IndicatorFillRange(data.sci.SelectionNStart(i), data.sci.SelectionNEnd(i) - data.sci.SelectionNStart(i));
    if (data.searchData.autoClearSelection) data.sci.SetEmptySelection(data.sci.CurrentPos());
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
                ? std::max(available / 2, available - 5 * data.sci.TextWidth(STYLE_DEFAULT, " "))
                : std::min(available / 2, 5 * data.sci.TextWidth(STYLE_DEFAULT, " "))));
        }
    }
    data.sci.SetSel(foundStart, foundEnd);
    data.sci.ChooseCaretX();
}

void ColumnsPlusPlusData::searchCount() {
    std::string sciFind[2];
    if (!searchPrepare(*this, sciFind)) return;
    if (!searchRegionReady()) {
        if (!convertSelectionToSearchRegion(*this)) return;
        searchData.wrap = false;
    }
    sci.CallTipCancel();
    Scintilla::Position documentLength = sci.Length();
    int count = 0;
    for (Scintilla::Position cpFrom = 0, cpTo; cpFrom < documentLength; cpFrom = cpTo) {
        cpTo = sci.IndicatorEnd(searchData.indicator, cpFrom);
        if (sci.IndicatorValueAt(searchData.indicator, cpFrom)) {
            int startOfSelectionSegment = 1;
            while (cpFrom < cpTo) {
                sci.SetTargetRange(cpFrom, cpTo);
                Scintilla::Position found = sci.SearchInTarget(sciFind[startOfSelectionSegment]);
                if (found == -1) break;
                if (found < -1) return showSearchError(*this, found);
                ++count;
                cpFrom = sci.TargetEnd();
                startOfSelectionSegment = 0;
            }
        }
    }
    setSearchMessage(*this, count == 0 ? L"No matches found."
                          : count == 1 ? L"One match found."
                                       : std::to_wstring(count) + L" matches found.");
}

void ColumnsPlusPlusData::searchFind(bool postReplace) {
    std::string sciFind[2];
    if (!searchPrepare(*this, sciFind)) return;
    bool fullSearch = searchData.wrap;
    Scintilla::Position documentLength = sci.Length();
    if (!searchRegionReady()) {
        if (!convertSelectionToSearchRegion(*this)) return;
        fullSearch = true;
    }
    searchData.wrap = false;
    bool backward = searchData.mode != SearchData::Regex && searchData.backward;
    sci.CallTipCancel();
    Scintilla::Position cpFrom = fullSearch ? (backward ? documentLength : 0)
                               : backward ? std::min(sci.Anchor(), sci.CurrentPos())
                               : std::max(sci.Anchor(), sci.CurrentPos());
    Scintilla::Position cpTo;
    int startOfSelectionSegment = fullSearch || backward || !cpFrom || !sci.IndicatorValueAt(searchData.indicator, cpFrom - 1) ? 1 : 0;
    for (;;) {
        cpTo = backward ? sci.IndicatorStart(searchData.indicator, cpFrom - 1) : sci.IndicatorEnd(searchData.indicator, cpFrom);
        if (sci.IndicatorValueAt(searchData.indicator, backward ? cpTo : cpFrom)) {
            sci.SetTargetRange(cpFrom, cpTo);
            Scintilla::Position found = sci.SearchInTarget(sciFind[startOfSelectionSegment]);
            if (found >= 0) {
                Scintilla::Position targetStart = sci.TargetStart();
                Scintilla::Position targetEnd   = sci.TargetEnd();
                showRange(*this, targetStart, targetEnd);
                if (targetStart == targetEnd) sci.CallTipShow(targetStart, "^ zero length match");
                setSearchMessage(*this, !postReplace        ? L""
                                      : backward ? L"Match replaced; previous match found."
                                                            : L"Match replaced; next match found.");
                return;
            }
            if (found < -1) return showSearchError(*this, found);
        }
        if (backward ? cpTo == 0 : cpTo == documentLength) break;
        cpFrom = cpTo;
        startOfSelectionSegment = 1;
    }
    setSearchMessage(*this, postReplace ? L"Match replaced; no more matches found."
                          : fullSearch  ? L"No matches found." : L"No more matches found.");
    searchData.wrap = true;
}

void ColumnsPlusPlusData::searchReplace() {
    if (!searchRegionReady()) return searchFind();
    std::string sciFind[2];
    std::vector<std::string> sciRepl;
    if (!searchPrepare(*this, sciFind, &sciRepl, true)) return;
    if (!prepareSubstitutions(*this, sciFind[0], sciRepl)) return;
    Scintilla::Position anchor = sci.Anchor();
    Scintilla::Position caret  = sci.CurrentPos();
    Scintilla::Position start  = std::min(anchor, caret);
    Scintilla::Position end    = std::max(anchor, caret);
    if (!sci.IndicatorValueAt(searchData.indicator, start)) return searchFind();
    sci.CallTipCancel();
    sci.SetTargetRange(start, sci.IndicatorEnd(searchData.indicator, start));
    Scintilla::Position found = sci.SearchInTarget(sciFind[start == 0 || !sci.IndicatorValueAt(searchData.indicator, start - 1) ? 1 :0]);
    if (found == start && sci.TargetEnd() == end) {
        std::string r = sciRepl.size() == 1 ? sciRepl[0] : calculateSubstitutions(*this, found);
        Scintilla::Position replacementLength = searchData.mode == SearchData::Regex ? sci.ReplaceTargetRE(r)
                                                                                     : sci.ReplaceTarget(r);
        sci.SetIndicatorCurrent(searchData.indicator);
        sci.SetIndicatorValue(1);
        sci.IndicatorFillRange(start, replacementLength);
        caret = searchData.mode != SearchData::Regex && searchData.backward ? start : start + replacementLength;
        showRange(*this, caret, caret);
        setSearchMessage(*this, L"Match replaced.");
        searchData.wrap = false;
        if (!replaceStaysPut) searchFind(true);
        return;
    }
    if (found >= -1) return searchFind();
    showSearchError(*this, found);
}

void ColumnsPlusPlusData::searchReplaceAll() {
    std::string sciFind[2];
    std::vector<std::string> sciRepl;
    if (!searchPrepare(*this, sciFind, &sciRepl)) return;
    if (!searchRegionReady()) {
        if (!convertSelectionToSearchRegion(*this)) return;
        searchData.wrap = false;
    }
    if (!prepareSubstitutions(*this, sciFind[0], sciRepl)) return;
    sci.CallTipCancel();
    int count = 0;
    sci.BeginUndoAction();
    for (Scintilla::Position cpFrom = 0, cpTo; cpFrom < sci.Length(); cpFrom = cpTo) {
        cpTo = sci.IndicatorEnd(searchData.indicator, cpFrom);
        if (sci.IndicatorValueAt(searchData.indicator, cpFrom)) {
            int startOfSelectionSegment = 1;
            while (cpFrom < cpTo) {
                sci.SetTargetRange(cpFrom, cpTo);
                Scintilla::Position found = sci.SearchInTarget(sciFind[startOfSelectionSegment]);
                if (found == -1) break;
                if (found < -1) return showSearchError(*this, found);
                ++count;
                Scintilla::Position oldLength = sci.TargetEnd() - found;
                std::string r = sciRepl.size() == 1 ? sciRepl[0] : calculateSubstitutions(*this, found);
                Scintilla::Position newLength = searchData.mode == SearchData::Regex ? sci.ReplaceTargetRE(r)
                                                                                     : sci.ReplaceTarget(r);
                cpFrom = found + newLength;
                cpTo += newLength - oldLength;
                sci.SetIndicatorCurrent(searchData.indicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorFillRange(found, newLength);
                startOfSelectionSegment = 0;
            }
        }
    }
    sci.EndUndoAction();
    searchData.regexCalc->clear();
    if (count > 0) {
        if (settings.elasticEnabled) {
            DocumentData& dd = *getDocument();
            analyzeTabstops(dd);
            setTabstops(dd);
        }
    }
    setSearchMessage(*this, count == 0 ? L"No matches found."
                          : count == 1 ? L"One replacement made."
                                       : std::to_wstring(count) + L" replacements made.");
}
