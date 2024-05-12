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

// Configuration level 1 - 0.0.0.1 - initial releases
// Configuration level 2 - 0.1.0.5 - reset extendX values to new default, false, since we now offer a dialog to extend selections
// Configuration level 3 - 0.5     - moved former calculateInsert, calculateAddLine and thousandsSeparator from LastSettings to Calculate section
// Configuration level 4 - 0.6.1   - Search section now uses enableCustomIndicator and forceUserIndicator instead of negative custom(user)Indicator
// Configuration level 5 - 0.7     - Removed Calculate: unitIsMinutes and showDays, added timeSegments and autoDecimals;
//                                   added LastSettings: timeScalarUnit, timePartialRule, timeFormatEnable; Search: autoClearSelection, autoSetSelection


#include "ColumnsPlusPlus.h"
#include <fstream>
#include <iostream>
#include <regex>
#include "Shlwapi.h"

static const std::regex configurationHeader("(?:\\xEF\\xBB\\xBF)?\\s*Notepad\\+\\+\\s+Columns\\+\\+\\s+Configuration\\s+(\\d+)\\s+(\\d+)\\s*", std::regex::icase | std::regex::optimize);
static const std::regex lastSettingsHeader("\\s*LastSettings\\s*", std::regex::icase | std::regex::optimize);
static const std::regex profileHeader("\\s*Profile\\s+(.*\\S)\\s*", std::regex::icase | std::regex::optimize);
static const std::regex extensionsHeader("\\s*Extensions\\s*", std::regex::icase | std::regex::optimize);
static const std::regex calcHeader("\\s*Calculate\\s*", std::regex::icase | std::regex::optimize);
static const std::regex searchHeader("\\s*Search\\s*", std::regex::icase | std::regex::optimize);
static const std::regex sortHeader("\\s*Sort\\s*", std::regex::icase | std::regex::optimize);
static const std::regex alignHeader("\\s*Align\\s*", std::regex::icase | std::regex::optimize);
static const std::regex timestampsHeader("\\s*Timestamps\\s*", std::regex::icase | std::regex::optimize);
static const std::regex updateHeader("\\s*Update\\s*", std::regex::icase | std::regex::optimize);
static const std::regex dataLine("\\s*(\\S+)\\s+(.*\\S)\\s*", std::regex::optimize);
static const std::regex integerValue("[+-]?\\d{1,20}", std::regex::optimize);

static std::basic_string<TCHAR> filePath;


std::wstring decodeDelimitedString(const std::string value) {
    if (value.length() > 2 && value.front() == '\\' && value.back() == '\\') {
        std::wstring s = toWide(value.substr(1, value.length() - 2), CP_UTF8);
        for (size_t i = 0; i < s.length() - 1; ++i) if (s[i] == L'\\') switch (s[i + 1]) {
        case L'r': s.replace(i, 2, L"\r"); break;
        case L'n': s.replace(i, 2, L"\n"); break;
        case L'\\': s.replace(i, 2, L"\\"); break;
        default:;
        }
        return s;
    }
    return L"";
}

std::string encodeDelimitedString(std::wstring s) {
    for (size_t i = 0; i < s.length(); ++i) switch (s[i]) {
    case L'\r': s.replace(i, 1, L"\\r" ); ++i; break;
    case L'\n': s.replace(i, 1, L"\\n" ); ++i; break;
    case L'\\': s.replace(i, 1, L"\\\\"); ++i; break;
    default:;
    }
    return "\\" + fromWide(s, CP_UTF8) + "\\";
}

void writeDelimitedStringHistory(std::ofstream& file, std::string key, std::vector<std::wstring> history) {
    for (auto it = history.size() > 16 ? history.end() - 16 : history.begin(); it != history.end(); ++it)
        file << key << "\t" << encodeDelimitedString(*it) << std::endl;
}


