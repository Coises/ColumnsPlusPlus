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
#include <chrono>
#include "resource.h"
#include "commctrl.h"
#include "Shlwapi.h"


INT_PTR CALLBACK checkingForUpdatesDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    ColumnsPlusPlusData* dp;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        dp = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else dp = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!dp) return FALSE;
    ColumnsPlusPlusData& data = *dp;

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
        data.getReleases(hwndDlg);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        break;

    }
    return FALSE;
}


INT_PTR CALLBACK aboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ColumnsPlusPlusData* data;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        data = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else data = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    return data->aboutDialogProc(hwndDlg, uMsg, wParam, lParam);
}

void ColumnsPlusPlusData::showAboutDialog() {
    DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_ABOUT), nppData._nppHandle,
                   ::aboutDialogProc, reinterpret_cast<LPARAM>(this));
}

void setUpdateCommandButtonText(HWND hwndDlg, UpdateInformation& updateInfo) {
    std::wstring s;
    if (updateInfo.newestVersion && !updateInfo.newestURL.empty()) {
        int v = updateInfo.newestVersion;
        int u = v % 100;
        if (u) s = L'.' + std::to_wstring(u);
        v /= 100;
        u = v % 100;
        if (u || !s.empty()) s = L'.' + std::to_wstring(u) + s;
        v /= 100;
        s = L"Get version " + std::to_wstring(v / 100) + L'.' + std::to_wstring(v % 100) + s + L" from GitHub.";
    }
    else s = L"Get newest release from GitHub.";
    SendDlgItemMessage(hwndDlg, IDC_ABOUT_NEWEST, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(s.data()));
    if (updateInfo.stableVersion && !updateInfo.stableURL.empty()) {
        s.clear();
        int v = updateInfo.stableVersion;
        int u = v % 100;
        if (u) s = L'.' + std::to_wstring(u);
        v /= 100;
        u = v % 100;
        if (u || !s.empty()) s = L'.' + std::to_wstring(u) + s;
        v /= 100;
        s = L"Get version " + std::to_wstring(v / 100) + L'.' + std::to_wstring(v % 100) + s + L" from GitHub.";
    }
    else s = L"Get latest stable (production) release from GitHub.";
    SendDlgItemMessage(hwndDlg, IDC_ABOUT_STABLE, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(s.data()));
}

