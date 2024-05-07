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

#pragma once

#include <algorithm>
#include <charconv>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include "Host\ScintillaTypes.h"
#include "Host\ScintillaMessages.h"
#include "Host\ScintillaStructures.h"
#include "Host\ScintillaCall.h"

namespace NPP {
   #include "Host\PluginInterface.h"
   #include "Host\menuCmdID.h"
}

#undef min
#undef max

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


inline std::wstring GetDlgItemString(HWND hwndDlg, int item) {
    HWND h = GetDlgItem(hwndDlg, item);
    int n = GetWindowTextLength(h);
    if (n <= 0) return L"";
    std::wstring s(n, 0);
    GetWindowText(h, s.data(), n + 1);
    return s;
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


class RectangularSelection;

class RegexCalc;

class UpdateInformation {
public:
    std::string newestURL;
    std::string stableURL;
    long long   timestamp     = 0;
    int         newestVersion = 0;
    int         stableVersion = 0;
    int         thisVersion   = 0;
    enum Check {DoNotCheck, NotifyAny, NotifyStable} check = NotifyStable;
};

class NumericFormat {
public:
    std::string thousands = "";    // thousands separator in current codepage (could be multibyte/utf-8), or "" for no thousands separator
    int         leftPad    = 1;    // left pad to at least this many digits before the decimal separator
    int         minDec     = -1;   // minimum digits after decimal; -1 = do not show separator if no digits after
    int         maxDec     = 6;    // round to this number of digits after the decimal separator
    int         timeEnable = 1;    // bit mask for enabled formats: 8 (4 segments) + 4 (3 segments) + 2 (2 segments) + 1 (1 segment)
};

class NumericParse {
public:
    double value         = std::numeric_limits<double>::quiet_NaN();
    int    decimalPlaces = 0;
    int    timeSegments  = 0;
    operator bool()   const { return isfinite(value); }
    operator double() const { return value; }
};

class SearchSettings {
public:
    enum {Normal = 0, Extended = 1, Regex = 2} mode = Normal;
    bool backward              = false;
    bool wholeWord             = false;
    bool matchCase             = false;
    bool autoClear             = true;
    bool autoClearSelection    = false;
    bool autoSetSelection      = true;
    bool enableCustomIndicator = true;      // assign a custom indicator for column searches
    bool forceUserIndicator    = false;     // when false, allocatedIndicator is used if available; when true, userIndicator is used
    int  indicator             = 18;        // indicator used for searches; any number between 9 and 20 is a custom indicator
    int  customAlpha           = 48;        // transparency for custom indicator
    int  customColor           = 0x007898;  // color for custom indicator
    int  customIndicator       = 18;        // custom indicator number
    int  userIndicator         = 18;        // user-specified custom indicator number
};

class SearchData : public SearchSettings {
public:
    HWND dialog = 0;
    int  dialogHeight;
    int  dialogMinWidth;
    int  dialogButtonLeft;
    int  dialogComboWidth;
    int  allocatedIndicator = 0;               // allocated indicator starting with Notepad++ 8.5.6; otherwise 0
    Scintilla::Position findStep = -1;         // starting position for the last stepwise regex Find, or -1 if not valid for stepwise Replace
    Scintilla::Position nullAt   = -1;         // used to avoid multiple matches to the same null string during incremental Find or Replace
    bool                wrap     = false;      // next stepwise Find/Replace should start at the beginning (or end, if backward) of document
    RECT dialogLastPosition = { 0, 0, 0, 0 };
    std::vector<std::wstring> findHistory;
    std::vector<std::wstring> replaceHistory;
    std::unique_ptr<RegexCalc> regexCalc;
    SearchData();
    ~SearchData();
};

class CsvSettings {
public:
    std::wstring replaceTab = L"(TAB)";
    std::wstring replaceLF  = L"(LF)";
    std::wstring replaceCR  = L"(CR)";
    wchar_t separator  = L',';
    wchar_t escapeChar = L'\\';
    wchar_t encodeTNR  = L'\\';
    wchar_t encodeURL  = L'%';
    bool quote          = true;
    bool apostrophe     = false;
    bool escape         = false;
    bool preserveQuotes = false;
    enum { Replace = 0, TNR = 1, URL = 2 } encodingStyle = Replace;
};

class CalculateSettings {
public:
    std::vector<std::wstring> formulaHistory;
    std::vector<std::wstring> regexHistory;
    enum { None, Comma, Apostrophe, Blank } thousands = None;  // format results with indicated thousands separator
    int  decimalPlaces = 2;         // Add/Average numbers, Calculate: decimal places to which to round result (0-16)
    int  timeSegments  = 0;         // Add/Average numbers:            time segments to show (1-4), or zero for auto
    bool decimalsFixed = false;     // Add/Average numbers, Calculate: don't suppress trailing zeros after the decimal point
    bool autoDecimals  = true;      // Add/Average numbers:            determine number of decimal places to show based on data
    bool insert        = true;      // Add/Average numbers:            insert results when empty space is available
    bool addLine       = false;     // Add/Average numbers:            add a line to insert results when no empty space is available
    bool matchCase     = false;     // Calculate: regular expression: match case
    bool skipUnmatched = false;     // Calculate: regular expression: skip unmatched lines
    bool formatAsTime  = false;     // Calculate: format the results as a time
    bool tabbed        = true;      // Calculate: use a tab to separate the new column
    bool aligned       = true;      // Calculate: pad text inserted in the new column to keep numbers aligned
    bool left          = false;     // Calculate: insert the new column at the left of the selection
};

class SortSettings {
public:
    std::vector<std::wstring> regexHistory;
    std::vector<std::wstring> keygroupHistory;
    std::wstring localeName;
    enum SortType {Binary, Locale, Numeric, Width           } sortType = Binary;
    enum KeyType  {EntireColumn, IgnoreBlanks, Tabbed, Regex} keyType  = EntireColumn;
    bool sortColumnSelectionOnly = false;
    bool sortDescending          = false;
    bool regexMatchCase          = false;
    bool regexUseKey             = false;
    bool localeCaseSensitive     = false;
    bool localeDigitsAsNumbers   = true;
    bool localeIgnoreDiacritics  = false;
    bool localeIgnoreSymbols     = false;
};

class AlignSettings {
public:
    std::vector<std::wstring> history;
    enum AlignOn {First, Last, Regex} alignOn = First;
    int  margin      = 0;
    bool marginRight = false;
    bool matchCase   = false;
};


class TimestampSettings {
public:

    enum class CounterType  { custom, Unix, File, Ex00, Ex04 };
    enum class DatePriority { custom, ymd, mdy, dmy };
    enum class DateFormat   { custom, iso8601, localeLong, localeShort };

    struct CounterSpec {
        int64_t epoch = 0;
        int64_t unit  = 10000000;
        bool    leap  = false;
    };

    struct Counter {
        CounterSpec custom;
        CounterType type = CounterType::Unix;
        const CounterSpec spec() const {
            switch (type) {
            case TimestampSettings::CounterType::Unix: return { 0                  ,     10000000, false };  // 1970-01-01, 1 second
            case TimestampSettings::CounterType::File: return { -116444736000000000,            1, true  };  // 1601-01-01, 100 ns
            case TimestampSettings::CounterType::Ex00: return { -22091616000000000 , 864000000000, false };  // 1899-12-30, 1 day
            case TimestampSettings::CounterType::Ex04: return { -20828448000000000 , 864000000000, false };  // 1904-01-01, 1 day
            }
            return custom;
        }
    };

    Counter      fromCounter;
    Counter      toCounter;
    std::wstring localeName;
    std::wstring fromZone;
    std::wstring toZone;
    std::vector<std::wstring> dateParse;
    std::vector<std::wstring> datePicture;

    DatePriority datePriority       = DatePriority::ymd;
    DateFormat   dateFormat         = DateFormat::iso8601;
    bool         enableFromCounter  = true;
    bool         enableFromDatetime = true;
    bool         enableTzAndLocale  = false;

};


class TabLayoutBlock {
public:
    Scintilla::Line firstLine, lastLine;
    int width;
    std::vector<TabLayoutBlock> right;
    TabLayoutBlock(Scintilla::Line line, int width = 0) : firstLine(line), lastLine(line), width(width) {}
};

class ElasticTabsProfile {
public:
    int minimumOrLeadingTabSize    = 4;
    int minimumSpaceBetweenColumns = 2;
    enum { MonospaceNever = 0, MonospaceAlways = 1, MonospaceBest = 2 } monospace = MonospaceBest;
    bool leadingTabsIndent         = false;
    bool lineUpAll                 = false;
    bool treatEolAsTab             = false;
    bool overrideTabSize           = false;
    bool monospaceNoMnemonics      = true;
    bool operator==(const ElasticTabsProfile&) const = default;
};

class DocumentDataSettings : public ElasticTabsProfile {
public:
    std::wstring profileName;
    bool elasticEnabled = false;
    bool decimalSeparatorIsComma = false;
};

class DocumentData {
public:
    DocumentDataSettings settings;
    std::vector<TabLayoutBlock> tabLayouts;
    UINT_PTR buffer;                              // identifier used by Notepad++ messages and notifications
    int      width24b = 0;                        // the widths of 24 blanks, 24 digits and 24 capital W letters
    int      width24d = 0;                        //     at which tabLayouts were calculated;
    int      width24w = 0;                        //     if these values change, full analysis is required
    int      tabOriginal;                         // set when buffer activated, used for restore when elasticTabsEnabled or overrideTabSize turned off
    bool     elasticAnalysisRequired = false;
    bool     deleteWithoutLayoutChange = false;
    bool     assumeMonospace = false;             // set when assuming all fonts used are monospaced
    Scintilla::Position deleteWithoutLayoutChangePosition;
    Scintilla::Position deleteWithoutLayoutChangeLength;
};

class ColumnsPlusPlusData {
public:

    NPP::NppData              nppData;               // handles exposed by Notepad++
    HINSTANCE                 dllInstance;           // Instance handle of this DLL
    HWND                      activeScintilla;       // Handle to active instance of Scintilla
    Scintilla::FunctionDirect directStatusScintilla; // Scintilla direct function address used with Scintilla::ScintillaCall C++ interface
    intptr_t                  pointerScintilla;      // Scintilla direct function pointer

    Scintilla::ScintillaCall sci;

    int aboutMenuItem;                 // Menu item identifiers of items that can be toggled or otherwise updated.
    int decimalSeparatorMenuItem;
    int elasticEnabledMenuItem;

    std::map<void*       , DocumentData>       documents;
    std::map<std::wstring, ElasticTabsProfile> profiles;
    std::map<std::wstring, std::wstring>       extensionToProfile = { {L"", L"*"}, {L"*", L"*"} };

    UpdateInformation     updateInfo;
    DocumentDataSettings  settings;    // these are the settings for the last active document, or else initial settings
    SearchData            searchData;  // status and settings remembered for the Find/Replace dialog
    CsvSettings           csv;
    CalculateSettings     calc;
    SortSettings          sort;
    AlignSettings         align;
    TimestampSettings     timestamps;
    int  disableOverSize     = 1000;      // active if greater than zero; if negative, inactive and is negative of last used setting   
    int  disableOverLines    = 5000;      // active if greater than zero; if negative, inactive and is negative of last used setting
    int  elasticProgressTime = 2;         // maximum estimated time remaining in seconds to skip progress dialog for slow elastic tabstop operations
    int  timeScalarUnit      = 3;         // time segment as which to interpert a scalar (no colons): 0 = days, 1 = hours, 2 = minutes, 3 = seconds
    int  timePartialRule     = 3;         // interpretation of 2 and 3 segment times: 0 = d:h, d:h:m; 1 = h:m, d:h:m; 2 = h:m, h:m:s; 3 = m:s, h:m:s
    int  timeFormatEnable    = 15;        // bit mask for enabled formats: 8 (4 segments) + 4 (3 segments) + 2 (2 segments) + 1 (1 segment)
    bool showOnMenuBar       = false;     // Show the Columns++ menu on the menu bar instead of the Plugins menu
    bool replaceStaysPut     = false;     // Same as "Replace: Don't move to the following occurrence" in Notepad++
    bool extendSingleLine    = false;     // Extend single line selections to the last line
    bool extendFullLines     = false;     // Extend selections of full lines to the enclosing rectangle
    bool extendZeroWidth     = false;     // Extend zero-width rectangular selections to the right

    Scintilla::Position positionFromLineAndPointX(Scintilla::Line line, int px) {
        // [Char]PositionFromPoint does not work with negative horizontal positions, but PointXFromPosition does
        Scintilla::Position begin = sci.PositionFromLine(line);
        Scintilla::Position end   = sci.LineEndPosition(line);
        Scintilla::Position lastI = begin;
        int                 lastX = px;
        for (Scintilla::Position i = begin; i <= end; i = sci.PositionAfter(i)) {
            int x = sci.PointXFromPosition(i);
            if (x == px) return i;
            if (x > px) return x - px > px - lastX ? lastI : i;
            lastI = i;
            lastX = x;
        }
        return end;
    }

    std::wstring getFilePath() const { return getFilePath(SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0)); }
    std::wstring getFilePath(UINT_PTR buffer) const {
        auto n = SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffer, 0);
        if (n < 1) return L"";
        std::wstring s(n, 0);
        SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffer, (LPARAM)s.data());
        return s;
    }

    std::wstring getFileExtension() const { return getFileExtension(SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0)); }
    std::wstring getFileExtension(UINT_PTR buffer) const { return getFileExtension(getFilePath(buffer)); }
    std::wstring getFileExtension(const std::wstring& filepath) const {
        size_t lastBackslash = filepath.find_last_of(L'\\');
        if (lastBackslash == std::wstring::npos) return L"";
        size_t lastPeriod = filepath.find_last_of(L'.');
        if (lastPeriod == std::wstring::npos || lastPeriod < lastBackslash || lastPeriod == filepath.length() - 1) return L".";
        std::wstring ext = filepath.substr(lastPeriod + 1);
        wcslwr(ext.data());
        return ext;
    }

    DocumentData* getDocument() {
        void* docptr = sci.DocPointer();
        bool isNew = !documents.contains(docptr);
        DocumentData& dd = documents[docptr];
        if (isNew) {
            dd.buffer = SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
            dd.settings = settings;
        }
        return &dd;
    }

    DocumentData* getDocument(HWND scintillaHandle) {
        activeScintilla  = scintillaHandle;
        pointerScintilla = SendMessage(activeScintilla, static_cast<UINT>(Scintilla::Message::GetDirectPointer), 0, 0);
        sci.SetFnPtr(directStatusScintilla, pointerScintilla);
        sci.SetStatus(Scintilla::Status::Ok);  // C-interface code can ignore an error status, which would cause the C++ interface to raise an exception 
        void* docptr = sci.DocPointer();
        if (!documents.contains(docptr)) return 0;
        return &documents[docptr];
    }

    DocumentData* getDocument(const Scintilla::NotificationData* scnp) { return getDocument(reinterpret_cast<HWND>(scnp->nmhdr.hwndFrom)); }

    RectangularSelection getRectangularSelection();

    bool fontSpacingChange(DocumentData& dd) {
        if (dd.width24b != sci.TextWidth(STYLE_DEFAULT, "                        ")) return true;
        if (dd.width24d != sci.TextWidth(STYLE_DEFAULT, "123456789012345678901234")) return true;
        if (dd.width24w != sci.TextWidth(STYLE_DEFAULT, "WWWWWWWWWWWWWWWWWWWWWWWW")) return true;
        return false;
    }

    bool guessMonospaced() {
        int width24b = sci.TextWidth(STYLE_DEFAULT, "                        ");
        int width24w = sci.TextWidth(STYLE_DEFAULT, "WWWWWWWWWWWWWWWWWWWWWWWW");
        if (width24b != width24w) return false;
        for (int style = 0; style < STYLE_DEFAULT; ++style)
            if ( sci.TextWidth(style, "WWWWWWWWWWWWWWWWWWWWWWWW") != width24b
              || sci.TextWidth(style, "                        ") != width24b) return false;
        for (int style = STYLE_LASTPREDEFINED + 1; style < 256; ++style)
            if ( sci.TextWidth(style, "WWWWWWWWWWWWWWWWWWWWWWWW") != width24b
              || sci.TextWidth(style, "                        ") != width24b) return false;
        return true;
    }

    void reselectRectangularSelectionAndControlCharSymbol(DocumentData& dd, bool setControlCharSymbol) {
        if (!dd.settings.elasticEnabled) return;
        Scintilla::SelectionMode selectionMode = sci.SelectionMode();
        if (selectionMode == Scintilla::SelectionMode::Rectangle || selectionMode == Scintilla::SelectionMode::Thin) {
            Scintilla::Position anchor        = sci.RectangularSelectionAnchor();
            Scintilla::Position anchorVirtual = sci.RectangularSelectionAnchorVirtualSpace();
            Scintilla::Position caret         = sci.RectangularSelectionCaret();
            Scintilla::Position caretVirtual  = sci.RectangularSelectionCaretVirtualSpace();
            Scintilla::Line anchorLine        = sci.LineFromPosition(anchor);
            Scintilla::Line caretLine         = sci.LineFromPosition(caret);
            if (setControlCharSymbol) {
                int ccsym = settings.monospaceNoMnemonics && dd.assumeMonospace ? '!' : 0;
                if (sci.ControlCharSymbol() != ccsym) sci.SetControlCharSymbol(ccsym);
            }
            setTabstops(dd, std::min(anchorLine, caretLine), std::max(anchorLine, caretLine));
            sci.SetRectangularSelectionAnchor(anchor);
            sci.SetRectangularSelectionAnchorVirtualSpace(anchorVirtual);
            sci.SetRectangularSelectionCaret(caret);
            sci.SetRectangularSelectionCaretVirtualSpace(caretVirtual);
        }
        else if (setControlCharSymbol) {
            int ccsym = settings.monospaceNoMnemonics && dd.assumeMonospace ? '!' : 0;
            if (sci.ControlCharSymbol() != ccsym) sci.SetControlCharSymbol(ccsym);
        }
    }

    bool searchRegionReady() {
        if (searchData.autoSetSelection) {
            if (sci.SelectionMode() != Scintilla::SelectionMode::Stream) return false;
            if (sci.Selections() > 1) return false;
        }
        if (sci.IndicatorValueAt(searchData.indicator, 0)) return true;
        return sci.IndicatorEnd(searchData.indicator, 0) != 0 && sci.IndicatorEnd(searchData.indicator, 0) != sci.Length();
    }

    void syncFindButton() {
        if (searchData.dialog) {
            bool backward = searchData.mode != SearchData::Regex && searchData.backward;
            HWND findButton = GetDlgItem(searchData.dialog, IDOK);
            if (searchRegionReady() && !searchData.wrap) SetWindowText(findButton, backward ? L"Find Previous" : L"Find Next" );
                                                    else SetWindowText(findButton, backward ? L"Find Last"     : L"Find First");
        }
    }

    int unwrappedWidth(Scintilla::Position from, Scintilla::Position to) {
        int width = 0;
        int xLoc = sci.PointXFromPosition(from);
        int xEnd = sci.PointXFromPosition(to);
        if (sci.WrapMode() != Scintilla::Wrap::None && sci.WrapCount(sci.LineFromPosition(from)) > 1) {
            int yLoc = sci.PointYFromPosition(from);
            int yEnd = sci.PointYFromPosition(to);
            for (Scintilla::Position next = from + 1; yLoc != yEnd; ++next) {
                int yNext = sci.PointYFromPosition(next);
                while (yNext == yLoc) {
                    ++next;
                    yNext = sci.PointYFromPosition(next);
                }
                width += sci.PointXFromPosition(next - 1) - xLoc;
                char lastBeforeWrap = sci.CharacterAt(next - 1);
                char cStringBeforeWrap[] = " ";
                if (lastBeforeWrap) cStringBeforeWrap[0] = lastBeforeWrap;
                width += sci.TextWidth(sci.StyleIndexAt(next - 1), cStringBeforeWrap);
                xLoc = sci.PointXFromPosition(next);
                yLoc = yNext;
            }
        }
        return xEnd - xLoc;
    }

    // ColumnsPlusPlus.cpp

    void analyzeTabstops(DocumentData& dd);
    bool findTabLayoutBlock(DocumentData& dd, Scintilla::Position position, Scintilla::Position length, TabLayoutBlock*& tlb, int& width);
    void setTabstops(DocumentData& dd, Scintilla::Line firstNeeded = -1, Scintilla::Line lastNeeded = -1);

    void bufferActivated();
    void fileClosed     (const NMHDR* nmhdr);
    void fileOpened     (const NMHDR* nmhdr);
    void modifyAll      (const NMHDR* nmhdr);
    void scnModified    (const Scintilla::NotificationData* scnp);
    void scnUpdateUI    (const Scintilla::NotificationData* scnp);
    void scnZoom        (const Scintilla::NotificationData* scnp);

    void toggleDecimalSeparator();
    void toggleElasticEnabled();

    // About.cpp

    BOOL aboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void showAboutDialog();

    // Align.cpp

    void alignLeft();
    void alignRight();
    void alignNumeric();
    void alignCustom();

    // Configuration.cpp

    void initializeBuiltinElasticTabstopsProfiles();
    void loadConfiguration();
    void saveConfiguration();

    // Convert.cpp

    void separatedValuesToTabs();
    void tabsToSeparatedValues();
    void tabsToSpaces();

    // Numeric.cpp

    void accumulate(bool isMean);
    void addNumbers()     { accumulate(false); }
    void averageNumbers() { accumulate(true ); }
    void calculate();

    // NumericFormat.cpp

    std::string  formatNumber(double value, const NumericFormat& format) const;
    NumericParse parseNumber(const std::string& text);
    bool         getNumericAlignment(const std::string& text, size_t& colonPosition, size_t& decimalPosition);

    // Options.cpp

    BOOL optionsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void moveMenuToMenuBar();
    void takeMenuOffMenuBar();
    void showOptionsDialog();

    // Profiles.cpp

    BOOL profileDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void showElasticProfile();

    // Search.cpp

    BOOL searchDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void showSearchDialog();
    void searchCount(bool select = false, bool partial = false, bool before = false);
    void searchFind(bool postReplace = false);
    void searchReplace();
    void searchReplaceAll(bool partial = false, bool before = false);

    // Sort.cpp

    void sortAscendingBinary();
    void sortDescendingBinary();
    void sortAscendingLocale();
    void sortDescendingLocale();
    void sortAscendingNumeric();
    void sortDescendingNumeric();
    void sortCustom();

    // TimeFormats.cpp

    BOOL timeFormatsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void showTimeFormatsDialog();

    // Timestamps.cpp

    void convertTimestamps();

    // Update.cpp

    void getReleases(HWND windowToNotify = 0);

};