void ColumnsPlusPlusData::loadConfiguration() {

    TCHAR pluginsConfigDirectory[MAX_PATH];

    SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)pluginsConfigDirectory);
    if (PathFileExists(pluginsConfigDirectory) == FALSE)
        if (!CreateDirectory(pluginsConfigDirectory, NULL)) return;
    filePath = pluginsConfigDirectory;
    filePath += TEXT("\\ColumnsPlusPlus.data");

    std::ifstream file(filePath);
    if (!file) return;

    std::string line;
    std::getline(file, line);
    if (!file) return;

    std::smatch match;
    if (!std::regex_match(line, match, configurationHeader)) return;
    int configLevel  = std::stoi(match[1]);
    int configCompat = std::stoi(match[2]);
    if (configCompat > 5) return;

    enum { sectionNone, sectionLastSettings, sectionCalc, sectionSearch, sectionSort, sectionAlign, sectionTimestamps, 
           sectionUpdate, sectionProfile, sectionExtensions }
         readingSection = sectionNone;
    std::wstring profileName;

    while (file) {
        std::getline(file, line);
        if      (std::regex_match(line, match, lastSettingsHeader)) readingSection = sectionLastSettings;
        else if (std::regex_match(line, match, calcHeader        )) readingSection = sectionCalc;
        else if (std::regex_match(line, match, searchHeader      )) readingSection = sectionSearch;
        else if (std::regex_match(line, match, sortHeader        )) readingSection = sectionSort;
        else if (std::regex_match(line, match, alignHeader       )) readingSection = sectionAlign;
        else if (std::regex_match(line, match, timestampsHeader  )) readingSection = sectionTimestamps;
        else if (std::regex_match(line, match, updateHeader      )) readingSection = sectionUpdate;
        else if (std::regex_match(line, match, extensionsHeader  )) readingSection = sectionExtensions;
        else if (std::regex_match(line, match, profileHeader     )) {
            readingSection = sectionProfile;
            profileName = toWide(match.str(1), CP_UTF8);
        }
        else if (std::regex_match(line, match, dataLine)) {
            if (readingSection == sectionLastSettings) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "name"                      ) settings.profileName             = toWide(value, CP_UTF8);
                else if (setting == "elasticenabled"            ) settings.elasticEnabled          = value != "0";
                else if (setting == "decimalseparatoriscomma"   ) settings.decimalSeparatorIsComma = value != "0";
                else if (setting == "leadingtabsindent"         ) settings.leadingTabsIndent       = value != "0";
                else if (setting == "lineupall"                 ) settings.lineUpAll               = value != "0";
                else if (setting == "treateolastab"             ) settings.treatEolAsTab           = value != "0";
                else if (setting == "overridetabsize"           ) settings.overrideTabSize         = value != "0";
                else if (setting == "monospacenomnemonics"      ) settings.monospaceNoMnemonics    = value != "0";
                else if (setting == "showonmenubar"             ) showOnMenuBar                    = value != "0";
                else if (setting == "replacestaysput"           ) replaceStaysPut                  = value != "0";
                else if (setting == "csvquote"                  ) csv.quote                        = value != "0";
                else if (setting == "csvapostrophe"             ) csv.apostrophe                   = value != "0";
                else if (setting == "csvescape"                 ) csv.escape                       = value != "0";
                else if (setting == "csvpreservequotes"         ) csv.preserveQuotes               = value != "0";
                else if (setting == "extendsingleline"          ) {if (configLevel > 1) extendSingleLine = value != "0";}
                else if (setting == "extendfulllines"           ) {if (configLevel > 1) extendFullLines  = value != "0";}
                else if (setting == "extendzerowidth"           ) {if (configLevel > 1) extendZeroWidth  = value != "0";}
                else if (setting == "csvseparator" ) { auto s = decodeDelimitedString(value); if (s.length() == 1) csv.separator  = s[0]; }
                else if (setting == "csvescapechar") { auto s = decodeDelimitedString(value); if (s.length() == 1) csv.escapeChar = s[0]; }
                else if (setting == "csvencodetnr" ) { auto s = decodeDelimitedString(value); if (s.length() == 1) csv.encodeTNR  = s[0]; }
                else if (setting == "csvencodeurl" ) { auto s = decodeDelimitedString(value); if (s.length() == 1) csv.encodeURL  = s[0]; }
                else if (setting == "csvreplacetab") csv.replaceTab = decodeDelimitedString(value);
                else if (setting == "csvreplacelf" ) csv.replaceLF  = decodeDelimitedString(value);
                else if (setting == "csvreplacecr" ) csv.replaceCR  = decodeDelimitedString(value);
                else if (setting == "monospace") {
                    strlwr(value.data());
                    settings.monospace = value == "yes" ? ElasticTabsProfile::MonospaceAlways
                                       : value == "no"  ? ElasticTabsProfile::MonospaceNever
                                                        : ElasticTabsProfile::MonospaceBest;
                }
                else if (setting == "csvencodingstyle") {
                    strlwr(value.data());
                    csv.encodingStyle = value == "backslash" ? CsvSettings::TNR
                                      : value == "url"       ? CsvSettings::URL
                                                             : CsvSettings::Replace;
                }
                else if (std::regex_match(value, integerValue)) {
                    if      (setting == "minimumorleadingtabsize"   ) settings.minimumOrLeadingTabSize    = std::stoi(value);
                    else if (setting == "minimumspacebetweencolumns") settings.minimumSpaceBetweenColumns = std::stoi(value);
                    else if (setting == "disableoversize"           ) disableOverSize                     = std::stoi(value);
                    else if (setting == "disableoverlines"          ) disableOverLines                    = std::stoi(value);
                    else if (setting == "elasticprogresstime"       ) elasticProgressTime                 = std::stoi(value);
                    else if (setting == "timescalarunit"            ) timeScalarUnit                      = std::stoi(value);
                    else if (setting == "timepartialrule"           ) timePartialRule                     = std::stoi(value);
                    else if (setting == "timeformatenable"          ) timeFormatEnable                    = std::stoi(value);
                }
            }
            else if (readingSection == sectionCalc) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "insert"       ) calc.insert        = value != "0";
                else if (setting == "addline"      ) calc.addLine       = value != "0";
                else if (setting == "matchcase"    ) calc.matchCase     = value != "0";
                else if (setting == "skipunmatched") calc.skipUnmatched = value != "0";
                else if (setting == "decimalsfixed") calc.decimalsFixed = value != "0";
                else if (setting == "formatastime" ) calc.formatAsTime  = value != "0";
                else if (setting == "tabbed"       ) calc.tabbed        = value != "0";
                else if (setting == "aligned"      ) calc.aligned       = value != "0";
                else if (setting == "left"         ) calc.left          = value != "0";
                else if (setting == "autodecimals" ) calc.autoDecimals  = value != "0";
                else if (setting == "formula"      ) calc.formulaHistory.push_back(decodeDelimitedString(value));
                else if (setting == "regex"        ) calc.regexHistory.push_back(decodeDelimitedString(value));
                else if (setting == "thousands") {
                    strlwr(value.data());
                    calc.thousands = value == "comma"      ? CalculateSettings::Comma
                                   : value == "period"     ? CalculateSettings::Comma
                                   : value == "apostrophe" ? CalculateSettings::Apostrophe
                                   : value == "blank"      ? CalculateSettings::Blank
                                                           : CalculateSettings::None;
                }
                else if (std::regex_match(value, integerValue)) {
                    if (setting == "decimalplaces") calc.decimalPlaces = std::stoi(value);
                    if (setting == "timesegments" ) calc.timeSegments  = std::stoi(value);
                }
            }
            else if (readingSection == sectionSearch) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "backward"             ) searchData.backward              = value != "0";
                else if (setting == "wholeword"            ) searchData.wholeWord             = value != "0";
                else if (setting == "matchcase"            ) searchData.matchCase             = value != "0";
                else if (setting == "autoclear"            ) searchData.autoClear             = value != "0";
                else if (setting == "autoclearselection"   ) searchData.autoClearSelection    = value != "0";
                else if (setting == "autosetselection"     ) searchData.autoSetSelection      = value != "0";
                else if (setting == "enablecustomindicator") searchData.enableCustomIndicator = value != "0";
                else if (setting == "forceuserindicator"   ) searchData.forceUserIndicator    = value != "0";
                else if (setting == "find"                 ) searchData.findHistory.push_back(decodeDelimitedString(value));
                else if (setting == "replace"              ) searchData.replaceHistory.push_back(decodeDelimitedString(value));
                else if (std::regex_match(value, integerValue)) {
                    if (setting == "mode") {
                        int i = std::stoi(value);
                        searchData.mode = i == SearchSettings::Extended ? SearchSettings::Extended
                                        : i == SearchSettings::Regex    ? SearchSettings::Regex
                                                                        : SearchSettings::Normal;
                    }
                    else if (setting == "indicator"      ) searchData.indicator     = std::stoi(value);
                    else if (setting == "customalpha"    ) searchData.customAlpha   = std::stoi(value);
                    else if (setting == "customcolor"    ) searchData.customColor   = std::stoi(value);
                    else if (setting == "customindicator") {
                        searchData.userIndicator = std::stoi(value);
                        if (searchData.userIndicator < 0) /* legacy format from before version 4 */ {
                            searchData.userIndicator         = -searchData.userIndicator;
                            searchData.enableCustomIndicator = false;
                        }
                    }
                }
            }
            else if (readingSection == sectionSort) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "columnselectiononly"   ) sort.sortColumnSelectionOnly = value != "0";
                else if (setting == "descending"            ) sort.sortDescending          = value != "0";
                else if (setting == "regexmatchcase"        ) sort.regexMatchCase          = value != "0";
                else if (setting == "regexusekey"           ) sort.regexUseKey             = value != "0";
                else if (setting == "localecasesensitive"   ) sort.localeCaseSensitive     = value != "0";
                else if (setting == "localedigitsasnumbers" ) sort.localeDigitsAsNumbers   = value != "0";
                else if (setting == "localeignorediacritics") sort.localeIgnoreDiacritics  = value != "0";
                else if (setting == "localeignoresymbols"   ) sort.localeIgnoreSymbols     = value != "0";
                else if (setting == "localename"            ) sort.localeName              = toWide(value, CP_UTF8);
                else if (setting == "regex"                 ) sort.regexHistory   .push_back(decodeDelimitedString(value));
                else if (setting == "keys"                  ) sort.keygroupHistory.push_back(decodeDelimitedString(value));
                else if (setting == "sorttype") {
                    strlwr(value.data());
                    sort.sortType = value == "locale"  ? SortSettings::Locale
                                  : value == "numeric" ? SortSettings::Numeric
                                  : value == "width"   ? SortSettings::Width
                                                       : SortSettings::Binary;
                }
                else if (setting == "keytype") {
                    strlwr(value.data());
                    sort.keyType = value == "ignoreblanks" ? SortSettings::IgnoreBlanks
                                 : value == "tabbed"       ? SortSettings::Tabbed
                                 : value == "regex"        ? SortSettings::Regex
                                                           : SortSettings::EntireColumn;
                }
            }
            else if (readingSection == sectionAlign) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "marginright") align.marginRight = value != "0";
                else if (setting == "matchcase"  ) align.matchCase   = value != "0";
                else if (setting == "history"    ) align.history.push_back(decodeDelimitedString(value));
                else if (setting == "alignon") {
                    strlwr(value.data());
                    align.alignOn = value == "last"  ? AlignSettings::Last
                                  : value == "regex" ? AlignSettings::Regex
                                                     : AlignSettings::First;
                }
                else if (std::regex_match(value, integerValue)) {
                    if (setting == "margin") align.margin = std::stoi(value);
                }
            }
            else if (readingSection == sectionTimestamps) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "fromcounter"  ) timestamps.enableFromCounter       = value != "0";
                else if (setting == "fromdate"     ) timestamps.enableFromDatetime      = value != "0";
                else if (setting == "fromleap"     ) timestamps.fromCounter.custom.leap = value != "0";
                else if (setting == "toleap"       ) timestamps.toCounter  .custom.leap = value != "0";
                else if (setting == "zoneandlocale") timestamps.enableTzAndLocale       = value != "0";
                else if (setting == "overwrite"    ) timestamps.overwrite               = value != "0";
                else if (setting == "fromzone"     ) timestamps.fromZone                = toWide(value, CP_UTF8);
                else if (setting == "tozone"       ) timestamps.toZone                  = toWide(value, CP_UTF8);
                else if (setting == "localename"   ) timestamps.localeName              = toWide(value, CP_UTF8);
                else if (setting == "parse"        ) timestamps.dateParse  .push_back(decodeDelimitedString(value));
                else if (setting == "picture"      ) timestamps.datePicture.push_back(decodeDelimitedString(value));
                else if (setting == "fromtype") {
                    strlwr(value.data());
                    timestamps.fromCounter.type = value == "unix" ? TimestampSettings::CounterType::Unix
                                                : value == "file" ? TimestampSettings::CounterType::File
                                                : value == "1900" ? TimestampSettings::CounterType::Ex00
                                                : value == "1904" ? TimestampSettings::CounterType::Ex04
                                                                  : TimestampSettings::CounterType::custom;
                }
                else if (setting == "totype") {
                    strlwr(value.data());
                    timestamps.toCounter.type = value == "unix" ? TimestampSettings::CounterType::Unix
                                              : value == "file" ? TimestampSettings::CounterType::File
                                              : value == "1900" ? TimestampSettings::CounterType::Ex00
                                              : value == "1904" ? TimestampSettings::CounterType::Ex04
                                                                : TimestampSettings::CounterType::custom;
                }
                else if (setting == "priority") {
                    strlwr(value.data());
                    timestamps.datePriority = value == "ymd" ? TimestampSettings::DatePriority::ymd
                                            : value == "mdy" ? TimestampSettings::DatePriority::mdy
                                            : value == "dmy" ? TimestampSettings::DatePriority::dmy
                                                             : TimestampSettings::DatePriority::custom;
                }
                else if (setting == "format") {
                    strlwr(value.data());
                    timestamps.dateFormat = value == "iso"   ? TimestampSettings::DateFormat::iso8601
                                          : value == "short" ? TimestampSettings::DateFormat::localeShort
                                          : value == "long"  ? TimestampSettings::DateFormat::localeLong
                                                             : TimestampSettings::DateFormat::custom;
                }
                else if (std::regex_match(value, integerValue)) {
                    if (setting == "fromepoch"    ) timestamps.fromCounter.custom.epoch = std::stoll(value);
                    if (setting == "fromunit"     ) timestamps.fromCounter.custom.unit  = std::stoll(value);
                    if (setting == "toepoch"      ) timestamps.toCounter  .custom.epoch = std::stoll(value);
                    if (setting == "tounit"       ) timestamps.toCounter  .custom.unit  = std::stoll(value);
                    if (setting == "twodigitlimit") timestamps.twoDigitYearLimit        = std::stoi(value);
                }
            }
            else if (readingSection == sectionUpdate) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "newesturl") updateInfo.newestURL = value;
                else if (setting == "stableurl") updateInfo.stableURL = value;
                else if (setting == "check") {
                    strlwr(value.data());
                    updateInfo.check = value == "any"    ? UpdateInformation::NotifyAny
                                     : value == "none"   ? UpdateInformation::DoNotCheck
                                                         : UpdateInformation::NotifyStable;
                }
                else if (std::regex_match(value, integerValue)) {
                    if      (setting == "newestversion") updateInfo.newestVersion = std::stoi(value);
                    else if (setting == "stableversion") updateInfo.stableVersion = std::stoi(value);
                    else if (setting == "timestamp"    ) updateInfo.timestamp     = std::stoll(value);
                }
            }
            else if (readingSection == sectionProfile) {
                std::string setting = match[1];
                std::string value = match[2];
                strlwr(setting.data());
                if      (setting == "leadingtabsindent"         ) profiles[profileName].leadingTabsIndent       = value != "0";
                else if (setting == "lineupall"                 ) profiles[profileName].lineUpAll               = value != "0";
                else if (setting == "treateolastab"             ) profiles[profileName].treatEolAsTab           = value != "0";
                else if (setting == "overridetabsize"           ) profiles[profileName].overrideTabSize         = value != "0";
                else if (setting == "monospacenomnemonics"      ) profiles[profileName].monospaceNoMnemonics    = value != "0";
                else if (setting == "monospace") {
                    strlwr(value.data());
                    profiles[profileName].monospace = value == "yes" ? ElasticTabsProfile::MonospaceAlways
                                                    : value == "no"  ? ElasticTabsProfile::MonospaceNever
                                                                     : ElasticTabsProfile::MonospaceBest;
                }
                else if (std::regex_match(value, integerValue)) {
                    if      (setting == "minimumorleadingtabsize"   ) profiles[profileName].minimumOrLeadingTabSize    = std::stoi(value);
                    else if (setting == "minimumspacebetweencolumns") profiles[profileName].minimumSpaceBetweenColumns = std::stoi(value);
                }
            }
            else if (readingSection == sectionExtensions) {
                std::wstring extension = toWide(match.str(1), CP_UTF8);
                std::wstring profile   = toWide(match.str(2), CP_UTF8);
                if      (extension == L"new")     extensionToProfile[L"" ] = profile;
                else if (extension == L"none")    extensionToProfile[L"."] = profile;
                else if (extension == L"default") extensionToProfile[L"*"] = profile;
                else if (extension.length() > 1 && extension[0] == L'.') {
                    extension = extension.substr(1);
                    std::replace(extension.begin(), extension.end(), L'.', L' ');
                    extensionToProfile[extension] = profile == L"(disable)" ? L"" : profile;
                }
            }
        }
    }

}


