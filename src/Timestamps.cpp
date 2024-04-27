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
#include "resource.h"
#include "commctrl.h"


// int64_t toFileTime(const std::wstring& textTime) {
//     std::wistringstream stream(textTime);
//     std::chrono::file_clock::time_point fileTime;
//     std::chrono::from_stream(stream, L"%F%n %T ", fileTime);
//     if (stream.fail()) {
//         stream.clear();
//         stream.seekg(0);
//         std::chrono::from_stream(stream, L" %F ", fileTime);
//         if (stream.fail()) return std::string::npos;
//     }
//     if (!stream.eof()) return std::string::npos;
//     return fileTime.time_since_epoch().count();
// }

// int64_t toFileTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int milliseconds = 0) {
//     std::chrono::system_clock::time_point tpt = std::chrono::sys_days(std::chrono::year(year) / month / day)
//         + std::chrono::hours(hour) + std::chrono::minutes(minute) + std::chrono::seconds(second) + std::chrono::milliseconds(milliseconds);
//     return std::chrono::clock_cast<std::chrono::file_clock>(tpt).time_since_epoch().count();
// }

namespace {

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
                : j - i == 3 ? std::format(L"{0:%b}", timePoint) : std::format(L"{0:%B}", timePoint);
            i = j;
            break;
        }
        case L'd':
        {
            size_t j = std::min(format.find_first_not_of(L'd', i), format.length());
            s += j - i < 2 && info[6] == L'0' ? info.substr(7, 1) : j - i < 3 ? info.substr(6, 2)
                : j - i == 3 ? std::format(L"{0:%a}", timePoint) : std::format(L"{0:%A}", timePoint);
            i = j;
            break;
        }
        case L'j':
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
        {
            size_t j = std::min(format.find_first_of(L'\'', i + 1), format.length());
            s += format.substr(i + 1, j - i - 1);
            i = j + 1;
            break;
        }
        case L'\\':
            if (i + 1 < format.length()) s += format[i + 1];
            i += 2;
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
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_UNIX            ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_FILE            ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_1900            ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_1904            ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM  ), enableCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM1 ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM2 ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM3 ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH           ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_UNIT            ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_LEAP            ), enableCount && customCount);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_YMD             ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_MDY             ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_DMY             ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_CUSTOM ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_CUSTOM1), enableDate && customDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_PARSE           ), enableDate && customDate);
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

        radioButton = ts.dateFormat == L"yyyy-MM-dd HH:mm:ss"                                                ? IDC_TIMESTAMP_TO_DATE_STD
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
            SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT, L"yyyy-MM-dd HH:mm:ss");
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

    unsigned int codepage = sci.CodePage();

    const std::wstring monthNames[36] = { localeInfo(LOCALE_SABBREVMONTHNAME1),
                                          localeInfo(LOCALE_SABBREVMONTHNAME2),
                                          localeInfo(LOCALE_SABBREVMONTHNAME3),
                                          localeInfo(LOCALE_SABBREVMONTHNAME4),
                                          localeInfo(LOCALE_SABBREVMONTHNAME5),
                                          localeInfo(LOCALE_SABBREVMONTHNAME6),
                                          localeInfo(LOCALE_SABBREVMONTHNAME7),
                                          localeInfo(LOCALE_SABBREVMONTHNAME8),
                                          localeInfo(LOCALE_SABBREVMONTHNAME9),
                                          localeInfo(LOCALE_SABBREVMONTHNAME10),
                                          localeInfo(LOCALE_SABBREVMONTHNAME11),
                                          localeInfo(LOCALE_SABBREVMONTHNAME12),
                                          localeInfo(LOCALE_SMONTHNAME1),
                                          localeInfo(LOCALE_SMONTHNAME2),
                                          localeInfo(LOCALE_SMONTHNAME3),
                                          localeInfo(LOCALE_SMONTHNAME4),
                                          localeInfo(LOCALE_SMONTHNAME5),
                                          localeInfo(LOCALE_SMONTHNAME6),
                                          localeInfo(LOCALE_SMONTHNAME7),
                                          localeInfo(LOCALE_SMONTHNAME8),
                                          localeInfo(LOCALE_SMONTHNAME9),
                                          localeInfo(LOCALE_SMONTHNAME10),
                                          localeInfo(LOCALE_SMONTHNAME11),
                                          localeInfo(LOCALE_SMONTHNAME12),
                                          localeGenetiveMonth(1),
                                          localeGenetiveMonth(2),
                                          localeGenetiveMonth(3),
                                          localeGenetiveMonth(4),
                                          localeGenetiveMonth(5),
                                          localeGenetiveMonth(6),
                                          localeGenetiveMonth(7),
                                          localeGenetiveMonth(8),
                                          localeGenetiveMonth(9),
                                          localeGenetiveMonth(10),
                                          localeGenetiveMonth(11),
                                          localeGenetiveMonth(12) };

    const std::wstring ampmNames[2] = { localeInfo(LOCALE_SAM), localeInfo(LOCALE_SPM) };

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

            bool sourceIsDateTime = !sourceIsCounter && timestamps.enableFromDatetime;
            std::vector<std::wstring> aToken;
            std::vector<std::wstring> nToken;

            if (sourceIsDateTime) {
                std::wstring wideSource = toWide(source, codepage);
                for (size_t i = 0; i < wideSource.length();) {
                    if (iswdigit(wideSource[i])) {
                        size_t j = i + 1;
                        while (j < wideSource.length() && iswdigit(wideSource[j])) ++j;
                        nToken.push_back(wideSource.substr(i, j - i));
                        i = j;
                    }
                    else if (iswalpha(wideSource[i])) {
                        size_t j = i + 1;
                        while (j < wideSource.length() && iswalpha(wideSource[j])) ++j;
                        aToken.push_back(wideSource.substr(i, j - i));
                        i = j;
                    }
                    else ++i;
                }
            }

            sourceIsDateTime &= nToken.size() > 1 && nToken.size() < 8 || aToken.size() + nToken.size() > 2;
            int aMonth = -1;
            int ampm   = -1;

            if (sourceIsDateTime) {
                for (size_t i = 0; i < aToken.size(); ++i) {
                    if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                      aToken[i].data(), static_cast<int>(aToken[i].length()), ampmNames[0].data(),
                                                      aToken[i].length() == 1 ? 1 : static_cast<int>(ampmNames[0].length()), 0, 0, 0)) {
                        ampm = ampm == -1 ? 0 : -2;
                    }
                    if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                      aToken[i].data(), static_cast<int>(aToken[i].length()), ampmNames[1].data(),
                                                      aToken[i].length() == 1 ? 1 : static_cast<int>(ampmNames[1].length()), 0, 0, 0)) {
                        ampm = ampm == -1 ? 12 : -2;
                    }
                    int j = 0;
                    while (j < 36 && CSTR_EQUAL != CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                                   aToken[i].data(), static_cast<int>(aToken[i].length()),
                                                                   monthNames[j].data(), static_cast<int>(monthNames[j].length()), 0, 0, 0)) ++j;
                    if (j < 36) aMonth = aMonth == -1 ? (j % 12) + 1 : -2;
                }
            }

            sourceIsDateTime &= aMonth > -2 && ampm > -2;
            sourceIsDateTime &= (aMonth > 0 ? (nToken.size() > 1 && nToken.size() < 7) : (nToken.size() > 2 && nToken.size() < 8));

            if (sourceIsDateTime) {
                std::wstring dt;
                size_t timeToken = 3;
                if (aMonth > 0) {
                    timeToken = 2;
                    if      (nToken[0].length() > 3 && nToken[1].length() < 3) dt = nToken[0] + L'-' + std::to_wstring(aMonth) + L'-' + nToken[1];
                    else if (nToken[0].length() < 3 && nToken[1].length() > 3) dt = nToken[1] + L'-' + std::to_wstring(aMonth) + L'-' + nToken[0];
                    else if (timestamps.datePriority == TimestampSettings::DatePriority::ymd) dt = nToken[0] + L'-' + std::to_wstring(aMonth) + L'-' + nToken[1];
                    else                                                                      dt = nToken[1] + L'-' + std::to_wstring(aMonth) + L'-' + nToken[0];
                }
                else if (nToken[0].length() > 3 && nToken[1].length() < 3 && nToken[2].length() < 3) {
                    dt = nToken[0] + L'-' + nToken[1] + L'-' + nToken[2];
                }
                else if (nToken[0].length() < 3 && nToken[1].length() > 3 && nToken[2].length() < 3) {
                    dt = nToken[1] + L'-' + nToken[0] + L'-' + nToken[2];
                }
                else if (nToken[0].length() < 3 && nToken[1].length() < 3 && nToken[2].length() > 3) {
                    if (timestamps.datePriority == TimestampSettings::DatePriority::dmy) dt = nToken[2] + L'-' + nToken[1] + L'-' + nToken[0];
                    else                                                                 dt = nToken[2] + L'-' + nToken[0] + L'-' + nToken[1];
                }
                else {
                    if      (timestamps.datePriority == TimestampSettings::DatePriority::dmy) dt = nToken[2] + L'-' + nToken[1] + L'-' + nToken[0];
                    else if (timestamps.datePriority == TimestampSettings::DatePriority::mdy) dt = nToken[2] + L'-' + nToken[0] + L'-' + nToken[1];
                    else                                                                      dt = nToken[0] + L'-' + nToken[1] + L'-' + nToken[2];
                }
                if (nToken.size() > timeToken) {
                    dt += L' ' + (ampm >= 0 ? std::to_wstring(stoi(nToken[timeToken]) % 12 + ampm) : nToken[timeToken]);
                    dt += nToken.size() > timeToken + 1 ? L':' + nToken[timeToken + 1] : L":00";
                    dt += nToken.size() > timeToken + 2 ? L':' + nToken[timeToken + 2] : L":00";
                    if (nToken.size() > timeToken + 3) dt += L'.' + nToken[timeToken + 3];
                }
                else dt += L" 00:00:00";
                std::wistringstream stream(dt);
                if (action == IDC_TIMESTAMP_TO_DATETIME || timestamps.toLeap) {
                    std::chrono::utc_clock::time_point tp;
                    std::chrono::from_stream(stream, L"%F%n%T", tp);
                    if (stream.fail()) sourceIsDateTime = false;
                    else counter = tp.time_since_epoch().count();
                }
                else {
                    std::chrono::system_clock::time_point tp;
                    std::chrono::from_stream(stream, L"%F%n%T", tp);
                    if (stream.fail()) sourceIsDateTime = false;
                    else counter = tp.time_since_epoch().count();
                }
            }

            if (!sourceIsCounter && !sourceIsDateTime) {
                r += cell.text() + cell.terminator();
                continue;
            }

            std::wstring s;
            if (action == IDC_TIMESTAMP_TO_DATETIME) {
                s = !sourceIsCounter || timestamps.fromLeap
                    ? formatTimePoint(std::chrono::utc_clock   ::time_point(std::chrono::utc_clock   ::duration(counter)), timestamps.dateFormat)
                    : formatTimePoint(std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter)), timestamps.dateFormat);
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
                s = timestamps.toUnit <=            1 ? std::format(L"{0}", counter - toEpoch)
                  : timestamps.toUnit <=           10 ? std::format(L"{0:.1f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=          100 ? std::format(L"{0:.2f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=         1000 ? std::format(L"{0:.3f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=        10000 ? std::format(L"{0:.4f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=       100000 ? std::format(L"{0:.5f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=      1000000 ? std::format(L"{0:.6f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=     10000000 ? std::format(L"{0:.7f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=    100000000 ? std::format(L"{0:.8f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=   1000000000 ? std::format(L"{0:.9f}" , static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <=  10000000000 ? std::format(L"{0:.10f}", static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                  : timestamps.toUnit <= 100000000000 ? std::format(L"{0:.11f}", static_cast<double>(counter - toEpoch) / timestamps.toUnit)
                                                      : std::format(L"{0:.12f}", static_cast<double>(counter - toEpoch) / timestamps.toUnit);
            }
            r += fromWide(s, codepage) + cell.terminator();
        }
        if (r != row.text()) row.replace(r);
    }

    rs.refit();
    sci.EndUndoAction();

}