// RectangularSelection presents as a container of RectangularSelection_Row
// instances, which in turn contain RectangularSelection_Cell instances.
//
// Note that these are not containers in the usual sense, since
// RectangularSelection_Row and RectangularSelection_Cell instances
// are only constructed when the corresponding iterators are dereferenced.
// 
// The primary purpose of these classes is to gather in one place the logic
// for traversing the selections in a rectangular selection and the columns
// within each line selection, since many commands need to do this.
// 
// Constructors and member functions that are not inline are in Rectangular.cpp.


class RectangularSelection_Row;
class RectangularSelection_Row_Iterator;
class RectangularSelection_Cell;
class RectangularSelection_Cell_Iterator;


class RectangularSelection {
    friend class RectangularSelection_Row;
    friend class RectangularSelection_Row_Iterator;
    friend class RectangularSelection_Cell;
    friend class RectangularSelection_Cell_Iterator;

public:
    struct Corner {
        Scintilla::Position cp, vs, st, en;
        Scintilla::Line ln;
        int sx, px;
    };

private:
    Corner _anchor, _caret;
    int    _size;
    bool   _reverse;
    Scintilla::SelectionMode _mode;

public:

    ColumnsPlusPlusData& data;
    const int blank1440;
    const int tabWidth;

    RectangularSelection(ColumnsPlusPlusData& data);

