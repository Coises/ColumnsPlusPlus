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
#include "resource.h"
#include "commctrl.h"


void showBalloonTip(HWND hwndDlg, int control, const std::wstring& text) {
    HWND hControl = GetDlgItem(hwndDlg, control);
    EDITBALLOONTIP ebt;
    ebt.cbStruct = sizeof(EDITBALLOONTIP);
    ebt.pszTitle = L"";
    ebt.ttiIcon = TTI_NONE;
    ebt.pszText = text.data();
    SendMessage(hControl, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
    SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(hControl), TRUE);

}


INT_PTR CALLBACK csvDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    ColumnsPlusPlusData* pData = 0;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        pData = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else pData = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!pData) return TRUE;
    ColumnsPlusPlusData& data = *pData;

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
        wchar_t oneWideChar[2] = { 0, 0 };
        oneWideChar[0] = data.csv.separator;
        SetDlgItemText(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT, oneWideChar);
        oneWideChar[0] = data.csv.escapeChar;
        SetDlgItemText(hwndDlg, IDC_CSV_ESCAPE_EDIT, oneWideChar);
        oneWideChar[0] = data.csv.encodeTNR;
        SetDlgItemText(hwndDlg, IDC_CSV_TNR_EDIT, oneWideChar);
        oneWideChar[0] = data.csv.encodeURL;
        SetDlgItemText(hwndDlg, IDC_CSV_URL_EDIT, oneWideChar);
        SetDlgItemText(hwndDlg, IDC_CSV_REPLACE_TAB, data.csv.replaceTab.data());
        SetDlgItemText(hwndDlg, IDC_CSV_REPLACE_LF , data.csv.replaceLF .data());
        SetDlgItemText(hwndDlg, IDC_CSV_REPLACE_CR , data.csv.replaceCR .data());
        SendDlgItemMessage(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT, EM_SETLIMITTEXT, 1, 0);
        SendDlgItemMessage(hwndDlg, IDC_CSV_ESCAPE_EDIT         , EM_SETLIMITTEXT, 1, 0);
        SendDlgItemMessage(hwndDlg, IDC_CSV_TNR_EDIT            , EM_SETLIMITTEXT, 1, 0);
        SendDlgItemMessage(hwndDlg, IDC_CSV_URL_EDIT            , EM_SETLIMITTEXT, 1, 0);
        SendDlgItemMessage(hwndDlg, data.csv.separator == L',' ? IDC_CSV_COMMA
                                  : data.csv.separator == L';' ? IDC_CSV_SEMICOLON
                                  : data.csv.separator == L'|' ? IDC_CSV_VERTICAL_LINE
                                                               : IDC_CSV_OTHER_SEPARATOR_RADIO, BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT),
            data.csv.separator != L',' && data.csv.separator != L';' && data.csv.separator != L'|' ? TRUE : FALSE);
        SendDlgItemMessage(hwndDlg, IDC_CSV_QUOTE          , BM_SETCHECK, data.csv.quote          ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CSV_APOSTROPHE     , BM_SETCHECK, data.csv.apostrophe     ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CSV_ESCAPE_CHECK   , BM_SETCHECK, data.csv.escape         ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CSV_PRESERVE_QUOTES, BM_SETCHECK, data.csv.preserveQuotes ? BST_CHECKED : BST_UNCHECKED, 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_ESCAPE_EDIT), data.csv.escape ? TRUE : FALSE);
        switch (data.csv.encodingStyle) {
        case CsvSettings::TNR:
            SendDlgItemMessage(hwndDlg, IDC_CSV_TNR_RADIO, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_TNR_EDIT   ), TRUE );
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_URL_EDIT   ), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_TAB), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_LF ), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_CR ), FALSE);
            break;
        case CsvSettings::URL:
            SendDlgItemMessage(hwndDlg, IDC_CSV_URL_RADIO, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_TNR_EDIT   ), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_URL_EDIT   ), TRUE );
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_TAB), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_LF ), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_CR ), FALSE);
            break;
        case CsvSettings::Replace:
            SendDlgItemMessage(hwndDlg, IDC_CSV_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_TNR_EDIT   ), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_URL_EDIT   ), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_TAB), TRUE );
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_LF ), TRUE );
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_CR ), TRUE );
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
            wchar_t separator [3] = { 0, 0, 0 };
            wchar_t escapeChar[3] = { 0, 0, 0 };
            wchar_t encodeTNR [3] = { 0, 0, 0 };
            wchar_t encodeURL [3] = { 0, 0, 0 };
            const bool quote      = SendDlgItemMessage(hwndDlg, IDC_CSV_QUOTE       , BM_GETCHECK, 0, 0) == BST_CHECKED;
            const bool apostrophe = SendDlgItemMessage(hwndDlg, IDC_CSV_APOSTROPHE  , BM_GETCHECK, 0, 0) == BST_CHECKED;
            const bool escape     = SendDlgItemMessage(hwndDlg, IDC_CSV_ESCAPE_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;
            const bool encTNR     = SendDlgItemMessage(hwndDlg, IDC_CSV_TNR_RADIO   , BM_GETCHECK, 0, 0) == BST_CHECKED;
            const bool encURL     = SendDlgItemMessage(hwndDlg, IDC_CSV_URL_RADIO   , BM_GETCHECK, 0, 0) == BST_CHECKED;
            GetDlgItemText(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT, separator , 3);
            if (escape) GetDlgItemText(hwndDlg, IDC_CSV_ESCAPE_EDIT, escapeChar, 3);
            if (encTNR) GetDlgItemText(hwndDlg, IDC_CSV_TNR_EDIT   , encodeTNR , 3);
            if (encURL) GetDlgItemText(hwndDlg, IDC_CSV_URL_EDIT   , encodeURL , 3);
            if ( separator[0] == 0 || separator[1] != 0 || separator[0] == L'\n' || separator[0] == L'\r'
              || (quote && separator[0] == L'"') || (apostrophe && separator[0] == L'\'') ) {
                showBalloonTip(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT,
                    L"The column separator must be a single character and cannot be the same as an enabled quoting character.");
                return TRUE;
            }
            if (escape && ( escapeChar[0] == 0 || escapeChar[1] != 0 || escapeChar[0] == L'\n' || escapeChar[0] == L'\r' || escapeChar[0] == separator[0]
                         || (quote && escapeChar[0] == L'"') || (apostrophe && escapeChar[0] == L'\'') )) {
                showBalloonTip(hwndDlg, IDC_CSV_ESCAPE_EDIT,
                    L"The escape character must be a single character and cannot be the same as the column separator or an enabled quoting character.");
                return TRUE;
            }
            if (encTNR && (encodeTNR[0] == 0 || encodeTNR[1] != 0 || encodeTNR[0] == L'\n' || encodeTNR[0] == L'\r' || encodeTNR[0] == L'\t')) {
                showBalloonTip(hwndDlg, IDC_CSV_TNR_EDIT,
                    L"The encoding character must be a single character.");
                return TRUE;
            }
            if (encURL && (encodeURL[0] == 0 || encodeURL[1] != 0 || encodeURL[0] == L'\n' || encodeURL[0] == L'\r' || encodeURL[0] == L'\t')) {
                showBalloonTip(hwndDlg, IDC_CSV_URL_EDIT,
                    L"The encoding character must be a single character.");
                return TRUE;
            }
            data.csv.separator  = separator [0];
            if (escape) data.csv.escapeChar = escapeChar[0];
            if (encTNR) data.csv.encodeTNR  = encodeTNR [0];
            if (encURL) data.csv.encodeURL  = encodeURL [0];
            HWND h = GetDlgItem(hwndDlg, IDC_CSV_REPLACE_TAB);
            int n = GetWindowTextLength(h);
            data.csv.replaceTab.resize(n);
            GetWindowText(h, data.csv.replaceTab.data(), n + 1);
            h = GetDlgItem(hwndDlg, IDC_CSV_REPLACE_LF);
            n = GetWindowTextLength(h);
            data.csv.replaceLF.resize(n);
            GetWindowText(h, data.csv.replaceLF.data(), n + 1);
            h = GetDlgItem(hwndDlg, IDC_CSV_REPLACE_CR);
            n = GetWindowTextLength(h);
            data.csv.replaceCR.resize(n);
            GetWindowText(h, data.csv.replaceCR.data(), n + 1);
            data.csv.quote          = quote;
            data.csv.apostrophe     = apostrophe;
            data.csv.escape         = escape;
            data.csv.preserveQuotes = SendDlgItemMessage(hwndDlg, IDC_CSV_PRESERVE_QUOTES, BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.csv.encodingStyle  = encTNR ? CsvSettings::TNR : encURL ? CsvSettings::URL : CsvSettings::Replace;
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        case IDC_CSV_COMMA:
            SetDlgItemText(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT, L",");
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT), FALSE);
            break;
        case IDC_CSV_SEMICOLON:
            SetDlgItemText(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT, L";");
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT), FALSE);
            break;
        case IDC_CSV_VERTICAL_LINE:
            SetDlgItemText(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT, L"|");
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT), FALSE);
            break;
        case IDC_CSV_OTHER_SEPARATOR_RADIO:
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_OTHER_SEPARATOR_EDIT), TRUE);
            break;
        case IDC_CSV_ESCAPE_CHECK:
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_ESCAPE_EDIT),
                SendDlgItemMessage(hwndDlg, IDC_CSV_ESCAPE_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
            break;
        case IDC_CSV_TNR_RADIO:
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_TNR_EDIT), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_URL_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_TAB), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_LF), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_CR), FALSE);
            break;
        case IDC_CSV_URL_RADIO:
            SendDlgItemMessage(hwndDlg, IDC_CSV_URL_RADIO, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_TNR_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_URL_EDIT), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_TAB), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_LF), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_CR), FALSE);
            break;
        case IDC_CSV_REPLACE:
            SendDlgItemMessage(hwndDlg, IDC_CSV_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_TNR_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_URL_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_TAB), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_LF), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CSV_REPLACE_CR), TRUE);
        }
        break;

    default:;
    }
    return FALSE;

}


