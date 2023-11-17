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
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_MENUBAR           , BM_SETCHECK, showOnMenuBar                    ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_REPLACE_STAYS_PUT , BM_SETCHECK, replaceStaysPut                  ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_SINGLELINE , BM_SETCHECK, extendSingleLine                 ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ROWS       , BM_SETCHECK, extendFullLines                  ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ZEROWIDTH  , BM_SETCHECK, extendZeroWidth                  ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ENABLED , BM_SETCHECK, searchData.enableCustomIndicator ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_OVERRIDE, BM_SETCHECK, searchData.forceUserIndicator    ? BST_CHECKED : BST_UNCHECKED, 0);
            if (searchData.dialog) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ENABLED     ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_OVERRIDE    ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER      ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_LABEL), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA       ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_LABEL ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED         ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED_LABEL   ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN       ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_LABEL ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE        ), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_LABEL  ), FALSE);
            }
            else {
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_OVERRIDE),
                    searchData.enableCustomIndicator && searchData.allocatedIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER),
                    searchData.enableCustomIndicator && (searchData.forceUserIndicator || !searchData.allocatedIndicator) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_LABEL),
                    searchData.enableCustomIndicator && (searchData.forceUserIndicator || !searchData.allocatedIndicator) ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA       ), searchData.enableCustomIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_LABEL ), searchData.enableCustomIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED         ), searchData.enableCustomIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED_LABEL   ), searchData.enableCustomIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN       ), searchData.enableCustomIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_LABEL ), searchData.enableCustomIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE        ), searchData.enableCustomIndicator ? TRUE : FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_LABEL  ), searchData.enableCustomIndicator ? TRUE : FALSE);
            }
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_ELASTIC_PROGRESS_SPIN, UDM_SETRANGE, 0, MAKELPARAM(20, 1));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_ELASTIC_PROGRESS_SPIN, UDM_SETPOS, 0, elasticProgressTime);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, UDM_SETRANGE, 0, MAKELPARAM( 20, 9));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_SPIN , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_RED_SPIN   , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_SPIN , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_SPIN  , UDM_SETRANGE, 0, MAKELPARAM(255, 0));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, UDM_SETPOS, 0, searchData.customIndicator);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_SPIN , UDM_SETPOS, 0, searchData.customAlpha);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_RED_SPIN   , UDM_SETPOS, 0, 255 & searchData.customColor);
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_SPIN , UDM_SETPOS, 0, 255 & (searchData.customColor >> 8));
            SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_SPIN  , UDM_SETPOS, 0, 255 & (searchData.customColor >> 16));
            switch (updateInfo.check) {
            case UpdateInformation::NotifyAny  : CheckRadioButton(hwndDlg, IDC_OPTIONS_UPDATE_ANY, IDC_OPTIONS_UPDATE_NONE, IDC_OPTIONS_UPDATE_ANY   ); break;
            case UpdateInformation::DoNotCheck : CheckRadioButton(hwndDlg, IDC_OPTIONS_UPDATE_ANY, IDC_OPTIONS_UPDATE_NONE, IDC_OPTIONS_UPDATE_NONE  ); break;
            default                            : CheckRadioButton(hwndDlg, IDC_OPTIONS_UPDATE_ANY, IDC_OPTIONS_UPDATE_NONE, IDC_OPTIONS_UPDATE_STABLE); break;
            }
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
        {
            int newElasticProgressTime;
            if (!validateSpin(newElasticProgressTime, hwndDlg, IDC_OPTIONS_ELASTIC_PROGRESS_SPIN, L"Time must be between 1 and 20 seconds.")) return TRUE;
            if (SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                int indicatorNumber = searchData.allocatedIndicator;
                bool override = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_OVERRIDE, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (!searchData.allocatedIndicator || override)
                    if (!validateSpin(indicatorNumber, hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, L"Indicator number must be between 9 and 20.")) return TRUE;
                int indicatorAlpha, indicatorRed, indicatorGreen, indicatorBlue;
                if (  !validateSpin(indicatorAlpha , hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_SPIN , L"Alpha transparency must be between 0 and 255.")
                   || !validateSpin(indicatorRed   , hwndDlg, IDC_OPTIONS_INDICATOR_RED_SPIN   , L"Red amount must be between 0 and 255.")
                   || !validateSpin(indicatorGreen , hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_SPIN , L"Green amount must be between 0 and 255.")
                   || !validateSpin(indicatorBlue  , hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_SPIN  , L"Blue amount must be between 0 and 255.")
                   ) return TRUE;
                int indicatorColor = indicatorRed | (indicatorGreen << 8) | (indicatorBlue << 16);
                if (searchData.customAlpha != indicatorAlpha || searchData.customColor != indicatorColor) {
                    searchData.customAlpha = indicatorAlpha;
                    searchData.customColor = indicatorColor;
                    sci.IndicSetStyle(searchData.customIndicator, Scintilla::IndicatorStyle::FullBox);
                    sci.IndicSetFore (searchData.customIndicator, searchData.customColor);
                    sci.IndicSetAlpha(searchData.customIndicator, static_cast<Scintilla::Alpha>(searchData.customAlpha));
                    sci.IndicSetUnder(searchData.customIndicator, true);
                }
                searchData.forceUserIndicator = override;
                searchData.customIndicator    = indicatorNumber;
                if (!searchData.allocatedIndicator || override) searchData.userIndicator = indicatorNumber;
                SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_OVERRIDE, BM_SETCHECK, searchData.forceUserIndicator ? BST_CHECKED : BST_UNCHECKED, 0);
                if (!searchData.enableCustomIndicator) {
                    searchData.enableCustomIndicator = true;
                    if (!searchData.dialog) {
                        searchData.indicator = searchData.customIndicator;
                        searchData.autoClear = true;
                    }
                }
                else if (searchData.indicator < 21 && !searchData.dialog) searchData.indicator = searchData.customIndicator;
            }
            else {
                searchData.enableCustomIndicator = false;
                if (searchData.indicator < 21 && !searchData.dialog) {
                    searchData.indicator = 31;
                    searchData.autoClear = false;
                }
            }
            elasticProgressTime = newElasticProgressTime;
            showOnMenuBar    = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_MENUBAR          , BM_GETCHECK, 0, 0) == BST_CHECKED;
            replaceStaysPut  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_REPLACE_STAYS_PUT, BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendSingleLine = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_SINGLELINE, BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendFullLines  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ROWS      , BM_GETCHECK, 0, 0) == BST_CHECKED;
            extendZeroWidth  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_EXTEND_ZEROWIDTH , BM_GETCHECK, 0, 0) == BST_CHECKED;
            updateInfo.check = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_UPDATE_ANY   , BM_GETCHECK, 0, 0) == BST_CHECKED ? UpdateInformation::NotifyAny
                             : SendDlgItemMessage(hwndDlg, IDC_OPTIONS_UPDATE_NONE  , BM_GETCHECK, 0, 0) == BST_CHECKED ? UpdateInformation::DoNotCheck
                                                                                                                        : UpdateInformation::NotifyStable;
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        case IDC_OPTIONS_INDICATOR_ENABLED:
        case IDC_OPTIONS_INDICATOR_OVERRIDE:
        {
            bool enabled = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_ENABLED , BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
            bool forced  = SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_OVERRIDE, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;
            bool number  = IsWindowEnabled(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER));
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA      ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_ALPHA_LABEL), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED        ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_RED_LABEL  ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN      ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_GREEN_LABEL), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE       ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_BLUE_LABEL ), enabled);
            EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_OVERRIDE), enabled && searchData.allocatedIndicator);
            if (enabled && (forced || !searchData.allocatedIndicator)) {
                if (!number) {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER      ), true);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_LABEL), true);
                    SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, UDM_SETPOS, 0, searchData.userIndicator);
                }
            }
            else {
                if (number) {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER      ), false);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_LABEL), false);
                }
                SendDlgItemMessage(hwndDlg, IDC_OPTIONS_INDICATOR_NUMBER_SPIN, UDM_SETPOS, 0,
                                  forced || !searchData.allocatedIndicator ? searchData.userIndicator : searchData.allocatedIndicator);
            }
        }
            break;
        }
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