    Scintilla::SelectionMode mode()        const { return _mode; }
    int                      size()        const { return _size; }
    bool                     leftToRight() const { return _anchor.px <= _caret.px; }
    bool                     topToBottom() const { return _anchor.ln <= _caret.ln; }

    int blankCount(intptr_t n) const { return static_cast<int>(n >= 0 ? (n * 2880 + blank1440) / (2 * blank1440) : (n * 2880 - blank1440) / (2 * blank1440)); }
    int blankWidth(intptr_t n) const { return static_cast<int>(n >= 0 ? (n * blank1440 + 720) / 1440             : (n * blank1440 - 720) / 1440            ); }

    RectangularSelection& natural()         { _reverse = false;                return *this;}
    RectangularSelection& reverse()         { _reverse = !_reverse;            return *this;}
    RectangularSelection& reverse(bool yes) { _reverse = yes == topToBottom(); return *this;}

    RectangularSelection& extend();
    RectangularSelection& refit(bool addLine = false);

    const Corner& anchor() const { return _anchor; }
    const Corner& caret () const { return _caret;  }
    const Corner& top   () const { return topToBottom() ? _anchor : _caret; }
    const Corner& bottom() const { return topToBottom() ? _caret : _anchor; }
    const Corner& left  () const { return leftToRight() ? _anchor : _caret; }
    const Corner& right () const { return leftToRight() ? _caret : _anchor; }

