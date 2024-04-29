// This file is part of Columns++ for Notepad++.
// Copyright 2023, 2024 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

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

#include <chrono>
#include "ColumnsPlusPlus.h"
#include "RegularExpression.h"
#include "resource.h"
#include "commctrl.h"


namespace {

bool ratioToDecimal(int64_t numerator, int64_t denominator, int64_t& integer, int64_t& decimal, size_t& places) {
    if (denominator == 0) return false;
    if (numerator == 0) {
        integer = 0;
        decimal = 0;
        places  = 0;
        return true;
    }
    if (denominator == 1) {
        integer = numerator;
        decimal = 0;
        places  = 0;
        return true;
    }
    if (denominator == -1) {
        integer = -numerator;
        decimal = 0;
        places  = 0;
        return true;
    }
    int64_t quotient  = numerator / denominator;
    int64_t remainder = numerator % denominator;
    if (remainder == 0) {
        integer = quotient;
        decimal = 0;
        places  = 0;
        return true;
    }
    uint64_t den = std::abs(denominator);
    uint64_t rem = std::abs(remainder);
    uint64_t limit = std::numeric_limits<uint64_t>::max() / (2 * rem + 1);
    for (uint64_t exponent = 0, power = 1; power < limit; ++exponent, power *= 10) {
        uint64_t fraction = (rem * power) / den;
        if (2 * ((rem * power) % den) >= den) ++fraction;
        if (rem == 0 || ((2 * den * fraction) > (2 * rem - 1) * power && (2 * den * fraction) < (2 * rem + 1) * power)) {
            integer = quotient;
            decimal = fraction;
            places  = static_cast<size_t>(exponent);
            return true;
        }
    }
    return false;
}

bool ratioToDecimal(int64_t numerator, int64_t denominator, std::string& result, char decimalSeparator = '.') {
    int64_t integer;
    int64_t decimal;
    size_t  places;
    if (!ratioToDecimal(numerator, denominator, integer, decimal, places)) return false;
    result = std::to_string(integer);
    if (places) {
        result += decimalSeparator;
        std::string f = std::to_string(decimal);
        if (places > f.length()) result += std::string(places - f.length(), '0');
        result += f;
    }
    return true;
}

int64_t utcTime(const std::wstring& textTime) {
    std::wistringstream stream(textTime);
    std::chrono::utc_clock::time_point tp;
    std::chrono::from_stream(stream, L"%F%n %T ", tp);
    if (stream.fail()) {
        stream.clear();
        stream.seekg(0);
        std::chrono::from_stream(stream, L" %F ", tp);
        if (stream.fail()) return std::string::npos;
    }
    if (!stream.eof()) return std::string::npos;
    return tp.time_since_epoch().count();
}

int64_t utcUnit(const std::wstring& textUnit) {
    std::wistringstream stream(textUnit);
    double parsed;
    if (!(stream >> parsed)) return 0;
    stream >> std::ws;
    if (!(stream.eof())) return 0;
    if (parsed <= 0 || parsed >= 4e10) return 0;
    return static_cast<int64_t>(std::round(parsed * 1e7));
}

std::chrono::utc_clock::time_point timePoint(int64_t t) {
    return std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(t));
}

std::wstring localeInfo(LCTYPE info, std::wstring locale = L"") {
    int n = GetLocaleInfoEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), info, 0, 0);
    if (n < 2) return L"";
    std::wstring result(n - 1, 0);
    return GetLocaleInfoEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), info, result.data(), n) ? result : L"";
}

std::wstring localeGenetiveMonth(WORD month, std::wstring locale = L"") {
    SYSTEMTIME st;
    st.wYear  = 1970;
    st.wMonth = month;
    st.wDay   = 10;
    int n = GetDateFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), 0, &st, L"ddMMMM", 0, 0, 0);
    if (n < 3) return L"";
    std::wstring answer(n - 1, 0);
    GetDateFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), 0, &st, L"ddMMMM", answer.data(), n, 0);
    return answer.substr(2);
}

