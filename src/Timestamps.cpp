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

// manipulating 64-bit integers

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
    int64_t remainder = std::abs(numerator % denominator);
    if (remainder == 0) {
        integer = quotient;
        decimal = 0;
        places  = 0;
        return true;
    }

    double divisor  = static_cast<double>(std::abs(denominator));
    double fraction = static_cast<double>(remainder) / divisor;
    double power    = 1;
    size_t exponent = 0;
    for (; exponent < 15 && remainder != static_cast<int64_t>(std::round(std::round(fraction * power) * divisor / power)); ++exponent, power *= 10);
    integer = quotient;
    decimal = static_cast<int64_t>(std::round(fraction * power));
    places = exponent;
    return true;
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

bool stringToCounter(const std::string& source, int64_t& counter, int64_t unit, char dsep) {
    try {
        std::string integer, decimal;
        bool negative = false;
        size_t pos = source.find_first_not_of(' ');
        if (pos == std::string::npos) return false;
        if (source[pos] == '-') {
            negative = true;
            ++pos;
        }
        else if (source[pos] == '+') ++pos;
        for (; pos < source.length(); ++pos) {
            const char ch = source[pos];
            if (ch == dsep) break;
            if (ch >= '0' && ch <= '9') {
                integer += ch;
                continue;
            }
            if (ch == ' ' || ch == '.' || ch == ',' || ch == '\'') continue;
            return false;
        }
        if (pos >= source.length()) counter = std::stoll(integer) * unit;
        else {
            ++pos;
            double power = 1;
            for (; pos < source.length(); ++pos) {
                const char ch = source[pos];
                if (ch == dsep) return false;
                if (ch >= '0' && ch <= '9') {
                    decimal += ch;
                    power *= 10;
                    continue;
                }
                if (ch == ' ' || ch == '.' || ch == ',' || ch == '\'') continue;
                return false;
            }
            counter = std::stoll(integer) * unit + static_cast<int64_t>(std::round(std::stod(decimal) * static_cast<double>(unit) / power));
        }
        if (negative) counter = -counter;
        return true;
    }
    catch (...) { return false; }
}


// convenient utc_clock::time_point functions

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


// Windows locale information

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

