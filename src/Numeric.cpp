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
#include <string.h>
#include "resource.h"
#undef min
#undef max


const std::wstring currency =
    L"(?:[$\u00A2-\u00A5\u058F\u060B\u07FE\u07FF\u09F2\u09F3\u09FB\u0AF1\u0BF9\u0E3F\u17DB"
    L"\u20A0\u20A1\u20A2\u20A3\u20A4\u20A5\u20A6\u20A7\u20A8\u20A9\u20AA\u20AB\u20AC\u20AD\u20AE\u20AF"
    L"\u20B0\u20B1\u20B2\u20B3\u20B4\u20B5\u20B6\u20B7\u20B8\u20B9\u20BA\u20BB\u20BC\u20BD\u20BE\u20BF"
    L"\uA838\uFDFC\uFE69\uFF04\uFFE0\uFFE1\uFFE5\uFFE6]"
    L"|\U00011FDD|\U00011FDE|\U00011FDF|\U00011FE0|\U0001E2FF|\U0001ECB0)";

const std::wregex wrxPeriod(
L"(?:"
    L"(\\d+|\\d{1,3}(?:[\\s,']\\d{3})*)"
    L"(?:(?:(?::(\\d{1,2}))?:(\\d{1,2}))?:(\\d{1,2}))?"
    L"(?:\\.(?!\\.))?"
    L"|(?=\\.)"
L")"
L"(?:"
    L"\\."
    L"(\\d+|(?:\\d{3}[\\s,'])*\\d{1,3})"
L")?"
,
std::regex::optimize);

const std::wregex wrxComma(
L"(?:"
    L"(\\d+|\\d{1,3}(?:[\\s\\.']\\d{3})*)"
    L"(?:(?:(?::(\\d{1,2}))?:(\\d{1,2}))?:(\\d{1,2}))?"
    L"(?:,(?!,))?"
    L"|(?=,)"
L")"
L"(?:"
    L","
    L"(\\d+|(?:\\d{3}[\\s\\.'])*\\d{1,3})"
L")?"
,
std::regex::optimize);

const std::wregex wrxColon(
L"(?:\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d])\\s*)?0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:.*\\s(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))?0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:(?:.*\\s)?0\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s*)"
L"|(?:(?:.*\\s)?0(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s.*)"
,
std::regex::optimize);

const std::wregex wrxNoColon(
L"(?:\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d])\\s*)?0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:.*\\s(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))?0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:(?:.*\\s)?0\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s*)"
L"|(?:(?:.*\\s)?0(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s.*)"
/* currency sign inside */
L"|(?:\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d])\\s*)?" + currency + L"\\s*0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:.*\\s(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))?" + currency + L"\\s*0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:(?:.*\\s)?0\\s*" + currency + L"\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s*)"
L"|(?:(?:.*\\s)?0\\s*" + currency + L"(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s.*)"
/* currency sign outside */
L"|(?:\\s*" + currency + L"\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d])\\s*)?0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:.*\\s" + currency + L"\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d])\\s*)?0(?:[^+\uff0b\\-\u2012\u2013\uff0d].*)?)"
L"|(?:(?:.*\\s)?0\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s*" + currency + L"\\s*)"
L"|(?:(?:.*\\s)?0\\s*(?:[+\uff0b]|([\\-\u2012\u2013\uff0d]))\\s*" + currency + L"\\s.*)"
,
std::regex::optimize);

const std::wregex wrxGroupingForPeriod(L"[\\s,']", std::regex::optimize);

const std::wregex wrxGroupingForComma(L"[\\s\\.']", std::regex::optimize);


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