template<class Clock> std::wstring formatTimePoint(std::chrono::time_point<Clock> timePoint, std::wstring format) {
    std::wstring info = std::format(L"{0:%Y}{0:%m}{0:%d}{0:%j}{0:%H}{0:%I}{0:%M}{0:%S}", timePoint);
    std::wstring s;
    for (size_t i = 0; i < format.length();) {
        switch (format[i]) {
        case L'y':
        {
            size_t j = std::min(format.find_first_not_of(L'y', i), format.length());
            s += j - i < 3 ? info.substr(2, 2) : info.substr(0, 4);
            i = j;
            break;
        }
        case L'M':
        {
            size_t j = std::min(format.find_first_not_of(L'M', i), format.length());
            s += j - i < 2 && info[4] == L'0' ? info.substr(5, 1) : j - i < 3 ? info.substr(4, 2)
                : j - i == 3 ? std::format(L"{0:%b}", timePoint)
                : j - i == 4 ? std::format(L"{0:%B}", timePoint)
                : (std::format(L"{0:%B}", timePoint) + std::wstring(j - i, L' ')).substr(0, j - i);
            i = j;
            break;
        }
        case L'd':
        {
            size_t j = std::min(format.find_first_not_of(L'd', i), format.length());
            s += j - i < 2 && info[6] == L'0' ? info.substr(7, 1) : j - i < 3 ? info.substr(6, 2)
                : j - i == 3 ? std::format(L"{0:%a}", timePoint)
                : j - i == 4 ? std::format(L"{0:%A}", timePoint)
                : (std::format(L"{0:%A}", timePoint) + std::wstring(j - i, L' ')).substr(0, j - i);
            i = j;
            break;
        }
        case L'D':
        {
            size_t j = std::min(format.find_first_not_of(L'j', i), format.length());
            s += j - i < 2 && info[8] == L'0' && info[9] == L'0' ? info.substr(10, 1) : j - i < 3 && info[8] == L'0' ? info.substr(9, 2) : info.substr(8, 3);
            i = j;
            break;
        }
        case L'H':
        {
            size_t j = std::min(format.find_first_not_of(L'H', i), format.length());
            s += j - i < 2 && info[11] == L'0' ? info.substr(12, 1) : info.substr(11, 2);
            i = j;
            break;
        }
        case L'h':
        {
            size_t j = std::min(format.find_first_not_of(L'h', i), format.length());
            s += j - i < 2 && info[13] == L'0' ? info.substr(14, 1) : info.substr(13, 2);
            i = j;
            break;
        }
        case L'm':
        {
            size_t j = std::min(format.find_first_not_of(L'm', i), format.length());
            s += j - i < 2 && info[15] == L'0' ? info.substr(16, 1) : info.substr(15, 2);
            i = j;
            break;
        }
        case L's':
        {
            size_t j = std::min(format.find_first_not_of(L's', i), format.length());
            s += j - i < 2 && info[17] == L'0' ? info.substr(18, 1) : info.substr(17, 2);
            i = j;
            if (format.substr(i, 2) == L".s" || format.substr(i, 2) == L",s") {
                s += format[i];
                j = std::min(format.find_first_not_of(L's', i + 1), format.length());
                s += info.substr(20, j - i - 1);
                i = j;
            }
            break;
        }
        case L't':
        {
            size_t j = std::min(format.find_first_not_of(L't', i), format.length());
            std::wstring ampm = std::format(L"{0:%p}", timePoint);
            if (j - i == 1) s += ampm[0];
            else if (j - i == 2) s += ampm;
            else if (j - i == 3) s += static_cast<wchar_t>(std::tolower(ampm[0]));
            else for (size_t k = 0; k < ampm.length(); ++k) s += static_cast<wchar_t>(std::tolower(ampm[k]));
            i = j;
            break;
        }
        case L'\'':
            for (;;) {
                size_t j = std::min(format.find_first_of(L'\'', i + 1), format.length());
                s += format.substr(i + 1, j - i - 1);
                i = j + 1;
                if (i >= format.length() || format[i] != L'\'') break;
                s += L'\'';
            }
            break;
        case L'z':
            if (i + 1 >= format.length()) {
                s += L'z';
                ++i;
                break;
            }
            switch (format[i + 1]) {
            case L'M':
                s += info[4] == L'0' ? L' ' + info.substr(5, 1) : info.substr(4, 2);
                i += 2;
                break;
            case L'd':
                s += info[6] == L'0' ? L' ' + info.substr(7, 1) : info.substr(6, 2);
                i += 2;
                break;
            case L'D':
                s += info[8] == L'0' && info[9] == L'0' ? L"  " + info.substr(10, 1) : info[8] == L'0' ? L' ' + info.substr(9, 2) : info.substr(8, 3);
                i += 2;
                break;
            case L'H':
                s += info[11] == L'0' ? L' ' + info.substr(12, 1) : info.substr(11, 2);
                i += 2;
                break;
            case L'h':
                s += info[13] == L'0' ? L' ' + info.substr(14, 1) : info.substr(13, 2);
                i += 2;
                break;
            case L'm':
                s += info[15] == L'0' ? L' ' + info.substr(16, 1) : info.substr(15, 2);
                i += 2;
                break;
            case L's':
            {
                s += info[17] == L'0' ? L' ' + info.substr(18, 1) : info.substr(17, 2);
                i += 2;
                if (format.substr(i, 2) == L".s" || format.substr(i, 2) == L",s") {
                    s += format[i];
                    size_t j = std::min(format.find_first_not_of(L's', i + 1), format.length());
                    s += info.substr(20, j - i - 1);
                    i = j;
                }
                break;
            }
            case L'z':
                s += format[i + 1];
                i += 2;
                break;
            default:
                s += L'z';
                ++i;
            }
            break;
        default:
            s += format[i];
            ++i;
        }
    }
    return s;
}