    RectangularSelection_Row_Iterator begin() const;
    RectangularSelection_Row_Iterator end  () const;
    RectangularSelection_Row operator[](int index) const;
    RectangularSelection_Row front() const;
    RectangularSelection_Row back () const;

    Corner corner(Scintilla::Position cp, Scintilla::Position vs) const {
        Corner c;
        c.cp = cp;
        c.vs = vs;
        c.ln = data.sci.LineFromPosition(c.cp);
        c.st = data.sci.PositionFromLine(c.ln);
        c.en = data.sci.LineEndPosition(c.ln);
        c.sx = data.sci.PointXFromPosition(c.st);
        c.px = data.sci.PointXFromPosition(c.cp) - c.sx + blankWidth(c.vs);
        return c;
    }

};

class RectangularSelection_Row {
    friend class RectangularSelection;
    friend class RectangularSelection_Row_Iterator;
    friend class RectangularSelection_Cell;
    friend class RectangularSelection_Cell_Iterator;
    Scintilla::Position _cpAnchor, _cpCaret, _vsAnchor, _vsCaret, _endOfLine;
    Scintilla::Line     _line;
    size_t              _offset;
    std::string         _text;
    RectangularSelection_Row(const RectangularSelection& rs, int index) : rs(rs), index(index) {
        _cpAnchor  = rs.data.sci.SelectionNAnchor(index);
        _cpCaret   = rs.data.sci.SelectionNCaret(index);
        _vsAnchor  = rs.data.sci.SelectionNAnchorVirtualSpace(index);
        _vsCaret   = rs.data.sci.SelectionNCaretVirtualSpace(index);
        _line      = rs.data.sci.LineFromPosition(_cpAnchor);
        _endOfLine = rs.data.sci.LineEndPosition(_line);
        if (rs.data.settings.elasticEnabled && rs.data.settings.leadingTabsIndent) {
            Scintilla::Position startOfLine = rs.data.sci.PositionFromLine(_line);
            _text   = rs.data.sci.StringOfRange(Scintilla::Span(startOfLine, cpMax()));
            _offset = cpMin() - startOfLine;
        }
        else {
            _text   = rs.data.sci.StringOfRange(Scintilla::Span(cpMin(), cpMax()));
            _offset = 0;
        }
    }
public:
    const RectangularSelection& rs;
    const int index;
    Scintilla::Position cpAnchor   () const { return _cpAnchor; }
    Scintilla::Position cpCaret    () const { return _cpCaret ; }
    Scintilla::Position vsAnchor   () const { return _vsAnchor; }
    Scintilla::Position vsCaret    () const { return _vsCaret ; }
    Scintilla::Position cpMin      () const { return std::min(_cpAnchor, _cpCaret); }
    Scintilla::Position cpMax      () const { return std::max(_cpAnchor, _cpCaret); }
    Scintilla::Position vsMin      () const { return std::min(_vsAnchor, _vsCaret); }
    Scintilla::Position vsMax      () const { return std::max(_vsAnchor, _vsCaret); }
    Scintilla::Line     line       () const { return _line; }
    Scintilla::Position endOfLine  () const { return _endOfLine; }
    std::string_view    text       () const { return std::string_view(_text).substr(_offset); }
    bool                isEndOfLine() const { return cpMax() == _endOfLine; }
    RectangularSelection_Cell_Iterator begin() const;
    RectangularSelection_Cell_Iterator end() const;
    void replace(const std::string& r) {
        rs.data.sci.SetTargetRange(cpMin(), cpMax());
        rs.data.sci.ReplaceTarget(r);
        if (vsMin()) {
            rs.data.sci.SetSelectionNAnchor            (index, cpMin() + r.length());
            rs.data.sci.SetSelectionNCaret             (index, cpMin() + r.length());
            rs.data.sci.SetSelectionNAnchorVirtualSpace(index, 0);
            rs.data.sci.SetSelectionNCaretVirtualSpace (index, 0);
        }
        else if (rs.leftToRight()) {
            rs.data.sci.SetSelectionNCaret(index, cpMin() + r.length());
            rs.data.sci.SetSelectionNCaretVirtualSpace(index, 0);
        }
        else {
            rs.data.sci.SetSelectionNAnchor(index, cpMin() + r.length());
            rs.data.sci.SetSelectionNAnchorVirtualSpace(index, 0);
        }
    }
};

