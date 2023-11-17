// This file is part of Columns++ for Notepad++.
// Copyright 2023 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

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

#include "ColumnsPlusPlus.h"
#include "nlohmann/json.hpp"
#include <regex>
#include <wininet.h>
#include "resource.h"

namespace {

constexpr long long minimumGitHubPollingInterval = 12 * 60 * 60;  // in seconds, minimum time between fetching release information from GitHub

struct AsyncData {
    ColumnsPlusPlusData& data;
    std::string buffer;
    size_t      bufferNext     = 0;
    DWORD       bufferAdded    = 0;
    HINTERNET   hInternet      = 0;
    HINTERNET   hURLReleases   = 0;
    HWND        windowToNotify = 0;
    AsyncData(ColumnsPlusPlusData& data) : data(data) {};
    ~AsyncData() { if (hURLReleases) InternetCloseHandle(hURLReleases); if (hInternet) InternetCloseHandle(hInternet); }
};

long long getTimestamp() {
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.HighPart = ft.dwHighDateTime;
    uli.LowPart = ft.dwLowDateTime;
    return uli.QuadPart / 10000000;
}

int getModuleVersion(HINSTANCE dllInstance) {
    std::wstring moduleFileName(MAX_PATH + 1, 0);
    for (;;) {
        size_t n = GetModuleFileName(dllInstance, moduleFileName.data(), static_cast<DWORD>(moduleFileName.length()));
        if (n < moduleFileName.length()) {
            moduleFileName.resize(n);
            break;
        }
        if (n > 32768) {
            moduleFileName.clear();
            break;
        }
        moduleFileName.resize(2 * moduleFileName.length());
    }
    if (moduleFileName.length()) {
        int n = GetFileVersionInfoSize(moduleFileName.data(), 0);
        if (n) {
            std::string versionInfo(n, 0);
            VS_FIXEDFILEINFO* fixedFileInfo;
            UINT size;
            GetFileVersionInfo(moduleFileName.data(), 0, n, versionInfo.data());
            if (VerQueryValue(versionInfo.data(), L"\\", reinterpret_cast<void**>(&fixedFileInfo), &size) && size > 0) {
                int part1 = 0x0000ffff & (fixedFileInfo->dwProductVersionMS >> 16);
                int part2 = 0x0000ffff & fixedFileInfo->dwProductVersionMS;
                int part3 = 0x0000ffff & (fixedFileInfo->dwProductVersionLS >> 16);
                int part4 = 0x0000ffff & fixedFileInfo->dwProductVersionLS;
                return ((part1 * 100 + part2) * 100 + part3) * 100 + part4;
            }
        }
    }
    return 0;
}

int parseVersionTag(const std::string& tag) {
    static const std::regex version("v(\\d+)(?:\\.(\\d+)(?:\\.(\\d+)(?:\\.(\\d+))?)?)?(?:-(.*))?", std::regex::optimize);
    std::smatch results;
    if (!std::regex_match(tag, results, version)) return 0;
    int part1 = stoi(results[1]);
    int part2 = results[2].matched ? stoi(results[2]) : 0;
    int part3 = results[3].matched ? stoi(results[3]) : 0;
    int part4 = results[4].matched ? stoi(results[4]) : 0;
    return ((part1 * 100 + part2) * 100 + part3) * 100 + part4;
}

void updateMenuToNotify(ColumnsPlusPlusData& data) {
    if (!data.updateInfo.thisVersion) return;
    switch (data.updateInfo.check) {
    case UpdateInformation::NotifyAny:
        if (data.updateInfo.newestVersion <= data.updateInfo.thisVersion) return;
        break;
    case UpdateInformation::NotifyStable:
        if (data.updateInfo.stableVersion <= data.updateInfo.thisVersion) return;
        break;
    default: return;
    }
    MENUITEMINFO mii;
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = const_cast<LPWSTR>(L"Help/About... | Update Available");
    HMENU nppMainMenu = reinterpret_cast<HMENU>(SendMessage(data.nppData._nppHandle, NPPM_GETMENUHANDLE, 1, 0));
    SetMenuItemInfo(nppMainMenu, data.aboutMenuItem, FALSE, &mii);
}

void parseReleaseData(AsyncData* asyncData) {
    UpdateInformation& updateInfo = asyncData->data.updateInfo;
    if (!asyncData->buffer.empty()) {
        nlohmann::json releases = nlohmann::json::parse(asyncData->buffer, 0, false, true);
        if (!releases.is_discarded()) {
            updateInfo.newestVersion = updateInfo.stableVersion = 0;
            updateInfo.newestURL     = updateInfo.stableURL     = "";
            for (const auto& r : releases) {
                if (!r.contains("tag_name") || !r.contains("draft") || !r.contains("prerelease") || !r.contains("html_url")) continue;
                auto& tag_name   = r["tag_name"  ];
                auto& draft      = r["draft"     ];
                auto& prerelease = r["prerelease"];
                auto& html_url   = r["html_url"  ];
                if (!tag_name.is_string() || !draft.is_boolean() || !prerelease.is_boolean() || !html_url.is_string()) continue;
                if (draft) continue;
                std::string tag  = tag_name;
                std::string html = html_url;
                int v = parseVersionTag(tag);
                if (updateInfo.newestVersion < v) {
                    updateInfo.newestVersion = v;
                    updateInfo.newestURL     = html;
                }
                if (!prerelease && updateInfo.stableVersion < v) {
                    updateInfo.stableVersion = v;
                    updateInfo.stableURL     = html;
                }
            }
            updateInfo.timestamp = getTimestamp();
        }
    }
}

void readReleaseData(AsyncData* asyncData) {
    constexpr static int increment = 32000;
    for (;;) {
        asyncData->bufferNext = asyncData->buffer.length();
        asyncData->buffer.resize(asyncData->bufferNext + increment);
        if (!InternetReadFile(asyncData->hURLReleases, asyncData->buffer.data() + asyncData->bufferNext, increment, &asyncData->bufferAdded)) {
            if (GetLastError() == ERROR_IO_PENDING) return;
            break;
        }
        asyncData->buffer.resize(asyncData->bufferNext + asyncData->bufferAdded);
        if (asyncData->bufferAdded == 0) {
            parseReleaseData(asyncData);
            break;
        }
    }
    updateMenuToNotify(asyncData->data);
    if (asyncData->windowToNotify) PostMessage(asyncData->windowToNotify, WM_COMMAND, IDOK, 0);
    delete asyncData;
}

void CALLBACK updateCallback2(HINTERNET, DWORD_PTR context, DWORD status, LPVOID information, DWORD) {
    if (status != INTERNET_STATUS_REQUEST_COMPLETE) return;
    AsyncData*             asyncData = reinterpret_cast<AsyncData*>(context);
    INTERNET_ASYNC_RESULT& iar       = *reinterpret_cast<INTERNET_ASYNC_RESULT*>(information);
    if (iar.dwResult) {
        asyncData->buffer.resize(asyncData->bufferNext + asyncData->bufferAdded);
        if (asyncData->bufferAdded) return readReleaseData(asyncData);
        parseReleaseData(asyncData);
    }
    updateMenuToNotify(asyncData->data);
    if (asyncData->windowToNotify) PostMessage(asyncData->windowToNotify, WM_COMMAND, IDOK, 0);
    delete asyncData;
}

void CALLBACK updateCallback1(HINTERNET, DWORD_PTR context, DWORD status, LPVOID information, DWORD) {
    if (status != INTERNET_STATUS_REQUEST_COMPLETE) return;
    AsyncData*             asyncData = reinterpret_cast<AsyncData*>(context);
    INTERNET_ASYNC_RESULT& iar       = *reinterpret_cast<INTERNET_ASYNC_RESULT*>(information);
    if (!iar.dwResult) {
        updateMenuToNotify(asyncData->data);
        if (asyncData->windowToNotify) PostMessage(asyncData->windowToNotify, WM_COMMAND, IDCANCEL, 0);
        delete asyncData;
        return;
    }
    asyncData->hURLReleases = reinterpret_cast<HINTERNET>(iar.dwResult);
    InternetSetStatusCallback(asyncData->hURLReleases, updateCallback2);
    readReleaseData(asyncData);
}

}