void showExampleOutput(HWND hwndDlg) {
    const std::wstring fmt = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT);
    std::chrono::system_clock::time_point tp = std::chrono::sys_days(std::chrono::year(1972) / 4 / 3)
        + std::chrono::hours(13) + std::chrono::minutes(28) + std::chrono::seconds(7) + std::chrono::milliseconds(135);
    std::wstring s = formatTimePoint(tp, fmt);
    SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_EXAMPLE, s.data());
}

void enableFromFields(HWND hwndDlg, bool counterChanged = false) {
    bool enableCount = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_ENABLE );
    bool enableDate  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_ENABLE);
    bool customCount = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM );
    bool customDate  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_CUSTOM);
    if (!enableCount && !enableDate) {
        if (counterChanged) {
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_ENABLE, BST_CHECKED);
            enableDate = true;
        }
        else {
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_ENABLE, BST_CHECKED);
            enableCount = true;
        }
    }
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_UNIX              ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_FILE              ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_1900              ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_1904              ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM    ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM1   ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM2   ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM3   ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH             ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_UNIT              ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_LEAP              ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_AMBIGUOUS), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_YMD               ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_MDY               ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_DMY               ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_CUSTOM   ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_PARSE             ), enableDate && customDate);
}

void enableToCounterFields(HWND hwndDlg) {
    bool customCount = SendDlgItemMessage(hwndDlg, IDC_TIMESTAMP_TO_COUNTER_CUSTOM, BM_GETCHECK, 0, 0) == BST_CHECKED;
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_COUNTER_CUSTOM1), customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_COUNTER_CUSTOM2), customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_COUNTER_CUSTOM3), customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_EPOCH          ), customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_UNIT           ), customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_LEAP           ), customCount);
}


