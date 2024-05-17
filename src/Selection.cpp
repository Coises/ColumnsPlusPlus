// This file is part of Columns++ for Notepad++.
// Copyright 2024 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

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

namespace {

    struct SelectionInformation {
        ColumnsPlusPlusData&           data;
        DocumentData* const            ddp;
        const int                      blank1440;
        const int                      count;
        const Scintilla::SelectionMode mode;
        Scintilla::Line top, bottom;
        int             left, right;

        int blankCount(intptr_t n) const { return static_cast<int>(n >= 0 ? (n * 2880 + blank1440) / (2 * blank1440) : (n * 2880 - blank1440) / (2 * blank1440)); }
        int blankWidth(intptr_t n) const { return static_cast<int>(n >= 0 ? (n * blank1440 + 720) / 1440 : (n * blank1440 - 720) / 1440); }

        SelectionInformation(ColumnsPlusPlusData& data) : data(data),
            blank1440(data.sci.TextWidth(STYLE_DEFAULT, std::string(1440, ' ').data())),
            ddp(data.settings.elasticEnabled ? data.getDocument() : 0),
            count(data.sci.Selections()), mode (data.sci.SelectionMode()) {
            if (mode == Scintilla::SelectionMode::Thin || mode == Scintilla::SelectionMode::Rectangle) {
                Scintilla::Position anchor   = data.sci.SelectionNAnchor(0);
                Scintilla::Position anchorVs = data.sci.SelectionNAnchorVirtualSpace(0);
                Scintilla::Position caret    = data.sci.SelectionNCaret(count - 1);
                Scintilla::Position caretVs  = data.sci.SelectionNCaretVirtualSpace(count - 1);
                Scintilla::Line lna = data.sci.LineFromPosition(anchor);
                Scintilla::Line lnc = data.sci.LineFromPosition(caret);
                int pxa = data.sci.PointXFromPosition(anchor) + blankWidth(anchorVs);
                int pxc = data.sci.PointXFromPosition(caret ) + blankWidth(caretVs );
                top    = std::min(lna, lnc);
                bottom = std::max(lna, lnc);
                left   = std::min(pxa, pxc);
                right  = std::max(pxa, pxc);
            }
            else {
                top    = std::numeric_limits<Scintilla::Line>::max();
                bottom = 0;
                left   = std::numeric_limits<int>::max();
                right  = std::numeric_limits<int>::min();
                for (int i = 0; i < count; ++i) {
                    Scintilla::Position p = data.sci.SelectionNStart(i);
                    Scintilla::Position q = data.sci.SelectionNEnd(i);
                    Scintilla::Line pLine = data.sci.LineFromPosition(p);
                    Scintilla::Line qLine = data.sci.LineFromPosition(q);
                    top    = std::min(top, pLine);
                    bottom = std::max(bottom, qLine);
                    if (ddp) data.setTabstops(*ddp, pLine, qLine);
                    if (pLine == qLine) {
                        left  = std::min(left , data.sci.PointXFromPosition(p));
                        right = std::max(right, data.sci.PointXFromPosition(q));
                    }
                    else {
                        left = data.sci.PointXFromPosition(0);
                        for (Scintilla::Line j = pLine; j < qLine; ++j) right = std::max(right, data.sci.PointXFromPosition(data.sci.LineEndPosition(j)));
                        right = std::max(right, data.sci.PointXFromPosition(q));
                    }
                }
            }
        }

        void select(Scintilla::Line anchorLine, int anchorPx, Scintilla::Line caretLine, int caretPx) {
            if (ddp) data.setTabstops(*ddp, std::min(anchorLine, caretLine), std::max(anchorLine, caretLine));
            Scintilla::Position anchor = data.positionFromLineAndPointX(anchorLine, anchorPx);
            Scintilla::Position caret  = data.positionFromLineAndPointX(caretLine , caretPx );
            data.sci.SetSelectionMode(Scintilla::SelectionMode::Rectangle);
            data.sci.SetRectangularSelectionAnchor(anchor);
            data.sci.SetRectangularSelectionAnchorVirtualSpace(blankCount(anchorPx - data.sci.PointXFromPosition(anchor)));
            data.sci.SetRectangularSelectionCaret(caret);
            data.sci.SetRectangularSelectionCaretVirtualSpace(blankCount(caretPx - data.sci.PointXFromPosition(caret)));
        }

    };

}