struct LocaleWords {
    std::vector<std::wstring> abbrMonth;
    std::vector<std::wstring> fullMonth;
    std::vector<std::wstring> geniMonth;
    std::vector<std::wstring> dayAbbrev;
    std::vector<std::wstring> dayOfWeek;
    std::vector<std::wstring> ampm;
    size_t fullMax = 0;
    size_t geniMax = 0;
    size_t weekMax = 0;
    LocaleWords(const std::wstring locale = L"") {
        abbrMonth.resize(12);
        fullMonth.resize(12);
        geniMonth.resize(12);
        dayAbbrev.resize( 7);
        dayOfWeek.resize( 7);
        ampm     .resize( 2);
        abbrMonth[ 0] = localeInfo(LOCALE_SABBREVMONTHNAME1 , locale);
        abbrMonth[ 1] = localeInfo(LOCALE_SABBREVMONTHNAME2 , locale);
        abbrMonth[ 2] = localeInfo(LOCALE_SABBREVMONTHNAME3 , locale);
        abbrMonth[ 3] = localeInfo(LOCALE_SABBREVMONTHNAME4 , locale);
        abbrMonth[ 4] = localeInfo(LOCALE_SABBREVMONTHNAME5 , locale);
        abbrMonth[ 5] = localeInfo(LOCALE_SABBREVMONTHNAME6 , locale);
        abbrMonth[ 6] = localeInfo(LOCALE_SABBREVMONTHNAME7 , locale);
        abbrMonth[ 7] = localeInfo(LOCALE_SABBREVMONTHNAME8 , locale);
        abbrMonth[ 8] = localeInfo(LOCALE_SABBREVMONTHNAME9 , locale);
        abbrMonth[ 9] = localeInfo(LOCALE_SABBREVMONTHNAME10, locale);
        abbrMonth[10] = localeInfo(LOCALE_SABBREVMONTHNAME11, locale);
        abbrMonth[11] = localeInfo(LOCALE_SABBREVMONTHNAME12, locale);
        fullMonth[ 0] = localeInfo(LOCALE_SMONTHNAME1 , locale);
        fullMonth[ 1] = localeInfo(LOCALE_SMONTHNAME2 , locale);
        fullMonth[ 2] = localeInfo(LOCALE_SMONTHNAME3 , locale);
        fullMonth[ 3] = localeInfo(LOCALE_SMONTHNAME4 , locale);
        fullMonth[ 4] = localeInfo(LOCALE_SMONTHNAME5 , locale);
        fullMonth[ 5] = localeInfo(LOCALE_SMONTHNAME6 , locale);
        fullMonth[ 6] = localeInfo(LOCALE_SMONTHNAME7 , locale);
        fullMonth[ 7] = localeInfo(LOCALE_SMONTHNAME8 , locale);
        fullMonth[ 8] = localeInfo(LOCALE_SMONTHNAME9 , locale);
        fullMonth[ 9] = localeInfo(LOCALE_SMONTHNAME10, locale);
        fullMonth[10] = localeInfo(LOCALE_SMONTHNAME11, locale);
        fullMonth[11] = localeInfo(LOCALE_SMONTHNAME12, locale);
        geniMonth[ 0] = localeGenetiveMonth( 1, locale);
        geniMonth[ 1] = localeGenetiveMonth( 2, locale);
        geniMonth[ 2] = localeGenetiveMonth( 3, locale);
        geniMonth[ 3] = localeGenetiveMonth( 4, locale);
        geniMonth[ 4] = localeGenetiveMonth( 5, locale);
        geniMonth[ 5] = localeGenetiveMonth( 6, locale);
        geniMonth[ 6] = localeGenetiveMonth( 7, locale);
        geniMonth[ 7] = localeGenetiveMonth( 8, locale);
        geniMonth[ 8] = localeGenetiveMonth( 9, locale);
        geniMonth[ 9] = localeGenetiveMonth(10, locale);
        geniMonth[10] = localeGenetiveMonth(11, locale);
        geniMonth[11] = localeGenetiveMonth(12, locale);
        dayAbbrev[ 0] = localeInfo(LOCALE_SABBREVDAYNAME7, locale);
        dayAbbrev[ 1] = localeInfo(LOCALE_SABBREVDAYNAME1, locale);
        dayAbbrev[ 2] = localeInfo(LOCALE_SABBREVDAYNAME2, locale);
        dayAbbrev[ 3] = localeInfo(LOCALE_SABBREVDAYNAME3, locale);
        dayAbbrev[ 4] = localeInfo(LOCALE_SABBREVDAYNAME4, locale);
        dayAbbrev[ 5] = localeInfo(LOCALE_SABBREVDAYNAME5, locale);
        dayAbbrev[ 6] = localeInfo(LOCALE_SABBREVDAYNAME6, locale);
        dayOfWeek[ 0] = localeInfo(LOCALE_SDAYNAME7, locale);
        dayOfWeek[ 1] = localeInfo(LOCALE_SDAYNAME1, locale);
        dayOfWeek[ 2] = localeInfo(LOCALE_SDAYNAME2, locale);
        dayOfWeek[ 3] = localeInfo(LOCALE_SDAYNAME3, locale);
        dayOfWeek[ 4] = localeInfo(LOCALE_SDAYNAME4, locale);
        dayOfWeek[ 5] = localeInfo(LOCALE_SDAYNAME5, locale);
        dayOfWeek[ 6] = localeInfo(LOCALE_SDAYNAME6, locale);
        ampm[0] = localeInfo(LOCALE_SAM, locale);
        ampm[1] = localeInfo(LOCALE_SPM, locale);
        for (const std::wstring& s : fullMonth) if (s.length() > fullMax) fullMax = s.length();
        for (const std::wstring& s : geniMonth) if (s.length() > geniMax) geniMax = s.length();
        for (const std::wstring& s : dayOfWeek) if (s.length() > weekMax) weekMax = s.length();
    }
};