INT_PTR CALLBACK timestampsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

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

        TimestampSettings& ts = data.timestamps;

        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH          , std::format(L"{0:%F} {0:%T}", timePoint(ts.fromEpoch)).data());
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_EPOCH            , std::format(L"{0:%F} {0:%T}", timePoint(ts.toEpoch  )).data());
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_UNIT           , std::format(L"{0:.7f}", static_cast<long double>(ts.fromUnit)*1e-7).data());
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_UNIT             , std::format(L"{0:.7f}", static_cast<long double>(ts.toUnit  )*1e-7).data());
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_LEAP           , ts.fromLeap           ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_TO_LEAP             , ts.toLeap             ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_ENABLE , ts.enableFromCounter  ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_ENABLE, ts.enableFromDatetime ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT      , ts.dateFormat.data());
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_PARSE          , ts.dateParse.data());
        
        int radioButton = 
              ts.fromEpoch == TimestampSettings::EpochUnix && ts.fromUnit == TimestampSettings::TUnitUnix && !ts.fromLeap ? IDC_TIMESTAMP_FROM_UNIX
            : ts.fromEpoch == TimestampSettings::EpochFile && ts.fromUnit == TimestampSettings::TUnitFile &&  ts.fromLeap ? IDC_TIMESTAMP_FROM_FILE
            : ts.fromEpoch == TimestampSettings::Epoch1900 && ts.fromUnit == TimestampSettings::TUnit1900 && !ts.fromLeap ? IDC_TIMESTAMP_FROM_1900
            : ts.fromEpoch == TimestampSettings::Epoch1904 && ts.fromUnit == TimestampSettings::TUnit1904 && !ts.fromLeap ? IDC_TIMESTAMP_FROM_1904
            : IDC_TIMESTAMP_FROM_COUNTER_CUSTOM;
        CheckRadioButton(hwndDlg, IDC_TIMESTAMP_FROM_UNIX, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM, radioButton);

        radioButton = ts.datePriority == TimestampSettings::DatePriority::ymd ? IDC_TIMESTAMP_FROM_YMD
                    : ts.datePriority == TimestampSettings::DatePriority::mdy ? IDC_TIMESTAMP_FROM_MDY
                    : ts.datePriority == TimestampSettings::DatePriority::dmy ? IDC_TIMESTAMP_FROM_DMY
                    : IDC_TIMESTAMP_FROM_DATETIME_CUSTOM;
        CheckRadioButton(hwndDlg, IDC_TIMESTAMP_FROM_YMD, IDC_TIMESTAMP_FROM_DATETIME_CUSTOM, radioButton);

        radioButton = ts.toEpoch == TimestampSettings::EpochUnix && ts.toUnit == TimestampSettings::TUnitUnix && !ts.toLeap ? IDC_TIMESTAMP_TO_UNIX
                    : ts.toEpoch == TimestampSettings::EpochFile && ts.toUnit == TimestampSettings::TUnitFile &&  ts.toLeap ? IDC_TIMESTAMP_TO_FILE
                    : ts.toEpoch == TimestampSettings::Epoch1900 && ts.toUnit == TimestampSettings::TUnit1900 && !ts.toLeap ? IDC_TIMESTAMP_TO_1900
                    : ts.toEpoch == TimestampSettings::Epoch1904 && ts.toUnit == TimestampSettings::TUnit1904 && !ts.toLeap ? IDC_TIMESTAMP_TO_1904
                    : IDC_TIMESTAMP_TO_COUNTER_CUSTOM;
        CheckRadioButton(hwndDlg, IDC_TIMESTAMP_TO_UNIX, IDC_TIMESTAMP_TO_COUNTER_CUSTOM, radioButton);

        radioButton = ts.dateFormat == L"yyyy-MM-dd'T'HH:mm:ss.sss"                                                ? IDC_TIMESTAMP_TO_DATE_STD
                    : ts.dateFormat == localeInfo(LOCALE_SSHORTDATE) + L" " + localeInfo(LOCALE_SSHORTTIME ) ? IDC_TIMESTAMP_TO_DATE_SHORT
                    : ts.dateFormat == localeInfo(LOCALE_SLONGDATE ) + L" " + localeInfo(LOCALE_STIMEFORMAT) ? IDC_TIMESTAMP_TO_DATE_LONG
                    : IDC_TIMESTAMP_TO_DATE_CUSTOM;
        CheckRadioButton(hwndDlg, IDC_TIMESTAMP_TO_DATE_STD, IDC_TIMESTAMP_TO_DATE_CUSTOM, radioButton);
        EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), radioButton == IDC_TIMESTAMP_TO_DATE_CUSTOM);

        enableFromFields(hwndDlg);
        enableToCounterFields(hwndDlg);
        showExampleOutput(hwndDlg);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            return TRUE;
        case IDC_TIMESTAMP_TO_COUNTER:
        case IDC_TIMESTAMP_TO_DATETIME:
        {
            const bool enableFromCounter  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_ENABLE );
            const bool enableFromDatetime = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_ENABLE);
            uint64_t fromEpoch = data.timestamps.fromEpoch;
            uint64_t fromUnit  = data.timestamps.fromUnit;
            uint64_t toEpoch   = data.timestamps.toEpoch;
            uint64_t toUnit    = data.timestamps.toUnit;
            if (enableFromCounter) {
                fromEpoch = utcTime(GetDlgItemString(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH));
                if (fromEpoch == std::string::npos) {
                    showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH, L"Enter a valid epoch in yyyy-mm-dd hh:mm:ss.xxxxxxx format.");
                    return TRUE;
                }
                fromUnit = utcUnit(GetDlgItemString(hwndDlg, IDC_TIMESTAMP_FROM_UNIT));
                if (fromUnit == 0) {
                    showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_UNIT, L"Enter a valid clock tick unit in seconds.");
                    return TRUE;
                }
            }
            if (LOWORD(wParam) == IDC_TIMESTAMP_TO_COUNTER) {
                toEpoch = utcTime(GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_EPOCH));
                if (toEpoch == std::string::npos) {
                    showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH, L"Enter a valid epoch in yyyy-mm-dd hh:mm:ss.xxxxxxx format.");
                    return TRUE;
                }
                toUnit = utcUnit(GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_UNIT));
                if (toUnit == 0) {
                    showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_UNIT, L"Enter a valid clock tick unit in seconds.");
                    return TRUE;
                }
            }
            data.timestamps.enableFromCounter  = enableFromCounter;
            data.timestamps.enableFromDatetime = enableFromDatetime;
            data.timestamps.fromEpoch          = fromEpoch;
            data.timestamps.fromUnit           = fromUnit;
            data.timestamps.fromLeap           = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_LEAP);
            data.timestamps.toEpoch            = toEpoch;
            data.timestamps.toUnit             = toUnit;
            data.timestamps.toLeap             = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_LEAP);
            data.timestamps.dateFormat         = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT);
            data.timestamps.dateParse          = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_FROM_PARSE);
            data.timestamps.datePriority = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_YMD) ? TimestampSettings::DatePriority::ymd
                                         : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_MDY) ? TimestampSettings::DatePriority::mdy
                                         : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_DMY) ? TimestampSettings::DatePriority::dmy
                                                                                               : TimestampSettings::DatePriority::custom;
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;
        }
        case IDC_TIMESTAMP_FROM_UNIX:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH, L"1970-01-01 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_UNIT , L"1.0000000");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_LEAP , BST_UNCHECKED);
            enableFromFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_FROM_FILE:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH, L"1601-01-01 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_UNIT , L"0.0000001");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_LEAP , BST_CHECKED);
            enableFromFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_FROM_1900:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH, L"1899-12-30 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_UNIT , L"86400.0000000");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_LEAP , BST_UNCHECKED);
            enableFromFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_FROM_1904:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH, L"1904-01-01 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_UNIT , L"86400.0000000");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_LEAP , BST_UNCHECKED);
            enableFromFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_FROM_COUNTER_ENABLE:
        case IDC_TIMESTAMP_FROM_COUNTER_CUSTOM:
            enableFromFields(hwndDlg, true);
            break;
        case IDC_TIMESTAMP_TO_UNIX:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_EPOCH, L"1970-01-01 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_UNIT , L"1.0000000");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_TO_LEAP , BST_UNCHECKED);
            enableToCounterFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_FILE:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_EPOCH, L"1601-01-01 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_UNIT , L"0.0000001");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_TO_LEAP , BST_CHECKED);
            enableToCounterFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_1900:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_EPOCH, L"1899-12-30 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_UNIT , L"86400.0000000");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_TO_LEAP , BST_UNCHECKED);
            enableToCounterFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_1904:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_EPOCH, L"1904-01-01 00:00:00.0000000");
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_UNIT , L"86400.0000000");
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP_TO_LEAP , BST_UNCHECKED);
            enableToCounterFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_COUNTER_CUSTOM:
            enableToCounterFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_FROM_YMD:
        case IDC_TIMESTAMP_FROM_MDY:
        case IDC_TIMESTAMP_FROM_DMY:
        case IDC_TIMESTAMP_FROM_DATETIME_CUSTOM:
        case IDC_TIMESTAMP_FROM_DATETIME_ENABLE:
            enableFromFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_DATE_STD:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT, L"yyyy-MM-dd'T'HH:mm:ss.sss");
            EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), FALSE);
            showExampleOutput(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_DATE_SHORT:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT, (localeInfo(LOCALE_SSHORTDATE) + L" " + localeInfo(LOCALE_SSHORTTIME)).data());
            EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), FALSE);
            showExampleOutput(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_DATE_LONG:
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT, (localeInfo(LOCALE_SLONGDATE) + L" " + localeInfo(LOCALE_STIMEFORMAT)).data());
            EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), FALSE);
            showExampleOutput(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_DATE_CUSTOM:
            EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), TRUE);
            break;
        case IDC_TIMESTAMP_TO_DATE_FORMAT:
            showExampleOutput(hwndDlg);
            break;
        }
        break;
    }
    return FALSE;

}


