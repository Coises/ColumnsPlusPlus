// This file is part of Columns++ for Notepad++.
// Copyright 2024 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

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

#include "Timestamps.h"

// LocaleWords

namespace {

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

    std::wstring getWindowsDateTimeFormat(int64_t counter, bool leap, bool longForm, std::wstring locale) {
        std::chrono::year_month_day ymd;
        int secs;
        if (leap) {
            auto timePoint = std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(counter));
            auto tp_seconds = std::chrono::round<std::chrono::seconds>(timePoint);
            auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(tp_seconds);
            ymd = std::chrono::floor<std::chrono::days>(sctp);
            secs = static_cast<int>(sctp.time_since_epoch().count() % 86400);
        }
        else {
            auto timePoint = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter));
            auto tp_seconds = std::chrono::round<std::chrono::seconds>(timePoint);
            auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(tp_seconds);
            ymd = std::chrono::floor<std::chrono::days>(sctp);
            secs = static_cast<int>(sctp.time_since_epoch().count() % 86400);
        }
        SYSTEMTIME st;
        st.wYear = static_cast<WORD>(int(ymd.year()));
        st.wMonth = static_cast<WORD>(unsigned(ymd.month()));
        st.wDay = static_cast<WORD>(unsigned(ymd.day()));
        st.wDayOfWeek = static_cast<WORD>(unsigned(std::chrono::weekday(ymd).c_encoding()));
        st.wHour = static_cast<WORD>(secs / 3600);
        st.wMinute = static_cast<WORD>((secs % 3600) / 60);
        st.wSecond = static_cast<WORD>(secs % 60);
        st.wMilliseconds = 0;
        int n = GetDateFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), longForm ? DATE_LONGDATE : DATE_SHORTDATE, &st, 0, 0, 0, 0);
        if (n < 3) return L"";
        std::wstring date(n - 1, 0);
        GetDateFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), longForm ? DATE_LONGDATE : DATE_SHORTDATE, &st, 0, date.data(), n, 0);
        std::wstring timeFormat = localeInfo(longForm ? LOCALE_STIMEFORMAT : LOCALE_SSHORTTIME, locale);
        n = GetTimeFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), 0, &st, timeFormat.data(), 0, 0);
        if (n < 3) return L"";
        std::wstring time(n - 1, 0);
        GetTimeFormatEx(locale.empty() ? LOCALE_NAME_USER_DEFAULT : locale.data(), 0, &st, timeFormat.data(), time.data(), n);
        return date + L" " + time;
    }

}


