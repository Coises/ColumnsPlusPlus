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
    std::vector<std::string> results;
    std::wstring showResults;
    std::string copyResults;
    int columns = 0;
    int numbers = 0;
    bool isMean = false;
    bool hasSpace = false;
    HFONT font;
    CalculationResultsInfo(ColumnsPlusPlusData& data) : data(data) {}
};


struct CalculateInfo {
    ColumnsPlusPlusData& data;
    exprtk::symbol_table<double> symbol_table;
    exprtk::expression<double> expression;
    exprtk::parser<double> parser;
    CalculateInfo(ColumnsPlusPlusData& data) : data(data) {}
};


std::string applyThousandsSeparator(std::string s, const char decimalSeparator, const char thousandsSeparator) {
    if (thousandsSeparator && !s.empty()) {
        bool negative = false;
        if (s[0] == '-') {
            s = s.substr(1);
            negative = true;
        }
        size_t j = s.find_first_not_of("0123456789");
        if (j == std::string::npos) j = s.length();
        else {
            size_t k = s.find_last_not_of("0123456789");
            if (s[k] == decimalSeparator) {
                size_t p = s.length() - k - 1;
                if (p > 3) {
                    for (size_t q = s.length() - (p - 1) % 3 - 1; q > k + 1; q -= 3)
                        s = s.substr(0, q) + thousandsSeparator + s.substr(q);
                }
            }
        }
        if (j > 3) for (ptrdiff_t q = j - 3; q > 0; q -= 3)
            s = s.substr(0, q) + thousandsSeparator + s.substr(q);
        if (negative) s = '-' + s;
    }
    return s;
}


void applyThousandsSeparator(CalculationResultsInfo& cri) {
    cri.showResults.clear();
    cri.copyResults.clear();
    const char decimal = cri.data.settings.decimalSeparatorIsComma ? ',' : '.';
    const char separator = cri.data.calc.thousands == CalculateSettings::None       ? 0
                         : cri.data.calc.thousands == CalculateSettings::Apostrophe ? '\''
                         : cri.data.calc.thousands == CalculateSettings::Blank      ? ' '
                         : cri.data.settings.decimalSeparatorIsComma                ? '.' : ',';
    for (size_t i = 0; i < cri.results.size(); ++i) {
        std::string s = applyThousandsSeparator(cri.results[i], decimal, separator);
        if (!cri.showResults.empty() && !s.empty()) cri.showResults += L" | ";
        cri.showResults += toWide(s, CP_UTF8);
        if (i > 0) cri.copyResults += '\t';
        cri.copyResults += s;
    }
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
        SetDlgItemText(hwndDlg, IDC_THOUSANDS_COMMA, data.settings.decimalSeparatorIsComma ? L"&Period" : L"&Comma");
        applyThousandsSeparator(cri);
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
        SetDlgItemText(hwndDlg, IDC_CALCULATION_RESULTS, cri.showResults.data());
        CheckDlgButton(hwndDlg, IDC_CALCULATION_INSERT, cri.hasSpace ? data.calc.insert : data.calc.addLine);
        switch (data.calc.thousands) {
        case CalculateSettings::None      : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_NONE      ); break;
        case CalculateSettings::Comma     : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_COMMA     ); break;
        case CalculateSettings::Apostrophe: CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_APOSTROPHE); break;
        case CalculateSettings::Blank     : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_BLANK     ); break;
        }
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
            applyThousandsSeparator(cri);
            SetDlgItemText(hwndDlg, IDC_CALCULATION_RESULTS, cri.showResults.data());
            break;
        case IDC_CALCULATION_INSERT:
            (cri.hasSpace ? data.calc.insert : data.calc.addLine) = IsDlgButtonChecked(hwndDlg, IDC_CALCULATION_INSERT) == BST_CHECKED;
            break;
        default:;
        }
        break;

    default:;
    }
    return FALSE;
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
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TIME          , BM_SETCHECK, data.calc.formatAsTime  ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TIME_MINUTES  , BM_SETCHECK, data.calc.unitIsMinutes ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TIME_DAYS     , BM_SETCHECK, data.calc.showDays      ? BST_CHECKED : BST_UNCHECKED, 0);
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
            data.calc.unitIsMinutes = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TIME_MINUTES  , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.showDays      = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TIME_DAYS     , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.tabbed        = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_TABBED        , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.aligned       = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_ALIGNED       , BM_GETCHECK, 0, 0) == BST_CHECKED;
            data.calc.left          = SendDlgItemMessage(hwndDlg, IDC_CALCULATE_LEFT          , BM_GETCHECK, 0, 0) == BST_CHECKED;
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        default:;
        }
        break;

    default:;
    }
    return FALSE;
}