struct ParsingInformation {
    ColumnsPlusPlusData&     data;
    const std::wstring       locale;
    const intptr_t           action;
    wchar_t*                 names      = 0;
    size_t                   nameLength = 0;
    unsigned int             codepage   = 0;
    ParsingInformation(ColumnsPlusPlusData& data, const intptr_t action, const std::wstring locale = L"");
    ~ParsingInformation() { if (names) delete[] names; }
    bool parseGenericDateText(const std::string_view source, int64_t& counter) const;
    bool parsePatternDateText(const std::string_view source, int64_t& counter) const;
};


ParsingInformation::ParsingInformation(ColumnsPlusPlusData& data, const intptr_t action, const std::wstring locale)
    : data(data), action(action), locale(locale) {
    const std::wstring nameList[38] = {
        localeInfo(LOCALE_SABBREVMONTHNAME1 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME2 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME3 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME4 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME5 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME6 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME7 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME8 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME9 , locale),
        localeInfo(LOCALE_SABBREVMONTHNAME10, locale),
        localeInfo(LOCALE_SABBREVMONTHNAME11, locale),
        localeInfo(LOCALE_SABBREVMONTHNAME12, locale),
        localeInfo(LOCALE_SMONTHNAME1       , locale),
        localeInfo(LOCALE_SMONTHNAME2       , locale),
        localeInfo(LOCALE_SMONTHNAME3       , locale),
        localeInfo(LOCALE_SMONTHNAME4       , locale),
        localeInfo(LOCALE_SMONTHNAME5       , locale),
        localeInfo(LOCALE_SMONTHNAME6       , locale),
        localeInfo(LOCALE_SMONTHNAME7       , locale),
        localeInfo(LOCALE_SMONTHNAME8       , locale),
        localeInfo(LOCALE_SMONTHNAME9       , locale),
        localeInfo(LOCALE_SMONTHNAME10      , locale),
        localeInfo(LOCALE_SMONTHNAME11      , locale),
        localeInfo(LOCALE_SMONTHNAME12      , locale),
        localeGenetiveMonth(1 , locale),
        localeGenetiveMonth(2 , locale),
        localeGenetiveMonth(3 , locale),
        localeGenetiveMonth(4 , locale),
        localeGenetiveMonth(5 , locale),
        localeGenetiveMonth(6 , locale),
        localeGenetiveMonth(7 , locale),
        localeGenetiveMonth(8 , locale),
        localeGenetiveMonth(9 , locale),
        localeGenetiveMonth(10, locale),
        localeGenetiveMonth(11, locale),
        localeGenetiveMonth(12, locale),
        localeInfo(LOCALE_SAM, locale),
        localeInfo(LOCALE_SPM, locale)
    };
    size_t maxlen = 0;
    for (int i = 0; i < 38; ++i) if (nameList[i].length() > maxlen) maxlen = nameList[i].length();
    nameLength = maxlen + 1;
    names = new wchar_t[38 * nameLength];
    for (int i = 0; i < 38; ++i) wcscpy(names + i * nameLength, nameList[i].data());
    codepage = data.sci.CodePage();
}


