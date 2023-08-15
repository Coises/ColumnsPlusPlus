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
using namespace NPP;

static ColumnsPlusPlusData data;

static bool bypassNotifications = false;
static bool fileIsOpening = false;
static bool startupOrShutdown = true;

static void getScintillaPointers() {
    int currentEdit = 0;
    SendMessage(data.nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&currentEdit));
    data.activeScintilla = currentEdit ? data.nppData._scintillaSecondHandle : data.nppData._scintillaMainHandle;
    data.pointerScintilla = SendMessage(data.activeScintilla, static_cast<UINT>(Scintilla::Message::GetDirectPointer), 0, 0);
    data.sci.SetFnPtr(data.directStatusScintilla, data.pointerScintilla);
}

static void cmdWrap(void (ColumnsPlusPlusData::* cmdFunction)()) {
    bypassNotifications = true;
    getScintillaPointers();
    (data.*cmdFunction)();
    bypassNotifications = false;
}

static struct MenuDefinition {
    FuncItem elasticEnabled          = {TEXT("Elastic tabstops"                   ), []() {cmdWrap(&ColumnsPlusPlusData::toggleElasticEnabled  );}, 0, false, 0};
    FuncItem elasticProfile          = {TEXT("Profile..."                         ), []() {cmdWrap(&ColumnsPlusPlusData::showElasticProfile    );}, 0, false, 0};
    FuncItem separatorElastic        = {TEXT("---"                                ), 0                                                            , 0, false, 0};
    FuncItem search                  = {TEXT("Search..."                          ), []() {cmdWrap(&ColumnsPlusPlusData::showSearchDialog      );}, 0, false, 0};
    FuncItem addNumbers              = {TEXT("Add numbers..."                     ), []() {cmdWrap(&ColumnsPlusPlusData::addNumbers            );}, 0, false, 0};
    FuncItem averageNumbers          = {TEXT("Average numbers..."                 ), []() {cmdWrap(&ColumnsPlusPlusData::averageNumbers        );}, 0, false, 0};
    FuncItem calculate               = {TEXT("Calculate..."                       ), []() {cmdWrap(&ColumnsPlusPlusData::calculate             );}, 0, false, 0};
    FuncItem separatorAlign          = {TEXT("---"                                ), 0                                                            , 0, false, 0};
    FuncItem alignLeft               = {TEXT("Align left"                         ), []() {cmdWrap(&ColumnsPlusPlusData::alignLeft             );}, 0, false, 0};
    FuncItem alignRight              = {TEXT("Align right"                        ), []() {cmdWrap(&ColumnsPlusPlusData::alignRight            );}, 0, false, 0};
    FuncItem alignNumeric            = {TEXT("Align numeric"                      ), []() {cmdWrap(&ColumnsPlusPlusData::alignNumeric          );}, 0, false, 0};
    FuncItem separatorSort           = {TEXT("---"                                ), 0                                                            , 0, false, 0};
    FuncItem sortAscendingBinary     = {TEXT("Sort ascending (binary)"            ), []() {cmdWrap(&ColumnsPlusPlusData::sortAscendingBinary   );}, 0, false, 0};
    FuncItem sortDescendingBinary    = {TEXT("Sort descending (binary)"           ), []() {cmdWrap(&ColumnsPlusPlusData::sortDescendingBinary  );}, 0, false, 0};
    FuncItem sortAscendingLocale     = {TEXT("Sort ascending (locale)"            ), []() {cmdWrap(&ColumnsPlusPlusData::sortAscendingLocale   );}, 0, false, 0};
    FuncItem sortDescendingLocale    = {TEXT("Sort descending (locale)"           ), []() {cmdWrap(&ColumnsPlusPlusData::sortDescendingLocale  );}, 0, false, 0};
    FuncItem sortAscendingNumeric    = {TEXT("Sort ascending (numeric)"           ), []() {cmdWrap(&ColumnsPlusPlusData::sortAscendingNumeric  );}, 0, false, 0};
    FuncItem sortDescendingNumeric   = {TEXT("Sort descending (numeric)"          ), []() {cmdWrap(&ColumnsPlusPlusData::sortDescendingNumeric );}, 0, false, 0};
    FuncItem sortCustom              = {TEXT("Sort..."                            ), []() {cmdWrap(&ColumnsPlusPlusData::sortCustom            );}, 0, false, 0};
    FuncItem separatorConvert        = {TEXT("---"                                ), 0                                                            , 0, false, 0};
    FuncItem convertTabsToSpaces     = {TEXT("Convert tabs to spaces"             ), []() {cmdWrap(&ColumnsPlusPlusData::tabsToSpaces          );}, 0, false, 0};
    FuncItem separatedValuesToTabs   = {TEXT("Convert separated values to tabs..."), []() {cmdWrap(&ColumnsPlusPlusData::separatedValuesToTabs );}, 0, false, 0};
    FuncItem sabsToSeparatedValues   = {TEXT("Convert tabs to separated values..."), []() {cmdWrap(&ColumnsPlusPlusData::tabsToSeparatedValues );}, 0, false, 0};
    FuncItem separatorSettings       = {TEXT("---"                                ), 0                                                            , 0, false, 0};
    FuncItem decimalSeparatorIsComma = {TEXT("Decimal separator is comma"         ), []() {cmdWrap(&ColumnsPlusPlusData::toggleDecimalSeparator);}, 0, false, 0};
    FuncItem options                 = {TEXT("Options..."                         ), []() {cmdWrap(&ColumnsPlusPlusData::showOptionsDialog     );}, 0, false, 0};
    FuncItem about                   = {TEXT("Help/About..."                      ), []() {cmdWrap(&ColumnsPlusPlusData::showAboutDialog       );}, 0, false, 0};
} menuDefinition;

BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reasonForCall, LPVOID) {
    try {
        switch (reasonForCall) {
            case DLL_PROCESS_ATTACH:
                data.dllInstance = instance;
                bypassNotifications = false;
                startupOrShutdown = true;
                break;

            case DLL_PROCESS_DETACH:
                break;

            case DLL_THREAD_ATTACH:
                break;

            case DLL_THREAD_DETACH:
                break;
        }
    }
    catch (...) { return FALSE; }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData nppData) {
    data.nppData = nppData;
    data.directStatusScintilla = reinterpret_cast<Scintilla::FunctionDirect>
        (SendMessage(data.nppData._scintillaMainHandle, static_cast<UINT>(Scintilla::Message::GetDirectStatusFunction), 0, 0));
    data.initializeBuiltinElasticTabstopsProfiles();
    data.loadConfiguration();
}

extern "C" __declspec(dllexport) const TCHAR * getName() {
    return TEXT("Columns++");
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *n) {
    *n = sizeof(MenuDefinition) / sizeof(FuncItem);
    return &menuDefinition.elasticEnabled;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *np) {

    auto*& scnp = reinterpret_cast<Scintilla::NotificationData*&>(np);
    auto*& nmhdr = reinterpret_cast<NMHDR*&>(np);

    if (!bypassNotifications) switch (scnp->nmhdr.code) {
        
    case Scintilla::Notification::Modified:
        data.scnModified(scnp);
        break;

    case Scintilla::Notification::UpdateUI:
        data.scnUpdateUI(scnp);
        break;

    case Scintilla::Notification::Zoom:
        data.scnZoom(scnp);
        break;

    default:
      
        switch (nmhdr->code) {

        case NPPN_BEFORESHUTDOWN:
            startupOrShutdown = true;
            break;

        case NPPN_BUFFERACTIVATED:
            if (!startupOrShutdown && !fileIsOpening) {
                getScintillaPointers();
                data.bufferActivated();
            }
            break;

        case NPPN_CANCELSHUTDOWN:
            startupOrShutdown = false;
            break;

        case NPPN_FILEBEFOREOPEN:
            fileIsOpening = true;
            break;

        case NPPN_FILECLOSED:
            data.fileClosed(nmhdr);
            break;

        case NPPN_FILEOPENED:
            fileIsOpening = false;
            data.fileOpened(nmhdr);
            break;

        case NPPN_READY:
            if (data.showOnMenuBar) data.moveMenuToMenuBar();
            startupOrShutdown = false;
            data.elasticEnabledMenuItem = menuDefinition.elasticEnabled._cmdID;
            data.decimalSeparatorMenuItem = menuDefinition.decimalSeparatorIsComma._cmdID;
            getScintillaPointers();
            data.bufferActivated();
            break;

        case NPPN_SHUTDOWN:
            data.saveConfiguration();
            break;

        default:;
        }
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM) {return TRUE;}
extern "C" __declspec(dllexport) BOOL isUnicode() {return TRUE;}