void ColumnsPlusPlusData::separatedValuesToTabs() {
    if (DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_CSV), nppData._nppHandle,
                       ::csvDialogProc, reinterpret_cast<LPARAM>(this))) return;

    const int codepage = sci.CodePage();
    const wchar_t activeEncode = csv.encodingStyle == CsvSettings::TNR ? csv.encodeTNR
                               : csv.encodingStyle == CsvSettings::URL ? csv.encodeURL
                                                                       : 0;
    const std::wstring commonStop = L"\t\n\r" + std::wstring(csv.escape, csv.escapeChar)
                                  + std::wstring(activeEncode && (!csv.escape || activeEncode != csv.escapeChar), activeEncode);
    const std::wstring stopUnquoted = commonStop + csv.separator;
    const std::wstring subTab = csv.encodingStyle == CsvSettings::TNR ? csv.encodeTNR + std::wstring(L"t")
                              : csv.encodingStyle == CsvSettings::URL ? csv.encodeURL + std::wstring(L"09")
                                                                      : csv.replaceTab;
    const std::wstring subLF  = csv.encodingStyle == CsvSettings::TNR ? csv.encodeTNR + std::wstring(L"n")
                              : csv.encodingStyle == CsvSettings::URL ? csv.encodeURL + std::wstring(L"0A")
                                                                      : csv.replaceLF;
    const std::wstring subCR  = csv.encodingStyle == CsvSettings::TNR ? csv.encodeTNR + std::wstring(L"r")
                              : csv.encodingStyle == CsvSettings::URL ? csv.encodeURL + std::wstring(L"0D")
                                                                      : csv.replaceCR;
    std::wstring subEnc;
    if (csv.encodingStyle == CsvSettings::TNR) subEnc = std::wstring(2, csv.encodeTNR);
    else if (csv.encodingStyle == CsvSettings::URL) {
        std::string s = fromWide(std::wstring(1, csv.encodeURL), codepage);
        for (auto c : s) {
            subEnc += csv.encodeURL;
            subEnc += L"0123456789ABCDEF"[(c >> 4) & 15];
            subEnc += L"0123456789ABCDEF"[c & 15];
        }
    }
    const std::wstring subEsc = (csv.encodingStyle == CsvSettings::TNR && csv.encodeTNR == csv.escapeChar)
                             || (csv.encodingStyle == CsvSettings::URL && csv.encodeURL == csv.escapeChar) ? subEnc : std::wstring(1, csv.escapeChar);
    const std::wstring subSep = (csv.encodingStyle == CsvSettings::TNR && csv.encodeTNR == csv.separator)
                             || (csv.encodingStyle == CsvSettings::URL && csv.encodeURL == csv.separator) ? subEnc : std::wstring(1, csv.separator);

    sci.BeginUndoAction();
    int selectionCount = sci.Selections();
    for (int selectionNumber = 0; selectionNumber < selectionCount; ++selectionNumber) {

        Scintilla::Position start = sci.SelectionNStart(selectionNumber);
        Scintilla::Position end = sci.SelectionNEnd(selectionNumber);
        if (start != end) sci.SetTargetRange(start, end);
        else if (selectionCount == 1) sci.TargetWholeDocument();
        else continue;

        const std::wstring s = toWide(sci.TargetText(), codepage);
        std::wstring t;
        for (size_t start_of_field = 0; start_of_field < s.length();) {

            size_t p = s.find_first_not_of(L' ', start_of_field);
            if (p == std::string::npos) {
                if (csv.preserveQuotes) t.append(s.length() - start_of_field, L' ');
                break;
            }
            if (csv.preserveQuotes) t.append(p - start_of_field, L' ');

            if ((s[p] == L'"' && csv.quote) || (s[p] == L'\'' && csv.apostrophe)) {
                wchar_t quote = s[p];
                const std::wstring stop = commonStop + quote;
                if (csv.preserveQuotes) t += quote;
                ++p;
                for (;;) {
                    size_t q = s.find_first_of(stop, p);
                    if (q == std::string::npos) {
                        t += s.substr(p);
                        p = s.length();
                        break;
                    }
                    t += s.substr(p, q - p);
                    p = q + 1;
                    if      (s[q] == '\t') t += subTab;
                    else if (s[q] == '\n') t += subLF;
                    else if (s[q] == '\r') t += subCR;
                    else if (s[q] == csv.escapeChar && csv.escape) {
                        if (p == s.length()) {
                            t += subEsc;
                            break;
                        }
                        else {
                            if (csv.preserveQuotes) t += subEsc;
                            if (s[p] == csv.escapeChar) {
                                t += subEsc;
                                ++p;
                            }
                            else if (s[p] == quote) {
                                t += quote;
                                ++p;
                            }
                        }
                    }
                    else if (s[q] == quote) {
                        if (csv.preserveQuotes) t += quote;
                        if (p < s.length() && s[p] == quote) {
                            t += quote;
                            ++p;
                        }
                        else break;
                    }
                    else if (csv.encodingStyle == CsvSettings::TNR) t += subEnc;
                    else /* URL-style encoding character */
                        t += p < s.length() - 1 && s[p] <= 'f' && s[p + 1] <= 'f' && std::isxdigit(s[p]) && std::isxdigit(s[p + 1])
                        ? subEnc : s.substr(q, 1);
                }
            }

            for (;;) {
                size_t q = s.find_first_of(stopUnquoted, p);
                if (q == std::wstring::npos || s[q] == csv.separator || s[q] == '\n' || s[q] == '\r') {
                    if (csv.preserveQuotes) t += s.substr(p, q - p);
                    else if (q > p) {
                        size_t u = s.find_last_not_of(' ', q - 1);
                        if (u != std::string::npos && u >= p) t += s.substr(p, u - p + 1);
                    }
                    if (q == std::wstring::npos) start_of_field = std::string::npos;
                    else {
                        t += s[q] == csv.separator ? '\t' : s[q];
                        start_of_field = q + 1;
                    }
                    break;
                }
                t += s.substr(p, q - p);
                p = q + 1;
                if (s[q] == L'\t') t += subTab;
                else if (s[q] == csv.escapeChar && csv.escape) {
                    if (p == s.length()) {
                        t += subEsc;
                        start_of_field = std::string::npos;
                        break;
                    }
                    if (csv.preserveQuotes) t += subEsc;
                    if      (s[p] == csv.escapeChar) { t += subEsc; ++p; }
                    else if (s[p] == csv.separator ) { t += subSep; ++p; }
                    else if (s[p] == '\n'          ) { t += subLF ; ++p; }
                    else if (s[p] == '\r'          ) { t += subCR ; ++p; }
                }
                else if (csv.encodingStyle == CsvSettings::TNR) t += subEnc;
                else /* URL-style encoding character */
                    t += p < s.length() - 1 && s[p] <= 'f' && s[p + 1] <= 'f' && std::isxdigit(s[p]) && std::isxdigit(s[p + 1])
                        ? subEnc : s.substr(q, 1);
            }

        }

        sci.ReplaceTarget(fromWide(t, codepage));
    }
    sci.EndUndoAction();
    if (settings.elasticEnabled) {
        DocumentData& dd = *getDocument();
        analyzeTabstops(dd);
        setTabstops(dd);
    }
}