bool ParsingInformation::parseGenericDateText(const std::string_view source, int64_t& counter) const {

    const TimestampSettings& ts = data.timestamps;
    const std::wstring s = toWide(source, codepage);

    if (!ts.enableFromDatetime || ts.datePriority == TimestampSettings::DatePriority::custom) return false;

    std::vector<std::wstring> aToken;
    std::vector<std::wstring> nToken;

    for (size_t i = 0; i < s.length();) {
        if (iswdigit(s[i])) {
            size_t j = i + 1;
            while (j < s.length() && iswdigit(s[j])) ++j;
            nToken.push_back(s.substr(i, j - i));
            i = j;
        }
        else if (iswalpha(s[i])) {
            size_t j = i + 1;
            while (j < s.length() && iswalpha(s[j])) ++j;
            aToken.push_back(s.substr(i, j - i));
            i = j;
        }
        else ++i;
    }

    if (nToken.size() < 2 || nToken.size() > 7 || aToken.size() + nToken.size() < 3) return false;

    int aMonth = -1;
    int ampm = -1;

    for (size_t i = 0; i < aToken.size(); ++i) {
        if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                          aToken[i].data(), static_cast<int>(aToken[i].length()), 
                                          names + 36 * nameLength, aToken[i].length() == 1 ? 1 : -1, 0, 0, 0))
            ampm = ampm == -1 ? 0 : -2;
        else if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                          aToken[i].data(), static_cast<int>(aToken[i].length()),
                                          names + 37 * nameLength, aToken[i].length() == 1 ? 1 : -1, 0, 0, 0))
            ampm = ampm == -1 ? 12 : -2;
        int j = 0;
        while (j < 36 && CSTR_EQUAL != CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                       aToken[i].data(), static_cast<int>(aToken[i].length()),
                                                       names + j * nameLength, -1, 0, 0, 0))
            ++j;
        if (j < 36) aMonth = aMonth == -1 ? (j % 12) + 1 : -2;
    }

    if (aMonth == -2 || ampm == -2) return false;
    if (aMonth > 0 ? (nToken.size() < 2 || nToken.size() > 6) : (nToken.size() < 3 && nToken.size() > 7) ) return false;

    int    nYear    = 0;
    int    nMonth   = 0;
    int    nDay     = 0;
    size_t tToken   = 3;

    if (aMonth > 0) {
        tToken = 2;
        nMonth = aMonth;
        if      (nToken[0].length() > 3 && nToken[1].length() < 3       ) { nYear = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        else if (nToken[0].length() < 3 && nToken[1].length() > 3       ) { nYear = stoi(nToken[1]); nDay = stoi(nToken[0]); }
        else if (ts.datePriority == TimestampSettings::DatePriority::ymd) { nYear = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        else                                                              { nYear = stoi(nToken[1]); nDay = stoi(nToken[0]); }
    }
    else {
        if (nToken[0].length() > 2 && nToken[1].length() < 3 && nToken[2].length() < 3) {
            nYear  = stoi(nToken[0]);
            nMonth = stoi(nToken[1]);
            nDay   = stoi(nToken[2]);
        }
        else if (nToken[0].length() < 3 && nToken[1].length() > 2 && nToken[2].length() < 3) {
            nYear  = stoi(nToken[1]);
            if (ts.datePriority == TimestampSettings::DatePriority::dmy) { nMonth = stoi(nToken[2]); nDay = stoi(nToken[0]); }
            else                                                         { nMonth = stoi(nToken[0]); nDay = stoi(nToken[2]); }
        }
        else if (nToken[0].length() < 3 && nToken[1].length() < 3 && nToken[2].length() > 2) {
            nYear  = stoi(nToken[2]);
            if (ts.datePriority == TimestampSettings::DatePriority::dmy) { nMonth = stoi(nToken[1]); nDay = stoi(nToken[0]); }
            else                                                         { nMonth = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        }
        else if (ts.datePriority == TimestampSettings::DatePriority::ymd) { nYear = stoi(nToken[0]); nMonth = stoi(nToken[1]); nDay = stoi(nToken[2]); }
        else if (ts.datePriority == TimestampSettings::DatePriority::mdy) { nYear = stoi(nToken[2]); nMonth = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        else                                                              { nYear = stoi(nToken[2]); nMonth = stoi(nToken[1]); nDay = stoi(nToken[0]); }
    }
    std::chrono::year_month_day ymd = std::chrono::sys_days(std::chrono::year(nYear) / nMonth / nDay);
    if (!ymd.ok()) return false;

    int64_t ticks = 0;
    if (tToken < nToken.size()) {
        ticks = 36000000000 * (ampm >= 0 ? stoi(nToken[tToken]) % 12 + ampm : stoi(nToken[tToken]));
        if (tToken + 1 < nToken.size()) {
            ticks += 600000000i64 * stoi(nToken[tToken + 1]);
            if (tToken + 2 < nToken.size()) {
                ticks += 10000000i64 * stoi(nToken[tToken + 2]);
                if (tToken + 3 < nToken.size()) ticks += stoi((nToken[tToken + 3] + L"000000").substr(0, 7));
            }
        }
    }

    if (action == IDC_TIMESTAMP_TO_DATETIME || ts.toLeap) {
        std::chrono::utc_clock::time_point tp = std::chrono::clock_cast<std::chrono::utc_clock>(std::chrono::sys_days(ymd));
        counter = tp.time_since_epoch().count() + ticks;
    }
    else {
        std::chrono::system_clock::time_point tp = std::chrono::sys_days(ymd);
        counter = tp.time_since_epoch().count() + ticks;
    }

    return true;

}


bool ParsingInformation::parsePatternDateText(const std::string_view source, int64_t& counter) const {

    RegularExpression rx(data);
    rx.find(data.timestamps.dateParse);
    if (!rx.can_search()) return false;
    if (!rx.search(source)) return false;
    std::string year    = rx.str("Y");
    std::string doy     = rx.str("D");
    std::string month   = rx.str("M");
    std::string dom     = rx.str("d");
    std::string hour12  = rx.str("h");
    std::string hour24  = rx.str("H");
    std::string minute  = rx.str("m");
    std::string seconds = rx.str("s");
    std::string ampm    = rx.str("t");
    if (year.empty()) return false;
    if (doy.empty() && (month.empty() || dom.empty())) return false;
    if (!doy.empty() && !(month.empty() && dom.empty())) return false;
    if (!hour24.empty() && !(hour12.empty() && ampm.empty())) return false;
    try {

        std::chrono::sys_days date;
        int nYear = stoi(year);
        if (year.length() == 2) nYear += nYear < 50 ? 2000 : 1900;
        if (doy.empty()) {
            int nMonth = 0;
            if (month.find_first_not_of("0123456789 ") == std::string::npos) nMonth = stoi(month);
            else {
                std::wstring wMonth = toWide(month, codepage);
                while (nMonth < 36 && CSTR_EQUAL != CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                    wMonth.data(), static_cast<int>(wMonth.length()),
                    names + nMonth * nameLength, -1, 0, 0, 0))
                    ++nMonth;
                if (nMonth < 36) nMonth = (nMonth % 12) + 1;
            }
            if (nMonth < 1 || nMonth > 12) return false;
            int nDom = stoi(dom);
            if (nDom < 1 || nDom > 31) return false;
            std::chrono::year_month_day ymd = std::chrono::year(nYear) / nMonth / nDom;
            if (!ymd.ok()) return false;
            date = std::chrono::sys_days(ymd);
        }
        else date = std::chrono::sys_days(std::chrono::year(nYear) / 1 / 1) + std::chrono::days(stoi(doy) - 1);

        int64_t ticks = hour24.empty() ? 0 : 36000000000i64 * stoi(hour24);
        if (!hour12.empty()) ticks += 36000000000i64 * (stoi(hour12) % 12);
        if (!ampm.empty()) {
            std::wstring wampm = toWide(ampm, codepage);
            if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              wampm.data(), static_cast<int>(wampm.length()),
                                              names + 37 * nameLength, wampm.length() == 1 ? 1 : -1, 0, 0, 0))
                ticks += 432000000000i64;
            else if (CSTR_EQUAL != CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              wampm.data(), static_cast<int>(wampm.length()),
                                              names + 36 * nameLength, wampm.length() == 1 ? 1 : -1, 0, 0, 0))
                return false;
        }
        if (!minute.empty()) ticks += 600000000i64 * stoi(minute);
        if (!seconds.empty()) {
            size_t p = seconds.find_first_not_of("0123456789 ");
            if (p == std::string::npos || (seconds[p] != '.' && seconds[p] != ',')) ticks += 10000000i64 * stoi(seconds);
            else {
                ticks += 10000000i64 * stoi(seconds.substr(0, p - 1));
                try {
                     ticks += stoi((seconds.substr(p + 1) + "0000000").substr(0, 7));
                }
                catch (...) {}
            }
        }

        if (action == IDC_TIMESTAMP_TO_DATETIME || data.timestamps.toLeap) {
            std::chrono::utc_clock::time_point tp = std::chrono::clock_cast<std::chrono::utc_clock>(date);
            counter = tp.time_since_epoch().count() + ticks;
        }
        else {
            std::chrono::system_clock::time_point tp = date;
            counter = tp.time_since_epoch().count() + ticks;
        }

        return true;

    }
    catch (...) {
        return false;
    }

}


}

