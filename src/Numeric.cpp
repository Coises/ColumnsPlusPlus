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

#include "Numeric.h"


struct CalculationResultsInfo {

    ColumnsPlusPlusData& data;
    HFONT font;
    std::vector<double       > result_values;
    std::vector<NumericFormat> result_auto_formats;
    std::wstring showResults;
    std::string  copyResults;
    int  columns      = 0;
    int  numbers      = 0;
    bool isMean       = false;
    bool hasSpace     = false;

    CalculationResultsInfo(ColumnsPlusPlusData& data) : data(data) {}

    void renderCalculationResults(HWND dialog) {
        showResults.clear();
        copyResults.clear();
        const std::string separator = data.calc.thousands == CalculateSettings::None ? ""
                                    : data.calc.thousands == CalculateSettings::Apostrophe ? "\'"
                                    : data.calc.thousands == CalculateSettings::Blank ? " "
                                    : data.settings.decimalSeparatorIsComma ? "." : ",";
        for (size_t i = 0; i < result_values.size(); ++i) {
            NumericFormat format = result_auto_formats[i];
            format.thousands = separator;
            if (!data.calc.autoDecimals) {
                format.maxDec = data.calc.decimalPlaces;
                format.minDec = data.calc.decimalsFixed ? format.maxDec : -1;
            }
            if (data.calc.timeSegments) format.timeEnable = 1 << (data.calc.timeSegments - 1);
            std::string s = data.formatNumber(result_values[i], format);
            if (!showResults.empty() && !s.empty()) showResults += L" | ";
            showResults += toWide(s, CP_UTF8);
            if (i > 0) copyResults += '\t';
            copyResults += s;
        }
        SetDlgItemText(dialog, IDC_CALCULATION_RESULTS, showResults.data());
    }

};


struct CalculateInfo {
    ColumnsPlusPlusData& data;
    exprtk::symbol_table<double> symbol_table;
    exprtk::expression<double> expression;
    exprtk::parser<double> parser;
    CalculateInfo(ColumnsPlusPlusData& data) : data(data) {}
};


void setupThousandsAndDecimals (HWND hwndDlg, ColumnsPlusPlusData& data) {
    SetDlgItemText(hwndDlg, IDC_THOUSANDS_COMMA, data.settings.decimalSeparatorIsComma ? L"&Period" : L"&Comma");
    switch (data.calc.thousands) {
    case CalculateSettings::None      : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_NONE      ); break;
    case CalculateSettings::Comma     : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_COMMA     ); break;
    case CalculateSettings::Apostrophe: CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_APOSTROPHE); break;
    case CalculateSettings::Blank     : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_BLANK     ); break;
    }
    SendDlgItemMessage(hwndDlg, IDC_CALCULATE_PLACES_SPIN, UDM_SETRANGE, 0, MAKELPARAM(16, 0));
    SendDlgItemMessage(hwndDlg, IDC_CALCULATE_PLACES_SPIN, UDM_SETPOS, 0, data.calc.decimalPlaces);
    SendDlgItemMessage(hwndDlg, IDC_CALCULATE_SUPPRESS_ZEROS, BM_SETCHECK, data.calc.decimalsFixed ? BST_UNCHECKED : BST_CHECKED, 0);
}


