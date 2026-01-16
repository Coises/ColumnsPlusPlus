// This file is part of Columns++ for Notepad++.
// Copyright 2024, 2026 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

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
#include "resource.h"
#include "commctrl.h"


namespace {

// helper routines for the dialog procedure

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

TimestampSettings::DateFormat dialogDateFormat(HWND hwndDlg) {
    if (IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_DATE_STD  )) return TimestampSettings::DateFormat::iso8601;
    if (IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_DATE_SHORT)) return TimestampSettings::DateFormat::localeShort;
    if (IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TO_DATE_LONG )) return TimestampSettings::DateFormat::localeLong;
                                                                  return TimestampSettings::DateFormat::custom;
}

const std::wstring dialogLocale(HWND hwndDlg) {
    if (!IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TZLOCALE_ENABLE)) return L"";
    std::wstring locale = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_LOCALE);
    return locale.substr(0, locale.find(L"  -  "));
}

void showExampleOutput(HWND hwndDlg, const TimestampsCommon& tsc) {
    const LocaleWords lw(dialogLocale(hwndDlg));
    const std::chrono::time_zone* tz = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TZLOCALE_ENABLE) ?
                                       tsc.zoneNamed(GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_REGION) + L"/" + 
                                                     GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_TIMEZONE)) : 0;
    std::wstring s = lw.formatTimePoint(6841659071350000, false, tz, dialogDateFormat(hwndDlg), GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT));
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
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_LIMIT_TEXT        ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_LIMIT_EDIT        ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_LIMIT_SPIN        ), enableDate);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_PARSE             ), enableDate && customDate);
}