void ColumnsPlusPlusData::selectDown() {
    SelectionInformation si(*this);
    Scintilla::Line lastLine = sci.LineCount() - 1;
    if (sci.PositionFromLine(lastLine) == sci.LineEndPosition(lastLine) && lastLine > 0) --lastLine;
    si.select(lastLine, si.left, si.top, si.right);
}

void ColumnsPlusPlusData::selectEnclose() {
    SelectionInformation si(*this);
    si.select(si.top, si.left, si.bottom, si.right);
}

void ColumnsPlusPlusData::selectExtend() {
    SelectionInformation si(*this);
    if (si.top == si.bottom) {
        Scintilla::Line lastLine = sci.LineCount() - 1;
        if (sci.PositionFromLine(lastLine) == sci.LineEndPosition(lastLine) && lastLine > 0) --lastLine;
        si.select(lastLine, si.left, 0, si.right);
    }
    else if (sci.SelectionEmpty()) {
        if (si.ddp) setTabstops(*si.ddp, si.top, si.bottom);
        int right = si.left;
        for (Scintilla::Line i = si.top; i <= si.bottom; ++i) right = std::max(right, sci.PointXFromPosition(sci.LineEndPosition(i)));
        si.select(si.bottom, right, si.top, sci.PointXFromPosition(0));
    }
    else si.select(si.top, si.left, si.bottom, si.right);
}

void ColumnsPlusPlusData::selectLeft() {
    SelectionInformation si(*this);
    si.select(si.top, sci.PointXFromPosition(0), si.bottom, si.right);
}

void ColumnsPlusPlusData::selectRight() {
    SelectionInformation si(*this);
    if (si.ddp) setTabstops(*si.ddp, si.top, si.bottom);
    int right = si.left;
    for (Scintilla::Line i = si.top; i <= si.bottom; ++i) right = std::max(right, sci.PointXFromPosition(sci.LineEndPosition(i)));
    si.select(si.top, right, si.bottom, si.left);
}

void ColumnsPlusPlusData::selectUp() {
    SelectionInformation si(*this);
    si.select(0, si.left, si.bottom, si.right);
}


void ColumnsPlusPlusData::buildSelectionMenu(int selectionItemIndex, int selectionMenuIndex) {

    MENUITEMINFO cmi;

    HMENU cppMenu = 0;
    HMENU nppPluginMenu = reinterpret_cast<HMENU>(SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, 0, 0));
    cmi.cbSize = sizeof(MENUITEMINFO);
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
                cppMenu = cmi.hSubMenu;
                break;
            }
            cmi.dwTypeData = 0;
        }
    }
    if (!cppMenu) return;

    int cppMenuItemCount = GetMenuItemCount(cppMenu);
    if (selectionItemIndex >= cppMenuItemCount) return;

    HMENU selectionMenu = CreateMenu();

    cmi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    for (int i = 0; i < cppMenuItemCount - selectionItemIndex; ++i) {
        cmi.dwTypeData = 0;
        GetMenuItemInfo(cppMenu, selectionItemIndex, TRUE, &cmi);
        std::wstring text(cmi.cch, 0);
        cmi.dwTypeData = text.data();
        ++cmi.cch;
        GetMenuItemInfo(cppMenu, selectionItemIndex, TRUE, &cmi);
        InsertMenuItem(selectionMenu, 255, TRUE, &cmi);
        RemoveMenu(cppMenu, selectionItemIndex, MF_BYPOSITION);
    }

    cmi.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_SUBMENU;
    cmi.fType = MFT_STRING;
    cmi.dwTypeData = const_cast<wchar_t*>(TEXT("Selection"));
    cmi.hSubMenu = selectionMenu;
    InsertMenuItem(cppMenu, selectionMenuIndex, TRUE, &cmi);
    DrawMenuBar(nppData._nppHandle);

}