LocaleWords::LocaleWords(const std::wstring locale) : locale(locale) {
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


std::wstring LocaleWords::formatTimePoint(int64_t counter, bool leap, const std::chrono::time_zone* tz,
                                          TimestampSettings::DateFormat format, const std::wstring& customPicture) const {
    
    std::wstring info, zoneInfo;
    if (tz) {
        std::chrono::system_clock::time_point systp;
        std::wstring seconds;
        if (leap) {
            auto utctp = std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(counter));
            systp = std::chrono::clock_cast<std::chrono::system_clock>(utctp);
            seconds = std::format(L"{0:%S}", utctp);
        }
        else {
            systp = std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter));
            seconds = std::format(L"{0:%S}", systp);
        }
        auto zt = std::chrono::zoned_time(tz, systp);
        if (format == TimestampSettings::DateFormat::localeShort)
            return getWindowsDateTimeFormat(zt.get_local_time().time_since_epoch().count(), false, false, locale);
        if (format == TimestampSettings::DateFormat::localeLong )
            return getWindowsDateTimeFormat(zt.get_local_time().time_since_epoch().count(), false, true, locale);
        info = std::format(L"{0:%Y}{0:%m}{0:%d}{0:%j}{0:%w}{0:%H}{0:%I}{0:%M}", zt) + seconds;
        zoneInfo = std::format(L"{0:%z}{0:%Z}", zt);
    }
    else if (format == TimestampSettings::DateFormat::localeShort) return getWindowsDateTimeFormat(counter, leap, false, locale);
    else if (format == TimestampSettings::DateFormat::localeLong ) return getWindowsDateTimeFormat(counter, leap, true , locale);
    else {
        info = leap ? std::format(L"{0:%Y}{0:%m}{0:%d}{0:%j}{0:%w}{0:%H}{0:%I}{0:%M}{0:%S}",
                                  std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(counter)))
                    : std::format(L"{0:%Y}{0:%m}{0:%d}{0:%j}{0:%w}{0:%H}{0:%I}{0:%M}{0:%S}",
                                  std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter)));
        zoneInfo = L"+0000UTC";
    }

    const std::wstring picture = format == TimestampSettings::DateFormat::custom ? customPicture : L"yyyy-MM-dd'T'HH:mm:ss.sssZ";

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
               : j - i == 3 ? abbrMonth[std::stoi(info.substr(4,2)) - 1]
               : j - i == 4 ? geniMonth[std::stoi(info.substr(4,2)) - 1]
               : (geniMonth[std::stoi(info.substr(4,2)) - 1] + std::wstring(geniMax, L' ')).substr(0, geniMax);
            i = j;
            break;
        }
        case L'N':
        {
            size_t j = std::min(picture.find_first_not_of(L'N', i), picture.length());
            s += j - i < 5 ? fullMonth[std::stoi(info.substr(4,2)) - 1]
               : (fullMonth[std::stoi(info.substr(4,2)) - 1] + std::wstring(fullMax, L' ')).substr(0, fullMax);
            i = j;
            break;
        }
        case L'd':
        {
            size_t j = std::min(picture.find_first_not_of(L'd', i), picture.length());
            s += j - i < 2 && info[6] == L'0' ? info.substr(7, 1) : j - i < 3 ? info.substr(6, 2)
                : j - i == 3 ? dayAbbrev[info[11] - L'0']
                : j - i == 4 ? dayOfWeek[info[11] - L'0']
                : (dayOfWeek[info[11] - L'0'] + std::wstring(weekMax, L' ')).substr(0, weekMax);
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
            size_t k = info.substr(12,2) < L"12" ? 0 : 1;
            if (j - i == 1) s += ampm[k][0];
            else s += ampm[k];
            i = j;
            break;
        }
        case L't':
        {
            size_t j = std::min(picture.find_first_not_of(L't', i), picture.length());
            size_t k = info.substr(12, 2) < L"12" ? 0 : 1;
            if (j - i == 1) s += static_cast<wchar_t>(std::tolower(ampm[k][0]));
            else for (auto c : ampm[k]) s += static_cast<wchar_t>(std::tolower(c));
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
        case L'Z':
        {
            size_t j = std::min(picture.find_first_not_of(L'Z', i), picture.length());
            s += j - i == 3                       ? zoneInfo.substr(5)
               : zoneInfo.substr(1, 4) == L"0000" ? L"Z"
               : j - i == 1                       ? zoneInfo.substr(0, 3) + L":" + zoneInfo.substr(3, 2)
               : j - i != 2                       ? zoneInfo.substr(0, 5)
               : zoneInfo.substr(3, 2) == L"00"   ? zoneInfo.substr(0, 3)
                                                  : zoneInfo.substr(0, 3) + L":" + zoneInfo.substr(3, 2);
            i = j;
            break;
        }
        case L'z':
        {
            size_t j = std::min(picture.find_first_not_of(L'z', i), picture.length());
            s += j - i == 1                       ? zoneInfo.substr(0, 3) + L":" + zoneInfo.substr(3, 2)
               : j - i != 2                       ? zoneInfo.substr(0, 5)
               : zoneInfo.substr(3, 2) == L"00"   ? zoneInfo.substr(0, 3)
                                                  : zoneInfo.substr(0, 3) + L":" + zoneInfo.substr(3, 2);
            i = j;
            break;
        }
        case L'x':
            if (i + 1 >= picture.length()) {
                s += L'x';
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
            case L'x':
                s += picture[i + 1];
                i += 2;
                break;
            default:
                s += L'x';
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


// TimestampsCommon

namespace {
    BOOL CALLBACK addLocaleCallback(LPWSTR pStr, DWORD, LPARAM lparam) {
        TimestampsCommon& si = *reinterpret_cast<TimestampsCommon*>(lparam);
        int nLanguage = GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDLANGUAGENAME, 0, 0);
        int nCountry = GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDCOUNTRYNAME, 0, 0);
        TimestampsCommon::Locale locale;
        locale.name = pStr;
        if (nLanguage) {
            locale.language.resize(nLanguage - 1);
            GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDLANGUAGENAME, locale.language.data(), nLanguage);
        }
        if (nCountry) {
            locale.country.resize(nCountry - 1);
            GetLocaleInfoEx(pStr, LOCALE_SLOCALIZEDCOUNTRYNAME, locale.country.data(), nCountry);
        }
        si.locales[locale.language][locale.name] = locale;
        return TRUE;
    }
}