void ColumnsPlusPlusData::saveConfiguration() {

    std::ofstream file(filePath);
    if (!file) return;

    file << "\xEF\xBB\xBF" << "Notepad++ Columns++ Configuration 5 1" << std::endl;

    file << std::endl << "LastSettings" << std::endl << std::endl;

    file << "elasticEnabled\t"              << settings.elasticEnabled                 << std::endl;
    file << "decimalSeparatorIsComma\t"     << settings.decimalSeparatorIsComma        << std::endl;
    file << "name\t"                        << fromWide(settings.profileName, CP_UTF8) << std::endl;
    file << "minimumOrLeadingTabSize\t"     << settings.minimumOrLeadingTabSize        << std::endl;
    file << "minimumSpaceBetweenColumns\t"  << settings.minimumSpaceBetweenColumns     << std::endl;
    file << "leadingTabsIndent\t"           << settings.leadingTabsIndent              << std::endl;
    file << "lineUpAll\t"                   << settings.lineUpAll                      << std::endl;
    file << "treatEolAsTab\t"               << settings.treatEolAsTab                  << std::endl;
    file << "overrideTabSize\t"             << settings.overrideTabSize                << std::endl;
    file << "monospace\t" << ( settings.monospace == ElasticTabsProfile::MonospaceAlways ? "yes"
                             : settings.monospace == ElasticTabsProfile::MonospaceNever  ? "no" 
                                                                                         : "best" ) << std::endl;
    file << "monospaceNoMnemonics\t"        << settings.monospaceNoMnemonics           << std::endl;
    file << "disableOverSize\t"             << disableOverSize                         << std::endl;
    file << "disableOverLines\t"            << disableOverLines                        << std::endl;
    file << "elasticProgressTime\t"         << elasticProgressTime                     << std::endl;
    file << "timeScalarUnit\t"              << timeScalarUnit                          << std::endl;
    file << "timePartialRule\t"             << timePartialRule                         << std::endl;
    file << "timeFormatEnable\t"            << timeFormatEnable                        << std::endl;
    file << "showOnMenuBar\t"               << showOnMenuBar                           << std::endl;
    file << "replaceStaysPut\t"             << replaceStaysPut                         << std::endl;
    file << "extendSingleLine\t"            << extendSingleLine                        << std::endl;
    file << "extendFullLines\t"             << extendFullLines                         << std::endl;
    file << "extendZeroWidth\t"             << extendZeroWidth                         << std::endl;
    file << "csvQuote\t"                    << csv.quote                                             << std::endl;
    file << "csvApostrophe\t"               << csv.apostrophe                                        << std::endl;
    file << "csvEscape\t"                   << csv.escape                                            << std::endl;
    file << "csvPreserveQuotes\t"           << csv.preserveQuotes                                    << std::endl;
    file << "csvSeparator\t"                << encodeDelimitedString(std::wstring(1,csv.separator )) << std::endl;
    file << "csvEscapeChar\t"               << encodeDelimitedString(std::wstring(1,csv.escapeChar)) << std::endl;
    file << "csvEncodeTNR\t"                << encodeDelimitedString(std::wstring(1,csv.encodeTNR )) << std::endl;
    file << "csvEncodeURL\t"                << encodeDelimitedString(std::wstring(1,csv.encodeURL )) << std::endl;
    file << "csvReplaceTab\t"               << encodeDelimitedString(csv.replaceTab                ) << std::endl;
    file << "csvReplaceLF\t"                << encodeDelimitedString(csv.replaceLF                 ) << std::endl;
    file << "csvReplaceCR\t"                << encodeDelimitedString(csv.replaceCR                 ) << std::endl;
    file << "csvEncodingStyle\t" << ( csv.encodingStyle == CsvSettings::TNR ? "backslash"
                                    : csv.encodingStyle == CsvSettings::URL ? "url" 
                                                                            : "replace" ) << std::endl;

    file << std::endl << "Calculate" << std::endl << std::endl;

    file << "thousands\t" << ( calc.thousands == CalculateSettings::Comma      ? (settings.decimalSeparatorIsComma ? "period" : "comma" )
                             : calc.thousands == CalculateSettings::Apostrophe ? "apostrophe" 
                             : calc.thousands == CalculateSettings::Blank      ? "blank"
                                                                               : "none" ) << std::endl;
    file << "insert\t"        << calc.insert        << std::endl;
    file << "addLine\t"       << calc.addLine       << std::endl;
    file << "decimalPlaces\t" << calc.decimalPlaces << std::endl;
    file << "matchCase\t"     << calc.matchCase     << std::endl;
    file << "skipUnmatched\t" << calc.skipUnmatched << std::endl;
    file << "decimalsFixed\t" << calc.decimalsFixed << std::endl;
    file << "formatAsTime\t"  << calc.formatAsTime  << std::endl;
    file << "tabbed\t"        << calc.tabbed        << std::endl;
    file << "aligned\t"       << calc.aligned       << std::endl;
    file << "left\t"          << calc.left          << std::endl;
    file << "timeSegments\t"  << calc.timeSegments  << std::endl;
    file << "autoDecimals\t"  << calc.autoDecimals  << std::endl;
    writeDelimitedStringHistory(file, "formula", calc.formulaHistory);
    writeDelimitedStringHistory(file, "regex"  , calc.regexHistory);

    file << std::endl << "Search" << std::endl << std::endl;

    file << "mode\t"                  << searchData.mode                  << std::endl;
    file << "backward\t"              << searchData.backward              << std::endl;
    file << "wholeWord\t"             << searchData.wholeWord             << std::endl;
    file << "matchCase\t"             << searchData.matchCase             << std::endl;
    file << "autoClear\t"             << searchData.autoClear             << std::endl;
    file << "autoClearSelection\t"    << searchData.autoClearSelection    << std::endl;
    file << "autoSetSelection\t"      << searchData.autoSetSelection      << std::endl;
    file << "enableCustomIndicator\t" << searchData.enableCustomIndicator << std::endl;
    file << "forceUserIndicator\t"    << searchData.forceUserIndicator    << std::endl;
    file << "indicator\t"             << searchData.indicator             << std::endl;
    file << "customAlpha\t"           << searchData.customAlpha           << std::endl;
    file << "customColor\t"           << searchData.customColor           << std::endl;
    file << "customIndicator\t"       << searchData.userIndicator         << std::endl;
    writeDelimitedStringHistory(file, "find"   , searchData.findHistory);
    writeDelimitedStringHistory(file, "replace", searchData.replaceHistory);

    file << std::endl << "Sort" << std::endl << std::endl;

    file << "columnSelectionOnly\t"    << sort.sortColumnSelectionOnly       << std::endl;
    file << "descending\t"             << sort.sortDescending                << std::endl;
    file << "regexMatchCase\t"         << sort.regexMatchCase                << std::endl;
    file << "regexUseKey\t"            << sort.regexUseKey                   << std::endl;
    file << "localeName\t"             << fromWide(sort.localeName, CP_UTF8) << std::endl;
    file << "localeCaseSensitive\t"    << sort.localeCaseSensitive           << std::endl;
    file << "localeDigitsAsNumbers\t"  << sort.localeDigitsAsNumbers         << std::endl;
    file << "localeIgnoreDiacritics\t" << sort.localeIgnoreDiacritics        << std::endl;
    file << "localeIgnoreSymbols\t"    << sort.localeIgnoreSymbols           << std::endl;
    file << "sortType\t" << ( sort.sortType == SortSettings::Locale      ? "Locale"
                            : sort.sortType == SortSettings::Numeric     ? "Numeric" 
                            : sort.sortType == SortSettings::Width       ? "Width" 
                                                                         : "Binary"      ) << std::endl;
    file << "keyType\t"  << ( sort.keyType == SortSettings::IgnoreBlanks ? "IgnoreBlanks"
                            : sort.keyType == SortSettings::Tabbed       ? "Tabbed"
                            : sort.keyType == SortSettings::Regex        ? "Regex"
                                                                         : "EntireColumn") << std::endl;
    writeDelimitedStringHistory(file, "regex", sort.regexHistory);
    writeDelimitedStringHistory(file, "keys" , sort.keygroupHistory);

    file << std::endl << "Align" << std::endl << std::endl;

    file << "alignOn\t" << ( align.alignOn == AlignSettings::Last  ? "Last"
                           : align.alignOn == AlignSettings::Regex ? "Regex" 
                                                                   : "First") << std::endl;
    file << "matchCase\t"   << align.matchCase   << std::endl;
    file << "margin\t"      << align.margin      << std::endl;
    file << "marginRight\t" << align.marginRight << std::endl;
    writeDelimitedStringHistory(file, "history", align.history);

    file << std::endl << "Timestamps" << std::endl << std::endl;

    file << "fromCounter\t"   << timestamps.enableFromCounter             << std::endl;
    file << "fromDate\t"      << timestamps.enableFromDatetime            << std::endl;
    file << "fromEpoch\t"     << timestamps.fromCounter.custom.epoch      << std::endl;
    file << "fromUnit\t"      << timestamps.fromCounter.custom.unit       << std::endl;
    file << "fromLeap\t"      << timestamps.fromCounter.custom.leap       << std::endl;
    file << "toEpoch\t"       << timestamps.toCounter.custom.epoch        << std::endl;
    file << "toUnit\t"        << timestamps.toCounter.custom.unit         << std::endl;
    file << "toLeap\t"        << timestamps.toCounter.custom.leap         << std::endl;
    file << "twoDigitLimit\t" << timestamps.twoDigitYearLimit             << std::endl;
    file << "zoneAndLocale\t" << timestamps.enableTzAndLocale             << std::endl;
    file << "overwrite\t"     << timestamps.overwrite                     << std::endl;
    file << "fromZone\t"      << fromWide(timestamps.fromZone  , CP_UTF8) << std::endl;
    file << "toZone\t"        << fromWide(timestamps.toZone    , CP_UTF8) << std::endl;
    file << "localeName\t"    << fromWide(timestamps.localeName, CP_UTF8) << std::endl;
    file << "fromType\t"      << ( timestamps.fromCounter.type == TimestampSettings::CounterType::Unix ? "Unix"
                                 : timestamps.fromCounter.type == TimestampSettings::CounterType::File ? "File"
                                 : timestamps.fromCounter.type == TimestampSettings::CounterType::Ex00 ? "1900"
                                 : timestamps.fromCounter.type == TimestampSettings::CounterType::Ex04 ? "1904"
                                                                                                       : "custom") << std::endl;
    file << "toType\t"        << ( timestamps.toCounter.type   == TimestampSettings::CounterType::Unix ? "Unix"
                                 : timestamps.toCounter.type   == TimestampSettings::CounterType::File ? "File"
                                 : timestamps.toCounter.type   == TimestampSettings::CounterType::Ex00 ? "1900"
                                 : timestamps.toCounter.type   == TimestampSettings::CounterType::Ex04 ? "1904"
                                                                                                       : "custom") << std::endl;
    file << "priority\t"      << ( timestamps.datePriority     == TimestampSettings::DatePriority::ymd ? "ymd"
                                 : timestamps.datePriority     == TimestampSettings::DatePriority::mdy ? "mdy"
                                 : timestamps.datePriority     == TimestampSettings::DatePriority::dmy ? "dmy"
                                                                                                       : "custom") << std::endl;
    file << "format\t"        << ( timestamps.dateFormat       == TimestampSettings::DateFormat::iso8601     ? "iso"
                                 : timestamps.dateFormat       == TimestampSettings::DateFormat::localeShort ? "short"
                                 : timestamps.dateFormat       == TimestampSettings::DateFormat::localeLong  ? "long"
                                                                                                             : "custom") << std::endl;
    writeDelimitedStringHistory(file, "parse"  , timestamps.dateParse  );
    writeDelimitedStringHistory(file, "picture", timestamps.datePicture);

    file << std::endl << "Update" << std::endl << std::endl;

    file << "check\t" << ( updateInfo.check == UpdateInformation::NotifyAny    ? "any"
                         : updateInfo.check == UpdateInformation::DoNotCheck   ? "none" 
                                                                               : "stable" ) << std::endl;
    if ((updateInfo.check == UpdateInformation::NotifyAny || updateInfo.check == UpdateInformation::NotifyStable) && updateInfo.timestamp) {
        file << "timestamp\t"     << updateInfo.timestamp     << std::endl;
        file << "newestURL\t"     << updateInfo.newestURL     << std::endl;
        file << "stableURL\t"     << updateInfo.stableURL     << std::endl;
        file << "newestVersion\t" << updateInfo.newestVersion << std::endl;
        file << "stableVersion\t" << updateInfo.stableVersion << std::endl;
    }

    for (const auto& p : profiles) if (p.first != L"Classic" && p.first != L"General" && p.first != L"Tabular") {
        file << std::endl << "Profile\t" << fromWide(p.first, CP_UTF8) << std::endl << std::endl;
        file << "minimumOrLeadingTabSize\t"     << p.second.minimumOrLeadingTabSize    << std::endl;
        file << "minimumSpaceBetweenColumns\t"  << p.second.minimumSpaceBetweenColumns << std::endl;
        file << "leadingTabsIndent\t"           << p.second.leadingTabsIndent          << std::endl;
        file << "lineUpAll\t"                   << p.second.lineUpAll                  << std::endl;
        file << "treatEolAsTab\t"               << p.second.treatEolAsTab              << std::endl;
        file << "overrideTabSize\t"             << p.second.overrideTabSize            << std::endl;
        file << "monospace\t" << ( p.second.monospace == ElasticTabsProfile::MonospaceAlways ? "yes"
                                 : p.second.monospace == ElasticTabsProfile::MonospaceNever  ? "no" 
                                                                                             : "best" ) << std::endl;
        file << "monospaceNoMnemonics\t"        << settings.monospaceNoMnemonics       << std::endl;
    }

    if (extensionToProfile.size()) {
        file << std::endl << "Extensions" << std::endl << std::endl;
        for (const auto& xtp : extensionToProfile)
            if      (xtp.first == L"" ) file << "new\t"     << fromWide(xtp.second, CP_UTF8) << std::endl;
            else if (xtp.first == L".") file << "none\t"    << fromWide(xtp.second, CP_UTF8) << std::endl;
            else if (xtp.first == L"*") file << "default\t" << fromWide(xtp.second, CP_UTF8) << std::endl;
            else  {
                std::string extension = fromWide(xtp.first, CP_UTF8);
                std::replace(extension.begin(), extension.end(), ' ', '.');
                file << "." << extension << "\t" << (xtp.second.length() ? fromWide(xtp.second, CP_UTF8) : "(disable)") << std::endl;
            }
    }

}


