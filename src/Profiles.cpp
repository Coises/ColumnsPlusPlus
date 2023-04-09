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
#include <regex>
#include <string.h>
#include "resource.h"
#include "commctrl.h"

auto FindStringExact(HWND dialog, int id, const std::wstring& wstring, int position = -1) {
    return SendDlgItemMessage(dialog, id, CB_FINDSTRINGEXACT, static_cast<WPARAM>(position), reinterpret_cast<LPARAM>(wstring.data()));
}

INT_PTR CALLBACK profileDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ColumnsPlusPlusData* data;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        data = reinterpret_cast<ColumnsPlusPlusData*>(lParam);
    }
    else data = reinterpret_cast<ColumnsPlusPlusData*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    return data->profileDialogProc(hwndDlg, uMsg, wParam, lParam);
}


void ColumnsPlusPlusData::showElasticProfile() {
    DocumentData* ddp = getDocument();
    if (!settings.elasticEnabled || !settings.overrideTabSize) ddp->tabOriginal = sci.TabWidth();
    if (!DialogBoxParam(dllInstance, MAKEINTRESOURCE(IDD_ELASTIC_TABSTOPS_PROFILE), nppData._nppHandle,
                        ::profileDialogProc, reinterpret_cast<LPARAM>(this))) {
        ddp->settings = settings;
        if (settings.elasticEnabled) {
            sci.SetTabWidth(settings.overrideTabSize ? settings.minimumOrLeadingTabSize : ddp->tabOriginal);
            analyzeTabstops(*ddp);
            setTabstops(*ddp);
        }
    }
}


void profileToDialog(HWND hwndDlg, const ElasticTabsProfile& etp) {
    SendDlgItemMessage(hwndDlg, IDC_LEADING_TABS_INDENT, BM_SETCHECK, etp.leadingTabsIndent ? BST_CHECKED : BST_UNCHECKED, 0);
    SendDlgItemMessage(hwndDlg, IDC_LINE_UP_ALL, BM_SETCHECK, etp.lineUpAll ? BST_CHECKED : BST_UNCHECKED, 0);
    SendDlgItemMessage(hwndDlg, IDC_TREAT_EOL_AS_TAB, BM_SETCHECK, etp.treatEolAsTab ? BST_CHECKED : BST_UNCHECKED, 0);
    SendDlgItemMessage(hwndDlg, IDC_OVERRIDE_TAB_SIZE, BM_SETCHECK, etp.overrideTabSize ? BST_CHECKED : BST_UNCHECKED, 0);
    SendDlgItemMessage(hwndDlg, IDC_OVERRIDE_TAB_SIZE_SPIN, UDM_SETPOS, 0, etp.minimumOrLeadingTabSize);
    SendDlgItemMessage(hwndDlg, IDC_MINIMUM_SPACE_SPIN, UDM_SETPOS, 0, etp.minimumSpaceBetweenColumns);
    if (!etp.overrideTabSize) EnableWindow(GetDlgItem(hwndDlg, IDC_OVERRIDE_TAB_SIZE_VALUE), 0);
}


ElasticTabsProfile dialogToProfile(HWND hwndDlg) {
    ElasticTabsProfile etp;
    etp.leadingTabsIndent = SendDlgItemMessage(hwndDlg, IDC_LEADING_TABS_INDENT, BM_GETCHECK, 0, 0) == BST_CHECKED;
    etp.lineUpAll = SendDlgItemMessage(hwndDlg, IDC_LINE_UP_ALL, BM_GETCHECK, 0, 0) == BST_CHECKED;
    etp.treatEolAsTab = SendDlgItemMessage(hwndDlg, IDC_TREAT_EOL_AS_TAB, BM_GETCHECK, 0, 0) == BST_CHECKED;
    etp.overrideTabSize = SendDlgItemMessage(hwndDlg, IDC_OVERRIDE_TAB_SIZE, BM_GETCHECK, 0, 0) == BST_CHECKED;
    auto r = SendDlgItemMessage(hwndDlg, IDC_OVERRIDE_TAB_SIZE_SPIN, UDM_GETPOS, 0, 0);
    etp.minimumOrLeadingTabSize = HIWORD(r) ? 4 : LOWORD(r);
    r = SendDlgItemMessage(hwndDlg, IDC_MINIMUM_SPACE_SPIN, UDM_GETPOS, 0, 0);
    etp.minimumSpaceBetweenColumns = HIWORD(r) ? 2 : LOWORD(r);
    return etp;
}


