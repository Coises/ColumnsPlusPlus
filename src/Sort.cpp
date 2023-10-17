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

#include "RegularExpression.h"
#include "commctrl.h"
#include "resource.h"
#include <algorithm>
#include <regex>

namespace {

const std::wregex keycap(L"(?:(\\d+)(?:([ad][bln]?|[bln][ad]?)?))"
                         L"(?:"
                             L"[,; ]*"
                             L"("
                                 L"\\d+(?:(?:[ad][bln]?|[bln][ad]?)?)"
                                 L"(?:[,; ]*\\d+(?:(?:[ad][bln]?|[bln][ad]?)?))*"
                             L")"
                         L")?"
                         , std::wregex::icase | std::wregex::optimize);

struct SortKeyDescriptor {
    std::string string;
    double      number;
    bool        numeric;
    bool        descending;
    SortKeyDescriptor() = default;
    SortKeyDescriptor(const std::string& string, bool descending = false) : string(string), numeric(false), descending(descending) {}
    SortKeyDescriptor(double number            , bool descending = false) : number(number), numeric(true ), descending(descending) {}
};

struct LinePointers {
    const char* line;
    const char* left;
    const char* right;
    Scintilla::Position vsLeft;
    Scintilla::Position vsRight;
    size_t lineLength;
    int selection;
};

struct SortSelectionLine : LinePointers {
    std::vector<SortKeyDescriptor> keys;
};

struct SortSelection : std::vector<SortSelectionLine> {
    Scintilla::Line textLine;
    Scintilla::Position textStart, textEnd;
    std::string text;
};

std::string getLocaleSortKey(const std::string& text, UINT codepage, DWORD options, LPCWSTR locale) {
    if (text.empty()) return "";
    std::wstring wideText = toWide(text, codepage);
    int wideTextLength = clamp_cast<int>(wideText.length());
    int m = LCMapStringEx(locale, options, wideText.data(), wideTextLength, 0, 0, 0, 0, 0);
    std::string key(m, 0);
    LCMapStringEx(locale, options, wideText.data(), wideTextLength, (LPWSTR) key.data(), m, 0, 0, 0);
    return key;
}


void replaceSortSelection(ColumnsPlusPlusData& data, SortSelection& ss, const RectangularSelection& rs) {

    std::string r;
    r.reserve(ss.text.length());
    Scintilla::Line lines = ss.size();
    for (int i = 0; i < lines; ++i) r.append(ss[i].line, ss[i].lineLength);

    if (ss.text.length() > static_cast<size_t>(ss.textEnd - ss.textStart)) /* remove line ending from last line of sorted text */ {
        if (r.back() == '\n') r.pop_back();
        if (r.back() == '\r') r.pop_back();
    }

    bool topToBottom = rs.topToBottom();
    bool tlbr        = topToBottom == rs.leftToRight();
    Scintilla::Position cpTop    = (tlbr ? ss[0        ].left    : ss[0        ].right) - ss[0].line;
    Scintilla::Position cpBottom = (tlbr ? ss[lines - 1].right   : ss[lines - 1].left ) - ss[lines - 1].line;
    Scintilla::Position vsTop    = (tlbr ? ss[0        ].vsLeft  : ss[0        ].vsRight);
    Scintilla::Position vsBottom = (tlbr ? ss[lines - 1].vsRight : ss[lines - 1].vsLeft );

    data.sci.SetTargetRange(ss.textStart, ss.textEnd);
    data.sci.ReplaceTarget(r);
    if (data.settings.elasticEnabled) {
        DocumentData* ddp = data.getDocument();
        data.analyzeTabstops(*ddp);
        Scintilla::Line firstVisible = data.sci.FirstVisibleLine();
        Scintilla::Line lastVisible = firstVisible + data.sci.LinesOnScreen();
        data.setTabstops(*ddp, std::min(ss.textLine, firstVisible), std::max(ss.textLine + lines - 1, lastVisible));
    }

    cpTop    += ss.textStart;
    cpBottom += data.sci.PositionFromLine(ss.textLine + lines - 1);
    data.sci.SetRectangularSelectionAnchor            (topToBottom ? cpTop    : cpBottom);
    data.sci.SetRectangularSelectionCaret             (topToBottom ? cpBottom : cpTop   );
    data.sci.SetRectangularSelectionAnchorVirtualSpace(topToBottom ? vsTop    : vsBottom);
    data.sci.SetRectangularSelectionCaretVirtualSpace (topToBottom ? vsBottom : vsTop   );

}


void replaceSortColumn(ColumnsPlusPlusData& data, const SortSelection& ss, const std::vector<LinePointers>& us, const RectangularSelection& rs) {

    std::string r;
    r.reserve(ss.textEnd - ss.textStart);
    Scintilla::Line lines = ss.size();
    for (Scintilla::Line i = 0; i < lines; ++i) {
        const auto& s = ss[i];
        const auto& u = us[i];
        r.append(u.line, u.left - u.line);
        r.append(u.vsLeft, ' ');
        r.append(s.left, s.right - s.left);
        r.append(s.vsRight - s.vsLeft, ' ');
        r.append(u.right, u.lineLength - (u.right - u.line));
    }

    bool topToBottom = rs.topToBottom();
    bool tlbr        = topToBottom == rs.leftToRight();
    Scintilla::Position cpTop    = us[0        ].left - us[0        ].line + us[0        ].vsLeft;
    Scintilla::Position cpBottom = us[lines - 1].left - us[lines - 1].line + us[lines - 1].vsLeft;
    if (tlbr) cpBottom += ss[lines - 1].right - ss[lines - 1].left + ss[lines - 1].vsRight;
         else cpTop    += ss[0        ].right - ss[0        ].left + ss[0        ].vsRight;

    data.sci.SetTargetRange(ss.textStart, ss.textEnd);
    data.sci.ReplaceTarget(r);
    if (data.settings.elasticEnabled) {
        DocumentData* ddp = data.getDocument();
        data.analyzeTabstops(*ddp);
        Scintilla::Line firstVisible = data.sci.FirstVisibleLine();
        Scintilla::Line lastVisible = firstVisible + data.sci.LinesOnScreen();
        data.setTabstops(*ddp, std::min(ss.textLine, firstVisible), std::max(ss.textLine + lines - 1, lastVisible));
    }

    cpTop    += ss.textStart;
    cpBottom += data.sci.PositionFromLine(ss.textLine + lines - 1);
    data.sci.SetRectangularSelectionAnchor            (topToBottom ? cpTop    : cpBottom);
    data.sci.SetRectangularSelectionCaret             (topToBottom ? cpBottom : cpTop   );
    data.sci.SetRectangularSelectionAnchorVirtualSpace(0);
    data.sci.SetRectangularSelectionCaretVirtualSpace (0);

}


void sortCommon(ColumnsPlusPlusData& data, const SortSettings& sortSettings) {

    auto rs = data.getRectangularSelection();
    int lines = rs.size();
    if (lines < 2) return;

    DWORD options = LCMAP_SORTKEY | NORM_LINGUISTIC_CASING;
    if (!sortSettings.localeCaseSensitive  ) options |= LINGUISTIC_IGNORECASE;
    if (sortSettings.localeDigitsAsNumbers ) options |= SORT_DIGITSASNUMBERS;
    if (sortSettings.localeIgnoreDiacritics) options |= LINGUISTIC_IGNOREDIACRITIC;
    if (sortSettings.localeIgnoreSymbols   ) options |= NORM_IGNORESYMBOLS;

    UINT    codepage = data.sci.CodePage();
    LPCWSTR locale   = sortSettings.localeName.data();
    bool    forward  = rs.topToBottom();

    std::vector<unsigned int>           capGroup;
    std::vector<bool>                   capDesc;
    std::vector<SortSettings::SortType> capType;
    if (sortSettings.keyType == SortSettings::Tabbed || (sortSettings.keyType == SortSettings::Regex && sortSettings.regexUseKey)) {
        std::wstring s = sortSettings.keygroupHistory.back();
        std::wsmatch m;
        while (std::regex_match(s, m, keycap)) {
            capGroup.push_back(std::stoi(m[1]));
            std::wstring t = m[2];
            if      (t.find_first_of(L"aA") != std::wstring::npos) capDesc.push_back(false);
            else if (t.find_first_of(L"dD") != std::wstring::npos) capDesc.push_back(true );
            else                                                   capDesc.push_back(sortSettings.sortDescending);
            if      (t.find_first_of(L"bB") != std::wstring::npos) capType.push_back(SortSettings::Binary );
            else if (t.find_first_of(L"lL") != std::wstring::npos) capType.push_back(SortSettings::Locale );
            else if (t.find_first_of(L"nN") != std::wstring::npos) capType.push_back(SortSettings::Numeric);
            else                                                   capType.push_back(sortSettings.sortType);
            s = m[3];
        }
    }
    else if (sortSettings.keyType == SortSettings::Regex) {
        capGroup.push_back(0);
        capDesc .push_back(sortSettings.sortDescending);
        capType .push_back(sortSettings.sortType);
    }

    SortSelection ss;
    std::vector<LinePointers> unsortedLinePointers;
    ss.resize(lines);
    unsortedLinePointers.resize(lines);
    std::string appendedEOL;

    ss.textLine  = rs.top().ln;
    ss.textStart = rs.top().st;
    ss.textEnd   = data.sci.PositionFromLine(ss.textLine + lines);
    ss.text      = data.sci.StringOfRange(Scintilla::Span(ss.textStart, ss.textEnd));
    if (ss.textLine + lines == data.sci.LineCount()) /* last line in selection has no line terminator */ {
        auto eolMode = data.sci.EOLMode();
        appendedEOL = eolMode == Scintilla::EndOfLine::Cr ? "\r" : eolMode == Scintilla::EndOfLine::Lf ? "\n" : "\r\n";
        ss.text += appendedEOL;
    }

    char* textPointer = ss.text.data();
    Scintilla::Position cpNextLine = ss.textEnd;

    for (int n = lines - 1; n >= 0; --n) {

        ss[n].selection = forward ? n : lines - 1 - n;
        auto row = rs[ss[n].selection];

        Scintilla::Position cpLine = data.sci.PositionFromLine(row.line());
        Scintilla::Position cpMin  = row.cpMin();
        Scintilla::Position cpMax  = row.cpMax();
        ss[n].line    = cpLine - ss.textStart + textPointer;
        ss[n].left    = cpMin  - ss.textStart + textPointer;
        ss[n].right   = cpMax  - ss.textStart + textPointer;
        ss[n].vsLeft  = row.vsMin();
        ss[n].vsRight = row.vsMax();

        if (sortSettings.keyType == SortSettings::Regex) {
            RegularExpression rx(data);
            rx.find(sortSettings.regexHistory.back(), sortSettings.regexMatchCase);
            if (rx.search(row.text())) {
                for (size_t i = 0; i < capGroup.size(); ++i) {
                    std::string s = rx.str(capGroup[i]);
                    if (capType[i] == SortSettings::Numeric) ss[n].keys.emplace_back(data.parseNumber(s), capDesc[i]);
                    else {
                        if (capType[i] == SortSettings::Locale) s = getLocaleSortKey(s, codepage, options, locale);
                        ss[n].keys.emplace_back(s, capDesc[i]);
                    }
                }
            }
        }
        else if (sortSettings.keyType == SortSettings::Tabbed) {
            std::vector<std::string> cells;
            cells.emplace_back(row.text());
            for (const auto& cell : row) cells.push_back(cell.text());
            for (size_t i = 0; i < capGroup.size(); ++i) {
                std::string s = capGroup[i] < cells.size() ? cells[capGroup[i]] : "";
                if (capType[i] == SortSettings::Numeric) ss[n].keys.emplace_back(data.parseNumber(s), capDesc[i]);
                else {
                    if (capType[i] == SortSettings::Locale) s = getLocaleSortKey(s, codepage, options, locale);
                    ss[n].keys.emplace_back(s, capDesc[i]);
                }
            }
        }
        else if (sortSettings.sortType == SortSettings::Numeric)
            for (const auto& cell : row) ss[n].keys.emplace_back(data.parseNumber(cell.trim()), sortSettings.sortDescending);
        else {
            std::string s(row.text());
            if (sortSettings.keyType == SortSettings::IgnoreBlanks) {
                size_t i = s.find_first_not_of("\t ");
                if (i == std::string::npos) s = "";
                else {
                    size_t j = s.find_last_not_of("\t ");
                    s = s.substr(i, j - i + 1);
                }
            }
            if (sortSettings.sortType == SortSettings::Locale) s = getLocaleSortKey(s, codepage, options, locale);
            ss[n].keys.emplace_back(s, sortSettings.sortDescending);
        }

        ss[n].lineLength = cpNextLine - cpLine;
        unsortedLinePointers[n] = ss[n];
        cpNextLine = cpLine;

    }

    ss[lines - 1].lineLength += appendedEOL.length();

    std::stable_sort(ss.begin(), ss.end(),
        [](const SortSelectionLine& a, const SortSelectionLine& b) {
            for (size_t i = 0; i < std::min(a.keys.size(), b.keys.size()); ++i) {
                if (!a.keys[i].numeric)
                    if (a.keys[i].string == b.keys[i].string) continue;
                    else return a.keys[i].descending ? a.keys[i].string > b.keys[i].string : a.keys[i].string < b.keys[i].string;
                if (std::_Is_nan(a.keys[i].number))
                    if (std::_Is_nan(b.keys[i].number)) continue; else return true;
                if (std::_Is_nan(b.keys[i].number)) return false;
                if (a.keys[i].number == b.keys[i].number) continue;
                return a.keys[i].descending ? a.keys[i].number > b.keys[i].number : a.keys[i].number < b.keys[i].number;
            }
            return a.keys.size() < b.keys.size();
        });

    if (sortSettings.sortColumnSelectionOnly) replaceSortColumn(data, ss, unsortedLinePointers, rs);
    else replaceSortSelection(data, ss, rs);

}


struct SortInfo {
    struct Locale {
        std::wstring name;
        std::wstring language;
        std::wstring country;
        std::wstring sorting;
    };
    ColumnsPlusPlusData& data;
    std::map<std::wstring, std::map<std::wstring, Locale>> locales;
    SortInfo(ColumnsPlusPlusData& data) : data(data) {}
};


INT_PTR CALLBACK sortDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    SortInfo* sip = 0;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        sip = reinterpret_cast<SortInfo*>(lParam);
    }
    else sip = reinterpret_cast<SortInfo*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!sip) return TRUE;
    SortInfo& si = *sip;
    ColumnsPlusPlusData& data = si.data;

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
        if (data.sort.sortColumnSelectionOnly) CheckRadioButton(hwndDlg, IDC_SORT_WHOLE_LINES, IDC_SORT_WITHIN_SELECTION, IDC_SORT_WITHIN_SELECTION);
                                          else CheckRadioButton(hwndDlg, IDC_SORT_WHOLE_LINES, IDC_SORT_WITHIN_SELECTION, IDC_SORT_WHOLE_LINES     );
        if (data.sort.sortDescending) CheckRadioButton(hwndDlg, IDC_SORT_ASCENDING, IDC_SORT_DESCENDING, IDC_SORT_DESCENDING);
                                 else CheckRadioButton(hwndDlg, IDC_SORT_ASCENDING, IDC_SORT_DESCENDING, IDC_SORT_ASCENDING );
        switch (data.sort.sortType) {
        case SortSettings::Binary : CheckRadioButton(hwndDlg, IDC_SORT_BINARY, IDC_SORT_NUMERIC, IDC_SORT_BINARY ); break;
        case SortSettings::Locale : CheckRadioButton(hwndDlg, IDC_SORT_BINARY, IDC_SORT_NUMERIC, IDC_SORT_LOCALE ); break;
        case SortSettings::Numeric: CheckRadioButton(hwndDlg, IDC_SORT_BINARY, IDC_SORT_NUMERIC, IDC_SORT_NUMERIC); break;
        }
        switch (data.sort.keyType) {
        case SortSettings::EntireColumn: CheckRadioButton(hwndDlg, IDC_SORT_ENTIRE_COLUMN, IDC_SORT_REGEX, IDC_SORT_ENTIRE_COLUMN); break;
        case SortSettings::IgnoreBlanks: CheckRadioButton(hwndDlg, IDC_SORT_ENTIRE_COLUMN, IDC_SORT_REGEX, IDC_SORT_IGNORE_BLANKS); break;
        case SortSettings::Tabbed      : CheckRadioButton(hwndDlg, IDC_SORT_ENTIRE_COLUMN, IDC_SORT_REGEX, IDC_SORT_TABBED       ); break;
        case SortSettings::Regex       : CheckRadioButton(hwndDlg, IDC_SORT_ENTIRE_COLUMN, IDC_SORT_REGEX, IDC_SORT_REGEX        ); break;
        }
        SendDlgItemMessage(hwndDlg, IDC_SORT_MATCH_CASE       , BM_SETCHECK, data.sort.regexMatchCase         ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SORT_USE_KEY          , BM_SETCHECK, data.sort.regexUseKey            ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SORT_CASE_SENSITIVE   , BM_SETCHECK, data.sort.localeCaseSensitive    ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SORT_DIGITS_AS_NUMBERS, BM_SETCHECK, data.sort.localeDigitsAsNumbers  ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SORT_IGNORE_DIACRITICS, BM_SETCHECK, data.sort.localeIgnoreDiacritics ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_SORT_IGNORE_SYMBOLS   , BM_SETCHECK, data.sort.localeIgnoreSymbols    ? BST_CHECKED : BST_UNCHECKED, 0);
        for (const auto& s : data.sort.regexHistory)
            SendDlgItemMessage(hwndDlg, IDC_SORT_FIND_WHAT, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        for (const auto& s : data.sort.keygroupHistory)
            SendDlgItemMessage(hwndDlg, IDC_SORT_KEY_CAPTURE, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        SendDlgItemMessage(hwndDlg, IDC_SORT_FIND_WHAT  , CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hwndDlg, IDC_SORT_KEY_CAPTURE, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hwndDlg, IDC_SORT_KEY_CAPTURE, CB_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(L"e.g.: 2,1d,3an -- a/d, b/l/n override sort type"));
        EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_FIND_WHAT        ), data.sort.keyType == SortSettings::Regex);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_FIND_WHAT_LABEL  ), data.sort.keyType == SortSettings::Regex);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_MATCH_CASE       ), data.sort.keyType == SortSettings::Regex);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_USE_KEY          ), data.sort.keyType == SortSettings::Regex);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_KEY_CAPTURE      ), data.sort.keyType == SortSettings::Tabbed
                                                                      || (data.sort.keyType == SortSettings::Regex && data.sort.regexUseKey));
        EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_KEY_CAPTURE_LABEL), data.sort.keyType == SortSettings::Tabbed
                                                                      || (data.sort.keyType == SortSettings::Regex && data.sort.regexUseKey));
        for (const auto& s : si.locales)
            SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_LANGUAGE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.first.data()));
        std::wstring localeName = data.sort.localeName;
        if (localeName.empty()) {
            int n = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, 0, 0);
            localeName.resize(n - 1);
            GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, localeName.data(), n);
        }
        int nLanguage = GetLocaleInfoEx(localeName.data(), LOCALE_SLOCALIZEDLANGUAGENAME, 0, 0);
        if (nLanguage) {
            std::wstring language(nLanguage - 1, 0);
            GetLocaleInfoEx(localeName.data(), LOCALE_SLOCALIZEDLANGUAGENAME, language.data(), nLanguage);
            auto n = SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_LANGUAGE, CB_FINDSTRINGEXACT,
                                        static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(language.data()));
            if (n != CB_ERR) {
                SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_LANGUAGE, CB_SETCURSEL, n, 0);
                auto& locales = si.locales[language];
                for (const auto& s : locales) {
                    std::wstring x = s.first + L"  -  " + s.second.country;
                    if (s.second.sorting != L"Default") x += L" (" + s.second.sorting + L')';
                    n = SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_NAME, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(x.data()));
                    if (s.first == localeName) SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_NAME, CB_SETCURSEL, n, 0);
                }
            }
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
        {
            if (IsDlgButtonChecked(hwndDlg, IDC_SORT_REGEX) == BST_CHECKED || IsDlgButtonChecked(hwndDlg, IDC_SORT_TABBED) == BST_CHECKED) {
                if (IsDlgButtonChecked(hwndDlg, IDC_SORT_REGEX) == BST_CHECKED) {
                    HWND h = GetDlgItem(hwndDlg, IDC_SORT_FIND_WHAT);
                    auto n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
                    std::wstring error = L"A regular expression is required to perform a regular expression sort.";
                    if (n) {
                        RegularExpression rx(data);
                        std::wstring w(n, 0);
                        SendMessage(h, WM_GETTEXT, n + 1, reinterpret_cast<LPARAM>(w.data()));
                        error = rx.find(w, 0);
                    }
                    if (!error.empty()) {
                        COMBOBOXINFO cbi;
                        cbi.cbSize = sizeof(COMBOBOXINFO);
                        GetComboBoxInfo(h, &cbi);
                        EDITBALLOONTIP ebt;
                        ebt.cbStruct = sizeof(EDITBALLOONTIP);
                        ebt.pszTitle = L"";
                        ebt.ttiIcon  = TTI_NONE;
                        ebt.pszText  = error.data();
                        SendMessage(cbi.hwndItem, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                        SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(cbi.hwndItem), TRUE);
                        return TRUE;
                    }
                }
                if (IsDlgButtonChecked(hwndDlg, IDC_SORT_USE_KEY) == BST_CHECKED || IsDlgButtonChecked(hwndDlg, IDC_SORT_TABBED) == BST_CHECKED) {
                    HWND h = GetDlgItem(hwndDlg, IDC_SORT_KEY_CAPTURE);
                    auto n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
                    std::wstring w(n, 0);
                    SendMessage(h, WM_GETTEXT, n + 1, reinterpret_cast<LPARAM>(w.data()));
                    std::wsmatch m;
                    if (!std::regex_match(w, m, keycap)) {
                        COMBOBOXINFO cbi;
                        cbi.cbSize = sizeof(COMBOBOXINFO);
                        GetComboBoxInfo(h, &cbi);
                        EDITBALLOONTIP ebt;
                        ebt.cbStruct = sizeof(EDITBALLOONTIP);
                        ebt.pszTitle = L"";
                        ebt.ttiIcon = TTI_NONE;
                        ebt.pszText = L"Invalid sort key specification.";
                        SendMessage(cbi.hwndItem, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                        SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(cbi.hwndItem), TRUE);
                        return TRUE;
                    }
                    updateComboHistory(hwndDlg, IDC_SORT_KEY_CAPTURE, data.sort.keygroupHistory);
                }
                if (IsDlgButtonChecked(hwndDlg, IDC_SORT_REGEX) == BST_CHECKED) {
                    data.sort.keyType = SortSettings::Regex;
                    updateComboHistory(hwndDlg, IDC_SORT_FIND_WHAT, data.sort.regexHistory);
                    data.sort.regexMatchCase = IsDlgButtonChecked(hwndDlg, IDC_SORT_MATCH_CASE) == BST_CHECKED;
                    data.sort.regexUseKey    = IsDlgButtonChecked(hwndDlg, IDC_SORT_USE_KEY   ) == BST_CHECKED;
                }
                else data.sort.keyType = SortSettings::Tabbed;
            }
            else data.sort.keyType = IsDlgButtonChecked(hwndDlg, IDC_SORT_IGNORE_BLANKS) == BST_CHECKED ? SortSettings::IgnoreBlanks
                                                                                                        : SortSettings::EntireColumn;
            data.sort.sortType = IsDlgButtonChecked(hwndDlg, IDC_SORT_BINARY) == BST_CHECKED ? SortSettings::Binary
                               : IsDlgButtonChecked(hwndDlg, IDC_SORT_LOCALE) == BST_CHECKED ? SortSettings::Locale
                                                                                             : SortSettings::Numeric;
            data.sort.sortColumnSelectionOnly = IsDlgButtonChecked(hwndDlg, IDC_SORT_WITHIN_SELECTION ) == BST_CHECKED;
            data.sort.sortDescending          = IsDlgButtonChecked(hwndDlg, IDC_SORT_DESCENDING       ) == BST_CHECKED;
            data.sort.localeCaseSensitive     = IsDlgButtonChecked(hwndDlg, IDC_SORT_CASE_SENSITIVE   ) == BST_CHECKED;
            data.sort.localeDigitsAsNumbers   = IsDlgButtonChecked(hwndDlg, IDC_SORT_DIGITS_AS_NUMBERS) == BST_CHECKED;
            data.sort.localeIgnoreDiacritics  = IsDlgButtonChecked(hwndDlg, IDC_SORT_IGNORE_DIACRITICS) == BST_CHECKED;
            data.sort.localeIgnoreSymbols     = IsDlgButtonChecked(hwndDlg, IDC_SORT_IGNORE_SYMBOLS   ) == BST_CHECKED;
            HWND h = GetDlgItem(hwndDlg, IDC_SORT_LOCALE_NAME);
            auto n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
            std::wstring locale(n, 0);
            SendMessage(h, WM_GETTEXT, n + 1, (LPARAM)locale.data());
            size_t p = locale.find(L"  -  ");
            if (p > 1 && p != std::wstring::npos) data.sort.localeName = locale.substr(0, p);
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        case IDC_SORT_ENTIRE_COLUMN:
        case IDC_SORT_IGNORE_BLANKS:
        case IDC_SORT_TABBED:
        case IDC_SORT_REGEX:
        case IDC_SORT_USE_KEY:
        {
            bool rx = IsDlgButtonChecked(hwndDlg, IDC_SORT_REGEX) == BST_CHECKED;
            bool uk = (rx && IsDlgButtonChecked(hwndDlg, IDC_SORT_USE_KEY) == BST_CHECKED) || IsDlgButtonChecked(hwndDlg, IDC_SORT_TABBED) == BST_CHECKED;
            EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_FIND_WHAT        ), rx);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_FIND_WHAT_LABEL  ), rx);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_MATCH_CASE       ), rx);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_USE_KEY          ), rx);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_KEY_CAPTURE      ), uk);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SORT_KEY_CAPTURE_LABEL), uk);
            break;
        }
        case IDC_SORT_LOCALE_LANGUAGE:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_NAME, CB_RESETCONTENT, 0, 0);
                HWND h = GetDlgItem(hwndDlg, IDC_SORT_LOCALE_LANGUAGE);
                auto n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
                std::wstring language(n, 0);
                SendMessage(h, WM_GETTEXT, n + 1, (LPARAM)language.data());
                auto& locales = si.locales[language];
                for (const auto& s : locales) {
                    std::wstring x = s.first + L"  -  " + s.second.country;
                    if (s.second.sorting != L"Default") x += L" (" + s.second.sorting + L')';
                    SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_NAME, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(x.data()));
                }
               SendDlgItemMessage(hwndDlg, IDC_SORT_LOCALE_NAME, CB_SETCURSEL, 0, 0);
            }
            break;
        default:;
        }
        break;

    default:;
    }
    return FALSE;
}


