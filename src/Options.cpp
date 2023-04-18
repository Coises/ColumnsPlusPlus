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

int validateSpin(HWND hwndDlg, int control, const wchar_t* message) {
    int n = static_cast<int>(SendDlgItemMessage(hwndDlg, control, UDM_GETPOS, 0, 0));
    if (HIWORD(n)) {
        HWND edit = reinterpret_cast<HWND>(SendDlgItemMessage(hwndDlg, control, UDM_GETBUDDY, 0, 0));
        EDITBALLOONTIP ebt;
        ebt.cbStruct = sizeof(EDITBALLOONTIP);
        ebt.pszTitle = L"";
        ebt.ttiIcon = TTI_NONE;
        ebt.pszText = message;
        SendMessage(edit, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
        SendMessage(hwndDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(edit), TRUE);
        return -1;
    }
    return n;
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
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_REPLACE_STAYS_PUT, BM_SETCHECK, replaceStaysPut  ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_SINGLELINE, BM_SETCHECK, extendSingleLine ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ROWS      , BM_SETCHECK, extendFullLines  ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ZEROWIDTH , BM_SETCHECK, extendZeroWidth  ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ENABLED, BM_SETCHECK, searchData.customIndicator > 0  ? BST_CHECKED : BST_UNCHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER), searchData.customIndicator > 0  ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA ), searchData.customIndicator > 0  ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED   ), searchData.customIndicator > 0  ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN ), searchData.customIndicator > 0  ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE  ), searchData.customIndicator > 0  ? TRUE : FALSE);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, UDM_SETRANGE, 0, MAKELPARAM( 20, 8));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_SPIN , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_RED_SPIN   , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_SPIN , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_SPIN  , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, UDM_SETPOS, 0, std::abs(searchData.customIndicator));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_SPIN , UDM_SETPOS, 0, searchData.customAlpha);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_RED_SPIN   , UDM_SETPOS, 0, 255 & searchData.customColor);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_SPIN , UDM_SETPOS, 0, 255 & (searchData.customColor >> 8));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_SPIN  , UDM_SETPOS, 0, 255 & (searchData.customColor >> 16));
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
            if (SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                int indicatorNumber = validateSpin(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, L"Indicator number must be between 8 and 20.");
                int indicatorAlpha  = validateSpin(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_SPIN , L"Alpha transparency must be between 0 and 255.");
                int indicatorRed    = validateSpin(hwndDlg, IDC_OPTIONS_INDICATOR_RED_SPIN   , L"Red amount must be between 0 and 255.");
                int indicatorGreen  = validateSpin(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_SPIN , L"Green amount must be between 0 and 255.");
                int indicatorBlue   = validateSpin(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_SPIN  , L"Blue amount must be between 0 and 255.");
                if (indicatorNumber < 0 || indicatorAlpha < 0 || indicatorRed < 0 || indicatorGreen < 0 || indicatorBlue < 0) return TRUE;
                searchData.customIndicator = indicatorNumber;
                searchData.customAlpha = indicatorAlpha;
                searchData.customColor = indicatorRed | (indicatorGreen << 8) | (indicatorBlue << 16);
                if (searchData.indicator > 21 && !searchData.dialog) {
                    searchData.indicator = searchData.customIndicator;
                    searchData.autoClear = true;
                }
            }
            else {
                if (searchData.indicator < 21 && !searchData.dialog) {
                    searchData.indicator = 31;
                    searchData.autoClear = false;
                }
                if (searchData.customIndicator > 0) searchData.customIndicator = -searchData.customIndicator;
            }
            showOnMenuBar    = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_MENUBAR          , BM_GETCHECK, 0, 0) == BST_CHECKED;
            replaceStaysPut  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_REPLACE_STAYS_PUT, BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendSingleLine = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_SINGLELINE, BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendFullLines  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ROWS      , BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendZeroWidth  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ZEROWIDTH , BM_GETCHECK, 0, 0) == BST_CHECKED;
            EndDialog(hwndDlg, 0);
            return TRUE;
        case IDC_OPTIONS_INDICATOR_ENABLED:
        {
            BOOL enabled = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED   ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE  ), enabled);
        }
            break;
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