INT_PTR CALLBACK calculationResultsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    CalculationResultsInfo* crip = 0;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        crip = reinterpret_cast<CalculationResultsInfo*>(lParam);
    }
    else crip = reinterpret_cast<CalculationResultsInfo*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!crip) return TRUE;
    CalculationResultsInfo& cri = *crip;
    ColumnsPlusPlusData& data = cri.data;

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
        std::wstring message = cri.isMean ? L"Mean" : L"Sum";
        message += cri.columns == 1 ? L" of " + std::to_wstring(cri.numbers) + L" &values:"
                                    : L"s of " + std::to_wstring(cri.numbers) + L" &values in " + std::to_wstring(cri.columns) + L" columns:";
        SetDlgItemText(hwndDlg, IDC_CALCULATION_MESSAGE, message.data());
        message = cri.hasSpace ? L"&Insert these results in the last line of the selection."
                               : L"&Insert a line containing these results following the last line of the selection.";
        SetDlgItemText(hwndDlg, IDC_CALCULATION_INSERT, message.data());
        HDC sciDC = GetDC(data.activeScintilla);
        int logpixelsy = GetDeviceCaps(sciDC, LOGPIXELSY);
        ReleaseDC(data.activeScintilla, sciDC);
        RECT resultsRectangle;
        SendDlgItemMessage(hwndDlg, IDC_CALCULATION_RESULTS, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&resultsRectangle));
        LOGFONT lf;
        lf.lfHeight = -std::min((data.sci.StyleGetSize(0) + data.sci.Zoom()) * logpixelsy / 72L, resultsRectangle.bottom - resultsRectangle.top);
        lf.lfWidth = 0;
        lf.lfEscapement = 0;
        lf.lfOrientation = 0;
        lf.lfWeight = static_cast<LONG>(data.sci.StyleGetWeight(0));
        lf.lfItalic = data.sci.StyleGetItalic(0);
        lf.lfUnderline = data.sci.StyleGetUnderline(0);
        lf.lfStrikeOut = FALSE;
        lf.lfCharSet = ANSI_CHARSET;
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = PROOF_QUALITY;
        lf.lfPitchAndFamily = 0;
        std::wstring fontName = toWide(data.sci.StyleGetFont(0), CP_UTF8);
        if (fontName.length() > LF_FACESIZE - 1) fontName.resize(LF_FACESIZE - 1);
        wcsncpy(lf.lfFaceName, fontName.data(), LF_FACESIZE);
        cri.font = CreateFontIndirect(&lf);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATION_RESULTS, WM_SETFONT, reinterpret_cast<WPARAM>(cri.font), 0);
        cri.renderCalculationResults(hwndDlg);
        CheckDlgButton(hwndDlg, IDC_CALCULATION_INSERT, cri.hasSpace ? data.calc.insert : data.calc.addLine);
        setupThousandsAndDecimals(hwndDlg, data);
        SetDlgItemText(hwndDlg, IDC_CALCULATION_TIME1, data.timeScalarUnit == 0 ? L"&1 (d)"
                                                     : data.timeScalarUnit == 1 ? L"&1 (h)"
                                                     : data.timeScalarUnit == 2 ? L"&1 (m)"
                                                                                : L"&1 (s)");
        SetDlgItemText(hwndDlg, IDC_CALCULATION_TIME2, data.timePartialRule == 0 ? L"&2 (d:m)" : data.timePartialRule == 3 ? L"&2 (m:s)" : L"&2 (h:m)");
        SetDlgItemText(hwndDlg, IDC_CALCULATION_TIME3, data.timePartialRule < 2 ? L"&3 (d:h:m)" : L"&3 (h:m:s)");
        CheckDlgButton(hwndDlg, IDC_CALCULATION_DECIMAL_AUTO, data.calc.autoDecimals);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_PLACES_TEXT   ), !data.calc.autoDecimals);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_PLACES_EDIT   ), !data.calc.autoDecimals);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_PLACES_SPIN   ), !data.calc.autoDecimals);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_SUPPRESS_ZEROS), !data.calc.autoDecimals);
        CheckRadioButton(hwndDlg, IDC_CALCULATION_TIME_AUTO, IDC_CALCULATION_TIME4,
              data.calc.timeSegments == 1 ? IDC_CALCULATION_TIME1
            : data.calc.timeSegments == 2 ? IDC_CALCULATION_TIME2
            : data.calc.timeSegments == 3 ? IDC_CALCULATION_TIME3
            : data.calc.timeSegments == 4 ? IDC_CALCULATION_TIME4 : IDC_CALCULATION_TIME_AUTO);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            DeleteObject(cri.font);
            return TRUE;
        case IDOK:
            data.sci.CopyText(cri.copyResults.length(), cri.copyResults.data());
            EndDialog(hwndDlg, 0);
            DeleteObject(cri.font);
            return TRUE;
        case IDC_THOUSANDS_NONE:
        case IDC_THOUSANDS_COMMA:
        case IDC_THOUSANDS_APOSTROPHE:
        case IDC_THOUSANDS_BLANK:
            data.calc.thousands = IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_COMMA     ) == BST_CHECKED ? CalculateSettings::Comma
                                : IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_APOSTROPHE) == BST_CHECKED ? CalculateSettings::Apostrophe
                                : IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_BLANK     ) == BST_CHECKED ? CalculateSettings::Blank
                                                                                                       : CalculateSettings::None;
            cri.renderCalculationResults(hwndDlg);
            break;
        case IDC_CALCULATION_INSERT:
            (cri.hasSpace ? data.calc.insert : data.calc.addLine) = IsDlgButtonChecked(hwndDlg, IDC_CALCULATION_INSERT) == BST_CHECKED;
            break;
        case IDC_CALCULATION_DECIMAL_AUTO:
        {
            data.calc.autoDecimals = IsDlgButtonChecked(hwndDlg, IDC_CALCULATION_DECIMAL_AUTO) == BST_CHECKED;
            EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_PLACES_TEXT   ), !data.calc.autoDecimals);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_PLACES_EDIT   ), !data.calc.autoDecimals);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_PLACES_SPIN   ), !data.calc.autoDecimals);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CALCULATE_SUPPRESS_ZEROS), !data.calc.autoDecimals);
            [[fallthrough]];
        }
        case IDC_CALCULATE_PLACES_TEXT:
        case IDC_CALCULATE_PLACES_EDIT:
        case IDC_CALCULATE_PLACES_SPIN:
        case IDC_CALCULATE_SUPPRESS_ZEROS:
            if (!data.calc.autoDecimals) {
                auto r = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_PLACES_SPIN, UDM_GETPOS, 0, 0);
                data.calc.decimalPlaces = HIWORD(r) ? 2 : LOWORD(r);
                data.calc.decimalsFixed = IsDlgButtonChecked(hwndDlg, IDC_CALCULATE_SUPPRESS_ZEROS) == BST_UNCHECKED;
            }
            cri.renderCalculationResults(hwndDlg);
            break;
        case IDC_CALCULATION_TIME_AUTO: data.calc.timeSegments = 0; cri.renderCalculationResults(hwndDlg); break;
        case IDC_CALCULATION_TIME1:     data.calc.timeSegments = 1; cri.renderCalculationResults(hwndDlg); break;
        case IDC_CALCULATION_TIME2:     data.calc.timeSegments = 2; cri.renderCalculationResults(hwndDlg); break;
        case IDC_CALCULATION_TIME3:     data.calc.timeSegments = 3; cri.renderCalculationResults(hwndDlg); break;
        case IDC_CALCULATION_TIME4:     data.calc.timeSegments = 4; cri.renderCalculationResults(hwndDlg); break;
        }
        break;

    }
    return FALSE;
}