TimestampsCommon::TimestampsCommon(ColumnsPlusPlusData& data) : data(data) {
    EnumSystemLocalesEx(addLocaleCallback, LOCALE_NEUTRALDATA | LOCALE_SUPPLEMENTAL | LOCALE_WINDOWS, reinterpret_cast<LPARAM>(this), 0);
    zones[L"(none)"][L"UTC"] = { 0, L"(none)", L"UTC" };
    try {
        for (const auto& tz : std::chrono::get_tzdb().zones) {
            const std::wstring s = toWide(tz.name().data(), CP_UTF8);
            size_t p = s.find_first_of(L'/');
            if (p > 0 && p < s.length() - 1) {
                TimeZone timeZone{ &tz, s.substr(0, p), s.substr(p + 1) };
                zones[timeZone.continent][timeZone.city] = timeZone;
            }
        }
    }
    catch (...) {}
}

void TimestampsCommon::initializeDialogLanguagesAndLocales(HWND hwndDlg, const std::wstring& initialLocale, int cbLanguage, int cbLocale) {
    for (const auto& s : locales)
        SendDlgItemMessage(hwndDlg, cbLanguage, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.first.data()));
    std::wstring localeName = initialLocale;
    if (localeName.empty()) {
        int n = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, 0, 0);
        localeName.resize(n - 1);
        GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, localeName.data(), n);
    }
    int nLanguage = GetLocaleInfoEx(localeName.data(), LOCALE_SLOCALIZEDLANGUAGENAME, 0, 0);
    if (nLanguage) {
        std::wstring language(nLanguage - 1, 0);
        GetLocaleInfoEx(localeName.data(), LOCALE_SLOCALIZEDLANGUAGENAME, language.data(), nLanguage);
        auto n = SendDlgItemMessage(hwndDlg, cbLanguage, CB_FINDSTRINGEXACT,
            static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(language.data()));
        if (n != CB_ERR) {
            SendDlgItemMessage(hwndDlg, cbLanguage, CB_SETCURSEL, n, 0);
            for (const auto& s : locales[language]) {
                std::wstring x = s.first + L"  -  " + s.second.country;
                n = SendDlgItemMessage(hwndDlg, cbLocale, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(x.data()));
                if (s.first == localeName) SendDlgItemMessage(hwndDlg, cbLocale, CB_SETCURSEL, n, 0);
            }
        }
    }
}

void TimestampsCommon::initializeDialogTimeZones(HWND hwndDlg, const std::wstring& timeZone, int cbRegion, int cbZone) {
    if (zones.empty()) {
        EnableWindow(GetDlgItem(hwndDlg, cbRegion), false);
        EnableWindow(GetDlgItem(hwndDlg, cbZone  ), false);
        return;
    }
    for (const auto& s : zones) SendDlgItemMessage(hwndDlg, cbRegion, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.first.data()));
    std::wstring continent = L"(none)";
    std::wstring city      = L"UTC";
    if (!timeZone.empty()) {
        size_t p = timeZone.find_first_of(L'/');
        if (p > 0 && p < timeZone.length() - 1) {
            continent = timeZone.substr(0, p);
            city      = timeZone.substr(p + 1);
        }
    }
    auto n = SendDlgItemMessage(hwndDlg, cbRegion, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(continent.data()));
    if (n == CB_ERR) {
        SendDlgItemMessage(hwndDlg, cbRegion, CB_SETCURSEL, 0, 0);
        continent = GetDlgItemString(hwndDlg, cbRegion);
    }
    else SendDlgItemMessage(hwndDlg, cbRegion, CB_SETCURSEL, n, 0);
    for (const auto& s : zones[continent]) {
        n = SendDlgItemMessage(hwndDlg, cbZone, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.first.data()));
        if (s.first == city) SendDlgItemMessage(hwndDlg, cbZone, CB_SETCURSEL, n, 0);
    }
}

void TimestampsCommon::updateDialogLocales(HWND hwndDlg, int cbLanguage, int cbLocale) const {
    std::wstring language = GetDlgItemString(hwndDlg, cbLanguage);
    SendDlgItemMessage(hwndDlg, cbLocale, CB_RESETCONTENT, 0, 0);
    if (!locales.contains(language)) return;
    auto localeMap = locales.at(language);
    for (const auto& s : localeMap) {
        std::wstring x = s.first + L"  -  " + s.second.country;
        SendDlgItemMessage(hwndDlg, cbLocale, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(x.data()));
    }
    SendDlgItemMessage(hwndDlg, cbLocale, CB_SETCURSEL, 0, 0);
}