void checkIfChanged(HWND hwndDlg, const std::map <std::wstring, ElasticTabsProfile>& profiles) {
    auto i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETCURSEL, 0, 0);
    size_t n = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXTLEN, i, 0);
    std::wstring profileName(n, 0);
    SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(profileName.data()));
    if (profileName[0] == L'(') return;
    if (profileName[0] == L'*') profileName = profileName.substr(2);
    if (profiles.contains(profileName)) {
        const ElasticTabsProfile& reference = profiles.at(profileName);
        ElasticTabsProfile dialog = dialogToProfile(hwndDlg);
        if (dialog == reference) {
            if (n != profileName.length()) {
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_DELETESTRING, i, 0);
                i = FindStringExact(hwndDlg, IDC_PROFILE_SELECT, profileName);
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL, i, 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE),
                             SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_GETCHECK, 0, 0) != BST_CHECKED);
            }
        }
        else {
            if (n == profileName.length()) {
                profileName = L"* " + profileName;
                i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(profileName.data()));
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL, i, 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE), FALSE);
            }
        }
    }
}


std::wstring getSelectedProfileName(HWND hwndDlg) {
    auto i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETCURSEL, 0, 0);
    auto n = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXTLEN, i, 0);
    std::wstring profileName(n, 0);
    SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(profileName.data()));
    return profileName;
}


INT_PTR CALLBACK profileSaveDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    std::wstring* pName;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        pName = reinterpret_cast<std::wstring*>(lParam);
    }
    else pName = reinterpret_cast<std::wstring*>(GetWindowLongPtr(hwndDlg, DWLP_USER));

    HWND hwndOwner;
    RECT rc, rcDlg, rcOwner;

    switch (uMsg) {

    case WM_DESTROY:
        return TRUE;

    case WM_INITDIALOG:
    {
        if ((hwndOwner = GetParent(hwndDlg)) == NULL) hwndOwner = GetDesktopWindow();
        GetWindowRect(hwndOwner, &rcOwner);
        GetWindowRect(hwndDlg, &rcDlg);
        CopyRect(&rc, &rcOwner);
        OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
        OffsetRect(&rc, -rc.left, -rc.top);
        OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
        SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + rc.right / 2, rcOwner.top + rc.bottom / 2, 0, 0, SWP_NOSIZE);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
            {
                int n = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PROFILE_SAVE_NAME));
                pName->resize(n);
                GetDlgItemText(hwndDlg, IDC_PROFILE_SAVE_NAME, pName->data(), n + 1);
                size_t i = pName->find_first_not_of(L' ');
                if (i == std::wstring::npos) return TRUE;
                *pName = pName->substr(i);
                i = pName->find_last_not_of(L' ');
                *pName = pName->substr(0, i + 1);
                if ((*pName)[0] == '(' || (*pName)[0] == '*') return TRUE;
                if (!_wcsicmp(pName->data(), L"Classic") || !_wcsicmp(pName->data(), L"General") || !_wcsicmp(pName->data(), L"Tabular"))
                    return TRUE;
                EndDialog(hwndDlg, 0);
                return TRUE;
            }
        case IDC_PROFILE_SAVE_NAME:
            if (HIWORD(wParam) == EN_CHANGE) {
                bool canOK = false;
                bool useBT = false;
                EDITBALLOONTIP ebt;
                ebt.cbStruct = sizeof(EDITBALLOONTIP);
                ebt.pszTitle = L"";
                ebt.ttiIcon = TTI_NONE;
                HWND edit = reinterpret_cast<HWND>(lParam);
                int n = GetWindowTextLength(edit);
                if (n) {
                    std::wstring name(n, 0);
                    GetWindowText(edit, name.data(), n + 1);
                    size_t i = name.find_first_not_of(L' ');
                    if (i != std::wstring::npos) {
                        name = name.substr(i);
                        i = name.find_last_not_of(L' ');
                        name = name.substr(0, i + 1);
                        if (name[0] == '(' || name[0] == '*') {
                            ebt.pszText = L"Profile names cannot begin with an open parenthesis or an asterisk.";
                            useBT = true;
                        }
                        else if ( !_wcsicmp(name.data(), L"Classic")
                               || !_wcsicmp(name.data(), L"General")
                               || !_wcsicmp(name.data(), L"Tabular") ) {
                            ebt.pszText = L"You cannot replace a built-in profile (Classic, General or Tabular).";
                            useBT = true;
                        }
                        else canOK = true;
                    }
                }
                EnableWindow(GetDlgItem(hwndDlg, IDOK), canOK);
                if (useBT) SendMessage(edit, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                      else SendMessage(edit, EM_HIDEBALLOONTIP, 0, 0);
                return TRUE;
            }
        }
        break;

    default:
        break;
    }
    return FALSE;
}


