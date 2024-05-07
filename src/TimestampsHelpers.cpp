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
    else info = leap ? std::format(L"{0:%Y}{0:%m}{0:%d}{0:%j}{0:%w}{0:%H}{0:%I}{0:%M}{0:%S}",
                                   std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(counter)))
                     : std::format(L"{0:%Y}{0:%m}{0:%d}{0:%j}{0:%w}{0:%H}{0:%I}{0:%M}{0:%S}",
                                   std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter)));

    const std::wstring picture = format == TimestampSettings::DateFormat::custom ? customPicture : L"yyyy-MM-dd'T'HH:mm:ss.sssZZ";

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
            s += j - i < 2 ? (zoneInfo.length() > 4 && zoneInfo.substr(1,4) != L"0000" ? zoneInfo.substr(0, 5) : L"")
               : j - i < 3 ? zoneInfo.substr(0,5) : zoneInfo.length() > 5 ? zoneInfo.substr(5) : L"";
            i = j;
            break;
        }
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
    if (tz) {
        std::chrono::local_seconds lt = std::chrono::local_days(pv.ymd);
        lt += std::chrono::hours(pv.hour);
        lt += std::chrono::minutes(pv.minute);
        tp = std::chrono::clock_cast<std::chrono::utc_clock>(std::chrono::zoned_time(tz, lt, std::chrono::choose::earliest).get_sys_time());
    }
    else {
        tp = std::chrono::clock_cast<std::chrono::utc_clock>(std::chrono::sys_days(pv.ymd));
        tp += std::chrono::hours(pv.hour);
        tp += std::chrono::minutes(pv.minute);
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

    std::vector<std::wstring> aToken;
    std::vector<std::wstring> nToken;

    for (size_t i = 0; i < s.length();) {
        if (iswdigit(s[i])) {
            size_t j = i + 1;
            while (j < s.length() && iswdigit(s[j])) ++j;
            nToken.push_back(s.substr(i, j - i));
            i = j;
        }
        else if (isParseDivider(s[i])) ++i;
        else {
            size_t j = i + 1;
            while (j < s.length() && !iswdigit(s[j]) && !isParseDivider(s[j])) ++j;
            aToken.push_back(s.substr(i, j - i));
            i = j;
        }
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
            localeWords.ampm[1].data(), aToken[i].length() == 1 ? 1 : -1, 0, 0, 0))
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
    if (aMonth > 0 ? (nToken.size() < 2 || nToken.size() > 6) : (nToken.size() < 3 && nToken.size() > 7)) return false;

    int    nYear = 0;
    int    nMonth = 0;
    int    nDay = 0;
    size_t tToken = 3;

    if (aMonth > 0) {
        tToken = 2;
        nMonth = aMonth;
        if (nToken[0].length() > 3 && nToken[1].length() < 3) { nYear = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        else if (nToken[0].length() < 3 && nToken[1].length() > 3) { nYear = stoi(nToken[1]); nDay = stoi(nToken[0]); }
        else if (ts.datePriority == TimestampSettings::DatePriority::ymd) { nYear = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        else { nYear = stoi(nToken[1]); nDay = stoi(nToken[0]); }
    }
    else {
        if (nToken[0].length() > 2 && nToken[1].length() < 3 && nToken[2].length() < 3) {
            nYear = stoi(nToken[0]);
            nMonth = stoi(nToken[1]);
            nDay = stoi(nToken[2]);
        }
        else if (nToken[0].length() < 3 && nToken[1].length() > 2 && nToken[2].length() < 3) {
            nYear = stoi(nToken[1]);
            if (ts.datePriority == TimestampSettings::DatePriority::dmy) { nMonth = stoi(nToken[2]); nDay = stoi(nToken[0]); }
            else { nMonth = stoi(nToken[0]); nDay = stoi(nToken[2]); }
        }
        else if (nToken[0].length() < 3 && nToken[1].length() < 3 && nToken[2].length() > 2) {
            nYear = stoi(nToken[2]);
            if (ts.datePriority == TimestampSettings::DatePriority::dmy) { nMonth = stoi(nToken[1]); nDay = stoi(nToken[0]); }
            else { nMonth = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        }
        else if (ts.datePriority == TimestampSettings::DatePriority::ymd) { nYear = stoi(nToken[0]); nMonth = stoi(nToken[1]); nDay = stoi(nToken[2]); }
        else if (ts.datePriority == TimestampSettings::DatePriority::mdy) { nYear = stoi(nToken[2]); nMonth = stoi(nToken[0]); nDay = stoi(nToken[1]); }
        else { nYear = stoi(nToken[2]); nMonth = stoi(nToken[1]); nDay = stoi(nToken[0]); }
    }

    pv.ymd = std::chrono::sys_days(std::chrono::year(nYear) / nMonth / nDay);
    if (!pv.ymd.ok()) return false;
    if (tToken < nToken.size()) {
        pv.hour = (ampm >= 0 ? stoi(nToken[tToken]) % 12 + ampm : stoi(nToken[tToken]));
        if (tToken + 1 < nToken.size()) {
            pv.minute = stoi(nToken[tToken + 1]);
            if (tToken + 2 < nToken.size()) {
                pv.ticks = 10000000i64 * stoi(nToken[tToken + 2]);
                if (tToken + 3 < nToken.size()) pv.ticks += stoi((nToken[tToken + 3] + L"000000").substr(0, 7));
            }
        }
    }
    return true;

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