void TimestampsCommon::updateDialogTimeZones(HWND hwndDlg, int cbRegion, int cbZone) const {
    std::wstring region = GetDlgItemString(hwndDlg, cbRegion);
    SendDlgItemMessage(hwndDlg, cbZone, CB_RESETCONTENT, 0, 0);
    if (!zones.contains(region)) return;
    auto zoneList = zones.at(region);
    for (const auto& s : zoneList) SendDlgItemMessage(hwndDlg, cbZone, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.first.data()));
    SendDlgItemMessage(hwndDlg, cbZone, CB_SETCURSEL, 0, 0);
}

const std::chrono::time_zone* TimestampsCommon::zoneNamed(const std::wstring& name) const {
    if (name.empty()) return 0;
    size_t p = name.find_first_of(L'/');
    if (p < 1 || p > name.length() - 2) return 0;
    auto sub1 = zones.find(name.substr(0, p));
    if (sub1 == zones.end()) return 0;
    auto sub2 = sub1->second.find(name.substr(p + 1));
    if (sub2 == sub1->second.end()) return 0;
    return sub2->second.tz;
}


// TimestampsParse

TimestampsParse::TimestampsParse(ColumnsPlusPlusData& data, const intptr_t action, const std::wstring locale)
        : data(data), rx(data), action(action), localeWords(locale), codepage(data.sci.CodePage()) {
    if (data.timestamps.enableFromDatetime && data.timestamps.datePriority == TimestampSettings::DatePriority::custom)
        rx.find(data.timestamps.dateParse.back(), true);
}


bool TimestampsParse::parseDateText(const std::string_view source, int64_t& counter, const std::chrono::time_zone* tz) {
    if (!data.timestamps.enableFromDatetime) return false;
    ParsedValues pv;
    if ( !(data.timestamps.datePriority == TimestampSettings::DatePriority::custom ? parsePatternDateText(source, pv)
                                                                                   : parseGenericDateText(source, pv)) ) return false;
    std::chrono::utc_clock::time_point tp;
    if (tz && !pv.offsetFound) {
        std::chrono::local_seconds lt = std::chrono::local_days(pv.ymd);
        lt += std::chrono::hours(pv.hour);
        lt += std::chrono::minutes(pv.minute);
        tp = std::chrono::clock_cast<std::chrono::utc_clock>(std::chrono::zoned_time(tz, lt, std::chrono::choose::earliest).get_sys_time());
    }
    else {
        tp = std::chrono::clock_cast<std::chrono::utc_clock>(std::chrono::sys_days(pv.ymd));
        tp += std::chrono::hours(pv.hour);
        tp += std::chrono::minutes(pv.minute - pv.offset);
    }
    counter = tp.time_since_epoch().count() + pv.ticks;
    return true;
}


namespace {
    bool isParseDivider(wchar_t c) {
        static const std::wstring nonAsciiDividers =
            L"\u2012\u2013\u2212\uFE63\uFF0D\u00A0\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F";
        if (c < 128) return !isalnum(c);
        return (nonAsciiDividers.find_first_of(c) != std::wstring::npos);
    }
}