bool isValidField(const std::wstring& s, const CsvSettings& csv, bool strict) {
    size_t p = s.find_first_not_of(' ');
    if (p == std::wstring::npos) return true;
    size_t q = s.find_last_not_of(' ');
    if ((s[p] == L'"' && csv.quote) || (s[p] == L'\'' && csv.apostrophe)) {
        if (p == q || s[p] != s[q]) return false;
        const std::wstring stop = csv.escape ? s.substr(p, 1) + csv.escapeChar : s.substr(p, 1);
        for (size_t i = p + 1; i = s.find_first_of(stop, i), i < q; i += 2)
            if (i == q - 1 || (s[i] == s[p] && s[i + 1] != s[p])) return false;
        return true;
    }
    std::wstring stop = std::wstring(L"\r\n") + csv.separator;
    if (strict && csv.quote     ) stop += L'"';
    if (strict && csv.apostrophe) stop += L'\'';
    if (csv.escape    ) stop += csv.escapeChar;
    for (size_t i = p + 1; i < q;) {
        size_t j = s.find_first_of(stop, i);
        if (j == std::string::npos) return true;
        if (s[j] != csv.escapeChar) return false;
        if (j == s.length() - 1) return false;
        else i = j + 2;
    }
    return true;
}


void ColumnsPlusPlusData::tabsToSeparatedValues() {
    if (DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_CSV), nppData._nppHandle,
                       ::csvDialogProc, reinterpret_cast<LPARAM>(this))) return;
    const int codepage = sci.CodePage();
    const std::string separator  = fromWide(std::wstring(1, csv.separator), codepage);
    const std::string escapeChar = fromWide(std::wstring(1, csv.escapeChar), codepage);
    sci.BeginUndoAction();
    int selectionCount = sci.Selections();
    for (int selectionNumber = 0; selectionNumber < selectionCount; ++selectionNumber) {
        Scintilla::Position start = sci.SelectionNStart(selectionNumber);
        Scintilla::Position end = sci.SelectionNEnd(selectionNumber);
        if (start != end) sci.SetTargetRange(start, end);
        else if (selectionCount == 1) sci.TargetWholeDocument();
        else continue;
        const std::wstring s = toWide(sci.TargetText(), codepage);
        std::string t;
        for (size_t p = 0;;) {
            size_t q = s.find_first_of(L"\t\n\r", p);
            std::wstring field = s.substr(p, q - p);
            if (!field.empty()) {
                if (csv.encodingStyle == CsvSettings::TNR) {
                    for (size_t i = 0; i = field.find_first_of(csv.encodeTNR, i), i < field.length() - 1; ++i) {
                        switch (field[i + 1]) {
                        case 't' : field.replace(i, 2, L"\t"); break;
                        case 'n' : field.replace(i, 2, L"\n"); break;
                        case 'r' : field.replace(i, 2, L"\r"); break;
                        default  : if (field[i + 1] == csv.encodeTNR) field.erase(i, 1);
                        }
                    }
                }
                else if (csv.encodingStyle == CsvSettings::URL) {
                    for (size_t i = 0; i = field.find_first_of(csv.encodeURL, i), i < field.length() - 2; ++i) {
                        if (field[i + 1] > 'f' || !std::isxdigit(field[i + 1])) continue;
                        if (field[i + 2] > 'f' || !std::isxdigit(field[i + 2])) continue;
                        field[i] = static_cast<wchar_t>(stoi(field.substr(i + 1, 2), 0, 16));
                        field.erase(i + 1, 2);
                    }
                }
                bool reduces = field[0] == L' '
                            || field.back() == L' '
                            || (field[0] == L'"' && csv.quote)
                            || (field[0] == L'\'' && csv.apostrophe);
                if ((csv.preserveQuotes || !reduces) && isValidField(field, csv, !csv.preserveQuotes)) t += fromWide(field, codepage);
                else if (csv.quote || csv.apostrophe) {
                    const char quote = !csv.quote ? '\''
                                     : !csv.apostrophe ? '"'
                                     : std::count(field.begin(), field.end(), '"') > std::count(field.begin(), field.end(), '\'') ? '\'' : '"';
                    t += quote;
                    std::wstring w;
                    for (size_t i = 0; i < field.length(); ++i) {
                        if (field[i] == quote || (field[i] == csv.escapeChar && csv.escape)) w += field[i];
                        w += field[i];
                    }
                    t += fromWide(w, codepage);
                    t += quote;
                }
                else if (csv.escape) {
                    size_t firstNonBlank = field.find_first_not_of(' ');
                    if (firstNonBlank == std::string::npos)
                        for (size_t i = 0; i < field.length(); ++i) { t += escapeChar; t += ' '; }
                    else {
                        size_t lastNonBlank  = field.find_last_not_of(' ');
                        for (size_t i = 0; i < firstNonBlank; ++i) { t += escapeChar; t += ' '; }
                        std::wstring w;
                        for (size_t i = firstNonBlank; i <= lastNonBlank; ++i) {
                            if (field[i] == csv.separator || field[i] == csv.escapeChar || field[i] == L'\n' || field[i] == L'\r')
                                w += csv.escapeChar;
                            w += field[i];
                        }
                        t += fromWide(w, codepage);
                        for (size_t i = lastNonBlank + 1; i < field.length(); ++i) { t += escapeChar; t += ' '; }
                    }
                }
                else {
                    sci.EndUndoAction();
                    MessageBox(nppData._nppHandle, L"The selection cannot be converted without quotes or escape characters, "
                                                   L"but no quotes or escape charaters are enabled.",
                                                   L"Convert tabs to separated values", MB_OK | MB_ICONERROR);
                    return;
                }
            }
            if (q == std::string::npos) break;
            if (s[q] == '\t') t += separator;
                         else t += static_cast<char>(s[q]);
            p = q + 1;
        }
        sci.ReplaceTarget(t);
    }
    sci.EndUndoAction();
    if (settings.elasticEnabled) {
        DocumentData& dd = *getDocument();
        analyzeTabstops(dd);
        setTabstops(dd);
    }
}