void ColumnsPlusPlusData::initializeBuiltinElasticTabstopsProfiles() {

    ElasticTabsProfile& classic = profiles[L"Classic"];
    classic.leadingTabsIndent          = false;
    classic.lineUpAll                  = false;
    classic.treatEolAsTab              = false;
    classic.overrideTabSize            = false;
    classic.minimumOrLeadingTabSize    = 4;
    classic.minimumSpaceBetweenColumns = 2;
    classic.monospace                  = ElasticTabsProfile::MonospaceBest;
    classic.monospaceNoMnemonics       = true;

    ElasticTabsProfile& general = profiles[L"General"];
    general.leadingTabsIndent          = true;
    general.lineUpAll                  = false;
    general.treatEolAsTab              = false;
    general.overrideTabSize            = false;
    general.minimumOrLeadingTabSize    = 4;
    general.minimumSpaceBetweenColumns = 2;
    general.monospace                  = ElasticTabsProfile::MonospaceBest;
    general.monospaceNoMnemonics       = true;

    ElasticTabsProfile& tabular = profiles[L"Tabular"];
    tabular.leadingTabsIndent          = false;
    tabular.lineUpAll                  = true;
    tabular.treatEolAsTab              = true;
    tabular.overrideTabSize            = true;
    tabular.minimumOrLeadingTabSize    = 1;
    tabular.minimumSpaceBetweenColumns = 2;
    tabular.monospace                  = ElasticTabsProfile::MonospaceBest;
    tabular.monospaceNoMnemonics       = true;

}