template<class Clock> std::wstring getWindowsDateTimeFormat(std::chrono::time_point<Clock> timePoint, bool longForm, std::wstring locale = L"") {
    auto tp_seconds = std::chrono::round<std::chrono::seconds>(timePoint);
    auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(tp_seconds);
    std::chrono::year_month_day ymd = std::chrono::floor<std::chrono::days>(sctp);
    int secs = static_cast<int>(sctp.time_since_epoch().count() % 86400);
    SYSTEMTIME st;
    st.wYear         = static_cast<WORD>(int     (ymd.year ()));
    st.wMonth        = static_cast<WORD>(unsigned(ymd.month()));
    st.wDay          = static_cast<WORD>(unsigned(ymd.day  ()));
    st.wDayOfWeek    = static_cast<WORD>(unsigned(std::chrono::weekday(ymd).c_encoding()));
    st.wHour         = static_cast<WORD>(secs / 3600);
    st.wMinute       = static_cast<WORD>((secs % 3600) / 60);
    st.wSecond       = static_cast<WORD>(secs % 60);
    st.wMilliseconds = 0;
    int n = GetDateFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), longForm ? DATE_LONGDATE : DATE_SHORTDATE, &st, 0, 0, 0, 0);
    if (n < 3) return L"";
    std::wstring date(n - 1, 0);
    GetDateFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), longForm ? DATE_LONGDATE : DATE_SHORTDATE, &st, 0, date.data(), n, 0);
    std::wstring timeFormat = localeInfo(longForm ? LOCALE_STIMEFORMAT :LOCALE_SSHORTTIME);
    n = GetTimeFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), 0, &st, timeFormat.data(), 0, 0);
    if (n < 3) return L"";
    std::wstring time(n - 1, 0);
    GetTimeFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), 0, &st, timeFormat.data(), time.data(), n);
    return date + L" " + time;
}


// std::chrono::time_point to wide string using format/picture information

template<class Clock> std::wstring formatTimePoint(std::chrono::time_point<Clock> timePoint, TimestampSettings::DateFormat format,
                                                   std::wstring picture, const LocaleWords& localeWords) {
    switch (format) {
    case TimestampSettings::DateFormat::iso8601:
        return std::format(L"{0:%F}T{0:%T}", std::chrono::time_point_cast<std::chrono::milliseconds>(timePoint));
    case TimestampSettings::DateFormat::localeShort:
        return getWindowsDateTimeFormat(timePoint, false);
    case TimestampSettings::DateFormat::localeLong:
        return getWindowsDateTimeFormat(timePoint, true);
    }

    std::wstring info = std::format(L"{0:%Y}{0:%m}{0:%d}{0:%j}{0:%w}{0:%H}{0:%I}{0:%M}{0:%S}", timePoint);
    std::wstring s;
    for (size_t i = 0; i < picture.length();) {
        switch (picture[i]) {
        case L'y':
        {
            size_t j = std::min(picture.find_first_not_of(L'y', i), picture.length());
            s += j - i < 3 ? info.substr(2, 2) : info.substr(0, 4);
            i = j;
            break;
        }
        case L'M':
        {
            size_t j = std::min(picture.find_first_not_of(L'M', i), picture.length());
            s += j - i < 2 && info[4] == L'0' ? info.substr(5, 1) : j - i < 3 ? info.substr(4, 2)
               : j - i == 3 ? localeWords.abbrMonth[std::stoi(info.substr(4,2)) - 1]
               : j - i == 4 ? localeWords.geniMonth[std::stoi(info.substr(4,2)) - 1]
               : (localeWords.geniMonth[std::stoi(info.substr(4,2)) - 1] + std::wstring(localeWords.geniMax, L' ')).substr(0, localeWords.geniMax);
            i = j;
            break;
        }
        case L'N':
        {
            size_t j = std::min(picture.find_first_not_of(L'N', i), picture.length());
            s += j - i < 5 ? localeWords.fullMonth[std::stoi(info.substr(4,2)) - 1]
               : (localeWords.fullMonth[std::stoi(info.substr(4,2)) - 1] + std::wstring(localeWords.fullMax, L' ')).substr(0, localeWords.fullMax);
            i = j;
            break;
        }
        case L'd':
        {
            size_t j = std::min(picture.find_first_not_of(L'd', i), picture.length());
            s += j - i < 2 && info[6] == L'0' ? info.substr(7, 1) : j - i < 3 ? info.substr(6, 2)
                : j - i == 3 ? localeWords.dayAbbrev[info[11] - L'0']
                : j - i == 4 ? localeWords.dayOfWeek[info[11] - L'0']
                : (localeWords.dayOfWeek[info[11] - L'0'] + std::wstring(localeWords.weekMax, L' ')).substr(0, localeWords.weekMax);
            i = j;
            break;
        }
        case L'D':
        {
            size_t j = std::min(picture.find_first_not_of(L'D', i), picture.length());
            s += j - i < 2 && info[8] == L'0' && info[9] == L'0' ? info.substr(10, 1) : j - i < 3 && info[8] == L'0' ? info.substr(9, 2) : info.substr(8, 3);
            i = j;
            break;
        }
        case L'H':
        {
            size_t j = std::min(picture.find_first_not_of(L'H', i), picture.length());
            s += j - i < 2 && info[12] == L'0' ? info.substr(13, 1) : info.substr(12, 2);
            i = j;
            break;
        }
        case L'h':
        {
            size_t j = std::min(picture.find_first_not_of(L'h', i), picture.length());
            s += j - i < 2 && info[14] == L'0' ? info.substr(15, 1) : info.substr(14, 2);
            i = j;
            break;
        }
        case L'm':
        {
            size_t j = std::min(picture.find_first_not_of(L'm', i), picture.length());
            s += j - i < 2 && info[16] == L'0' ? info.substr(17, 1) : info.substr(16, 2);
            i = j;
            break;
        }
        case L's':
        {
            size_t j = std::min(picture.find_first_not_of(L's', i), picture.length());
            s += j - i < 2 && info[18] == L'0' ? info.substr(19, 1) : info.substr(18, 2);
            i = j;
            if (picture.substr(i, 2) == L".s" || picture.substr(i, 2) == L",s") {
                s += picture[i];
                j = std::min(picture.find_first_not_of(L's', i + 1), picture.length());
                s += info.substr(21, j - i - 1);
                i = j;
            }
            break;
        }
        case L'T':
        {
            size_t j = std::min(picture.find_first_not_of(L'T', i), picture.length());
            size_t k = info[12] == L'0' && info[13] < L'2' ? 0 : 1;
            if (j - i == 1) s += localeWords.ampm[k][0];
            else s += localeWords.ampm[k];
            i = j;
            break;
        }
        case L't':
        {
            size_t j = std::min(picture.find_first_not_of(L't', i), picture.length());
            size_t k = info[12] == L'0' && info[13] < L'2' ? 0 : 1;
            if (j - i == 1) s += static_cast<wchar_t>(std::tolower(localeWords.ampm[k][0]));
            else for (auto c : localeWords.ampm[k]) s += static_cast<wchar_t>(std::tolower(c));
            i = j;
            break;
        }
        case L'\'':
            for (;;) {
                size_t j = std::min(picture.find_first_of(L'\'', i + 1), picture.length());
                s += picture.substr(i + 1, j - i - 1);
                i = j + 1;
                if (i >= picture.length() || picture[i] != L'\'') break;
                s += L'\'';
            }
            break;
        case L'z':
            if (i + 1 >= picture.length()) {
                s += L'z';
                ++i;
                break;
            }
            switch (picture[i + 1]) {
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
                s += info[12] == L'0' ? L' ' + info.substr(13, 1) : info.substr(12, 2);
                i += 2;
                break;
            case L'h':
                s += info[14] == L'0' ? L' ' + info.substr(15, 1) : info.substr(14, 2);
                i += 2;
                break;
            case L'm':
                s += info[16] == L'0' ? L' ' + info.substr(15, 1) : info.substr(16, 2);
                i += 2;
                break;
            case L's':
            {
                s += info[18] == L'0' ? L' ' + info.substr(19, 1) : info.substr(18, 2);
                i += 2;
                if (picture.substr(i, 2) == L".s" || picture.substr(i, 2) == L",s") {
                    s += picture[i];
                    size_t j = std::min(picture.find_first_not_of(L's', i + 1), picture.length());
                    s += info.substr(21, j - i - 1);
                    i = j;
                }
                break;
            }
            case L'z':
                s += picture[i + 1];
                i += 2;
                break;
            default:
                s += L'z';
                ++i;
            }
            break;
        default:
            s += picture[i];
            ++i;
        }
    }
    return s;
}