void enableTzLFields(HWND hwndDlg) {
    bool tzl = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TZLOCALE_ENABLE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_LANGUAGE      ), tzl);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_LOCALE        ), tzl);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TZLOCALE_ARROW), tzl);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_REGION   ), tzl);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_FROM_TIMEZONE ), tzl);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_REGION     ), tzl);
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_TIMEZONE   ), tzl);
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

    TimestampsCommon* tscp = 0;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        tscp = reinterpret_cast<TimestampsCommon*>(lParam);
    }
    else tscp = reinterpret_cast<TimestampsCommon*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!tscp) return TRUE;
    TimestampsCommon&    tsc  = *tscp;
    ColumnsPlusPlusData& data = tsc.data;

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
        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_TZLOCALE_ENABLE     , ts.enableTzAndLocale  ? BST_CHECKED : BST_UNCHECKED);
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

        SendDlgItemMessage(hwndDlg, IDC_TIMESTAMP_FROM_LIMIT_SPIN, UDM_SETRANGE, 0, MAKELPARAM(9999, 99));
        SendDlgItemMessage(hwndDlg, IDC_TIMESTAMP_FROM_LIMIT_SPIN, UDM_SETPOS, 0, ts.twoDigitYearLimit);

        CheckDlgButton(hwndDlg, IDC_TIMESTAMP_OVERWRITE, ts.overwrite ? BST_CHECKED : BST_UNCHECKED);

        if (ts.enableTzAndLocale) {
            tsc.initializeDialogLanguagesAndLocales(hwndDlg, data.timestamps.localeName, IDC_TIMESTAMP_LANGUAGE, IDC_TIMESTAMP_LOCALE);
            tsc.initializeDialogTimeZones(hwndDlg, data.timestamps.fromZone, IDC_TIMESTAMP_FROM_REGION, IDC_TIMESTAMP_FROM_TIMEZONE);
            tsc.initializeDialogTimeZones(hwndDlg, data.timestamps.toZone  , IDC_TIMESTAMP_TO_REGION  , IDC_TIMESTAMP_TO_TIMEZONE  );
            tsc.dialogTzLInitialized = true;
        }

        enableFromFields(hwndDlg);
        enableTzLFields(hwndDlg);
        enableToCounterFields(hwndDlg);
        showExampleOutput(hwndDlg, tsc);
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
            uint64_t fromEpoch     = data.timestamps.fromCounter.custom.epoch;
            uint64_t fromUnit      = data.timestamps.fromCounter.custom.unit;
            uint64_t toEpoch       = data.timestamps.toCounter.custom.epoch;
            uint64_t toUnit        = data.timestamps.toCounter.custom.unit;
            int      tdyLimit      = data.timestamps.twoDigitYearLimit;
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
            if (enableFromDatetime) {
                if (!validateSpin(tdyLimit, hwndDlg, IDC_TIMESTAMP_FROM_LIMIT_SPIN, L"Enter the maximum year represented by two digits.")) return TRUE;
                if (datePriority == TimestampSettings::DatePriority::custom) {
                    dateParse = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_FROM_PARSE);
                    if (dateParse.empty()) {
                        showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_PARSE, L"Enter a regular expression to use for parsing dates and times.", true);
                        return TRUE;
                    }
                    RegularExpression rx(data.sci);
                    std::wstring error = rx.find(dateParse, true);
                    if (!error.empty()) {
                        showBalloonTip(hwndDlg, IDC_TIMESTAMP_FROM_PARSE, error, true);
                        return TRUE;
                    }
                }
            } 
            data.timestamps.enableFromCounter  = enableFromCounter;
            data.timestamps.enableFromDatetime = enableFromDatetime;
            data.timestamps.enableTzAndLocale  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TZLOCALE_ENABLE);
            data.timestamps.overwrite          = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_OVERWRITE);
            if (enableFromCounter) {
                data.timestamps.fromCounter.type = fromCounter;
                if (fromCounter == TimestampSettings::CounterType::custom) {
                    data.timestamps.fromCounter.custom.epoch = fromEpoch;
                    data.timestamps.fromCounter.custom.unit  = fromUnit;
                    data.timestamps.fromCounter.custom.leap  = IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_FROM_LEAP);
                }
            }
            if (enableFromDatetime) {
                data.timestamps.datePriority      = datePriority;
                data.timestamps.twoDigitYearLimit = tdyLimit;
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
            if (data.timestamps.enableTzAndLocale) {
                data.timestamps.localeName = dialogLocale(hwndDlg);
                data.timestamps.fromZone = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_FROM_REGION) + L'/' + GetDlgItemString(hwndDlg, IDC_TIMESTAMP_FROM_TIMEZONE);
                data.timestamps.toZone   = GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_REGION  ) + L'/' + GetDlgItemString(hwndDlg, IDC_TIMESTAMP_TO_TIMEZONE  );
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
        case IDC_TIMESTAMP_TZLOCALE_ENABLE:
            if (!tsc.dialogTzLInitialized && IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP_TZLOCALE_ENABLE)) {
                tsc.initializeDialogLanguagesAndLocales(hwndDlg, data.timestamps.localeName, IDC_TIMESTAMP_LANGUAGE, IDC_TIMESTAMP_LOCALE);
                tsc.initializeDialogTimeZones(hwndDlg, data.timestamps.fromZone, IDC_TIMESTAMP_FROM_REGION, IDC_TIMESTAMP_FROM_TIMEZONE);
                tsc.initializeDialogTimeZones(hwndDlg, data.timestamps.toZone  , IDC_TIMESTAMP_TO_REGION  , IDC_TIMESTAMP_TO_TIMEZONE  );
                tsc.dialogTzLInitialized = true;
            }
            enableTzLFields(hwndDlg);
            showExampleOutput(hwndDlg, tsc);
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
            showExampleOutput(hwndDlg, tsc);
            break;
        case IDC_TIMESTAMP_TO_DATE_CUSTOM:
            EnableWindow(GetDlgItem(hwndDlg, IDC_TIMESTAMP_TO_DATE_FORMAT), TRUE);
            showExampleOutput(hwndDlg, tsc);
            break;
        case IDC_TIMESTAMP_TO_DATE_FORMAT:
            showExampleOutput(hwndDlg, tsc);
            break;
        case IDC_TIMESTAMP_LANGUAGE:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                tsc.updateDialogLocales(hwndDlg, IDC_TIMESTAMP_LANGUAGE, IDC_TIMESTAMP_LOCALE);
                showExampleOutput(hwndDlg, tsc);
            }
            break;
        case IDC_TIMESTAMP_FROM_REGION:
            if (HIWORD(wParam) == CBN_SELCHANGE) tsc.updateDialogTimeZones(hwndDlg, IDC_TIMESTAMP_FROM_REGION, IDC_TIMESTAMP_FROM_TIMEZONE);
            break;
        case IDC_TIMESTAMP_TO_REGION:
            if (HIWORD(wParam) == CBN_SELCHANGE) tsc.updateDialogTimeZones(hwndDlg, IDC_TIMESTAMP_TO_REGION, IDC_TIMESTAMP_TO_TIMEZONE);
            showExampleOutput(hwndDlg, tsc);
            break;
        case IDC_TIMESTAMP_TO_TIMEZONE:
            showExampleOutput(hwndDlg, tsc);
            break;
        case IDC_TIMESTAMP_LOCALE:
            if (HIWORD(wParam) == CBN_SELCHANGE) showExampleOutput(hwndDlg, tsc);
            break;
        }
        break;
    }
    return FALSE;

}


// helper routines for the main routine - manipulating 64-bit integers