void ColumnsPlusPlusData::tabsToSpaces() {

    if (!settings.elasticEnabled) {
        SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_TAB2SW);
        return;
    }

    DocumentData& dd = *getDocument();

    std::vector<std::pair<Scintilla::Position, Scintilla::Position>> selections;
    if (sci.SelectionEmpty()) selections.emplace_back(0, sci.Length());
    else {
        int n = sci.Selections();
        for (int i = 0; i < n; ++i)
            selections.emplace_back(sci.SelectionNStart(i), sci.SelectionNEnd(i));
        std::sort(selections.begin(), selections.end(),
            [](const std::pair<Scintilla::Position, Scintilla::Position>& x,
                const std::pair<Scintilla::Position, Scintilla::Position>& y) {return x.first > y.first; });
    }

    Scintilla::Line firstSelectedLine = sci.LineFromPosition(selections[selections.size() - 1].first);
    Scintilla::Line lastSelectedLine = sci.LineFromPosition(selections[0].second);
    setTabstops(dd, firstSelectedLine, lastSelectedLine);

    int blankWidth = sci.TextWidth(STYLE_DEFAULT, " ");
    sci.SetSearchFlags(Scintilla::FindOption::MatchCase);
    sci.BeginUndoAction();

    for (auto& sel : selections) {
        sci.SetTargetRange(sel.first, sel.second);
        std::string text = sci.TargetText();
        std::string repl;
        for (size_t i = 0;;) {
            size_t j = text.find_first_of('\t', i);
            if (j == std::string::npos) {
                repl += text.substr(i);
                break;
            }
            repl += text.substr(i, j - i);
            int width = sci.PointXFromPosition(sel.first + j + 1) - sci.PointXFromPosition(sel.first + j);
            int count = (2 * width + blankWidth) / (2 * blankWidth);
            repl += std::string(count, ' ');
            i = j + 1;
        }
        sci.ReplaceTarget(repl);
    }

    sci.EndUndoAction();
    analyzeTabstops(dd);
    setTabstops(dd);

}