void applyThousandsSeparator(CalculationResultsInfo& cri) {
    cri.showResults.clear();
    cri.copyResults.clear();
    const char decimal = cri.data.settings.decimalSeparatorIsComma ? ',' : '.';
    const char separator = cri.data.thousands == ColumnsPlusPlusData::Thousands::None       ? 0
                         : cri.data.thousands == ColumnsPlusPlusData::Thousands::Apostrophe ? '\''
                         : cri.data.thousands == ColumnsPlusPlusData::Thousands::Blank      ? ' '
                         : cri.data.settings.decimalSeparatorIsComma                        ? '.' : ',';
    for (size_t i = 0; i < cri.results.size(); ++i) {
        std::string s = cri.results[i];
        if (separator && !s.empty()) {
            size_t j = s.find_first_not_of("0123456789");
            if (j == std::string::npos) j = s.length();
            else {
                size_t k = s.find_last_not_of("0123456789");
                if (s[k] == decimal) {
                    size_t p = s.length() - k - 1;
                    if (p > 3) {
                        for (size_t q = s.length() - (p - 1) % 3 - 1; q > k + 1; q -= 3)
                            s = s.substr(0, q) + separator + s.substr(q);
                    }
                }
            }
            if (j > 3) for (ptrdiff_t q = j - 3; q > 0; q -= 3)
                s = s.substr(0, q) + separator + s.substr(q);
        }
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
        CheckDlgButton(hwndDlg, IDC_CALCULATION_INSERT, cri.hasSpace ? data.calculateInsert : data.calculateAddLine);
        switch (data.thousands) {
        case ColumnsPlusPlusData::Thousands::None      : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_NONE      ); break;
        case ColumnsPlusPlusData::Thousands::Comma     : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_COMMA     ); break;
        case ColumnsPlusPlusData::Thousands::Apostrophe: CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_APOSTROPHE); break;
        case ColumnsPlusPlusData::Thousands::Blank     : CheckRadioButton(hwndDlg, IDC_THOUSANDS_NONE, IDC_THOUSANDS_BLANK, IDC_THOUSANDS_BLANK     ); break;
        }
        return FALSE;
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
            data.thousands = IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_COMMA     ) == BST_CHECKED ? ColumnsPlusPlusData::Thousands::Comma     
                           : IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_APOSTROPHE) == BST_CHECKED ? ColumnsPlusPlusData::Thousands::Apostrophe
                           : IsDlgButtonChecked(hwndDlg, IDC_THOUSANDS_BLANK     ) == BST_CHECKED ? ColumnsPlusPlusData::Thousands::Blank
                                                                                                  : ColumnsPlusPlusData::Thousands::None;
            applyThousandsSeparator(cri);
            SetDlgItemText(hwndDlg, IDC_CALCULATION_RESULTS, cri.showResults.data());
            break;
        case IDC_CALCULATION_INSERT:
            (cri.hasSpace ? data.calculateInsert : data.calculateAddLine) = IsDlgButtonChecked(hwndDlg, IDC_CALCULATION_INSERT) == BST_CHECKED;
            break;
        default:;
        }
        break;

    default:;
    }
    return FALSE;
}