void ColumnsPlusPlusData::getReleases(HWND windowToNotify) {
    updateInfo.thisVersion = getModuleVersion(dllInstance);
    if (!windowToNotify && updateInfo.check == UpdateInformation::DoNotCheck) return;
    long long ts = getTimestamp();
    if (windowToNotify || ts - updateInfo.timestamp >= minimumGitHubPollingInterval) {
        AsyncData* asyncData = new AsyncData(*this);
        if (asyncData) {
            asyncData->windowToNotify = windowToNotify;
            asyncData->hInternet = InternetOpen(L"Coises-ColumnsPlusPlus-UpdateCheck", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, INTERNET_FLAG_ASYNC);
            if (asyncData->hInternet) {
                InternetSetStatusCallback(asyncData->hInternet, updateCallback1);
                InternetOpenUrl(asyncData->hInternet, L"https://api.github.com/repos/Coises/ColumnsPlusPlus/releases", 0, 0,
                                INTERNET_FLAG_NO_COOKIES | (windowToNotify ? INTERNET_FLAG_RELOAD : INTERNET_FLAG_RESYNCHRONIZE),
                                reinterpret_cast<DWORD_PTR>(asyncData));
                return;
            }
            delete asyncData;
        }
    }
    if (windowToNotify) PostMessage(windowToNotify, WM_COMMAND, IDCANCEL, 0);
                   else updateMenuToNotify(*this);
}