class RectangularSelection_Cell {
    friend class RectangularSelection;
    friend class RectangularSelection_Row;
    friend class RectangularSelection_Row_Iterator;
    friend class RectangularSelection_Cell_Iterator;
    size_t _start;
    size_t _left;
    size_t _right;
    size_t _end;
    size_t _pastLeadingTabs;
    RectangularSelection_Cell(const RectangularSelection_Row& row, size_t start, size_t& next) : row(row), _start(start) {
        if (_start >= row._text.length()) {
            _start = _left = _right = _end = _pastLeadingTabs = row._text.length();
            next = std::string::npos;
            return;
        }
        _pastLeadingTabs = _start;
        if (_start == row._offset && row.rs.data.settings.elasticEnabled && row.rs.data.settings.leadingTabsIndent) {
            size_t firstNonTab = row._text.find_first_not_of('\t');
            if (firstNonTab > _start) _pastLeadingTabs = firstNonTab == std::string::npos ? row._text.length() : firstNonTab;
        }
        _end = row._text.find_first_of('\t', _pastLeadingTabs);
        if (_end == std::string::npos) {
            _end = row._text.length();
            next = std::string::npos;
        }
        else next = _end + 1;
        if (_end == _pastLeadingTabs) {
            _left = _right = _end;
            return;
        }
        _left = row._text.find_first_not_of(' ', _pastLeadingTabs);
        if (_left >= _end) {
            _left = _right = _end;
            return;
        }
        _right = row._text.find_last_not_of(' ', _end - 1) + 1;
    }
public:
    const RectangularSelection_Row& row;
    std::string         text        () const { return row._text.substr(_start, _end   - _start); }
    std::string         trim        () const { return row._text.substr(_left , _right - _left ); }
    std::string         terminator  () const { return isLastInRow() ? "" : "\t"; }
    intptr_t            textLength  () const { return _end   - _start; }
    intptr_t            trimLength  () const { return _right - _left ; }
    intptr_t            leading     () const { return (_pastLeadingTabs - _start) * row.rs.tabWidth + _left - _pastLeadingTabs; }
    intptr_t            trailing    () const { return _end - _right; }
    Scintilla::Position start       () const { return row.cpMin() - row._offset + _start; }
    Scintilla::Position left        () const { return row.cpMin() - row._offset + _left;  }
    Scintilla::Position right       () const { return row.cpMin() - row._offset + _right; }
    Scintilla::Position end         () const { return row.cpMin() - row._offset + _end;   }
    bool                isLastInRow () const { return _end == row._text.length(); }
    bool                isEndOfLine () const { return end() == row._endOfLine; }
};