bool parseNumber(ColumnsPlusPlusData& data, const std::string& text, long double* value, size_t* decimalPlaces, int* timeSegments, size_t* decimalIndex) {

    static const std::wstring spaces = L" \u00A0\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F";
    static const std::wstring inside = L".,:'0123456789" + spaces;

    size_t left = text.find_first_of("0123456789");
    if (left == std::string::npos) return false;
    size_t right = text.find_last_of("0123456789");
    size_t decimalPosition;
    if (left > 0 && text[left - 1] == (data.settings.decimalSeparatorIsComma ? ',' : '.')) {
        --left;
        decimalPosition = left;
    }
    else decimalPosition = text.find_first_of(data.settings.decimalSeparatorIsComma ? ',' : '.', left);
    if (decimalPosition == std::string::npos) decimalPosition = right + 1;
    else if (std::count(std::next(text.begin(), left), std::next(text.begin(), right),
                        data.settings.decimalSeparatorIsComma ? ',' : '.')
             > 1) return false;
    size_t colonPosition = text.find_last_of(':', right);
    if (colonPosition != std::string::npos) {
        if (colonPosition > decimalPosition) return false;
        if (std::count(std::next(text.begin(), left), std::next(text.begin(), right), ':')
            > 3) return false;
    }

    UINT codepage = data.sci.CodePage();
    std::wstring s = toWide(text, codepage);
    left = s.find_first_of(L"0123456789");
    right = s.find_last_of(L"0123456789");

    if (s.find_first_not_of(inside, left) < right) return false;
    if (left > 0 && s[left - 1] == (data.settings.decimalSeparatorIsComma ? L',' : L'.')) --left;

    if (decimalIndex) *decimalIndex = decimalPosition;
    if (!value) return true;

    std::wstring number = s.substr(left, right - left + 1);
    std::wstring shell = s.substr(0, left) + L"0" + s.substr(right + 1);

    std::wsmatch match;
    if (!std::regex_match(number, match, data.settings.decimalSeparatorIsComma ? wrxComma : wrxPeriod)) return std::string::npos;
    std::wstring sInteger = match[1];
    std::wstring sTimeH   = match[2];
    std::wstring sTimeM   = match[3];
    std::wstring sTimeS   = match[4];
    std::wstring sDecimal = match[5];
    sInteger = std::regex_replace(sInteger, data.settings.decimalSeparatorIsComma ? wrxGroupingForComma : wrxGroupingForPeriod, L"");
    sDecimal = std::regex_replace(sDecimal, data.settings.decimalSeparatorIsComma ? wrxGroupingForComma : wrxGroupingForPeriod, L"");
    *value = sInteger.length() ? std::stold(sInteger) : 0.0L;
    *timeSegments = 0;
    if (sTimeH.length()) { *value = *value * 24.0L + std::stold(sTimeH); *timeSegments = 3; }
    if (sTimeM.length()) { *value = *value * 60.0L + std::stold(sTimeM); if (!*timeSegments) *timeSegments = 2; }
    if (sTimeS.length()) { *value = *value * 60.0L + std::stold(sTimeS); if (!*timeSegments) *timeSegments = 1; }
    if (sDecimal.length()) *value += std::stold(L"." + sDecimal);
    *decimalPlaces = sDecimal.length();

    if (!std::regex_match(shell, match, colonPosition == std::string::npos ? wrxNoColon : wrxColon)) return false;
    for (size_t i = 1; i < match.size(); ++i) if (match[i].matched) { *value = -*value; break; }

    return true;

}


size_t ColumnsPlusPlusData::findDecimal(const std::string& text) {
    size_t decimalIndex;
    return parseNumber(*this, text, 0, 0, 0, &decimalIndex) ? decimalIndex : std::string::npos;
}


bool ColumnsPlusPlusData::getNumber(const std::string& text, long double& value, size_t& decimalPlaces, int& timeSegments) {
    return parseNumber(*this, text, &value, &decimalPlaces, &timeSegments, 0);
}


std::string formatValue(long double value, size_t decimalPlaces, size_t timeSegments, bool decimalSeparatorIsComma) {
    char answer[100];
    std::to_chars_result result;
    int reasonableDecimalPlaces = static_cast<int>(std::min(static_cast<size_t>(20), decimalPlaces));
    if (timeSegments && std::abs(value) < std::numeric_limits<long long>::max()) {
        long double t = value;
        if (decimalPlaces) {
            int m = 10;
            for (size_t i = 2; i < decimalPlaces; ++i) m *= 10;
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
            if (n >= 60 && timeSegments > 1) {
                mm = n % 60 + 100;
                n /= 60;
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
            if (mm) {
                if (hh) {
                    result = std::to_chars(p, answer + sizeof(answer), hh);
                    if (result.ec != std::errc{}) return "?";
                    *p = ':';
                    p = result.ptr;
                }
                result = std::to_chars(p, answer + sizeof(answer), mm);
                if (result.ec != std::errc{}) return "?";
                *p = ':';
                p = result.ptr;
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
        long double sum      = 0;
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
            long double value;
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

    if (cri.hasSpace ? !calculateInsert : !calculateAddLine) return;

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