INT_PTR CALLBACK profileRenameDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    std::wstring* pName;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        pName = reinterpret_cast<std::wstring*>(lParam);
    }
    else pName = reinterpret_cast<std::wstring*>(GetWindowLongPtr(hwndDlg, DWLP_USER));

    HWND hwndOwner;
    RECT rc, rcDlg, rcOwner;

    switch (uMsg) {

    case WM_DESTROY:
        return TRUE;

    case WM_INITDIALOG:
    {
        if ((hwndOwner = GetParent(hwndDlg)) == NULL) hwndOwner = GetDesktopWindow();
        GetWindowRect(hwndOwner, &rcOwner);
        GetWindowRect(hwndDlg, &rcDlg);
        CopyRect(&rc, &rcOwner);
        OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
        OffsetRect(&rc, -rc.left, -rc.top);
        OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
        SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + rc.right / 2, rcOwner.top + rc.bottom / 2, 0, 0, SWP_NOSIZE);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;
        case IDOK:
        {
            int n = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PROFILE_RENAME_NAME));
            pName->resize(n);
            GetDlgItemText(hwndDlg, IDC_PROFILE_RENAME_NAME, pName->data(), n + 1);
            size_t i = pName->find_first_not_of(L' ');
            if (i == std::wstring::npos) return TRUE;
            *pName = pName->substr(i);
            i = pName->find_last_not_of(L' ');
            *pName = pName->substr(0, i + 1);
            if ((*pName)[0] == '(' || (*pName)[0] == '*') return TRUE;
            if (!_wcsicmp(pName->data(), L"Classic") || !_wcsicmp(pName->data(), L"General") || !_wcsicmp(pName->data(), L"Tabular"))
                return TRUE;
            HWND parent = GetParent(hwndDlg);
            if (FindStringExact(parent, IDC_PROFILE_SELECT, *pName) != CB_ERR) {
                EDITBALLOONTIP ebt;
                ebt.cbStruct = sizeof(EDITBALLOONTIP);
                ebt.pszTitle = L"";
                ebt.ttiIcon = TTI_NONE;
                ebt.pszText = L"This profile name is already in use.";
                SendMessage(GetDlgItem(hwndDlg, IDC_PROFILE_RENAME_NAME), EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                return TRUE;
            }
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        case IDC_PROFILE_RENAME_NAME:
            if (HIWORD(wParam) == EN_CHANGE) {
                bool canOK = false;
                bool useBT = false;
                EDITBALLOONTIP ebt;
                ebt.cbStruct = sizeof(EDITBALLOONTIP);
                ebt.pszTitle = L"";
                ebt.ttiIcon = TTI_NONE;
                HWND edit = reinterpret_cast<HWND>(lParam);
                int n = GetWindowTextLength(edit);
                if (n) {
                    std::wstring name(n, 0);
                    GetWindowText(edit, name.data(), n + 1);
                    size_t i = name.find_first_not_of(L' ');
                    if (i != std::wstring::npos) {
                        name = name.substr(i);
                        i = name.find_last_not_of(L' ');
                        name = name.substr(0, i + 1);
                        if (name[0] == '(' || name[0] == '*') {
                            ebt.pszText = L"Profile names cannot begin with an open parenthesis or an asterisk.";
                            useBT = true;
                        }
                        else canOK = true;
                    }
                }
                EnableWindow(GetDlgItem(hwndDlg, IDOK), canOK);
                if (useBT) SendMessage(edit, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&ebt));
                else SendMessage(edit, EM_HIDEBALLOONTIP, 0, 0);
                return TRUE;
            }
            break;
        default:;
        }
        break;

    default:;
    }
    return FALSE;
}