bool parseNumber2(ColumnsPlusPlusData& data, const std::string& text, double* value, size_t* decimalPlaces, int* timeSegments,
                  size_t* decimalIndex, bool timeUnitIsMinutes = false) {

    static const std::wstring currency =
        L"$\u00A2\u00A3\u00A4\u00A5\u058F\u060B\u07FE\u07FF\u09F2\u09F3\u09FB\u0AF1\u0BF9\u0E3F\u17DB"
        L"\u20A0\u20A1\u20A2\u20A3\u20A4\u20A5\u20A6\u20A7\u20A8\u20A9\u20AA\u20AB\u20AC\u20AD\u20AE\u20AF"
        L"\u20B0\u20B1\u20B2\u20B3\u20B4\u20B5\u20B6\u20B7\u20B8\u20B9\u20BA\u20BB\u20BC\u20BD\u20BE\u20BF"
        L"\uA838\uFDFC\uFE69\uFF04\uFFE0\uFFE1\uFFE5\uFFE6";
    static const std::wstring plus   = L"+\uff0b";
    static const std::wstring minus  = L"-\u2012\u2013\uff0d";
    static const std::wstring spaces = L" \u00A0\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F";
    static const std::wstring inside = L".,:'0123456789" + spaces;

    const wchar_t decimal = data.settings.decimalSeparatorIsComma ? L',' : L'.';

    const std::wstring s = toWide(text, data.sci.CodePage());

    size_t left = s.find_first_of(L"0123456789");
    if (left == std::string::npos) return false;
    size_t right = s.find_last_of(L"0123456789");

    size_t decimalPosition;
    if (left > 0 && s[left - 1] == decimal) {
        --left;
        decimalPosition = left;
    }
    else decimalPosition = s.find_first_of(decimal, left);
    if (decimalPosition == std::string::npos) decimalPosition = right + 1;
    else {
        if (decimalPosition > right + 1) decimalPosition = right + 1;
        else if (decimalPosition == right + 1) ++right;
        else if (std::count(std::next(s.begin(), left), std::next(s.begin(), right), decimal) > 1) return false;
    }
    size_t colonPosition = s.find_last_of(L':', right);
    if (colonPosition < left) colonPosition = std::wstring::npos;
    if (colonPosition != std::wstring::npos) {
        if (colonPosition > decimalPosition) return false;
        if (std::count(std::next(s.begin(), left), std::next(s.begin(), right), ':') > (timeUnitIsMinutes ? 2 : 3)) return false;
    }
    if (s.find_first_not_of(inside, left) < right) return false;

    if (decimalIndex) *decimalIndex = decimalPosition;
    if (!value) return true;

    const std::wstring_view prefix = std::wstring_view(s).substr(0, left);
    const std::wstring_view suffix = std::wstring_view(s).substr(right + 1);
    const std::wstring_view number = std::wstring_view(s).substr(left, right - left + 1);

    // prefix can be:
    // - empty or all space characters
    // - a sign and/or a currency symbol, with optional space characters before, after or between them
    // - any characters, followed by a space, followed by a sign and/or a currency symbol
    // - any mix of characters ending in a space

    // suffix can be:
    // - empty or all space characters
    // - a sign and/or a currency symbol, with optional space characters before, after or between them
    // - a sign and/or a currency symbol, followed by a space, followed by any characters
    // - any mix of characters that does not begin with a sign or with a currency symbol followed by a sign

    // additionally, a sign cannot appear in both the prefix and the suffix

    bool negative  = false;
    bool foundSign = false;

    if (prefix.find_first_not_of(spaces) != std::wstring::npos) /* not empty or all space characters */ {
        const size_t lastArbitrary = prefix.find_last_not_of(spaces + currency + plus + minus);
        if (size_t signAt = 0, cncyAt = 0;
              lastArbitrary == std::wstring::npos
           && (signAt = prefix.find_first_of(plus + minus), signAt == std::wstring::npos || signAt == prefix.find_last_of(plus + minus))
           && (cncyAt = prefix.find_first_of(currency)    , cncyAt == std::wstring::npos || cncyAt == prefix.find_last_of(currency    )) ) {
            // at most one sign, at most one currency symbol, and the rest space characters
            if (signAt != std::wstring::npos) {
                foundSign = true;
                if (minus.find_first_of(prefix[signAt]) != std::string::npos) negative = true;
            }
        }
        else {
            const size_t lastSpace = prefix.find_last_of(spaces);
            if (lastSpace == std::wstring::npos) return false;
            switch (prefix.length() - lastSpace) {
            case 1 /* space is last, OK */:
                break;
            case 2 /* one character, OK if sign or currency */:
                if (currency.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                foundSign = true;
                if (plus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                negative = true;
                if (minus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                return false;
            case 3 /* two characters, OK if one is sign and one is currency */:
                if      (plus    .find_first_of(prefix[lastSpace + 2]) != std::wstring::npos)   foundSign = true;
                else if (minus   .find_first_of(prefix[lastSpace + 2]) != std::wstring::npos) { foundSign = true; negative = true; }
                else if (currency.find_first_of(prefix[lastSpace + 2]) == std::wstring::npos) return false;
                if (currency.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos)
                    if (foundSign) break;
                    else return false;
                foundSign = true;
                if (plus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                negative = true;
                if (minus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                return false;
            default:
                return false;
            }
        }
    }

    if (suffix.find_first_not_of(spaces) != std::wstring::npos) /* not empty or all space characters */ {
        const size_t firstArbitrary = suffix.find_first_not_of(spaces + currency + plus + minus);
        if (size_t signAt = 0, cncyAt = 0;
              firstArbitrary == std::wstring::npos
           && (signAt = suffix.find_first_of(plus + minus), signAt == std::wstring::npos || signAt == suffix.find_last_of(plus + minus))
           && (cncyAt = suffix.find_first_of(currency)    , cncyAt == std::wstring::npos || cncyAt == suffix.find_last_of(currency    )) ) {
            // at most one sign, at most one currency symbol, and the rest space characters
            if (signAt != std::wstring::npos) {
                if (foundSign) return false;
                foundSign = true;
                if (minus.find_first_of(suffix[signAt]) != std::string::npos) negative = true;
            }
        }
        else if (firstArbitrary != 0) {
            // There will always be at least two characters here; any single character would have been handled already
            size_t firstSpace = suffix.find_first_of(spaces);
            if (firstSpace != 0) {
                cncyAt = suffix.find_first_of(currency);
                if (cncyAt == 0) /* first character in suffix is a currency symbol -- only matters if followed by a sign */ {
                    bool isMinus = minus.find_first_of(suffix[1]);
                    bool isPlus  = !isMinus && plus.find_first_of(suffix[1]);
                    if (isPlus || isMinus) {
                        if (foundSign || firstSpace != 2) return false;
                        foundSign = true;
                        if (isMinus) negative = true;
                    }
                }
                else /* first character in suffix is a sign */ {
                    if (foundSign || !(firstSpace == 1 || (cncyAt == 1 && firstSpace == 2))) return false;
                    foundSign = true;
                    negative = minus.find_first_of(suffix[0]) != std::wstring::npos;
                }
            }
        }
    }

    std::wstring n[4];
    int nLevel = 0;
    bool separatorOK = true;
    for (size_t i = 0; i < number.length(); ++i) {
        if (number[i] >= L'0' && number[i] <= L'9') {
            n[nLevel] += number[i];
            separatorOK = true;
            continue;
        }
        else if (!separatorOK) return false;
        separatorOK = false;
        if (number[i] == decimal) n[nLevel] += L'.';
        else if (number[i] == L':') ++nLevel;
    }
    double v = std::stod(n[nLevel]);
    if (nLevel > 0) v += 60 * std::stod(n[nLevel - 1]);
    if (nLevel > 1 && !timeUnitIsMinutes) v += 60 * std::stod(n[nLevel - 2]);
    if (nLevel > 2 || (timeUnitIsMinutes && nLevel > 1)) v += 24 * std::stod(n[0]);
    *value = negative ? -v : v;
    if (decimalPlaces) *decimalPlaces = right > decimalPosition ? right - decimalPosition : 0;
    if (timeSegments) *timeSegments = nLevel;
    return true;

}


size_t ColumnsPlusPlusData::findDecimal(const std::string& text, bool timeUnitIsMinutes) {

    static const std::wstring inside = L".,:'0123456789 \u00A0\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F";

    const char decimal = settings.decimalSeparatorIsComma ? ',' : '.';

    size_t left = text.find_first_of("0123456789");
    if (left == std::string::npos) return std::string::npos;
    size_t right = text.find_last_of("0123456789");

    size_t decimalPosition = (left > 0 && text[left - 1] == decimal) ? --left : text.find_first_of(decimal, left);
    if (decimalPosition > right) decimalPosition = right + 1;
    else if (decimalPosition != text.find_last_of(decimal, right)) return std::string::npos;

    size_t colonPosition = text.find_last_of(':', right);
    if (colonPosition != std::string::npos) {
        if (colonPosition > decimalPosition) return std::string::npos;
        if (std::count(std::next(text.begin(), left), std::next(text.begin(), right), ':')
            > (timeUnitIsMinutes ? 2 : 3)) return std::string::npos;
    }

    std::wstring s = toWide(text, sci.CodePage());
    left = s.find_first_of(L"0123456789");
    right = s.find_last_of(L"0123456789");
    if (s.find_first_not_of(inside, left) < right) return std::string::npos;

    return decimalPosition;

}


bool ColumnsPlusPlusData::getNumber(const std::string& text, double& value, size_t& decimalPlaces, int& timeSegments, bool timeUnitIsMinutes) {

    static const std::wstring currency =
        L"$\u00A2\u00A3\u00A4\u00A5\u058F\u060B\u07FE\u07FF\u09F2\u09F3\u09FB\u0AF1\u0BF9\u0E3F\u17DB"
        L"\u20A0\u20A1\u20A2\u20A3\u20A4\u20A5\u20A6\u20A7\u20A8\u20A9\u20AA\u20AB\u20AC\u20AD\u20AE\u20AF"
        L"\u20B0\u20B1\u20B2\u20B3\u20B4\u20B5\u20B6\u20B7\u20B8\u20B9\u20BA\u20BB\u20BC\u20BD\u20BE\u20BF"
        L"\uA838\uFDFC\uFE69\uFF04\uFFE0\uFFE1\uFFE5\uFFE6";
    static const std::wstring plus = L"+\uff0b";
    static const std::wstring minus = L"-\u2012\u2013\uff0d";
    static const std::wstring spaces = L" \u00A0\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F";
    static const std::wstring inside = L".,:'0123456789" + spaces;

    const wchar_t decimal = settings.decimalSeparatorIsComma ? L',' : L'.';
    const std::wstring s = toWide(text, sci.CodePage());

    size_t left = s.find_first_of(L"0123456789");
    if (left == std::string::npos) return false;
    size_t right = s.find_last_of(L"0123456789");

    size_t decimalPosition = (left > 0 && s[left - 1] == decimal) ? --left : s.find_first_of(decimal, left);
    if (decimalPosition == right + 1) ++right;
    else if (decimalPosition > right) decimalPosition = right + 1;
    else if (decimalPosition != s.find_last_of(decimal, right)) return false;

    size_t colonPosition = s.find_last_of(L':', right);
    if (colonPosition < left) colonPosition = std::wstring::npos;
    if (colonPosition != std::wstring::npos) {
        if (colonPosition > decimalPosition) return false;
        if (std::count(std::next(s.begin(), left), std::next(s.begin(), right), ':') > (timeUnitIsMinutes ? 2 : 3)) return false;
    }

    const std::wstring_view prefix = std::wstring_view(s).substr(0, left);
    const std::wstring_view number = std::wstring_view(s).substr(left, right - left + 1);
    const std::wstring_view suffix = std::wstring_view(s).substr(right + 1);

    if (number.find_first_not_of(inside) != std::wstring::npos) return false;

    // prefix can be:
    // - empty or all space characters
    // - a sign and/or a currency symbol, with optional space characters before, after or between them
    // - any characters, followed by a space, followed by a sign and/or a currency symbol
    // - any mix of characters ending in a space

    // suffix can be:
    // - empty or all space characters
    // - a sign and/or a currency symbol, with optional space characters before, after or between them
    // - a sign and/or a currency symbol, followed by a space, followed by any characters
    // - any mix of characters that does not begin with a sign or with a currency symbol followed by a sign

    // a sign cannot appear in both the prefix and the suffix

    bool negative = false;
    bool foundSign = false;

    if (prefix.find_first_not_of(spaces) != std::wstring::npos) /* not empty or all space characters */ {
        const size_t lastArbitrary = prefix.find_last_not_of(spaces + currency + plus + minus);
        const size_t signAt = prefix.find_first_of(plus + minus);
        const size_t cncyAt = prefix.find_first_of(currency);
        if (lastArbitrary == std::wstring::npos
            && (signAt == std::wstring::npos || signAt == prefix.find_last_of(plus + minus))
            && (cncyAt == std::wstring::npos || cncyAt == prefix.find_last_of(currency))) {
            // at most one sign, at most one currency symbol, and the rest space characters
            if (signAt != std::wstring::npos) {
                foundSign = true;
                negative = minus.find_first_of(prefix[signAt]) != std::wstring::npos;
            }
        }
        else {
            const size_t lastSpace = prefix.find_last_of(spaces);
            if (lastSpace == std::wstring::npos) return false;
            switch (prefix.length() - lastSpace) {
            case 1 /* space is last, OK */:
                break;
            case 2 /* one character, OK if sign or currency */:
                if (currency.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                foundSign = true;
                if (plus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                negative = true;
                if (minus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                return false;
            case 3 /* two characters, OK if one is sign and one is currency */:
                if (plus.find_first_of(prefix[lastSpace + 2]) != std::wstring::npos)   foundSign = true;
                else if (minus.find_first_of(prefix[lastSpace + 2]) != std::wstring::npos) { foundSign = true; negative = true; }
                else if (currency.find_first_of(prefix[lastSpace + 2]) == std::wstring::npos) return false;
                if (currency.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos)
                    if (foundSign) break;
                    else return false;
                foundSign = true;
                if (plus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                negative = true;
                if (minus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                return false;
            default:
                return false;
            }
        }
    }

    if (suffix.find_first_not_of(spaces) != std::wstring::npos) /* not empty or all space characters */ {
        const size_t firstArbitrary = suffix.find_first_not_of(spaces + currency + plus + minus);
        const size_t signAt = suffix.find_first_of(plus + minus);
        const size_t cncyAt = suffix.find_first_of(currency);
        if (firstArbitrary == std::wstring::npos
            && (signAt == std::wstring::npos || signAt == suffix.find_last_of(plus + minus))
            && (cncyAt == std::wstring::npos || cncyAt == suffix.find_last_of(currency))) {
            // at most one sign, at most one currency symbol, and the rest space characters
            if (signAt != std::wstring::npos) {
                if (foundSign) return false;
                foundSign = true;
                if (minus.find_first_of(suffix[signAt]) != std::wstring::npos) negative = true;
            }
        }
        else if (firstArbitrary != 0) {
            // There will always be at least two characters here; any single character would have been handled already
            size_t firstSpace = suffix.find_first_of(spaces);
            if (firstSpace != 0) {
                if (cncyAt == 0) /* first character in suffix is a currency symbol -- only matters if followed by a sign */ {
                    bool isMinus = minus.find_first_of(suffix[1]);
                    bool isPlus = !isMinus && plus.find_first_of(suffix[1]);
                    if (isPlus || isMinus) {
                        if (foundSign || firstSpace != 2) return false;
                        foundSign = true;
                        if (isMinus) negative = true;
                    }
                }
                else /* first character in suffix is a sign */ {
                    if (foundSign || !(firstSpace == 1 || (cncyAt == 1 && firstSpace == 2))) return false;
                    foundSign = true;
                    negative = minus.find_first_of(suffix[0]) != std::wstring::npos;
                }
            }
        }
    }

    std::string n[4];
    int nLevel = 0;
    bool separatorOK = true;
    for (size_t i = 0; i < number.length(); ++i) {
        if (number[i] >= L'0' && number[i] <= L'9') {
            n[nLevel] += static_cast<char>(number[i]);
            separatorOK = true;
            continue;
        }
        else if (!separatorOK) return false;
        separatorOK = false;
        if (number[i] == decimal) n[nLevel] += '.';
        else if (number[i] == L':') ++nLevel;
    }
    double v = std::stod(n[nLevel]);
    if (nLevel > 0) v += 60 * std::stod(n[nLevel - 1]);
    if (nLevel > 1 && !timeUnitIsMinutes) v += 60 * std::stod(n[nLevel - 2]);
    if (nLevel > 2 || (timeUnitIsMinutes && nLevel > 1)) v += 24 * std::stod(n[0]);
    value = negative ? -v : v;
    decimalPlaces = right > decimalPosition ? right - decimalPosition : 0;
    timeSegments = nLevel;
    return true;

}


std::string formatValue(double value, size_t decimalPlaces, size_t timeSegments, bool decimalSeparatorIsComma, bool unitIsMinutes = false) {
    char answer[100];
    std::to_chars_result result;
    int reasonableDecimalPlaces = static_cast<int>(std::min(static_cast<size_t>(20), decimalPlaces));
    if (timeSegments && std::abs(value) < std::numeric_limits<long long>::max()) {
        double t = value;
        if (decimalPlaces) {
            int m = 10;
            for (size_t i = 1; i < decimalPlaces; ++i) m *= 10;
            t = std::round(m * t) / m;
        }
        bool negative = false;
        if (t < 0) {
            t = -t;
            negative = true;
        }
        long long n = static_cast<long long>(std::trunc(t));
        t -= n;
        int ss = 0;
        int mm = 0;
        int hh = 0;
        if (n >= 60) {
            ss = n % 60 + 100;
            n /= 60;
            if ((unitIsMinutes || n >= 60) && timeSegments > 1) {
                if (!unitIsMinutes) {
                    mm = n % 60 + 100;
                    n /= 60;
                }
                if (n >= 24 && timeSegments > 2) {
                    hh = n % 24 + 100;
                    n /= 24;
                }
            }
        }
        if (ss) {
            char* p = answer;
            if (negative) {
                *p = '-';
                ++p;
            }
            result = std::to_chars(p, answer + sizeof(answer), n);
            if (result.ec != std::errc{}) return "?";
            p = result.ptr;
            if (mm || unitIsMinutes) {
                if (hh) {
                    result = std::to_chars(p, answer + sizeof(answer), hh);
                    if (result.ec != std::errc{}) return "?";
                    *p = ':';
                    p = result.ptr;
                }
                if (!unitIsMinutes) {
                    result = std::to_chars(p, answer + sizeof(answer), mm);
                    if (result.ec != std::errc{}) return "?";
                    *p = ':';
                    p = result.ptr;
                }
            }
            t += ss;
            result = std::to_chars(p, answer + sizeof(answer), t, std::chars_format::fixed, reasonableDecimalPlaces);
            if (result.ec != std::errc{}) return "?";
            *p = ':';
        }
        else {
            result = std::to_chars(answer, answer + sizeof(answer), value, std::chars_format::fixed, reasonableDecimalPlaces);
            if (result.ec != std::errc{}) return "?";
        }
    }
    else {
        result = std::to_chars(answer, answer + sizeof(answer), value, std::chars_format::fixed, reasonableDecimalPlaces);
        if (result.ec != std::errc{}) return "?";
    }
    if (decimalSeparatorIsComma && decimalPlaces) {
        char* p = (char*)memchr(answer, '.', result.ptr - answer);
        if (p) *p = ',';
    }
    *result.ptr = 0;
    return answer;
}


void ColumnsPlusPlusData::accumulate(bool isMean) {

    auto rs = getRectangularSelection();
    if (!rs.size()) return;

    struct Column {
        double sum      = 0;
        size_t decimalPlaces = 0;
        int timeSegments     = 0;
        int count            = 0;
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
            double value;
            size_t dp;
            int ts;
            if (!getNumber(cell.trim(), value, dp, ts)) {
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
        if (col.count) {
            if (isMean) {
                int dp = col.count == 1 ? 0 : col.count == 2 || col.count == 5 || col.count == 10 ? 1
                    : col.count == 20 || col.count == 25 || col.count == 50 || col.count == 100 ? 2 : 3;
                cri.results.push_back(formatValue(col.sum / col.count, col.decimalPlaces + dp, col.timeSegments, settings.decimalSeparatorIsComma));
            }
            else cri.results.push_back(formatValue(col.sum, col.decimalPlaces, col.timeSegments, settings.decimalSeparatorIsComma));
            ++cri.columns;
            cri.numbers += col.count;
        }
        else cri.results.push_back("");
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

    const char decimalSeparator   = settings.decimalSeparatorIsComma ? ',' : '.';
    const char thousandsSeparator = calc.thousands == CalculateSettings::None       ? 0
                                  : calc.thousands == CalculateSettings::Apostrophe ? '\''
                                  : calc.thousands == CalculateSettings::Blank      ? ' '
                                  : settings.decimalSeparatorIsComma                ? '.' : ',';

    sci.SetSearchFlags(calc.matchCase ? Scintilla::FindOption::RegExp | Scintilla::FindOption::MatchCase : Scintilla::FindOption::RegExp);

    size_t matches   = 0;
    size_t maxLeft   = 0;
    size_t maxRight  = 0;
    size_t maxString = 0;
    std::vector<std::string> textResults;
    std::vector<bool> canAlign;

    history.results .reserve(rs.size());
    history.skipMap .reserve(rs.size());
    history.skipFlag.reserve(rs.size());
    textResults     .reserve(rs.size());
    canAlign        .reserve(rs.size());

    for (auto row : rs) {

        ++exIndex;
        exLine  = static_cast<double>(row.line() + 1);
        history.push();
        if (calc.regexHistory.size() && !calc.regexHistory.back().empty()) {
            Scintilla::Position found;
            exThis = history.reg(0, 0, &found);
            if (found < 0) {
                if (calc.skipUnmatched) {
                    history.skip();
                    textResults.push_back("");
                    continue;
                }
                exMatch = 0;
            }
            else exMatch = static_cast<double>(++matches);
        }
        else exThis = history.col(0, 0);
        double result = ci.expression.value();
        history.results.back() = result;

        if (std::isfinite(result)) {
            std::string s = formatValue(result, calc.decimalPlaces, !calc.formatAsTime ? 0 : calc.showDays ? 3 : 2,
                                        settings.decimalSeparatorIsComma, calc.unitIsMinutes);
            if (!calc.decimalsFixed && calc.decimalPlaces > 0) {
                s = s.substr(0, s.find_last_not_of('0') + 1);
                if (s.back() == decimalSeparator) s.pop_back();
            }
            s = applyThousandsSeparator(s, decimalSeparator, thousandsSeparator);
            if (calc.aligned) {
                size_t j = s.find_first_of(decimalSeparator);
                if (j == std::string::npos) j = s.length();
                if (j > maxLeft) maxLeft = j;
                if (s.length() - j > maxRight) maxRight = s.length() - j;
            }
            else if (s.length() > maxRight) maxRight = s.length();
            textResults.push_back(s);
            canAlign.push_back(true);
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
                        if (std::isfinite(sv())) {
                            std::string v = formatValue(sv(), calc.decimalPlaces, !calc.formatAsTime ? 0 : calc.showDays ? 3 : 2,
                                                              settings.decimalSeparatorIsComma, calc.unitIsMinutes);
                            if (!calc.decimalsFixed && calc.decimalPlaces > 0) {
                                v = v.substr(0, v.find_last_not_of('0') + 1);
                                if (v.back() == decimalSeparator) v.pop_back();
                            }
                            s += applyThousandsSeparator(v, decimalSeparator, thousandsSeparator);
                        }
                        lastWasNumeric = true;
                    }
                }
                textResults.push_back(s);
                if (maxString < s.length()) maxString = s.length();
            }
            else textResults.push_back("");
            canAlign.push_back(false);
        }

    }

    if (maxString > maxLeft + maxRight) maxLeft = maxString - maxRight;
    int resultsIndex = 0;
    sci.BeginUndoAction();
    for (auto row : rs) {
        if (!history.skipped(resultsIndex)) {
            std::string s = textResults[resultsIndex];
            if (calc.aligned && canAlign[resultsIndex]) {
                size_t j = s.find_first_of(decimalSeparator);
                if (j == std::string::npos) j = s.length();
                s = std::string(maxLeft - j, ' ') + s;
            }
            if (!settings.elasticEnabled || !calc.tabbed) s += std::string(maxLeft + maxRight - s.length(), ' ');
            if (calc.left) {
                s += calc.tabbed ? '\t' : ' ';
                row.replace(s + row.text());
            }
            else {
                if (calc.tabbed) s = row.text().length() > 0 && row.text()[row.text().length() - 1] == '\t' ? s + '\t' : '\t' + s;
                else s = ' ' + s;
                row.replace(row.text() + s);
            }
        }
        ++resultsIndex;
    }
    rs.refit();
    sci.EndUndoAction();

}