bool TimestampsParse::parseGenericDateText(const std::string_view source, ParsedValues& pv) const {

    const TimestampSettings& ts = data.timestamps;
    const std::wstring s = toWide(source, codepage);

    struct Token : std::wstring_view {
        enum TokenType {unknown, number, divider, ampm, month} tokenType = unknown;
        int tokenValue = -1;
        Token(std::wstring_view sv, size_t pos, size_t len) : std::wstring_view(sv.substr(pos, len)) {}
        int iValue() const {
            if (tokenType == ampm || tokenType == month) return tokenValue;
            if (tokenType != number || length() > 9) return -1;
            int v = 0;
            for (size_t i = 0;;) {
                v += at(i) - L'0';
                if (++i >= length()) break;
                v *= 10;
            }
            return v;
        }
    };
 
    std::vector<Token>  tokens;
    std::vector<size_t> alphaTokens;
    std::vector<size_t> numberTokens;
    std::vector<size_t> dividerTokens;
    size_t ampmToken  = std::wstring::npos;
    size_t monthToken = std::wstring::npos;
 
    for (size_t i = 0; i < s.length();) {
        if (iswdigit(s[i])) {
            size_t j = i + 1;
            while (j < s.length() && iswdigit(s[j])) ++j;
            numberTokens.push_back(tokens.size());
            tokens.emplace_back(s, i, j - i);
            tokens.back().tokenType = Token::number;
            i = j;
        }
        else if (isParseDivider(s[i])) {
            size_t j = i + 1;
            while (j < s.length() && isParseDivider(s[j])) ++j;
            dividerTokens.push_back(tokens.size());
            tokens.emplace_back(s, i, j - i);
            tokens.back().tokenType = Token::divider;
            i = j;
        }
        else {
            size_t j = i + 1;
            while (j < s.length() && !iswdigit(s[j]) && !isParseDivider(s[j])) ++j;
            alphaTokens.push_back(tokens.size());
            tokens.emplace_back(s, i, j - i);
            Token& t = tokens.back();
            if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE, t.data(), static_cast<int>(t.length()),
                                              localeWords.ampm[0].data(), t.length() == 1 ? 1 : -1, 0, 0, 0)) {
                if (ampmToken != std::wstring::npos) return false;
                t.tokenType  = Token::ampm;
                t.tokenValue = 0;
                ampmToken = tokens.size() - 1;
            }
            else if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE, t.data(), static_cast<int>(t.length()),
                                                   localeWords.ampm[1].data(), t.length() == 1 ? 1 : -1, 0, 0, 0)) {
                if (ampmToken != std::wstring::npos) return false;
                t.tokenType  = Token::ampm;
                t.tokenValue = 12;
                ampmToken = tokens.size() - 1;
            }
            else for (int k = 0; k < 12; ++k) {
                if ( ( CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE, t.data(), static_cast<int>(t.length()),
                                                     localeWords.abbrMonth[k].data(), -1, 0, 0, 0) )
                  || ( CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE, t.data(), static_cast<int>(t.length()),
                                                     localeWords.fullMonth[k].data(), -1, 0, 0, 0) )
                  || ( CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE, t.data(), static_cast<int>(t.length()),
                                                     localeWords.geniMonth[k].data(), -1, 0, 0, 0) ) ) {
                    if (monthToken != std::wstring::npos) return false;
                    t.tokenType  = Token::month;
                    t.tokenValue = k + 1;
                    monthToken = tokens.size() - 1;
                    break;
                }
            }
            i = j;
        }
    }
 
    if (numberTokens.size() == 0) return false;
    const Token& num1 = tokens[numberTokens[0]];
 
    if (num1.length() == 6 || num1.length() > 8) return false;
 
    size_t timeToken, timeNumber; // these will tell the indices into tokens and into numberTokens where the time can start
 
    if (num1.length() > 6) /* must be yyyyMMdd or yyyyDDD */ {
        if (monthToken != std::wstring::npos) /* shouldn't be a named month with this form */ return false;
        int y = (num1[0] - L'0') * 1000 + (num1[1] - L'0') * 100 + (num1[2] - L'0') * 10 + (num1[3] - L'0');
        if (num1.length() == 8) /* yyyyMMdd */ {
            int m = (num1[4] - L'0') * 10 + (num1[5] - L'0');
            int d = (num1[6] - L'0') * 10 + (num1[7] - L'0');
            pv.ymd = std::chrono::year(y) / m / d;
            if (!pv.ymd.ok()) return false;
        }
        else {
            int d = (num1[4] - L'0') * 100 + (num1[5] - L'0') * 10 + (num1[6] - L'0');
            if (d > (std::chrono::year(y).is_leap() ? 366 : 365)) return false;
            pv.ymd = std::chrono::sys_days(std::chrono::year(y) / 1 / 1) + std::chrono::days(d - 1);
        }
        timeNumber = 1;
        timeToken = numberTokens[0] + 1;
    }
    else if (monthToken != std::wstring::npos) /* named month given */ {
        if (numberTokens.size() < 2) /* must have a year and a day */ return false;
        if (numberTokens.size() > 2 && numberTokens[2] < monthToken) /* should not have three numbers before named month */ return false;
        const Token& num2 = tokens[numberTokens[1]];
        if (num1.length() > 2 || (num2.length() < 3 && ts.datePriority == TimestampSettings::DatePriority::ymd)) /* year first */ {
            if (num2.length() > 2) /* day should not be more than two digits */ return false;
            pv.ymd = std::chrono::year(num1.iValue()) / tokens[monthToken].tokenValue / num2.iValue();
        }
        else pv.ymd = std::chrono::year(num2.iValue()) / tokens[monthToken].tokenValue / num1.iValue();
        if (!pv.ymd.ok()) return false;
        timeNumber = 2;
        timeToken = std::max(numberTokens[1], monthToken) + 1;
    }
    else /* year, month and day are first three numbers (not necessarily in that order) */ {
        if (numberTokens.size() < 3) return false;
        const Token& num2 = tokens[numberTokens[1]];
        const Token& num3 = tokens[numberTokens[2]];
        int yearPosition = 0;
        if (num1.length() > 2) yearPosition = 1;
        if (num2.length() > 2) { if (yearPosition == 0) yearPosition = 2; else return false; }
        if (num3.length() > 2) { if (yearPosition == 0) yearPosition = 3; else return false; }
        if (yearPosition == 0) yearPosition = ts.datePriority == TimestampSettings::DatePriority::ymd ? 1 : 3;
        if (yearPosition == 1) pv.ymd = std::chrono::year(num1.iValue()) / num2.iValue() / num3.iValue();
        else if (yearPosition == 2)
            pv.ymd = ts.datePriority == TimestampSettings::DatePriority::dmy ? std::chrono::year(num2.iValue()) / num3.iValue() / num1.iValue()
            : std::chrono::year(num2.iValue()) / num1.iValue() / num3.iValue();
        else
            pv.ymd = ts.datePriority == TimestampSettings::DatePriority::dmy ? std::chrono::year(num3.iValue()) / num2.iValue() / num1.iValue()
            : std::chrono::year(num3.iValue()) / num1.iValue() / num2.iValue();
        if (!pv.ymd.ok()) return false;
        timeNumber = 3;
        timeToken = numberTokens[2] + 1;
    }

    if (timeNumber >= numberTokens.size()) /* no time given; should be no am/pm */ return ampmToken == std::wstring::npos;

    if (tokens[numberTokens[timeNumber]].length() > 2) /* could be Hmm, HHmm or Hmmss or HHmmss with optional decimals following */ {
        const Token& time = tokens[numberTokens[timeNumber]];
        switch (time.length()) {
        case 3:
            pv.hour = time[0] - L'0';
            pv.minute = (time[1] - L'0') * 10 + (time[2] - L'0');
            break;
        case 4:
            pv.hour = (time[0] - L'0') * 10 + time[1] - L'0';
            pv.minute = (time[2] - L'0') * 10 + (time[3] - L'0');
            break;
        case 5:
            pv.hour = time[0] - L'0';
            pv.minute = (time[1] - L'0') * 10 + (time[2] - L'0');
            pv.ticks = (time[3] - L'0') * 100000000LL + (time[4] - L'0') * 10000000LL;
            break;
        case 6:
            pv.hour = (time[0] - L'0') * 10 + time[1] - L'0';
            pv.minute = (time[2] - L'0') * 10 + (time[3] - L'0');
            pv.ticks = (time[4] - L'0') * 100000000LL + (time[5] - L'0') * 10000000LL;
            break;
        default:
            return false;
        }
        if (pv.hour > 23 || pv.minute > 59 || pv.ticks > 600000000LL) return false;
    }
    else /* hour */ {
        pv.hour = tokens[numberTokens[timeNumber]].iValue();
        if (ampmToken != std::wstring::npos) {
            if (ampmToken < timeToken) /* am/pm should not appear within the date section */ return false;
            if (pv.hour < 1 || pv.hour > 12) return false;
            pv.hour = (pv.hour % 12) + tokens[ampmToken].tokenValue;
        }
        else if (pv.hour < 0 || pv.hour > 23) return false;
    }

    /* if there is a "Z" time zone offset specified, get that now */

    pv.offsetFound = tokens.back() == L"Z" || tokens.back() == L"z";

    if (timeNumber + 1 == numberTokens.size()) /* done */ return true;

    // time zone offests other than "Z" begin with a plus or minus sign and have two digits, four digits or two digits, a colon and two digits
    // they can only appear at the very end of an entry

    size_t zoneNumber = numberTokens.size();

    if (!pv.offsetFound && tokens.back().tokenType == Token::number) {
        const Token& tz1 = tokens.back();
        const Token& tz2 = tokens[tokens.size() - 2];
        const Token& tz3 = tokens[tokens.size() - 3];
        const Token& tz4 = tokens[tokens.size() - 4];
        if (tz1.length() == 4 && (tz2.back() == L'+' || tz2.back() == L'-' || tz2.back() == L'\u2212')) {
            zoneNumber = numberTokens.size() - 1;
            pv.offset = (tz1[0] - L'0') * 600 + (tz1[1] - L'0') * 60 + (tz1[2] - L'0') * 10 + (tz1[3] - L'0');
            if (tz2.back() != L'+') pv.offset = -pv.offset;
            pv.offsetFound = true;
        }
        else if (tz1.length() < 3 && (tz2.back() == L'+' || tz2.back() == L'-' || tz2.back() == L'\u2212')) {
            if ( tokens[numberTokens[timeNumber]].length() > 2         // allowed with unseparated time
              || timeNumber + 3 < numberTokens.size()                  // allowed if already have three time numbers without this
              || tz2.length() > 1 ) {                                  // allowed when separator is more than just the +/- sign
                zoneNumber = numberTokens.size() - 1;
                pv.offset = tz1.iValue() * 60;
                if (tz2.back() != L'+') pv.offset = -pv.offset;
                pv.offsetFound = true;
            }
        }
        else if ( tz3.tokenType == Token::number && tz1.length() == 2 && tz3.length() < 3 && tz2 == L":" 
               && (tz4.back() == L'+' || tz4.back() == L'-' || tz4.back() == L'\u2212') ) {
            zoneNumber = numberTokens.size() - 2;
            pv.offset  = tz3.iValue() * 60 + tz1.iValue();
            if (tz4.back() != L'+') pv.offset = -pv.offset;
            pv.offsetFound = true;
        }
    }
 
    if (timeNumber + 1 == zoneNumber) /* done */ return true;
 
    if (tokens[numberTokens[timeNumber]].length() > 2) /* decimal minutes or seconds, or cannot parse */ {
        if ( timeNumber + 2 != zoneNumber || numberTokens[timeNumber + 1] == numberTokens[timeNumber] + 2
          || (tokens[numberTokens[timeNumber] + 1] != L"." && tokens[numberTokens[timeNumber] + 1] != L",") ) return false;
        const std::wstring_view decimalString = tokens[numberTokens[timeNumber + 1]];
        int64_t v = 0;
        for (size_t i = 0;;) {
            if (i < decimalString.length()) v += decimalString.at(i) - L'0';
            if (++i >= 9) break;
            v *= 10;
        }
        if (tokens[numberTokens[timeNumber]].length() > 4) /* decimal seconds */ pv.ticks += (v + 50) / 100;
                                                      else /* decimal minutes */ pv.ticks  = (v * 6 + 5) / 10;
        return true;
    }
 
    // decimal hours are recognized only if separated from whole hours by a single period or comma,
    // and there are no more numbers before the time zone offset (if any)
 
    if ( timeNumber + 2 == zoneNumber
      && numberTokens[timeNumber + 1] == numberTokens[timeNumber] + 2
      && (tokens[numberTokens[timeNumber] + 1] == L"." || tokens[numberTokens[timeNumber] + 1] == L",") ) {
        const std::wstring_view decimalString = tokens[numberTokens[timeNumber + 1]];
        int64_t v = 0;
        for (size_t i = 0;;) {
            if (i < decimalString.length()) v += decimalString.at(i) - L'0';
            if (++i >= 9) break;
            v *= 10;
        }
        v *= 36;
        pv.minute = static_cast<int>(v / 600000000);
        pv.ticks  = v % 600000000;
        return true;
    }
 
    // minute
 
    pv.minute = tokens[numberTokens[timeNumber + 1]].iValue();
    if (pv.minute < 0 || pv.minute > 59) return false;
    if (timeNumber + 2 == zoneNumber) /* done */ return true;
 
    // decimal minutes are recognized only if separated from whole minutes by a single period or comma,
    // and there are no more numbers before the time zone offset (if any)
 
    if ( timeNumber + 3 == zoneNumber
      && numberTokens[timeNumber + 2] == numberTokens[timeNumber + 1] + 2
      && (tokens[numberTokens[timeNumber + 1] + 1] == L"." || tokens[numberTokens[timeNumber + 1] + 1] == L",") ) {
        const std::wstring_view decimalString = tokens[numberTokens[timeNumber + 2]];
        int64_t v = 0;
        for (size_t i = 0;;) {
            if (i < decimalString.length()) v += decimalString.at(i) - L'0';
            if (++i >= 9) break;
            v *= 10;
        }
        pv.ticks = (v * 6 + 5) / 10;
        return true;
    }
 
    
    // seconds
    
    pv.ticks = tokens[numberTokens[timeNumber + 2]].iValue();
    if (pv.ticks < 0 || pv.ticks > 60) return false;
    pv.ticks *= 10000000;
    if (timeNumber + 3 == zoneNumber) /* done */ return true;
 
    // decimal seconds are recognized only if separated from whole seconds by a single period or comma,
    // and there are no more numbers before the time zone offset (if any)
 
    if ( timeNumber + 4 == zoneNumber
      && numberTokens[timeNumber + 3] == numberTokens[timeNumber + 2] + 2
      && (tokens[numberTokens[timeNumber + 2] + 1] == L"." || tokens[numberTokens[timeNumber + 2] + 1] == L",") ) {
        const std::wstring_view decimalString = tokens[numberTokens[timeNumber + 3]];
        int64_t v = 0;
        for (size_t i = 0;;) {
            if (i < decimalString.length()) v += decimalString.at(i) - L'0';
            if (++i >= 9) break;
            v *= 10;
        }
        pv.ticks += (v + 50) / 100;
        return true;
    }
 
    return false;
 
 


}