void saveProfile(HWND hwndDlg, ColumnsPlusPlusData& data) {
    std::wstring saveName;
    if (!DialogBoxParam(data.dllInstance, MAKEINTRESOURCE(IDD_PROFILE_SAVE), hwndDlg,
        ::profileSaveDialogProc, reinterpret_cast<LPARAM>(&saveName))) {
        if (saveName.length()) {
            bool saving = false;
            LRESULT i = FindStringExact(hwndDlg, IDC_PROFILE_SELECT, saveName);
            if (i == CB_ERR) {
                saving = true;
                i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(saveName.data()));
                SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(saveName.data()));
            }
            else if (IDOK == MessageBox(hwndDlg,
                (L"There is already a profile named " + saveName + L".\nDo you want to replace it?").data(),
                L"Save Elastic Tabstops Profile", MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2))
                saving = true;
            if (saving) {
                data.profiles[saveName] = dialogToProfile(hwndDlg);
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL, i, 0);
                LRESULT n = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXTLEN, 0, 0);
                std::wstring firstProfile(n, 0);
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXT, 0, reinterpret_cast<LPARAM>(firstProfile.data()));
                if (!n || firstProfile[0] == L'(' || firstProfile[0] == L'*')
                    SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_DELETESTRING, 0, 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE),
                    SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_GETCHECK, 0, 0) != BST_CHECKED ? TRUE : FALSE);
                std::wstring extension = data.getFileExtension();
                SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_SETCHECK,
                    data.extensionToProfile.contains(extension) && data.extensionToProfile[extension] == saveName
                    ? BST_CHECKED : BST_UNCHECKED, 0);
            }
        }
    }
}


void renameProfile(HWND hwndDlg, ColumnsPlusPlusData& data) {
    std::wstring newName;
    if (!DialogBoxParam(data.dllInstance, MAKEINTRESOURCE(IDD_PROFILE_RENAME), hwndDlg,
                        profileRenameDialogProc, reinterpret_cast<LPARAM>(&newName))
      && newName.length()
      && FindStringExact(hwndDlg, IDC_PROFILE_SELECT, newName) == CB_ERR) {
        LRESULT i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETCURSEL, 0, 0);
        LRESULT n = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXTLEN, i, 0);
        std::wstring oldName(n, 0);
        SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(oldName.data()));
        SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_DELETESTRING, i, 0);
        i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(newName.data()));
        SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL, i, 0);
        auto node = data.profiles.extract(oldName);
        node.key() = newName;
        data.profiles.insert(std::move(node));
        for (auto etp : data.extensionToProfile) if (etp.second == oldName) etp.second = newName;
        i = FindStringExact(hwndDlg, IDC_DEFAULT_PROFILE, oldName);
        bool isDefault = i == SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_GETCURSEL, 0, 0);
        SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_DELETESTRING, i, 0);
        i = SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(newName.data()));
        if (isDefault) SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_SETCURSEL, i, 0);
    }
}


