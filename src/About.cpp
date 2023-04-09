// This file is part of Columns++ for Notepad++.
// Copyright 2023 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "ColumnsPlusPlus.h"
#include "resource.h"
#include "commctrl.h"
#include "Shlwapi.h"

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
                moduleFileName.resize(2*moduleFileName.length());
            }
            if (moduleFileName.length()) {
                int n = GetFileVersionInfoSize(moduleFileName.data(), 0);
                if (n) {
                    std::string versionInfo(n, 0);
                    VS_FIXEDFILEINFO* fixedFileInfo;
                    UINT size;
                    GetFileVersionInfo(moduleFileName.data(), 0, n, versionInfo.data());
                    if (VerQueryValue(versionInfo.data(), L"\\", reinterpret_cast<void**>(&fixedFileInfo), &size) && size > 0) {
                        int versionPart1 = 0x0000ffff & (fixedFileInfo->dwProductVersionMS >> 16);
                        int versionPart2 = 0x0000ffff & fixedFileInfo->dwProductVersionMS;
                        int versionPart3 = 0x0000ffff & (fixedFileInfo->dwProductVersionLS >> 16);
                        int versionPart4 = 0x0000ffff & fixedFileInfo->dwProductVersionLS;
                        std::wstring version = L"This is version " + std::to_wstring(versionPart1)
                                             + L"." + std::to_wstring(versionPart2);
                        if (versionPart3 || versionPart4) version += L"." + std::to_wstring(versionPart3);
                        if (versionPart4) version += L"." + std::to_wstring(versionPart4);
                        if constexpr (sizeof(size_t) == 8) version += L" (x64)";
                        else if constexpr (sizeof(size_t) == 4) version += L" (x86)";
                        HANDLE file = CreateFile(moduleFileName.data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                        if (file != INVALID_HANDLE_VALUE) {
                            FILETIME fileTime;
                            SYSTEMTIME sysTime;
                            GetFileTime(file, 0, 0, &fileTime);
                            CloseHandle(file);
                            FileTimeToSystemTime(&fileTime, &sysTime);
                            int dateLen = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_LONGDATE, &sysTime, 0, 0, 0, 0);
                            if (dateLen) {
                                std::wstring date(dateLen, 0);
                                GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_LONGDATE, &sysTime, 0, date.data(), dateLen, 0);
                                date.resize(dateLen - 1);
                                version += L".\n\nFile date: " + date;
                                int timeLen = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &sysTime, L"HH':'mm", 0, 0);
                                if (timeLen) {
                                    std::wstring time(timeLen, 0);
                                    GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &sysTime, L"HH':'mm", time.data(), timeLen);
                                    time.resize(timeLen - 1);
                                    version += L" at " + time + L" UTC";
                                }
                            }
                        }
                        version += L'.';
                        SetDlgItemText(hwndDlg, IDC_ABOUT_VERSION, version.data());
                    }
                }
            }

            SendDlgItemMessage(hwndDlg, IDC_ABOUT_HELP, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(
                L"Open user documentation."));
            SendDlgItemMessage(hwndDlg, IDC_ABOUT_MORE, BCM_SETNOTE, 0, reinterpret_cast<LPARAM>(
                L"Show change log, license and source information."));
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            EndDialog(hwndDlg, 0);
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
            break;
        case IDC_ABOUT_MORE:
            {
                auto n = SendMessage(nppData._nppHandle, NPPM_GETPLUGINHOMEPATH, 0, 0);
                std::wstring path(n, 0);
                SendMessage(nppData._nppHandle, NPPM_GETPLUGINHOMEPATH, n + 1, reinterpret_cast<LPARAM>(path.data()));
                std::wstring changes = path + L"\\ColumnsPlusPlus\\CHANGELOG.md";
                if (PathFileExists(changes.data()) == TRUE) {
                    SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(changes.data()));
                    SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_SETREADONLY);
                }
                std::wstring license = path + L"\\ColumnsPlusPlus\\LICENSE.txt";;
                if (PathFileExists(license.data()) == TRUE) {
                    SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(license.data()));
                    SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_SETREADONLY);
                }
                std::wstring source = path + L"\\ColumnsPlusPlus\\source.txt";
                if (PathFileExists(source.data()) == TRUE) {
                    SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(source.data()));
                    SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_SETREADONLY);
                }
                SendMessage(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, reinterpret_cast<LPARAM>(changes.data()));
            }
            EndDialog(hwndDlg, 0);
            return TRUE;
            break;
        }
        break;

    default:
        break;

    }
    return FALSE;
}