std::wstring enabledTimeString(int unit, int rule, int mask) {
    if (!(mask & 15)) mask = 1;
    std::wstring s = L"&Enabled (";
    if (mask & 1) s += (unit == 0 ? L"d, " : unit == 1 ? L"h, " : unit == 2 ? L"m, " : L"s, ");
    if (mask & 2) s += (rule == 0 ? L"d:m, " : rule == 3 ? L"m:s, " : L"h:m, ");
    if (mask & 4) s += (rule < 2 ? L"d:h:m, " : L"h:m:s, ");
    if (mask & 8) s += L"d:h:m:s)";
             else s = s.substr(0, s.length() - 2) + L')';
    return s;
}


INT_PTR CALLBACK calculateDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    CalculateInfo* cip = 0;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        cip = reinterpret_cast<CalculateInfo*>(lParam);
    }
    else cip = reinterpret_cast<CalculateInfo*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!cip) return TRUE;
    CalculateInfo& ci = *cip;
    ColumnsPlusPlusData& data = ci.data;

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
        for (const auto& s : data.calc.formulaHistory)
            SendDlgItemMessage(hwndDlg, IDC_CALCULATE_FORMULA, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        for (const auto& s : data.calc.regexHistory)
            SendDlgItemMessage(hwndDlg, IDC_CALCULATE_REGEX, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_FORMULA, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_REGEX, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_MATCH_CASE    , BM_SETCHECK, data.calc.matchCase     ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_SKIP_UNMATCHED, BM_SETCHECK, data.calc.skipUnmatched ? BST_CHECKED : BST_UNCHECKED, 0);
        setupThousandsAndDecimals(hwndDlg, data);
        SetDlgItemText(hwndDlg, IDC_CALCULATE_TIME, enabledTimeString(data.timeScalarUnit, data.timePartialRule, data.timeFormatEnable).data());
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TIME, BM_SETCHECK, data.calc.formatAsTime ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TABBED        , BM_SETCHECK, data.calc.tabbed        ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_ALIGNED       , BM_SETCHECK, data.calc.aligned       ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_LEFT          , BM_SETCHECK, data.calc.left          ? BST_CHECKED : BST_UNCHECKED, 0);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
        {
            HWND h = GetDlgItem(hwndDlg, IDC_CALCULATE_FORMULA);
            auto n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
            std::wstring s(n, 0);
            SendMessage(h, WM_GETTEXT, n + 1, (LPARAM)s.data());
            if (!ci.parser.compile(fromWide(s, CP_UTF8), ci.expression)) {
                auto error = ci.parser.get_error(0);
                std::wstring msg = toWide(error.diagnostic, CP_UTF8);
                COMBOBOXINFO cbi;
                cbi.cbSize = sizeof(COMBOBOXINFO);
                GetComboBoxInfo(h, &cbi);
                EDITBALLOONTIP ebt;
                ebt.cbStruct = sizeof(EDITBALLOONTIP);
                ebt.pszTitle = L"";
                ebt.ttiIcon = TTI_NONE;
                ebt.pszText = msg.data();
                SendMessage(cbi.hwndItem, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(cbi.hwndItem), TRUE);
                return TRUE;
            }
            h = GetDlgItem(hwndDlg, IDC_CALCULATE_REGEX);
            n = SendMessage(h, WM_GETTEXTLENGTH, 0, 0);
            if (n) {
                std::wstring w(n, 0);
                SendMessage(h, WM_GETTEXT, n + 1, reinterpret_cast<LPARAM>(w.data()));
                data.sci.SetSearchFlags(Scintilla::FindOption::RegExp | Scintilla::FindOption::Posix);
                data.sci.SetTargetRange(0, 0);
                Scintilla::Position found = data.sci.SearchInTarget(fromWide(w, data.sci.CodePage()));
                if (found < -1) {
                    COMBOBOXINFO cbi;
                    cbi.cbSize = sizeof(COMBOBOXINFO);
                    GetComboBoxInfo(h, &cbi);
                    EDITBALLOONTIP ebt;
                    ebt.cbStruct = sizeof(EDITBALLOONTIP);
                    ebt.pszTitle = L"";
                    ebt.ttiIcon = TTI_NONE;
                    std::wstring ebtText;
                    if (found == -2) {
                        if (size_t msglen = data.sci.Call(static_cast<Scintilla::Message>(SCI_GETBOOSTREGEXERRMSG), 0, 0)) {
                            std::string msg(msglen, 0);
                            data.sci.Call(static_cast<Scintilla::Message>(SCI_GETBOOSTREGEXERRMSG), msglen, reinterpret_cast<LPARAM>(msg.data()));
                            ebtText = toWide(msg, CP_UTF8);
                            ebt.pszText = ebtText.data();
                        }
                        else ebt.pszText = L"Invalid regular expression.";
                    }
                    else ebt.pszText = L"An unidentified error occurred processing this regular expression.";
                    SendMessage(cbi.hwndItem, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                    SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(cbi.hwndItem), TRUE);
                    return TRUE;
                }
            }
            updateComboHistory(hwndDlg, IDC_CALCULATE_FORMULA, data.calc.formulaHistory);
            updateComboHistory(hwndDlg, IDC_CALCULATE_REGEX, data.calc.regexHistory);
            data.calc.matchCase     = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_MATCH_CASE    , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.skipUnmatched = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_SKIP_UNMATCHED, BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.thousands = IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_COMMA     ) == BST_CHECKED ? CalculateSettings::Comma
                                : IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_APOSTROPHE) == BST_CHECKED ? CalculateSettings::Apostrophe
                                : IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_BLANK     ) == BST_CHECKED ? CalculateSettings::Blank
                                                                                                       : CalculateSettings::None;
            auto r = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_PLACES_SPIN, UDM_GETPOS, 0, 0);
            data.calc.decimalPlaces = HIWORD(r) ? 2 : LOWORD(r);
            data.calc.decimalsFixed = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_SUPPRESS_ZEROS, BM_GETCHECK, 0, 0) == BST_UNCHECKED;
            data.calc.formatAsTime  = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TIME          , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.tabbed        = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TABBED        , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.aligned       = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_ALIGNED       , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.left          = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_LEFT          , BM_GETCHECK, 0, 0) == BST_CHECKED;
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        case IDC_CALCULATE_FORMATS:
            data.showTimeFormatsDialog();
            SetDlgItemText(hwndDlg, IDC_CALCULATE_TIME, enabledTimeString(data.timeScalarUnit, data.timePartialRule, data.timeFormatEnable).data());
            InvalidateRect(hwndDlg, 0, TRUE);
            break;
        }

    }
    return FALSE;
}