// some helper routines for the dialog procedure

TimestampSettings::DateFormat dialogDateFormat(HWND hwndDlg) {
    if (IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_DATE_STD  )) return TimestampSettings::DateFormat::iso8601;
    if (IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_DATE_SHORT)) return TimestampSettings::DateFormat::localeShort;
    if (IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_DATE_LONG )) return TimestampSettings::DateFormat::localeLong;
                                                                  return TimestampSettings::DateFormat::custom;
}

void showExampleOutput(HWND hwndDlg) {
    std::chrono::system_clock::time_point tp = std::chrono::sys_days(std::chrono::year(1991) / 9 / 6)
        + std::chrono::hours(14) + std::chrono::minutes(5) + std::chrono::seconds(7) + std::chrono::milliseconds(135);
    std::wstring s = formatTimePoint(tp, dialogDateFormat(hwndDlg), GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), LocaleWords());
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


// dialog procedure

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
        int radioButton;
        
        radioButton = ts.fromCounter.type == TimestampSettings::CounterType::Unix ? IDC_TIMESTAMP_FROM_UNIX
                    : ts.fromCounter.type == TimestampSettings::CounterType::File ? IDC_TIMESTAMP_FROM_FILE
                    : ts.fromCounter.type == TimestampSettings::CounterType::Ex00 ? IDC_TIMESTAMP_FROM_1900
                    : ts.fromCounter.type == TimestampSettings::CounterType::Ex04 ? IDC_TIMESTAMP_FROM_1904
                    : IDC_TIMESTAMP_FROM_COUNTER_CUSTOM;
        CheckRadioButton(hwndDlg, IDC_TIMESTAMP_FROM_UNIX, IDC_TIMESTAMP_FROM_COUNTER_CUSTOM, radioButton);
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_EPOCH, std::format(L"{0:%F} {0:%T}", timePoint(ts.fromCounter.custom.epoch)).data());
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_FROM_UNIT , std::format(L"{0:.7f}", static_cast<long double>(ts.fromCounter.custom.unit)*1e-7).data());
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_LEAP , ts.fromCounter.custom.leap ? BST_CHECKED : BST_UNCHECKED);

        radioButton = ts.toCounter.type == TimestampSettings::CounterType::Unix ? IDC_TIMESTAMP_TO_UNIX
                    : ts.toCounter.type == TimestampSettings::CounterType::File ? IDC_TIMESTAMP_TO_FILE
                    : ts.toCounter.type == TimestampSettings::CounterType::Ex00 ? IDC_TIMESTAMP_TO_1900
                    : ts.toCounter.type == TimestampSettings::CounterType::Ex04 ? IDC_TIMESTAMP_TO_1904
                    : IDC_TIMESTAMP_TO_COUNTER_CUSTOM;
        CheckRadioButton(hwndDlg, IDC_TIMESTAMP_TO_UNIX, IDC_TIMESTAMP_TO_COUNTER_CUSTOM, radioButton);
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_EPOCH, std::format(L"{0:%F} {0:%T}", timePoint(ts.toCounter.custom.epoch)).data());
        SetDlgItemText(hwndDlg, IDC_TIMESTAMP_TO_UNIT , std::format(L"{0:.7f}", static_cast<long double>(ts.toCounter.custom.unit)*1e-7).data());
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_TO_LEAP , ts.toCounter.custom.leap ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_COUNTER_ENABLE , ts.enableFromCounter  ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_FROM_DATETIME_ENABLE, ts.enableFromDatetime ? BST_CHECKED : BST_UNCHECKED);
        for (const auto& s : ts.dateParse) SendDlgItemMessage(hwndDlg, IDC_TIMESTAMP_FROM_PARSE, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        SendDlgItemMessage(hwndDlg, IDC_TIMESTAMP_FROM_PARSE, CB_SETCURSEL, 0, 0);
        for (const auto& s : ts.datePicture) SendDlgItemMessage(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        SendDlgItemMessage(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT, CB_SETCURSEL, 0, 0);

        radioButton = ts.datePriority == TimestampSettings::DatePriority::ymd ? IDC_TIMESTAMP_FROM_YMD
                    : ts.datePriority == TimestampSettings::DatePriority::mdy ? IDC_TIMESTAMP_FROM_MDY
                    : ts.datePriority == TimestampSettings::DatePriority::dmy ? IDC_TIMESTAMP_FROM_DMY
                    : IDC_TIMESTAMP_FROM_DATETIME_CUSTOM;
        CheckRadioButton(hwndDlg, IDC_TIMESTAMP_FROM_YMD, IDC_TIMESTAMP_FROM_DATETIME_CUSTOM, radioButton);

        radioButton = ts.dateFormat == TimestampSettings::DateFormat::iso8601     ? IDC_TIMESTAMP_TO_DATE_STD
                    : ts.dateFormat == TimestampSettings::DateFormat::localeShort ? IDC_TIMESTAMP_TO_DATE_SHORT
                    : ts.dateFormat == TimestampSettings::DateFormat::localeLong  ? IDC_TIMESTAMP_TO_DATE_LONG
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
            const TimestampSettings::CounterType  fromCounter  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_UNIX) ? TimestampSettings::CounterType::Unix
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_FILE) ? TimestampSettings::CounterType::File
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_1900) ? TimestampSettings::CounterType::Ex00
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_1904) ? TimestampSettings::CounterType::Ex04
                                                               : TimestampSettings::CounterType::custom;
            const TimestampSettings::CounterType  toCounter    = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_UNIX)   ? TimestampSettings::CounterType::Unix
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_FILE)   ? TimestampSettings::CounterType::File
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_1900)   ? TimestampSettings::CounterType::Ex00
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_1904)   ? TimestampSettings::CounterType::Ex04
                                                               : TimestampSettings::CounterType::custom;
            const TimestampSettings::DatePriority datePriority = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_YMD) ? TimestampSettings::DatePriority::ymd
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_MDY) ? TimestampSettings::DatePriority::mdy
                                                               : IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_DMY) ? TimestampSettings::DatePriority::dmy
                                                               : TimestampSettings::DatePriority::custom;
            uint64_t     fromEpoch = data.timestamps.fromCounter.custom.epoch;
            uint64_t     fromUnit  = data.timestamps.fromCounter.custom.unit;
            uint64_t     toEpoch   = data.timestamps.toCounter.custom.epoch;
            uint64_t     toUnit    = data.timestamps.toCounter.custom.unit;
            std::wstring dateParse = data.timestamps.dateParse.empty() ? std::wstring() : data.timestamps.dateParse[0];
            if (enableFromCounter && fromCounter == TimestampSettings::CounterType::custom) {
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
            if (LOWORD(wParam) == IDC_TIMESTAMP_TO_COUNTER && toCounter == TimestampSettings::CounterType::custom) {
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
            if (enableFromDatetime && datePriority == TimestampSettings::DatePriority::custom) {
                dateParse = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_FROM_PARSE);
                if (dateParse.empty()) {
                    showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_PARSE, L"Enter a regular expression to use for parsing dates and times.", true);
                    return TRUE;
                }
                RegularExpression rx(data);
                std::wstring error = rx.find(dateParse);
                if (!error.empty()) {
                    showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_PARSE, error, true);
                    return TRUE;
                }
            }
            data.timestamps.enableFromCounter  = enableFromCounter;
            data.timestamps.enableFromDatetime = enableFromDatetime;
            if (enableFromCounter) {
                data.timestamps.fromCounter.type = fromCounter;
                if (fromCounter == TimestampSettings::CounterType::custom) {
                    data.timestamps.fromCounter.custom.epoch = fromEpoch;
                    data.timestamps.fromCounter.custom.unit  = fromUnit;
                    data.timestamps.fromCounter.custom.leap  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_LEAP);
                }
            }
            if (enableFromDatetime) {
                data.timestamps.datePriority = datePriority;
                if (datePriority == TimestampSettings::DatePriority::custom) updateComboHistory(hwndDlg, IDC_TIMESTAMP_FROM_PARSE, data.timestamps.dateParse);
            }
            if (LOWORD(wParam) == IDC_TIMESTAMP_TO_COUNTER) {
                data.timestamps.toCounter.type = toCounter;
                if (toCounter == TimestampSettings::CounterType::custom) {
                    data.timestamps.toCounter.custom.epoch = toEpoch;
                    data.timestamps.toCounter.custom.unit  = toUnit;
                    data.timestamps.toCounter.custom.leap  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_LEAP);
                }
            }
            else {
                data.timestamps.dateFormat = dialogDateFormat(hwndDlg);
                if (data.timestamps.dateFormat == TimestampSettings::DateFormat::custom)
                    updateComboHistory(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT, data.timestamps.datePicture);
            }
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;
        }
        case IDC_TIMESTAMP_FROM_UNIX:
        case IDC_TIMESTAMP_FROM_FILE:
        case IDC_TIMESTAMP_FROM_1900:
        case IDC_TIMESTAMP_FROM_1904:
        case IDC_TIMESTAMP_FROM_COUNTER_CUSTOM:
        case IDC_TIMESTAMP_FROM_YMD:
        case IDC_TIMESTAMP_FROM_MDY:
        case IDC_TIMESTAMP_FROM_DMY:
        case IDC_TIMESTAMP_FROM_DATETIME_CUSTOM:
        case IDC_TIMESTAMP_FROM_DATETIME_ENABLE:
            enableFromFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_FROM_COUNTER_ENABLE:
            enableFromFields(hwndDlg, true);
            break;
        case IDC_TIMESTAMP_TO_UNIX:
        case IDC_TIMESTAMP_TO_FILE:
        case IDC_TIMESTAMP_TO_1900:
        case IDC_TIMESTAMP_TO_1904:
        case IDC_TIMESTAMP_TO_COUNTER_CUSTOM:
            enableToCounterFields(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_DATE_STD:
        case IDC_TIMESTAMP_TO_DATE_SHORT:
        case IDC_TIMESTAMP_TO_DATE_LONG:
            EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), FALSE);
            showExampleOutput(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_DATE_CUSTOM:
            EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), TRUE);
            showExampleOutput(hwndDlg);
            break;
        case IDC_TIMESTAMP_TO_DATE_FORMAT:
            showExampleOutput(hwndDlg);
            break;
        }
        break;
    }
    return FALSE;

}


