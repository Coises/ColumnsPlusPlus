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
#include "resource.h"
#include "commctrl.h"
#include "Shlwapi.h"


INT_PTR CALLBACK optionsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ColumnsPlusPlusData* data;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        data = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else data = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    return data->optionsDialogProc(hwndDlg, uMsg, wParam, lParam);
}

void ColumnsPlusPlusData::showOptionsDialog() {
    bool originalShowOnMenuBar = showOnMenuBar;
    if (!DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_OPTIONS), nppData._nppHandle,
        ::optionsDialogProc, reinterpret_cast<LPARAM>(this))) {
        if (showOnMenuBar != originalShowOnMenuBar)
            if (showOnMenuBar) moveMenuToMenuBar();
                          else takeMenuOffMenuBar();
    }
}

BOOL ColumnsPlusPlusData::optionsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {

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
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_MENUBAR          , BM_SETCHECK, showOnMenuBar    ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_SINGLELINE, BM_SETCHECK, extendSingleLine ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ROWS      , BM_SETCHECK, extendFullLines  ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ZEROWIDTH , BM_SETCHECK, extendZeroWidth  ? BST_CHECKED : BST_UNCHECKED, 0);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
            showOnMenuBar    = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_MENUBAR          , BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendSingleLine = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_SINGLELINE, BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendFullLines  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ROWS      , BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendZeroWidth  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ZEROWIDTH , BM_GETCHECK, 0, 0) == BST_CHECKED;
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        break;

    default:
        break;

    }
    return FALSE;
}

void ColumnsPlusPlusData::moveMenuToMenuBar() {
    MENUITEMINFO cmi;
    cmi.cbSize = sizeof(MENUITEMINFO);
    HMENU nppMainMenu = reinterpret_cast<HMENU>(SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 1, 0));
    HMENU nppPluginMenu = reinterpret_cast<HMENU>(SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 0, 0));
    int mainMenuItemCount = GetMenuItemCount(nppMainMenu);
    int addMenuIndex = 0;
    while (addMenuIndex < mainMenuItemCount && GetSubMenu(nppMainMenu, addMenuIndex) != nppPluginMenu) ++addMenuIndex;
    int plugMenuItemCount = GetMenuItemCount(nppPluginMenu);
    cmi.fMask = MIIM_STRING;
    cmi.dwTypeData = 0;
    for (int i = 0; i < plugMenuItemCount; ++i) {
        GetMenuItemInfo(nppPluginMenu, i, TRUE, &cmi);
        if (cmi.cch == 9) {
            TCHAR columnsText[10];
            cmi.dwTypeData = columnsText;
            cmi.cch = 10;
            GetMenuItemInfo(nppPluginMenu, i, TRUE, &cmi);
            if (!_tcscmp(columnsText, TEXT("Columns++"))) {
                cmi.fMask = MIIM_SUBMENU;
                GetMenuItemInfo(nppPluginMenu, i, TRUE, &cmi);
                RemoveMenu(nppPluginMenu, i, MF_BYPOSITION);
                cmi.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_SUBMENU;
                cmi.fType = MFT_STRING;
                cmi.dwTypeData = const_cast<wchar_t*>(TEXT("Columns++"));
                InsertMenuItem(nppMainMenu, addMenuIndex, TRUE, &cmi);
                DrawMenuBar(nppData._nppHandle);
                break;
            }
            cmi.dwTypeData = 0;
        }
    }
}

void ColumnsPlusPlusData::takeMenuOffMenuBar() {
    MENUITEMINFO cmi;
    cmi.cbSize = sizeof(MENUITEMINFO);
    HMENU nppMainMenu = reinterpret_cast<HMENU>(SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 1, 0));
    HMENU nppPluginMenu = reinterpret_cast<HMENU>(SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 0, 0));
    int mainMenuItemCount = GetMenuItemCount(nppMainMenu);
    cmi.fMask = MIIM_STRING;
    cmi.dwTypeData = 0;
    for (int i = 0; i < mainMenuItemCount; ++i) {
        GetMenuItemInfo(nppMainMenu, i, TRUE, &cmi);
        if (cmi.cch == 9) {
            TCHAR columnsText[10];
            cmi.dwTypeData = columnsText;
            cmi.cch = 10;
            GetMenuItemInfo(nppMainMenu, i, TRUE, &cmi);
            if (!_tcscmp(columnsText, TEXT("Columns++"))) {
                cmi.fMask = MIIM_SUBMENU;
                GetMenuItemInfo(nppMainMenu, i, TRUE, &cmi);
                RemoveMenu(nppMainMenu, i, MF_BYPOSITION);
                cmi.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_SUBMENU;
                cmi.fType = MFT_STRING;
                cmi.dwTypeData = const_cast<wchar_t*>(TEXT("Columns++"));
                InsertMenuItem(nppPluginMenu, 0, TRUE, &cmi);
                DrawMenuBar(nppData._nppHandle);
                break;
            }
            cmi.dwTypeData = 0;
        }
    }
}