bool ratioToDecimal(int64_t numerator, int64_t denominator, int64_t& integer, int64_t& decimal, size_t& places) {
    if (denominator == 0) return false;
    if (numerator == 0) {
        integer = 0;
        decimal = 0;
        places = 0;
        return true;
    }
    if (denominator == 1) {
        integer = numerator;
        decimal = 0;
        places = 0;
        return true;
    }
    if (denominator == -1) {
        integer = -numerator;
        decimal = 0;
        places = 0;
        return true;
    }
    int64_t quotient = numerator / denominator;
    int64_t remainder = std::abs(numerator % denominator);
    if (remainder == 0) {
        integer = quotient;
        decimal = 0;
        places = 0;
        return true;
    }

    double divisor = static_cast<double>(std::abs(denominator));
    double fraction = static_cast<double>(remainder) / divisor;
    double power = 1;
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


} // end unnamed namespace


void ColumnsPlusPlusData::convertTimestamps() {

    auto rs = getRectangularSelection();
    if (!rs.size()) return;

    TimestampsCommon tsc(*this);

    auto action = DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_TIMESTAMP), nppData._nppHandle, ::timestampsDialogProc, reinterpret_cast<LPARAM>(&tsc));
    if (!action) return;

    const std::chrono::time_zone* tzFrom = timestamps.enableTzAndLocale ? tsc.zoneNamed(timestamps.fromZone) : 0;
    const std::chrono::time_zone* tzTo   = timestamps.enableTzAndLocale ? tsc.zoneNamed(timestamps.toZone  ) : 0;
    
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

    TimestampsParse pi(*this, action, timestamps.enableTzAndLocale ? timestamps.localeName : L"");

    struct Replacement {
        std::string text;
        size_t left = 0;
        bool isLastInRow;
        bool isEndOfLine;
    };

    std::vector<std::vector<Replacement>> replacements;
    bool needsSpace = false;

    rs.natural();

    for (auto row : rs) {

        auto& replaceLine = replacements.emplace_back();

        for (const auto& cell : row) {

            if (!cell.textLength() && cell.isLastInRow()) continue;

            auto& replaceCell = replaceLine.emplace_back();
            replaceCell.isLastInRow = cell.isLastInRow();
            replaceCell.isEndOfLine = cell.isEndOfLine();
            if (timestamps.overwrite) {
                const std::string text = cell.text();
                const size_t n = text.find_last_not_of(' ');
                if (n == std::string::npos) continue;
                replaceCell.text = text.substr(0, n + 1);
            }
            else if (cell.isLastInRow() && cell.text().back() != ' ') needsSpace = true;

            const std::string source = cell.trim();
            int64_t counter = 0;

            bool sourceIsCounter  = false;
            if (timestamps.enableFromCounter && stringToCounter(source, counter, fromCounter.unit, settings.decimalSeparatorIsComma ? ',' : '.')) {
                counter += fromEpoch;
                sourceIsCounter = true;
                if (timestamps.fromCounter.type == TimestampSettings::CounterType::Ex00 && counter < -22039776000000000) counter += fromCounter.unit;
            }
            else if (!pi.parseDateText(source, counter, tzFrom)) continue;

            if (action == IDC_TIMESTAMP_TO_DATETIME) {
                std::wstring s = pi.localeWords.formatTimePoint(counter, !sourceIsCounter || fromCounter.leap, tzTo, timestamps.dateFormat,
                                                                timestamps.datePicture.empty() ? std::wstring() : timestamps.datePicture.back());
                replaceCell.text = fromWide(s, pi.codepage);
            }
            else {
                if (sourceIsCounter ? fromCounter.leap != toCounter.leap : !toCounter.leap) {
                    counter = toCounter.leap
                        ? std::chrono::clock_cast<std::chrono::utc_clock>(
                             std::chrono::system_clock::time_point(std::chrono::system_clock::duration(counter))
                             ).time_since_epoch().count()
                        : std::chrono::clock_cast<std::chrono::system_clock>(
                             std::chrono::utc_clock::time_point(std::chrono::utc_clock::duration(counter))
                             ).time_since_epoch().count();
                }
                if (timestamps.toCounter.type == TimestampSettings::CounterType::Ex00 && counter < -22038912000000000) counter -= toCounter.unit;
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
        if (needsSpace) r = ' ';
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
        if (!timestamps.overwrite) {
            if (row.isEndOfLine() && r.empty()) continue;
            if (row.vsMax()) {
                sci.SetTargetRange(row.endOfLine(), row.endOfLine());
                sci.SetTargetStartVirtualSpace(row.vsMax());
                sci.SetTargetEndVirtualSpace(row.vsMax());
            }
            else sci.SetTargetRange(row.cpMax(), row.cpMax());
            sci.ReplaceTarget(r);
        }
        else if (r != row.text()) row.replace(r);
    }

    rs.refit();
    sci.EndUndoAction();

}