// a helper class for the main routine

struct ParsingInformation {
    ColumnsPlusPlusData& data;
    const std::wstring   locale;
    const intptr_t       action;
    LocaleWords          localeWords;
    unsigned int         codepage   = 0;
    ParsingInformation(ColumnsPlusPlusData& data, const intptr_t action, const std::wstring locale = L"")
        : data(data), locale(locale), action(action), localeWords(locale), codepage(data.sci.CodePage()) {}
    bool parseGenericDateText(const std::string_view source, int64_t& counter) const;
    bool parsePatternDateText(const std::string_view source, int64_t& counter) const;
};


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
                                          localeWords.ampm[0].data(), aToken[i].length() == 1 ? 1 : -1, 0, 0, 0))
            ampm = ampm == -1 ? 0 : -2;
        else if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                          aToken[i].data(), static_cast<int>(aToken[i].length()),
                                          localeWords.ampm[0].data(), aToken[i].length() == 1 ? 1 : -1, 0, 0, 0))
            ampm = ampm == -1 ? 12 : -2;
        for (int j = 0; j < 12; ++j) {
            if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              aToken[i].data(), -1, localeWords.abbrMonth[j].data(), -1, 0, 0, 0)) {
                aMonth = aMonth == -1 ? j + 1 : -2;
                break;
            }
            if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              aToken[i].data(), -1, localeWords.fullMonth[j].data(), -1, 0, 0, 0)) {
                aMonth = aMonth == -1 ? j + 1 : -2;
                break;
            }
            if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              aToken[i].data(), -1, localeWords.geniMonth[j].data(), -1, 0, 0, 0)) {
                aMonth = aMonth == -1 ? j + 1 : -2;
                break;
            }
        }
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

    if (action == IDC_TIMESTAMP_TO_DATETIME || ts.toCounter.spec().leap) {
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

    if (data.timestamps.dateParse.empty()) return false;
    RegularExpression rx(data);
    rx.find(data.timestamps.dateParse.back());
    if (!rx.can_search()) return false;
    if (!rx.search(source)) return false;
    std::string year    = rx.str("y");
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
                while (++nMonth < 13) {
                    if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                        wMonth.data(), -1, localeWords.abbrMonth[nMonth - 1].data(), -1, 0, 0, 0)) break;
                    if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                        wMonth.data(), -1, localeWords.fullMonth[nMonth - 1].data(), -1, 0, 0, 0)) break;
                    if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                        wMonth.data(), -1, localeWords.geniMonth[nMonth - 1].data(), -1, 0, 0, 0)) break;
                }
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
                                              localeWords.ampm[0].data(), wampm.length() == 1 ? 1 : -1, 0, 0, 0))
                ticks += 432000000000i64;
            else if (CSTR_EQUAL != CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              wampm.data(), static_cast<int>(wampm.length()),
                                              localeWords.ampm[1].data(), wampm.length() == 1 ? 1 : -1, 0, 0, 0))
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

        if (action == IDC_TIMESTAMP_TO_DATETIME || data.timestamps.toCounter.spec().leap) {
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

    TimestampSettings::CounterSpec fromCounter = timestamps.fromCounter.spec();
    TimestampSettings::CounterSpec toCounter   = timestamps.toCounter  .spec();

    int64_t fromEpoch = !timestamps.enableFromCounter || fromCounter.leap ? fromCounter.epoch
        : std::chrono::clock_cast<std::chrono::system_clock>(
            std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(fromCounter.epoch))
        ).time_since_epoch().count();

    int64_t toEpoch = action == IDC_TIMESTAMP_TO_DATETIME || toCounter.leap ? toCounter.epoch
        : std::chrono::clock_cast<std::chrono::system_clock>(
            std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(toCounter.epoch))
        ).time_since_epoch().count();

    ParsingInformation pi(*this, action);

    struct Replacement {
        std::string text;
        size_t left = 0;
        bool isLastInRow;
        bool isEndOfLine;
    };

    std::vector<std::vector<Replacement>> replacements;

    rs.natural();

    for (auto row : rs) {

        auto& replaceLine = replacements.emplace_back();

        for (const auto& cell : row) {

            if (!cell.textLength() && cell.isLastInRow()) continue;

            auto& replaceCell = replaceLine.emplace_back();
            replaceCell.isLastInRow = cell.isLastInRow();
            replaceCell.isEndOfLine = cell.isEndOfLine();
            {
                const std::string text = cell.text();
                const size_t n = text.find_last_not_of(' ');
                if (n == std::string::npos) continue;
                replaceCell.text = text.substr(0, n + 1);
            }

            const std::string source = cell.trim();
            int64_t counter = 0;

            bool sourceIsCounter  = false;
            if (timestamps.enableFromCounter && stringToCounter(source, counter, fromCounter.unit, settings.decimalSeparatorIsComma ? ',' : '.')) {
                counter += fromEpoch;
                sourceIsCounter = true;
                if (timestamps.fromCounter.type == TimestampSettings::CounterType::Ex00 && counter < -22039776000000000) counter += fromCounter.unit;
            }
            else if ( !timestamps.enableFromDatetime ||
                     (timestamps.datePriority == TimestampSettings::DatePriority::custom ? !pi.parsePatternDateText(source, counter)
                                                                                         : !pi.parseGenericDateText(source, counter)) ) continue;

            if (action == IDC_TIMESTAMP_TO_DATETIME) {
                std::wstring s = !sourceIsCounter || fromCounter.leap
                    ? formatTimePoint(std::chrono::utc_clock   ::time_point(std::chrono::utc_clock   ::duration(counter)), timestamps.dateFormat,
                                                                            timestamps.datePicture.empty() ? std::wstring() : timestamps.datePicture.back(),
                                                                            pi.localeWords)
                    : formatTimePoint(std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter)), timestamps.dateFormat,
                                                                            timestamps.datePicture.empty() ? std::wstring() : timestamps.datePicture.back(),
                                                                            pi.localeWords);
                replaceCell.text = fromWide(s, pi.codepage);
            }
            else {
                if (sourceIsCounter && fromCounter.leap != toCounter.leap) {
                    counter = toCounter.leap
                        ? std::chrono::clock_cast<std::chrono::utc_clock>(
                             std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter))
                             ).time_since_epoch().count()
                        : std::chrono::clock_cast<std::chrono::system_clock>(
                             std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(counter))
                             ).time_since_epoch().count();
                }
                if (timestamps.toCounter.type == TimestampSettings::CounterType::Ex00 && counter < -22039776000000000) counter -= toCounter.unit;
                std::string s;
                if (!ratioToDecimal(counter - toEpoch, toCounter.unit, s, settings.decimalSeparatorIsComma ? ',' : '.')) continue;
                replaceCell.text = s;
                size_t n = s.find_first_of(settings.decimalSeparatorIsComma ? ',' : '.');
                replaceCell.left = n == std::string::npos ? s.length() : n;
            }

        }
    }

    struct ColumnWidth {
        size_t left  = 0;
        size_t right = 0;
        size_t total = 0;
    };

    std::vector<ColumnWidth> maxWidth;

    for (const auto& replaceRow : replacements) {
        for (size_t i = 0; i < replaceRow.size(); ++i) {
            if (maxWidth.size() <= i) maxWidth.resize(i + 1);
            const auto& repl = replaceRow[i];
            auto& columnMax = maxWidth[i];
            if (repl.left) {
                if (columnMax.left < repl.left) columnMax.left = repl.left;
                if (columnMax.right < repl.text.length() - repl.left) columnMax.right = repl.text.length() - repl.left;
            }
            if (columnMax.total < repl.text.length()) columnMax.total = repl.text.length();
        }
    }
    for (auto& columnMax : maxWidth) if (columnMax.total < columnMax.left + columnMax.right) columnMax.total = columnMax.left + columnMax.right;

    sci.BeginUndoAction();

    for (auto row : rs) {
        std::string r;
        const auto& replacementRow = replacements[row.index];
        for (size_t column = 0; column < replacementRow.size(); ++column) {
            const auto& repl      = replacementRow[column];
            const auto& columnMax = maxWidth[column];
            std::string s = (repl.left && repl.left < columnMax.left) ? std::string(columnMax.left - repl.left, ' ') : "";
            s += repl.text;
            if (!repl.isEndOfLine && (!settings.elasticEnabled || repl.isLastInRow) && s.length() < columnMax.total) s.resize(columnMax.total, ' ');
            r += s;
            if (!repl.isLastInRow) r += '\t';
        }
        if (r != row.text()) row.replace(r);
    }

    rs.refit();
    sci.EndUndoAction();

}