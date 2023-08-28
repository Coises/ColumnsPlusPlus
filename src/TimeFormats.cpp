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


INT_PTR CALLBACK timeFormatsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ColumnsPlusPlusData* data;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        data = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else data = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    return data->timeFormatsDialogProc(hwndDlg, uMsg, wParam, lParam);
}

void ColumnsPlusPlusData::showTimeFormatsDialog() {
    DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_TIME_FORMATS), nppData._nppHandle,
                   ::timeFormatsDialogProc, reinterpret_cast<LPARAM>(this));
}

BOOL ColumnsPlusPlusData::timeFormatsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {

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
        switch (timeScalarUnit) {
        case 0:
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_DAYS, IDC_TIME_FORMATS_SECONDS, IDC_TIME_FORMATS_DAYS   );
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"days");
            break;
        case 1:
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_DAYS, IDC_TIME_FORMATS_SECONDS, IDC_TIME_FORMATS_HOURS  );
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"hours");
            break;
        case 2:
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_DAYS, IDC_TIME_FORMATS_SECONDS, IDC_TIME_FORMATS_MINUTES);
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"minutes");
            break;
        default:
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_DAYS, IDC_TIME_FORMATS_SECONDS, IDC_TIME_FORMATS_SECONDS);
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"seconds");
        }
        switch (timePartialRule) {
        case 0:
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_PARTIAL_0, IDC_TIME_FORMATS_PARTIAL_3, IDC_TIME_FORMATS_PARTIAL_0);
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"days:hours");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"days:hours:minutes");
            break;
        case 1:  
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_PARTIAL_0, IDC_TIME_FORMATS_PARTIAL_3, IDC_TIME_FORMATS_PARTIAL_1);
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"hours:minutes");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"days:hours:minutes");
            break;
        case 2:  
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_PARTIAL_0, IDC_TIME_FORMATS_PARTIAL_3, IDC_TIME_FORMATS_PARTIAL_2);
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"hours:minutes");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"hours:minutes:seconds");
            break;
        default: 
            CheckRadioButton(hwndDlg, IDC_TIME_FORMATS_PARTIAL_0, IDC_TIME_FORMATS_PARTIAL_3, IDC_TIME_FORMATS_PARTIAL_3);
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"minutes:seconds");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"hours:minutes:seconds");
        }
        CheckDlgButton(hwndDlg, IDC_TIME_FORMATS_ENABLE_0, timeFormatEnable & 1);
        CheckDlgButton(hwndDlg, IDC_TIME_FORMATS_ENABLE_1, timeFormatEnable & 2);
        CheckDlgButton(hwndDlg, IDC_TIME_FORMATS_ENABLE_2, timeFormatEnable & 4);
        CheckDlgButton(hwndDlg, IDC_TIME_FORMATS_ENABLE_3, timeFormatEnable & 8);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
            timeScalarUnit   = IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_DAYS     ) == BST_CHECKED ? 0
                             : IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_HOURS    ) == BST_CHECKED ? 1
                             : IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_MINUTES  ) == BST_CHECKED ? 2 : 3;
            timePartialRule  = IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_PARTIAL_0) == BST_CHECKED ? 0
                             : IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_PARTIAL_1) == BST_CHECKED ? 1
                             : IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_PARTIAL_2) == BST_CHECKED ? 2 : 3;
            timeFormatEnable = (IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_ENABLE_0) == BST_CHECKED ? 1 : 0)
                             + (IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_ENABLE_1) == BST_CHECKED ? 2 : 0)
                             + (IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_ENABLE_2) == BST_CHECKED ? 4 : 0)
                             + (IsDlgButtonChecked(hwndDlg, IDC_TIME_FORMATS_ENABLE_3) == BST_CHECKED ? 8 : 0);
            EndDialog(hwndDlg, 0);
            return TRUE;
        case IDC_TIME_FORMATS_DAYS:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"days");
            break;
        case IDC_TIME_FORMATS_HOURS:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"hours");
            break;
        case IDC_TIME_FORMATS_MINUTES:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"minutes");
            break;
        case IDC_TIME_FORMATS_SECONDS:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_0, L"seconds");
            break;
        case IDC_TIME_FORMATS_PARTIAL_0:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"days:hours");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"days:hours:minutes");
            break;
        case IDC_TIME_FORMATS_PARTIAL_1:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"hours:minutes");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"days:hours:minutes");
            break;
        case IDC_TIME_FORMATS_PARTIAL_2:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"hours:minutes");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"hours:minutes:seconds");
            break;
        case IDC_TIME_FORMATS_PARTIAL_3:
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_1, L"minutes:seconds");
            SetDlgItemText(hwndDlg, IDC_TIME_FORMATS_LABEL_2, L"hours:minutes:seconds");
            break;
        }
    }
    return FALSE;
}