BOOL CALLBACK addLocale(LPWSTR pStr, DWORD, LPARAM lparam) {
    SortInfo& si = *reinterpret_cast<SortInfo*>(lparam);
    int nLanguage = GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDLANGUAGENAME, 0, 0);
    int nCountry  = GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDCOUNTRYNAME , 0, 0);
    int nSorting  = GetLocaleInfoEx(pStr, LOCALE_SSORTNAME             , 0, 0);
    SortInfo::Locale locale;
    locale.name = pStr;
    if (nLanguage) {
        locale.language.resize(nLanguage - 1);
        GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDLANGUAGENAME, locale.language.data(), nLanguage);
    }
    if (nCountry) {
        locale.country.resize(nCountry - 1);
        GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDCOUNTRYNAME, locale.country.data(), nCountry);
    }
    if (nSorting) {
        locale.sorting.resize(nSorting - 1);
        GetLocaleInfoEx(pStr, LOCALE_SSORTNAME, locale.sorting.data(), nSorting);
    }
    si.locales[locale.language][locale.name] = locale;
    return TRUE;
}


void sortStandard(ColumnsPlusPlusData& data, SortSettings::SortType sortType, bool descending) {
    SortSettings sortSettings;
    sortSettings.sortType = sortType;
    sortSettings.sortDescending = descending;
    sortCommon(data, sortSettings);
}

} // end anonymous namespace


void ColumnsPlusPlusData::sortAscendingBinary  () {sortStandard(*this, SortSettings::Binary , false);}
void ColumnsPlusPlusData::sortDescendingBinary () {sortStandard(*this, SortSettings::Binary , true );}
void ColumnsPlusPlusData::sortAscendingLocale  () {sortStandard(*this, SortSettings::Locale , false);}
void ColumnsPlusPlusData::sortDescendingLocale () {sortStandard(*this, SortSettings::Locale , true );}
void ColumnsPlusPlusData::sortAscendingNumeric () {sortStandard(*this, SortSettings::Numeric, false);}
void ColumnsPlusPlusData::sortDescendingNumeric() {sortStandard(*this, SortSettings::Numeric, true );}

void ColumnsPlusPlusData::sortCustom() {
    SortInfo si(*this);
    EnumSystemLocalesEx(addLocale, LOCALE_ALL, reinterpret_cast<LPARAM>(&si), 0);
    if (DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_SORT), nppData._nppHandle, sortDialogProc, reinterpret_cast<LPARAM>(&si))) return;
    sortCommon(*this, sort);
}
