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

#include <chrono>
#include "ColumnsPlusPlus.h"
#include "RegularExpression.h"


struct LocaleWords {
    std::vector<std::wstring> abbrMonth;
    std::vector<std::wstring> fullMonth;
    std::vector<std::wstring> geniMonth;
    std::vector<std::wstring> dayAbbrev;
    std::vector<std::wstring> dayOfWeek;
    std::vector<std::wstring> ampm;
    const std::wstring locale;
    size_t fullMax = 0;
    size_t geniMax = 0;
    size_t weekMax = 0;
    LocaleWords(const std::wstring locale = L"");
    std::wstring formatTimePoint(int64_t counter, bool leap, const std::chrono::time_zone* tz,
                                 TimestampSettings::DateFormat format, const std::wstring& customPicture) const;
};


class TimestampsCommon {

public:

    struct Locale {
        std::wstring name;
        std::wstring language;
        std::wstring country;
    };

    struct TimeZone {
        const std::chrono::time_zone* tz;
        std::wstring continent;
        std::wstring city;
    };

    ColumnsPlusPlusData& data;
    std::map<std::wstring, std::map<std::wstring, Locale>> locales;
    std::map<std::wstring, std::map<std::wstring, TimeZone>> zones;

    TimestampsCommon(ColumnsPlusPlusData& data);
    void initializeDialogLanguagesAndLocales(HWND hwndDlg, const std::wstring& initialLocale, int cbLanguage, int cbLocale);
    void initializeDialogTimeZones(HWND hwndDlg, const std::wstring& timeZone, int cbRegion, int cbZone);
    void updateDialogLocales(HWND hwndDlg, int cbLanguage, int cbLocale) const;
    void updateDialogTimeZones(HWND hwndDlg, int cbRegion, int cbZone) const;
    const std::chrono::time_zone* zoneNamed(const std::wstring& name) const;

};


struct TimestampsParse {

    struct ParsedValues {
        std::chrono::year_month_day ymd;
        int64_t ticks;
        int hour, minute;
    };

    ColumnsPlusPlusData& data;
    RegularExpression    rx;
    const intptr_t       action;
    LocaleWords          localeWords;
    unsigned int         codepage = 0;

    TimestampsParse(ColumnsPlusPlusData& data, const intptr_t action, const std::wstring locale = L"");

    bool parseDateText(const std::string_view source, int64_t& counter, const std::chrono::time_zone* tz);
    bool parseGenericDateText(const std::string_view source, ParsedValues& pv) const;
    bool parsePatternDateText(const std::string_view source, ParsedValues& pv);

};