bool TimestampsParse::parsePatternDateText(const std::string_view source, ParsedValues& pv) {

    if (data.timestamps.dateParse.empty()) return false;
    if (!rx.can_search()) return false;
    if (!rx.search(source)) return false;
    std::string year    = rx.str("y");
    std::string doy     = rx.str("D");
    std::string month   = rx.str("M");
    std::string dom     = rx.str("d");
    std::string hour    = rx.str("H");
    std::string minute  = rx.str("m");
    std::string seconds = rx.str("s");
    std::string ampm    = rx.str("t");
    std::string offset  = rx.str("Z");
    if (year.empty()) return false;
    if (doy.empty() && (month.empty() || dom.empty())) return false;
    if (!doy.empty() && !(month.empty() && dom.empty())) return false;

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
            pv.ymd = std::chrono::year(nYear) / nMonth / nDom;
            if (!pv.ymd.ok()) return false;
        }
        else pv.ymd = std::chrono::sys_days(std::chrono::year(nYear) / 1 / 1) + std::chrono::days(stoi(doy) - 1);

        if (!offset.empty()) {
            if (offset == "Z" || offset == "z") pv.offset = 0;
            else {
                bool west = false;
                std::wstring s = toWide(offset, codepage);
                if (s[0] == L'-' || s[0] == L'\u2212') west = true;
                else if (s[0] != L'+') return false;
                switch (s.length()) {
                case 2:
                    if (!iswdigit(s[1])) return false;
                    pv.offset = 60 * stoi(s.substr(1, 1));
                    break;
                case 3:
                    if (!iswdigit(s[1]) && iswdigit(s[2])) return false;
                    pv.offset = 60 * stoi(s.substr(1, 2));
                    break;
                case 5:
                    if (s[2] == L':') {
                        if (!iswdigit(s[1]) && iswdigit(s[3]) && iswdigit(s[4])) return false;
                        pv.offset = 60 * stoi(s.substr(1, 1)) + stoi(s.substr(3, 2));
                    }
                    else {
                        if (!iswdigit(s[1]) && iswdigit(s[2]) && iswdigit(s[3]) && iswdigit(s[4])) return false;
                        pv.offset = 60 * stoi(s.substr(1, 2)) + stoi(s.substr(3, 2));
                    }
                    break;
                case 6:
                    if (!iswdigit(s[1]) && iswdigit(s[2]) && s[3] != L':' && iswdigit(s[4]) && iswdigit(s[5])) return false;
                    pv.offset = 60 * stoi(s.substr(1, 2)) + stoi(s.substr(4, 2));
                    break;
                default:
                    return false;
                }
                if (west) pv.offset = -pv.offset;
            }
            pv.offsetFound = true;
        }

        pv.hour = hour.empty() ? 0 : stoi(hour);
        if (!ampm.empty()) {
            pv.hour %= 12;
            std::wstring wampm = toWide(ampm, codepage);
            if (CSTR_EQUAL == CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              wampm.data(), static_cast<int>(wampm.length()),
                                              localeWords.ampm[1].data(), wampm.length() == 1 ? 1 : -1, 0, 0, 0))
                pv.hour += 12;
            else if (CSTR_EQUAL != CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                              wampm.data(), static_cast<int>(wampm.length()),
                                              localeWords.ampm[0].data(), wampm.length() == 1 ? 1 : -1, 0, 0, 0))
                return false;
        }
        pv.minute = minute.empty() ? 0 : stoi(minute);
        if (seconds.empty()) pv.ticks = 0;
        else {
            size_t p = seconds.find_first_not_of("0123456789 ");
            if (p == std::string::npos || (seconds[p] != '.' && seconds[p] != ',')) pv.ticks = 10000000i64 * stoi(seconds);
            else {
                pv.ticks = 10000000i64 * stoi(seconds.substr(0, p - 1));
                try {
                     pv.ticks += stoi((seconds.substr(p + 1) + "0000000").substr(0, 7));
                }
                catch (...) {}
            }
        }

        return true;

    }
    catch (...) {
        return false;
    }
}