void ColumnsPlusPlusData::convertTimestamps() {

    auto rs = getRectangularSelection();
    if (!rs.size()) return;

    auto action = DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_TIMESTAMP), nppData._nppHandle, ::timestampsDialogProc, reinterpret_cast<LPARAM>(this));
    if (!action) return;

    int64_t fromEpoch = !timestamps.enableFromCounter || timestamps.fromLeap ? timestamps.fromEpoch
        : std::chrono::clock_cast<std::chrono::system_clock>(
            std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(timestamps.fromEpoch))
        ).time_since_epoch().count();

    int64_t toEpoch = action == IDC_TIMESTAMP_TO_DATETIME || timestamps.toLeap ? timestamps.toEpoch
        : std::chrono::clock_cast<std::chrono::system_clock>(
            std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(timestamps.toEpoch))
        ).time_since_epoch().count();

    ParsingInformation pi(*this, action);
    unsigned int codepage = sci.CodePage();


//    struct Column {
//        int width = 0;
//    };
//    std::vector<Column> column;

    sci.BeginUndoAction();

    for (auto row : rs) {
        std::string r;
//        size_t columnNumber = 0;
        for (const auto& cell : row) {

//            if (columnNumber >= column.size()) column.emplace_back();
//            Column& col = column[columnNumber];
//            ++columnNumber;
            if (!cell.trimLength()) {
                r += cell.text() + cell.terminator();
                continue;
            }
            std::string source = cell.trim();
            int64_t counter = 0;

            bool sourceIsCounter  = false;
            if (timestamps.enableFromCounter && source.find_first_not_of("0123456789., '") == std::string::npos) {
                auto [value, dp, ts] = parseNumber(source);
                if (isfinite(value)) {
                    counter = static_cast<int64_t>(value * timestamps.fromUnit) + fromEpoch;
                    sourceIsCounter = true;
                }
            }

            if (!sourceIsCounter) {
                if ( !timestamps.enableFromDatetime ||
                     (timestamps.datePriority == TimestampSettings::DatePriority::custom ? !pi.parsePatternDateText(source, counter)
                                                                                         : !pi.parseGenericDateText(source, counter)) ) {
                    r += cell.text() + cell.terminator();
                    continue;
                }
            }

            if (action == IDC_TIMESTAMP_TO_DATETIME) {
                std::wstring s = !sourceIsCounter || timestamps.fromLeap
                    ? formatTimePoint(std::chrono::utc_clock   ::time_point(std::chrono::utc_clock   ::duration(counter)), timestamps.dateFormat)
                    : formatTimePoint(std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter)), timestamps.dateFormat);
                r += fromWide(s, codepage) + cell.terminator();
            }
            else {
                if (sourceIsCounter && timestamps.fromLeap != timestamps.toLeap) {
                    counter = timestamps.toLeap
                        ? std::chrono::clock_cast<std::chrono::utc_clock>(
                             std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter))
                             ).time_since_epoch().count()
                        : std::chrono::clock_cast<std::chrono::system_clock>(
                             std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(counter))
                             ).time_since_epoch().count();
                }
                std::string s;
                if (ratioToDecimal(counter - toEpoch, timestamps.toUnit, s)) r += s + cell.terminator();
                                                                        else r += cell.text() + cell.terminator();
            }
        }
        if (r != row.text()) row.replace(r);
    }

    rs.refit();
    sci.EndUndoAction();

}