void ColumnsPlusPlusData::accumulate(bool isMean) {

    auto rs = getRectangularSelection();
    if (!rs.size()) return;

    struct Column {
        double sum           = 0;
        int    decimalPlaces = 0;
        int    timeSegments  = 0;
        int    count         = 0;
    };
    std::vector<Column> column;
    Column total;
    int indexOfLastRowWithValues = -1;

    for (auto row : rs) {
        size_t columnNumber = 0;
        for (const auto& cell : row) {
            if (columnNumber >= column.size()) column.emplace_back();
            Column& col = column[columnNumber];
            ++columnNumber;
            auto [value, dp, ts] = parseNumber(cell.trim());
            if (!isfinite(value)) {
                if (cell.trim().find_first_of("0123456789") != std::string::npos) /* ambiguous: likely an error */ {
                    sci.SetSel(cell.left(), cell.right());
                    return;
                }
                continue;
            }
            col.sum += value;
            ++col.count;
            if (ts > col.timeSegments) col.timeSegments = ts;
            if (dp > col.decimalPlaces) col.decimalPlaces = dp;
            total.sum += value;
            ++total.count;
            if (ts > total.timeSegments) total.timeSegments = ts;
            if (dp > total.decimalPlaces) total.decimalPlaces = dp;
            indexOfLastRowWithValues = row.index;
        }
    }

    if (!total.count) {
        MessageBox(nppData._nppHandle, L"No numbers found in the selection.", isMean ? L"Average Numbers" : L"Add Numbers", MB_ICONINFORMATION);
        return;
    }

    CalculationResultsInfo cri(*this);
    cri.isMean = isMean;
    cri.hasSpace = rs.back().text().find_first_not_of("\t ") == std::string::npos;
    for (Column& col : column) {
        NumericFormat format;
        if (col.count) {
            format.maxDec = col.decimalPlaces;
            if (col.timeSegments) format.timeEnable = timeFormatEnable;
            if (isMean) {
                format.maxDec += col.count == 1 ? 0 : col.count == 2 || col.count == 5 || col.count == 10 ? 1
                               : col.count == 20 || col.count == 25 || col.count == 50 || col.count == 100 ? 2 : 3;
                cri.result_values.push_back(col.sum / col.count);
            }
            else cri.result_values.push_back(col.sum);
            ++cri.columns;
            cri.numbers += col.count;
        }
        else {
            cri.result_values.push_back(std::numeric_limits<double>::quiet_NaN());
        }
        cri.result_auto_formats.push_back(format);
    }

    DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_CALCULATION_RESULTS), nppData._nppHandle, calculationResultsDialogProc, reinterpret_cast<LPARAM>(&cri));

    if (cri.hasSpace ? !calc.insert : !calc.addLine) return;

    sci.BeginUndoAction();

    if (!cri.hasSpace) {
        std::string EOL;
        switch (sci.EOLMode()) {
        case Scintilla::EndOfLine::Cr  : EOL = "\r";
        case Scintilla::EndOfLine::Lf  : EOL = "\n";
        case Scintilla::EndOfLine::CrLf: EOL = "\r\n";
        }
        sci.InsertText(rs.bottom().en, EOL.data());
        rs.refit(true);
        if (!rs.topToBottom()) indexOfLastRowWithValues++;
    }

    auto answerRow = rs.back();
    auto& answer = cri.copyResults;

    if (answerRow.vsMin() == 0 || !settings.elasticEnabled) {
        if (column.size() == 1) {
            size_t space = answerRow.cpMax() - answerRow.cpMin() + answerRow.vsMax();
            answerRow.replace(space > answer.length() ? std::string(space - answer.length(), ' ') + answer : answer);
        }
        else answerRow.replace(std::string(answerRow.vsMin(), ' ') + answer);
    }
    else {
        auto modelRow = rs[indexOfLastRowWithValues];
        std::string model  = sci.StringOfRange(Scintilla::Span(sci.PositionFromLine(modelRow .line()), modelRow .cpMin()));
        std::string prefix = sci.StringOfRange(Scintilla::Span(sci.PositionFromLine(answerRow.line()), answerRow.cpMin()));
        if (settings.leadingTabsIndent) {
            size_t modelFirstNonTab = model.find_first_not_of('\t');
            size_t prefixFirstNonTab = prefix.find_first_not_of('\t');
            model = modelFirstNonTab == std::string::npos ? "" : model.substr(modelFirstNonTab);
            prefix = prefixFirstNonTab == std::string::npos ? "" : prefix.substr(prefixFirstNonTab);
        }
        auto modelTabs = std::count(model.begin(), model.end(), '\t');
        auto prefixTabs = std::count(prefix.begin(), prefix.end(), '\t');
        std::string leftPad;
        if (modelTabs > prefixTabs) {
            if (settings.leadingTabsIndent && prefix.length() == 0) leftPad = " ";
            leftPad += std::string(modelTabs - prefixTabs, '\t');
            size_t charsAfterLastTab = model.length() - model.find_last_of('\t') - 1;
            size_t padTabToLeft = 0;
            if (charsAfterLastTab > 0) {
                int pxDifference = sci.PointXFromPosition(modelRow.cpMin()) - sci.PointXFromPosition(modelRow.cpMin() - charsAfterLastTab);
                padTabToLeft = (2 * pxDifference + rs.blankWidth) / (2 * rs.blankWidth);
            }
            if (column.size() == 1) {
                size_t space = padTabToLeft + answerRow.vsMax() - answerRow.vsMin();
                if (space > answer.length()) leftPad += std::string(space - answer.length(), ' ');
            }
            else {
                leftPad += std::string(padTabToLeft, ' ');
            }
            answerRow.replace(leftPad + answer);
        }
        else {
            if (column.size() == 1) {
                size_t space = answerRow.vsMax();
                answerRow.replace(space > answer.length() ? std::string(space - answer.length(), ' ') + answer : answer);
            }
            else answerRow.replace(std::string(answerRow.vsMin(), ' ') + answer);
        }
    }

    rs.refit();
    sci.EndUndoAction();
}


