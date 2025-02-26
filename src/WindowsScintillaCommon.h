// This file is part of Columns++ for Notepad++.
// Copyright 2025 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

// The Columns++ source code contained in this file is independent of Notepad++ code.
// It is released under the MIT (Expat) license:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial 
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// This file contains Windows, Scintilla and C++ standard library includes, and some helper functions,
// which are used throughout Columns++.

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include "Host\ScintillaTypes.h"
#include "Host\ScintillaMessages.h"
#include "Host\ScintillaStructures.h"
#include "Host\ScintillaCall.h"


template< class T, class S > constexpr const T clamp_cast(const S& v) {
    using s = std::numeric_limits<S>;
    using t = std::numeric_limits<T>;
    if constexpr (!s::is_integer || !t::is_integer) return v;
    else if constexpr (s::is_signed == t::is_signed)
        if constexpr (t::max() >= s::max())return static_cast<T>(v);
        else return static_cast<T>(std::clamp(v, static_cast<S>(t::min()), static_cast<S>(t::max())));
    else if constexpr (s::is_signed)
        if constexpr (t::max() >= static_cast<uintmax_t>(s::max())) return static_cast<T>(std::max(v, static_cast<S>(0)));
        else return static_cast<T>(std::clamp(v, static_cast<S>(0), static_cast<S>(t::max())));
    else if constexpr (static_cast<uintmax_t>(t::max()) >= s::max()) return static_cast<T>(v);
    else return static_cast<T>(std::min(v, static_cast<S>(t::max())));
}


inline std::string fromWide(std::wstring_view s, unsigned int codepage) {
    std::string r;
    size_t inputLength = s.length();
    if (!inputLength) return r;
    constexpr unsigned int safeSize = std::numeric_limits<int>::max() / 8;
    size_t workingPoint = 0;
    while (inputLength - workingPoint > safeSize) {
        int ss = safeSize;
        if (s[ss - 1] >= 0xD800 && s[ss - 1] <= 0xDBFF) --ss;  // proposed block ends with high surrogate: leave it for the next block
        int segmentLength = WideCharToMultiByte(codepage, 0, s.data() + workingPoint, ss, 0, 0, 0, 0);
        size_t outputPoint = r.length();
        r.resize(outputPoint + segmentLength);
        WideCharToMultiByte(codepage, 0, s.data() + workingPoint, ss, r.data() + outputPoint, segmentLength, 0, 0);
        workingPoint += ss;
    }
    int segmentLength = WideCharToMultiByte(codepage, 0, s.data() + workingPoint, static_cast<int>(inputLength - workingPoint), 0, 0, 0, 0);
    size_t outputPoint = r.length();
    r.resize(outputPoint + segmentLength);
    WideCharToMultiByte(codepage, 0, s.data() + workingPoint, static_cast<int>(inputLength - workingPoint), r.data() + outputPoint, segmentLength, 0, 0);
    return r;
}


inline std::wstring toWide(std::string_view s, unsigned int codepage) {
    std::wstring r;
    size_t inputLength = s.length();
    if (!inputLength) return r;
    constexpr unsigned int safeSize = std::numeric_limits<int>::max() / 2;
    size_t workingPoint = 0;
    while (inputLength - workingPoint > safeSize) {
        int ss = safeSize;
        if (codepage == CP_UTF8 && ((s[ss] & 0xC0) == 0x80)) while ((s[--ss] & 0xC0) == 0x80);  // find a first byte to start the next block
        int segmentLength = MultiByteToWideChar(codepage, 0, s.data() + workingPoint, ss, 0, 0);
        size_t outputPoint = r.length();
        r.resize(outputPoint + segmentLength);
        MultiByteToWideChar(codepage, 0, s.data() + workingPoint, ss, r.data() + outputPoint, segmentLength);
        workingPoint += ss;
    }
    int segmentLength = MultiByteToWideChar(codepage, 0, s.data() + workingPoint, static_cast<int>(inputLength - workingPoint), 0, 0);
    size_t outputPoint = r.length();
    r.resize(outputPoint + segmentLength);
    MultiByteToWideChar(codepage, 0, s.data() + workingPoint, static_cast<int>(inputLength - workingPoint), r.data() + outputPoint, segmentLength);
    return r;
}


inline std::wstring GetWindowString(HWND hWnd) {
    std::wstring s(GetWindowTextLength(hWnd), 0);
    if (!s.empty()) s.resize(GetWindowText(hWnd, s.data(), static_cast<int>(s.length() + 1)));
    return s;
}

inline std::wstring GetDlgItemString(HWND hwndDlg, int item) {
    return GetWindowString(GetDlgItem(hwndDlg, item));
}


inline void showBalloonTip(HWND hwndDlg, int control, const std::wstring& text, bool combobox = false) {
    HWND hControl = GetDlgItem(hwndDlg, control);
    if (combobox) {
        COMBOBOXINFO cbi;
        cbi.cbSize = sizeof(COMBOBOXINFO);
        GetComboBoxInfo(hControl, &cbi);
        hControl = cbi.hwndItem;
    }
    EDITBALLOONTIP ebt;
    ebt.cbStruct = sizeof(EDITBALLOONTIP);
    ebt.pszTitle = L"";
    ebt.ttiIcon = TTI_NONE;
    ebt.pszText = text.data();
    SendMessage(hControl, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
    SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(hControl), TRUE);
}


inline std::wstring updateComboHistory(HWND dialog, int control, std::vector<std::wstring>& history) {
    HWND h = GetDlgItem(dialog, control);
    auto n = GetWindowTextLength(h);
    std::wstring s(n, 0);
    s.resize(GetWindowText(h, s.data(), n + 1));
    if (history.empty() || history.back() != s) {
        if (!history.empty()) {
            auto existing = std::find(history.begin(), history.end(), s);
            if (existing != history.end()) {
                SendMessage(h, CB_DELETESTRING, history.end() - existing - 1, 0);
                history.erase(existing);
            }
        }
        SendMessage(h, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(s.data()));
        SendMessage(h, CB_SETCURSEL, 0, 0);
        history.push_back(s);
    }
    return s;
}


inline bool validateSpin(int& value, HWND hwndDlg, int control, const wchar_t* message) {
    BOOL error;
    int n = static_cast<int>(SendDlgItemMessage(hwndDlg, control, UDM_GETPOS32, 0, reinterpret_cast<LPARAM>(&error)));
    if (error) {
        HWND edit = reinterpret_cast<HWND>(SendDlgItemMessage(hwndDlg, control, UDM_GETBUDDY, 0, 0));
        EDITBALLOONTIP ebt;
        ebt.cbStruct = sizeof(EDITBALLOONTIP);
        ebt.pszTitle = L"";
        ebt.ttiIcon = TTI_NONE;
        ebt.pszText = message;
        SendMessage(edit, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
        SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(edit), TRUE);
        return false;
    }
    value = n;
    return true;
}