BOOL ColumnsPlusPlusData::aboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {

    switch (uMsg) {

    case WM_DESTROY:
        return TRUE;

    case WM_INITDIALOG:
        {
            RECT rcNpp, rcDlg;
            GetWindowRect(nppData._nppHandle, &rcNpp);
            GetWindowRect(hwndDlg, &rcDlg);
            SetWindowPos(hwndDlg, HWND_TOP, (rcNpp.left + rcNpp.right + rcDlg.left - rcDlg.right) / 2,
                                            (rcNpp.top + rcNpp.bottom + rcDlg.top - rcDlg.bottom) / 2, 0, 0, SWP_NOSIZE);

            {
                std::wstring version;
                VS_FIXEDFILEINFO* fixedFileInfo;
                UINT size;
                HRSRC hRes = FindResource(dllInstance, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
                if (hRes) {
                    HGLOBAL hGlobal = LoadResource(dllInstance, hRes);
                    if (hGlobal) {
                        void* pVersionInfo = LockResource(hGlobal);
                        if (pVersionInfo) {
                            if (VerQueryValue(pVersionInfo, L"\\", reinterpret_cast<void**>(&fixedFileInfo), &size) && size > 0) {
                                int versionPart1 = 0x0000ffff & (fixedFileInfo->dwProductVersionMS >> 16);
                                int versionPart2 = 0x0000ffff & fixedFileInfo->dwProductVersionMS;
                                int versionPart3 = 0x0000ffff & (fixedFileInfo->dwProductVersionLS >> 16);
                                int versionPart4 = 0x0000ffff & fixedFileInfo->dwProductVersionLS;
                                version = L"This is version " + std::to_wstring(versionPart1)
                                    + L"." + std::to_wstring(versionPart2);
                                if (versionPart3 || versionPart4) version += L"." + std::to_wstring(versionPart3);
                                if (versionPart4) version += L"." + std::to_wstring(versionPart4);
                                if constexpr (sizeof(size_t) == 8) version += L" (x64)";
                                else if constexpr (sizeof(size_t) == 4) version += L" (x86)";
                                version += L".\n\n";
                            }
                        }
                    }
                }
                auto pidh = reinterpret_cast<IMAGE_DOS_HEADER*>(dllInstance);
                auto pnth = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<char*>(pidh) + pidh->e_lfanew);
                auto timepoint = std::chrono::sys_seconds(std::chrono::seconds(pnth->FileHeader.TimeDateStamp));
                version += std::format(L"Build time: {0:%Y} {0:%b} {0:%d} at {0:%H}:{0:%M}:{0:%S} UTC.", timepoint);
                SetDlgItemText(hwndDlg, IDC_ABOUT_VERSION, version.data());
            }

            SendDlgItemMessage(hwndDlg, IDC_ABOUT_HELP, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(
                L"Open user documentation."));
            SendDlgItemMessage(hwndDlg, IDC_ABOUT_MORE, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(
                L"Show change log, license and source information."));
            setUpdateCommandButtonText(hwndDlg, updateInfo);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            EndDialog(hwndDlg, 0);
            return TRUE;
        case IDC_ABOUT_CHECK_NOW:
            if (!DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_CHECKING_FOR_UPDATES), nppData._nppHandle,
                                checkingForUpdatesDialogProc, reinterpret_cast<LPARAM>(this)))
                setUpdateCommandButtonText(hwndDlg, updateInfo);
            return TRUE;
        case IDC_ABOUT_HELP:
            {
                auto n = SendMessage(nppData._nppHandle, NPPM_GETPLUGINHOMEPATH, 0, 0);
                std::wstring path(n, 0);
                SendMessage(nppData._nppHandle, NPPM_GETPLUGINHOMEPATH, n + 1, reinterpret_cast<LPARAM>(path.data()));
                path += L"\\ColumnsPlusPlus\\help.htm";
                ShellExecute(0, 0, path.data(), 0, 0, 0);
            }
            EndDialog(hwndDlg, 0);
            return TRUE;
        case IDC_ABOUT_MORE:
            {
                auto n = SendMessage(nppData._nppHandle, NPPM_GETPLUGINHOMEPATH, 0, 0);
                std::wstring path(n, 0);
                SendMessage(nppData._nppHandle, NPPM_GETPLUGINHOMEPATH, n + 1, reinterpret_cast<LPARAM>(path.data()));
                std::wstring changes = path + L"\\ColumnsPlusPlus\\CHANGELOG.md";
                if (PathFileExists(changes.data()) == TRUE) {
                    SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(changes.data()));
                    SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_TOGGLEREADONLY);
                }
                std::wstring license = path + L"\\ColumnsPlusPlus\\LICENSE.txt";;
                if (PathFileExists(license.data()) == TRUE) {
                    SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(license.data()));
                    SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_TOGGLEREADONLY);
                }
                std::wstring source = path + L"\\ColumnsPlusPlus\\source.txt";
                if (PathFileExists(source.data()) == TRUE) {
                    SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(source.data()));
                    SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_TOGGLEREADONLY);
                }
                SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, reinterpret_cast<LPARAM>(changes.data()));
            }
            EndDialog(hwndDlg, 0);
            return TRUE;
        case IDC_ABOUT_NEWEST:
            ShellExecute(0, 0,
                updateInfo.newestVersion && !updateInfo.newestURL.empty() ? toWide(updateInfo.newestURL, CP_UTF8).data()
                                                                          : L"https://github.com/Coises/ColumnsPlusPlus/releases",
                0, 0, SW_NORMAL);
            EndDialog(hwndDlg, 0);
            return TRUE;
        case IDC_ABOUT_STABLE:
            ShellExecute(0, 0,
                updateInfo.stableVersion && !updateInfo.stableURL.empty() ? toWide(updateInfo.stableURL, CP_UTF8).data()
                                                                          : L"https://github.com/Coises/ColumnsPlusPlus/releases/latest",
                0, 0, SW_NORMAL);
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        break;

    }
    return FALSE;
}