void ColumnsPlusPlusData::calculate() {

    auto rs = getRectangularSelection();
    if (!rs.size()) return;

    CalculateInfo ci(*this);
    CalculateHistory history(*this);

    double exCount = rs.size();
    double exIndex = 0;
    double exMatch = 0;
    double exLine  = 0;
    double exThis  = 0;

    ExCol  exCol  (history);
    ExReg  exReg  (history);
    ExTab  exTab  (history);
    ExLast exLast (history);

    ci.symbol_table.add_variable("count", exCount);
    ci.symbol_table.add_variable("index", exIndex);
    ci.symbol_table.add_variable("match", exMatch);
    ci.symbol_table.add_variable("line" , exLine );
    ci.symbol_table.add_variable("this" , exThis );

    ci.symbol_table.add_function("col"  , exCol );
    ci.symbol_table.add_function("reg"  , exReg );
    ci.symbol_table.add_function("tab"  , exTab );
    ci.symbol_table.add_function("last" , exLast);

    ci.expression.register_symbol_table(ci.symbol_table);

    if (DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_CALCULATE), nppData._nppHandle, calculateDialogProc, reinterpret_cast<LPARAM>(&ci)))
        return;

    const std::string thousandsSeparator = calc.thousands == CalculateSettings::None ? ""
                                         : calc.thousands == CalculateSettings::Apostrophe ? "\'"
                                         : calc.thousands == CalculateSettings::Blank ? " "
                                         : settings.decimalSeparatorIsComma ? "." : ",";

    sci.SetSearchFlags(calc.matchCase ? Scintilla::FindOption::RegExp | Scintilla::FindOption::MatchCase : Scintilla::FindOption::RegExp);

    size_t   matches   = 0;
    intptr_t maxLeft   = 0;
    intptr_t maxRight  = 0;
    intptr_t maxString = 0;

    struct Item {
        std::string text;
        intptr_t left  = 0;
        bool     align = false;
    };

    std::vector<Item> items;

    history.results .reserve(rs.size());
    history.skipMap .reserve(rs.size());
    history.skipFlag.reserve(rs.size());
    items           .reserve(rs.size());

    NumericFormat format;
    intptr_t      colonDecimalOffset = 0;

    format.maxDec    = calc.decimalPlaces;
    format.thousands = thousandsSeparator;
    if (calc.decimalsFixed) format.minDec = calc.decimalPlaces;
    if (calc.formatAsTime) {
        format.timeEnable  = timeFormatEnable;
        colonDecimalOffset = (timeScalarUnit - (timePartialRule == 0 ? 0 : timePartialRule == 3 ? 2 : 1)) * 3;
    }

    for (auto row : rs) {

        ++exIndex;
        exLine = static_cast<double>(row.line() + 1);
        history.push();
        items.emplace_back();
        if (calc.regexHistory.size() && !calc.regexHistory.back().empty()) {
            Scintilla::Position found;
            exThis = history.reg(0, 0, &found);
            if (found < 0) {
                if (calc.skipUnmatched) {
                    history.skip();
                    continue;
                }
                exMatch = 0;
            }
            else exMatch = static_cast<double>(++matches);
        }
        else exThis = history.col(0, 0);
        double result = ci.expression.value();
        history.results.back() = result;
        Item& item = items.back();

        if (std::isfinite(result)) {
            item.text = formatNumber(result, format);
            intptr_t textLength = item.text.length();
            if (calc.aligned) {
                size_t cp = 0;
                size_t dp = 0;
                getNumericAlignment(item.text, cp, dp);
                item.left = cp == std::string::npos ? static_cast<intptr_t>(dp) - colonDecimalOffset : cp;
                if (item.left > maxLeft) maxLeft = item.left;
                if (textLength - item.left > maxRight) maxRight = textLength - item.left;
                item.align = true;
            }
            else if (textLength > maxString) maxString = textLength;
        }
        else {
            if (ci.expression.results().count()) {
                const auto& results = ci.expression.results();
                std::string s;
                bool lastWasNumeric = false;
                for (size_t i = 0; i < results.count(); ++i) {
                    auto r = results[i];
                    if (r.type == r.e_string) {
                        exprtk::igeneric_function<double>::generic_type::string_view sv(r);
                        s += std::string_view(sv.data_, sv.size());
                        lastWasNumeric = false;
                    }
                    else if (r.type == exprtk::type_store<double>::store_type::e_scalar) {
                        if (lastWasNumeric) s += calc.tabbed ? '\t' : ' ';
                        exprtk::igeneric_function<double>::generic_type::scalar_view sv(r);
                        if (std::isfinite(sv())) s += formatNumber(sv(), format);
                        lastWasNumeric = true;
                    }
                }
                item.text = s;
                if (maxString < static_cast<intptr_t>(s.length())) maxString = s.length();
            }
        }

    }

    if (maxString > maxLeft + maxRight) maxLeft = maxString - maxRight;
    int resultsIndex = 0;
    sci.BeginUndoAction();
    for (auto row : rs) {
        if (!history.skipped(resultsIndex)) {
            Item& item = items[resultsIndex];
            std::string s = item.text;
            if (calc.aligned && item.align) s = std::string(maxLeft - item.left, ' ') + s;
            if (!settings.elasticEnabled || !calc.tabbed) s += std::string(maxLeft + maxRight - s.length(), ' ');
            if (calc.left) {
                s += calc.tabbed ? '\t' : ' ';
                s += row.text();
                row.replace(s);
            }
            else {
                if (calc.tabbed) s = row.text().length() > 0 && row.text()[row.text().length() - 1] == '\t' ? s + '\t' : '\t' + s;
                else s = ' ' + s;
                s.insert(0, row.text());
                row.replace(s);
            }
        }
        ++resultsIndex;
    }
    rs.refit();
    sci.EndUndoAction();

}