class RectangularSelection_Row_Iterator {
    friend class RectangularSelection;
    friend class RectangularSelection_Row;
    friend class RectangularSelection_Cell;
    friend class RectangularSelection_Cell_Iterator;
    const RectangularSelection& rs;
    int index;
    RectangularSelection_Row_Iterator(const RectangularSelection& rs, int index) : rs(rs), index(index) {}
public:
    bool operator!=(const RectangularSelection_Row_Iterator& rsri) { return index != rsri.index; }
    RectangularSelection_Row_Iterator& operator++() { rs._reverse ? --index : ++index; return *this; }
    RectangularSelection_Row operator*() { return RectangularSelection_Row(rs, index);}
};

class RectangularSelection_Cell_Iterator {
    friend class RectangularSelection;
    friend class RectangularSelection_Row;
    friend class RectangularSelection_Row_Iterator;
    friend class RectangularSelection_Cell;
    const RectangularSelection_Row& row;
    size_t start, next;
    RectangularSelection_Cell_Iterator(const RectangularSelection_Row& row, size_t start) : row(row), start(start), next(start) {}
public:
    bool operator!=(const RectangularSelection_Cell_Iterator& rsci) { return start != rsci.start; }
    RectangularSelection_Cell_Iterator& operator++() {
        if (next == start) RectangularSelection_Cell(row, start, next);
        start = next;
        return *this;
    }
    RectangularSelection_Cell operator*() {return RectangularSelection_Cell(row, start, next);}
};

inline RectangularSelection_Row_Iterator RectangularSelection::begin() const {
    return RectangularSelection_Row_Iterator(*this, _reverse ?_size - 1 : 0);
}
inline RectangularSelection_Row_Iterator RectangularSelection::end() const {
    return RectangularSelection_Row_Iterator(*this, _reverse ? -1 : _size);
}
inline RectangularSelection_Row RectangularSelection::operator[](int index) const {
    return RectangularSelection_Row(*this, index);
}
inline RectangularSelection_Row RectangularSelection::front() const { return (*this)[_reverse ? _size - 1 : 0]; }
inline RectangularSelection_Row RectangularSelection::back () const { return (*this)[_reverse ? 0 : _size - 1]; }

inline RectangularSelection_Cell_Iterator RectangularSelection_Row::begin() const {
    return RectangularSelection_Cell_Iterator(*this, _offset);
}
inline RectangularSelection_Cell_Iterator RectangularSelection_Row::end() const {
    return RectangularSelection_Cell_Iterator(*this, std::string::npos);
}

inline RectangularSelection ColumnsPlusPlusData::getRectangularSelection() {
    return RectangularSelection(*this).extend();
}