BOOL ColumnsPlusPlusData::profileDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

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

            SendDlgItemMessage(hwndDlg, IDC_OVERRIDE_TAB_SIZE_SPIN, UDM_SETRANGE, 0, MAKELPARAM(999, 1));
            SendDlgItemMessage(hwndDlg, IDC_MINIMUM_SPACE_SPIN, UDM_SETRANGE, 0, MAKELPARAM(999, 1));

            profileToDialog(hwndDlg, settings);
            
            if (disableOverSize > 0) {
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_SIZE, BM_SETCHECK, BST_CHECKED, 0);
                SetDlgItemInt(hwndDlg, IDC_DISABLE_FILE_SIZE_VALUE, disableOverSize, FALSE);
            }
            else {
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_SIZE, BM_SETCHECK, BST_UNCHECKED, 0);
                SetDlgItemInt(hwndDlg, IDC_DISABLE_FILE_SIZE_VALUE, -disableOverSize, FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE_FILE_SIZE_VALUE), FALSE);
            }

            if (disableOverLines > 0) {
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_LINES, BM_SETCHECK, BST_CHECKED, 0);
                SetDlgItemInt(hwndDlg, IDC_DISABLE_FILE_LINES_VALUE, disableOverLines, FALSE);
            }
            else {
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_LINES, BM_SETCHECK, BST_UNCHECKED, 0);
                SetDlgItemInt(hwndDlg, IDC_DISABLE_FILE_LINES_VALUE, -disableOverLines, FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE_FILE_LINES_VALUE), FALSE);
            }

            bool profileFound = false;

            for (const auto& p : profiles) {
                auto i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(p.first.data()));
                SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(p.first.data()));
                if (p.first == settings.profileName) {
                    SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL, i, 0);
                    SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_SETCURSEL, i, 0);
                    profileFound = true;
                }
            }

            if (!profileFound) {
                auto i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"(not saved)"));
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL, i, 0);
                SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_SETCURSEL, 0, 0);
            }

            std::wstring defaultProfile = extensionToProfile.contains(L"*") ? extensionToProfile[L"*"] : L"*";
            if (defaultProfile == L"*") {
                SendDlgItemMessage(hwndDlg, IDC_DEFAULT_KEEP, BM_SETCHECK, BST_CHECKED, 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_PROFILE), FALSE);
            }
            else if (defaultProfile == L"") {
                SendDlgItemMessage(hwndDlg, IDC_DEFAULT_DISABLE, BM_SETCHECK, BST_CHECKED, 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_PROFILE), FALSE);
            }
            else {
                auto i = FindStringExact(hwndDlg, IDC_PROFILE_SELECT, defaultProfile);
                if (i == CB_ERR) {
                    SendDlgItemMessage(hwndDlg, IDC_DEFAULT_KEEP, BM_SETCHECK, BST_CHECKED, 0);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_PROFILE), FALSE);
                    extensionToProfile[L"*"] = L"*";
                }
                else {
                    SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_SETCURSEL, i, 0);
                    SendDlgItemMessage(hwndDlg, IDC_DEFAULT_ENABLE, BM_SETCHECK, BST_CHECKED, 0);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_PROFILE), TRUE);
                }
            }

            std::wstring extension = getFileExtension();
            std::wstring extText = extension == L"" ? L"new files." : extension == L"." ? L"files with no extension." : L"*." + extension + L" files.";
            SetDlgItemText(hwndDlg, IDC_ENABLE_FILE_TYPE, (L"&Automatically enable this profile when opening " + extText).data());
            SetDlgItemText(hwndDlg, IDC_DISABLE_FILE_TYPE, (L"&when opening " + extText).data());
            if (extensionToProfile.contains(extension)) {
                std::wstring extProfileName = extensionToProfile[extension];
                if (extProfileName == L"") {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE), FALSE);
                    SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_SETCHECK, BST_UNCHECKED, 0);
                    SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_SETCHECK, BST_CHECKED, 0);
                }
                else if (extProfileName == settings.profileName) {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE), TRUE);
                    SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_SETCHECK, BST_CHECKED, 0);
                    SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_SETCHECK, BST_UNCHECKED, 0);
                }
                else {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE), profileFound);
                    SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_SETCHECK, BST_UNCHECKED, 0);
                    SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_SETCHECK, BST_UNCHECKED, 0);
                }
            }
            else {
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE), profileFound);
                SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_SETCHECK, BST_UNCHECKED, 0);
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_SETCHECK, BST_UNCHECKED, 0);
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
                static_cast<ElasticTabsProfile&>(settings) = dialogToProfile(hwndDlg);
                disableOverSize = GetDlgItemInt(hwndDlg, IDC_DISABLE_FILE_SIZE_VALUE, NULL, FALSE);
                if (!SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_SIZE, BM_GETCHECK, 0, 0)) disableOverSize *= -1;
                disableOverLines = GetDlgItemInt(hwndDlg, IDC_DISABLE_FILE_LINES_VALUE, NULL, FALSE);
                if (!SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_LINES, BM_GETCHECK, 0, 0)) disableOverLines *= -1;
                auto i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETCURSEL, 0, 0);
                auto n = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXTLEN, i, 0);
                std::wstring profileName(n, 0);
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(profileName.data()));
                settings.profileName = profiles.contains(profileName) ? profileName : L"";
                std::wstring extension = getFileExtension();
                if (SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    extensionToProfile[extension] = L"";
                else if (settings.profileName.length() && SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    extensionToProfile[extension] = settings.profileName;
                else {
                    auto p = extensionToProfile.find(extension);
                    if (p != extensionToProfile.end() && (p->second == profileName || p->second == L"")) extensionToProfile.erase(p);
                }
                if (SendDlgItemMessage(hwndDlg, IDC_DEFAULT_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    i = SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_GETCURSEL, 0, 0);
                    n = SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_GETLBTEXTLEN, i, 0);
                    std::wstring defaultProfile(n, 0);
                    SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(defaultProfile.data()));
                    extensionToProfile[L"*"] = profiles.contains(defaultProfile) ? defaultProfile : L"*";
                }
                else extensionToProfile[L"*"] = SendDlgItemMessage(hwndDlg, IDC_DEFAULT_DISABLE, BM_GETCHECK, 0, 0) == BST_CHECKED ? L"" : L"*";
                EndDialog(hwndDlg, 0);
                return TRUE;
            }
        case IDC_OVERRIDE_TAB_SIZE:
            EnableWindow(GetDlgItem(hwndDlg, IDC_OVERRIDE_TAB_SIZE_VALUE),
                SendDlgItemMessage(hwndDlg, IDC_OVERRIDE_TAB_SIZE, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
            checkIfChanged(hwndDlg, profiles);
            break;
        case IDC_DEFAULT_KEEP:
        case IDC_DEFAULT_DISABLE:
        case IDC_DEFAULT_ENABLE:
            EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_PROFILE),
                SendDlgItemMessage(hwndDlg, IDC_DEFAULT_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
            break;
        case IDC_DISABLE_FILE_TYPE:
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE),
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_GETCHECK, 0, 0) != BST_CHECKED ? TRUE : FALSE);
            break;
        case IDC_DISABLE_FILE_SIZE:
            EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE_FILE_SIZE_VALUE),
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_SIZE, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
            break;
        case IDC_DISABLE_FILE_LINES:
            EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE_FILE_LINES_VALUE),
                SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_LINES, BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE);
            break;
        case IDC_LEADING_TABS_INDENT:
        case IDC_LINE_UP_ALL:
        case IDC_TREAT_EOL_AS_TAB:
        case IDC_OVERRIDE_TAB_SIZE_SPIN:
        case IDC_MINIMUM_SPACE_SPIN:
            checkIfChanged(hwndDlg, profiles);
            break;
        case IDC_PROFILE_SELECT:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                auto i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETCURSEL, 0, 0);
                auto n = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXTLEN, i, 0);
                std::wstring profileName(n, 0);
                SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(profileName.data()));
                if (profiles.contains(profileName)) {
                    profileToDialog(hwndDlg, profiles[profileName]);
                    n = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXTLEN, 0, 0);
                    std::wstring firstProfile(n, 0);
                    SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_GETLBTEXT, 0, reinterpret_cast<LPARAM>(firstProfile.data()));
                    if (!n || firstProfile[0] == L'(' || firstProfile[0] == L'*')
                        SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_DELETESTRING, 0, 0);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE),
                        SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_GETCHECK, 0, 0) != BST_CHECKED ? TRUE : FALSE);
                    std::wstring extension = getFileExtension();
                    SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_SETCHECK,
                        extensionToProfile.contains(extension) && extensionToProfile[extension] == profileName
                        ? BST_CHECKED : BST_UNCHECKED, 0);
                }
            }
            break;
        case IDC_PROFILE_SAVE:
            saveProfile(hwndDlg, *this);
            break;
        default:;
        }
        break;

    case WM_NOTIFY:
        switch (reinterpret_cast<NMHDR*>(lParam)->code) {
        case BCN_DROPDOWN:
            if (NMBCDROPDOWN& bd = *reinterpret_cast<NMBCDROPDOWN*>(lParam); bd.hdr.idFrom == IDC_PROFILE_SAVE) {
                std::wstring selectedProfile = getSelectedProfileName(hwndDlg);
                bool canRenameOrDelete = selectedProfile != L"Classic"
                                      && selectedProfile != L"General"
                                      && selectedProfile != L"Tabular"
                                      && profiles.contains(selectedProfile);
                bool canQuickSave      = selectedProfile.length() > 2
                                      && selectedProfile.substr(0, 2) == L"* "
                                      && selectedProfile != L"* Classic"
                                      && selectedProfile != L"* General"
                                      && selectedProfile != L"* Tabular"
                                      && profiles.contains(selectedProfile.substr(2));
                POINT pt;
                pt.x = bd.rcButton.left;
                pt.y = bd.rcButton.bottom;
                ClientToScreen(bd.hdr.hwndFrom, &pt);
                HMENU pum = CreatePopupMenu();
                AppendMenu(pum, canQuickSave ? MF_STRING : MF_STRING | MF_GRAYED, 1, L"&Save");
                AppendMenu(pum, MF_STRING, 2, L"Save &As...");
                AppendMenu(pum, canRenameOrDelete ? MF_STRING : MF_STRING|MF_GRAYED, 3, L"&Rename...");
                AppendMenu(pum, canRenameOrDelete ? MF_STRING : MF_STRING|MF_GRAYED, 4, L"&Delete...");
                int choice = TrackPopupMenu(pum, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_NONOTIFY|TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                DestroyMenu(pum);
                switch (choice) {
                case 1:
                    {
                        std::wstring saveName = selectedProfile.substr(2);
                        profiles[saveName] = dialogToProfile(hwndDlg);
                        SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_DELETESTRING, 0, 0);
                        SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL,
                            FindStringExact(hwndDlg, IDC_PROFILE_SELECT, saveName), 0);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_FILE_TYPE),
                            SendDlgItemMessage(hwndDlg, IDC_DISABLE_FILE_TYPE, BM_GETCHECK, 0, 0) != BST_CHECKED ? TRUE : FALSE);
                        std::wstring extension = getFileExtension();
                        SendDlgItemMessage(hwndDlg, IDC_ENABLE_FILE_TYPE, BM_SETCHECK,
                            extensionToProfile.contains(extension) && extensionToProfile[extension] == saveName
                            ? BST_CHECKED : BST_UNCHECKED, 0);
                    }
                    break;
                case 2:
                    saveProfile(hwndDlg, *this);
                    break;
                case 3:
                    renameProfile(hwndDlg, *this);
                    break;
                case 4:
                    {
                        int n = 0;
                        std::wstring msg;
                        for (const auto& etp : extensionToProfile) if (etp.second == selectedProfile) { msg = etp.first; ++n; }
                        bool isDefaultProfile = extensionToProfile.contains(L"*") && extensionToProfile[L"*"] == selectedProfile;
                        msg = isDefaultProfile ?
                            ( n < 2 ? L"Profile \"" + selectedProfile + L"\" is set as the default profile.\nAre you sure you want to delete it?"
                            : n > 2 ? L"Profile \"" + selectedProfile + L"\" is automatically enabled for "
                                                    + std::to_wstring(n-1) 
                                                    + L" file extensions and is set as the default profile.\nAre you sure you want to delete it?"
                                    : L"Profile \"" + selectedProfile + L"\" is automatically enabled for "
                                                    + ( msg == L""  ? L"new files"
                                                      : msg == L"." ? L"files with no extension"
                                                                    : L"\"*." + msg + L"\" files" )
                                                    + L" and is set as the default profile.\nAre you sure you want to delete it?" )
                          : ( n < 1 ? L"Are you sure you want to delete profile \"" + selectedProfile + L"\"?"
                            : n > 1 ? L"Profile \"" + selectedProfile + L"\" is automatically enabled for "
                                                    + std::to_wstring(n) 
                                                    + L" file extensions.\nAre you sure you want to delete it?"
                                    : L"Profile \"" + selectedProfile + L"\" is automatically enabled for "
                                                    + ( msg == L""  ? L"new files."
                                                      : msg == L"." ? L"files with no extension."
                                                                    : L"\"*." + msg + L"\" files." )
                                                    + L"\nAre you sure you want to delete it?" );
                        if (IDYES == MessageBox(hwndDlg, msg.data(), L"Delete Elastic Tabstops Profile", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1)) {
                            profiles.erase(selectedProfile);
                            for (auto p = extensionToProfile.begin(); p != extensionToProfile.end();)
                                if (p->second == selectedProfile) extensionToProfile.erase(p++);
                                else ++p;
                            auto i = FindStringExact(hwndDlg, IDC_PROFILE_SELECT, selectedProfile);
                            if (i != CB_ERR) SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_DELETESTRING, i, 0);
                            i = SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"(not saved)"));
                            SendDlgItemMessage(hwndDlg, IDC_PROFILE_SELECT, CB_SETCURSEL, i, 0);
                            i = FindStringExact(hwndDlg, IDC_DEFAULT_PROFILE, selectedProfile);
                            if (i != CB_ERR) SendDlgItemMessage(hwndDlg, IDC_DEFAULT_PROFILE, CB_DELETESTRING, i, 0);
                            if (isDefaultProfile) {
                                extensionToProfile[L"*"] = L"*";
                                SendDlgItemMessage(hwndDlg, IDC_DEFAULT_KEEP, BM_SETCHECK, BST_CHECKED, 0);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_DEFAULT_PROFILE), FALSE);
                            }
                        }
                    }
                    break;
                default:;
                }
                return TRUE;
            }
            break;
        default:;
        }
        break;

    default:;

    }
    return FALSE;
}
