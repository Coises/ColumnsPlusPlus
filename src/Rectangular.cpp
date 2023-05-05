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

struct ChooseRectangularSelectionInfo {
    ColumnsPlusPlusData& data;
    bool hasWidth, hasHeight;   //  These describe the current selection and affect the button drawings.
    int edge;                   //  Edge can be 2, 4, 6 or 8 corresponding to numpad layout; otherwise, currect selection is not on a document edge.
    ChooseRectangularSelectionInfo(ColumnsPlusPlusData& data, bool hasWidth, bool hasHeight, int edge = 0)
        : data(data), hasWidth(hasWidth), hasHeight(hasHeight), edge(edge) {}
};


INT_PTR CALLBACK chooseRectangularSelectionDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    ChooseRectangularSelectionInfo* crsip = 0;
    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        crsip = reinterpret_cast<ChooseRectangularSelectionInfo*>(lParam);
    }
    else crsip = reinterpret_cast<ChooseRectangularSelectionInfo*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (!crsip) return TRUE;
    ChooseRectangularSelectionInfo& crsi = *crsip;
    ColumnsPlusPlusData& data = crsi.data;

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
        SetDlgItemText(hwndDlg, IDC_MAKE_A_RECT, crsi.hasWidth || crsi.hasHeight ? L"Make a rectangular selection\nbased on the current selection?"
                                                                                 : L"Make a rectangular selection\nbased on the cursor position?");
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SELECT_RECTANGULAR_UPLEFT   : EndDialog(hwndDlg, 7); return TRUE;
        case IDC_SELECT_RECTANGULAR_UP       : EndDialog(hwndDlg, 8); return TRUE;
        case IDC_SELECT_RECTANGULAR_UPRIGHT  : EndDialog(hwndDlg, 9); return TRUE;
        case IDC_SELECT_RECTANGULAR_LEFT     : EndDialog(hwndDlg, 4); return TRUE;
        case IDC_SELECT_RECTANGULAR_ALL      : EndDialog(hwndDlg, 5); return TRUE;
        case IDC_SELECT_RECTANGULAR_RIGHT    : EndDialog(hwndDlg, 6); return TRUE;
        case IDC_SELECT_RECTANGULAR_DOWNLEFT : EndDialog(hwndDlg, 1); return TRUE;
        case IDC_SELECT_RECTANGULAR_DOWN     : EndDialog(hwndDlg, 2); return TRUE;
        case IDC_SELECT_RECTANGULAR_DOWNRIGHT: EndDialog(hwndDlg, 3); return TRUE;
        case IDCANCEL                        : EndDialog(hwndDlg, 0); return TRUE;
        default:;
        }
        break;

    case WM_DRAWITEM:
    {
        const DRAWITEMSTRUCT& dis = *reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        RECT rect = dis.rcItem;
        DrawFrameControl(dis.hDC, &rect, DFC_BUTTON, DFCS_BUTTONPUSH|DFCS_FLAT|DFCS_ADJUSTRECT);
        int hBlock = (rect.bottom - rect.top) / 16;
        int wBlock = (rect.right - rect.left) / 16;
        rect.top += (rect.bottom - rect.top - 15 * hBlock) / 2;
        rect.left += (rect.right - rect.left - 15 * wBlock) / 2;
        rect.bottom = rect.top + 15 * hBlock;
        rect.right = rect.left + 15 * wBlock;
        RECT oldSelection;
        oldSelection.top    = rect.top  + (crsi.hasHeight ?  4 * hBlock : 6 * hBlock);
        oldSelection.left   = rect.left + (crsi.hasWidth  ?  5 * wBlock : 7 * wBlock);
        oldSelection.bottom = rect.top  + (crsi.hasHeight ? 11 * hBlock : 9 * hBlock);
        oldSelection.right  = rect.left + (crsi.hasWidth  ? 10 * wBlock : 8 * wBlock);
        switch (crsi.edge) {
        case 2:
            oldSelection.top += rect.bottom - oldSelection.bottom;
            oldSelection.bottom = rect.bottom;
            break;
        case 4:
            oldSelection.right -= oldSelection.left - rect.left;
            oldSelection.left = rect.left;
            break;
        case 6:
            oldSelection.left += rect.right - oldSelection.right;
            oldSelection.right = rect.right;
            break;
        case 8:
            oldSelection.bottom -= oldSelection.top - rect.top;
            oldSelection.top = rect.top;
            break;
        default:;
        }
        RECT newSelection;
        switch (wParam) {
        case IDC_SELECT_RECTANGULAR_UPLEFT:
            newSelection.top    = rect.top;
            newSelection.left   = rect.left;
            newSelection.bottom = oldSelection.bottom;
            newSelection.right  = oldSelection.right;
            break;
        case IDC_SELECT_RECTANGULAR_UP:
            newSelection.top    = rect.top;
            newSelection.left   = crsi.hasWidth ? oldSelection.left : rect.left;
            newSelection.bottom = oldSelection.bottom;
            newSelection.right  = crsi.hasWidth ? oldSelection.right : rect.right;
            break;
        case IDC_SELECT_RECTANGULAR_UPRIGHT:
            newSelection.top    = rect.top;
            newSelection.left   = oldSelection.left;
            newSelection.bottom = oldSelection.bottom;
            newSelection.right  = rect.right;
            break;
        case IDC_SELECT_RECTANGULAR_LEFT:
            newSelection.top    = crsi.hasHeight ? oldSelection.top : rect.top;
            newSelection.left   = rect.left;
            newSelection.bottom = crsi.hasHeight ? oldSelection.bottom : rect.bottom;
            newSelection.right  = oldSelection.right;
            break;
        case IDC_SELECT_RECTANGULAR_ALL:
            newSelection.top    = crsi.hasHeight ? oldSelection.top    : rect.top;
            newSelection.left   = crsi.hasWidth  ? oldSelection.left   : rect.left;
            newSelection.bottom = crsi.hasHeight ? oldSelection.bottom : rect.bottom;
            newSelection.right  = crsi.hasWidth  ? oldSelection.right  : rect.right;
            break;
        case IDC_SELECT_RECTANGULAR_RIGHT:
            newSelection.top    = crsi.hasHeight ? oldSelection.top : rect.top;
            newSelection.left   = oldSelection.left;
            newSelection.bottom = crsi.hasHeight ? oldSelection.bottom : rect.bottom;
            newSelection.right  = rect.right;
            break;
        case IDC_SELECT_RECTANGULAR_DOWNLEFT:
            newSelection.top    = oldSelection.top;
            newSelection.left   = rect.left;
            newSelection.bottom = rect.bottom;
            newSelection.right  = oldSelection.right;
            break;
        case IDC_SELECT_RECTANGULAR_DOWN:
            newSelection.top    = oldSelection.top;
            newSelection.left   = crsi.hasWidth ? oldSelection.left : rect.left;
            newSelection.bottom = rect.bottom;
            newSelection.right  = crsi.hasWidth ? oldSelection.right : rect.right;
            break;
        case IDC_SELECT_RECTANGULAR_DOWNRIGHT:
            newSelection.top    = oldSelection.top;
            newSelection.left   = oldSelection.left;
            newSelection.bottom = rect.bottom;
            newSelection.right  = rect.right;
            break;
        }
        Scintilla::Colour pageColor = data.sci.StyleGetBack(0);
        Scintilla::Colour oldColor = data.sci.StyleGetFore(0);
        unsigned int newAlpha = data.sci.ElementColour(Scintilla::Element::SelectionBack);
        unsigned int na = newAlpha >> 24;
        unsigned int nr = newAlpha & 255;
        unsigned int ng = (newAlpha >> 8) & 255;
        unsigned int nb = (newAlpha >> 16 ) & 255;
        unsigned int pr = pageColor & 255;
        unsigned int pg = (pageColor >> 8) & 255;
        unsigned int pb = (pageColor >> 16 ) & 255;
        nr = (na * nr + (255 - na) * pr) / 255;
        ng = (na * ng + (255 - na) * pg) / 255;
        nb = (na * nb + (255 - na) * pb) / 255;
        unsigned int newColor = nr + (ng << 8) + (nb << 16);
        HBRUSH brush = CreateSolidBrush(pageColor);
        FillRect(dis.hDC, &rect, brush);
        DeleteObject(brush);
        brush = CreateSolidBrush(newColor);
        FillRect(dis.hDC, &newSelection, brush);
        DeleteObject(brush);
        brush = CreateSolidBrush(oldColor);
        FillRect(dis.hDC, &oldSelection, brush);
        DeleteObject(brush);
    }
        break;

    default:;
    }
    return FALSE;
}


RectangularSelection& RectangularSelection::refit(bool addLine) {

    if (!_size) return *this;
    if (data.settings.elasticEnabled) {
        DocumentData* ddp = data.getDocument();
        data.analyzeTabstops(*ddp);
        Scintilla::Position firstVisible = data.sci.FirstVisibleLine();
        Scintilla::Position lastVisible  = firstVisible + data.sci.LinesOnScreen();
        data.setTabstops(*ddp, std::min(std::min(_anchor.ln, _caret.ln), firstVisible),
                               std::max(std::max(_anchor.ln, _caret.ln), lastVisible));
    }
    data.sci.PointXFromPosition(0);  // Scintilla bug?  The next PointXFromPosition call can give
                                     // wrong results unless the function is called on a *different*
                                     // position first.  Not sure why... sounds like a dirty cache
                                     // that isn't marked as such.  Likely related to setTabstops().

    int pxLeft  = std::numeric_limits<int>::max();
    int pxRight = 0;
    for (int i = 0; i < _size; ++i) {
        Scintilla::Position cpMin = data.sci.SelectionNStart(i);
        Scintilla::Position vsMin = data.sci.SelectionNStartVirtualSpace(i);
        Scintilla::Position cpMax = data.sci.SelectionNEnd(i);
        int px = data.sci.PointXFromPosition(cpMin) + static_cast<int>(vsMin) * blankWidth;
        if (px < pxLeft) pxLeft = px;
        px = data.sci.PointXFromPosition(cpMax);
        if (px > pxRight) pxRight = px;
    }

    if (leftToRight()) { _anchor.px = pxLeft ; _caret.px = pxRight; }
                  else { _anchor.px = pxRight; _caret.px = pxLeft ; }

    if (addLine) {
        (topToBottom() ? _caret.ln : _anchor.ln)++;
        _size++;
    }

    for (Corner* corner : {&_anchor, &_caret}) {
        corner->en = data.sci.LineEndPosition(corner->ln);
        Scintilla::Position pxEnd = data.sci.PointXFromPosition(corner->en);
        corner->cp = corner->px < pxEnd ? data.positionFromLineAndPointX(corner->ln, corner->px) : corner->en;
        corner->vs = corner->px < pxEnd ? 0 : (2 * (corner->px - pxEnd) + blankWidth) / (2 * blankWidth);
        corner->st = data.sci.PositionFromLine(corner->ln);
        corner->sx = data.sci.PointXFromPosition(corner->st);
        corner->px -= corner->sx;
    }
    data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
    data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
    data.sci.SetRectangularSelectionCaret             (_caret.cp );
    data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
    return *this;

}


RectangularSelection::RectangularSelection(ColumnsPlusPlusData& data)
    : data(data), blankWidth(data.sci.TextWidth(STYLE_DEFAULT, " ")), tabWidth(data.sci.TabWidth()) {
    _mode   = data.sci.SelectionMode();
    _size   = data.sci.Selections();
    _anchor = corner(data.sci.SelectionNAnchor(0), data.sci.SelectionNAnchorVirtualSpace(0));
    _caret  = corner(data.sci.SelectionNCaret(_size - 1), data.sci.SelectionNCaretVirtualSpace(_size - 1));
    _reverse    = _anchor.ln > _caret.ln;
    if (_mode != Scintilla::SelectionMode::Rectangle && _mode != Scintilla::SelectionMode::Thin) _size = 0;
}


RectangularSelection& RectangularSelection::extend() {

    if (_mode == Scintilla::SelectionMode::Rectangle || _mode == Scintilla::SelectionMode::Thin) {

        if (_size == 0) {
            MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.", L"Columns++ rectangular selection", MB_OK);
            return *this;
        }

        bool canExtendRight = false;
        int maxExtent = 0;
        for (int i = 0; i < _size; ++i) {
            Scintilla::Position ap = data.sci.SelectionNAnchor(i);
            if (_mode == Scintilla::SelectionMode::Rectangle) {
                Scintilla::Position cp = data.sci.SelectionNCaret(i);
                if (ap != cp) return *this;
                Scintilla::Position av = data.sci.SelectionNAnchorVirtualSpace(i);
                Scintilla::Position cv = data.sci.SelectionNCaretVirtualSpace(i);
                if (av != cv) return *this;
            }
            Scintilla::Position lep = data.sci.LineEndPosition(data.sci.LineFromPosition(ap));
            if (ap < lep) canExtendRight = true;
            int extent = data.sci.PointXFromPosition(lep);
            if (extent > maxExtent) maxExtent = extent;
        }

        intptr_t choice;
        if (data.extendZeroWidth && canExtendRight) choice = 6;
        else if (_caret.cp == _caret.st && _caret.vs == 0) {
            if (!canExtendRight) {
                MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.", L"Columns++ rectangular selection", MB_OK);
                _size = 0;
                return *this;
            }
            if (MessageBox(data.nppData._nppHandle, L"This command requires a non-zero-width rectangular selection.\n\n"
                L"Extend selection to enclose the selected lines?",
                L"Columns++ rectangular selection", MB_OKCANCEL) != IDOK) {
                _size = 0;
                return *this;
            }
            choice = 6;
        }
        else if (!canExtendRight) {
            if (MessageBox(data.nppData._nppHandle, L"This command requires a non-zero-width rectangular selection.\n\n"
                L"Extend selection to enclose the selected lines?",
                L"Columns++ rectangular selection", MB_OKCANCEL) != IDOK) {
                _size = 0;
                return *this;
            }
            choice = 4;
        }
        else {
            ChooseRectangularSelectionInfo crsi(data, false, true);
            choice = DialogBoxParam(data.dllInstance, MAKEINTRESOURCE(IDD_CHOOSE_RECTANGULAR_SELECTION_HORIZONTAL),
                data.nppData._nppHandle, chooseRectangularSelectionDialogProc, reinterpret_cast<LPARAM>(&crsi));
            if (!choice) { _size = 0; return *this; }
        }

        if (choice == 4) {
            _anchor.cp = _anchor.st;
            _anchor.vs = 0;
            _anchor.px = 0;
        }
        else {
            _anchor.cp    = _anchor.en;
            int pxEOL     = data.sci.PointXFromPosition(_anchor.en);
            int pxVirtual = maxExtent - pxEOL;
            _anchor.vs    = (2 * pxVirtual + blankWidth) / (2 * blankWidth);
            _anchor.px    = pxEOL - _anchor.sx + blankWidth * static_cast<int>(_anchor.vs);
            if (choice == 5) {
                _caret.cp = _caret.st;
                _caret.vs = 0;
                _caret.px = 0;
            }
        }
        data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
        data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
        data.sci.SetRectangularSelectionCaret             (_caret.cp );
        data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
        _mode = Scintilla::SelectionMode::Rectangle;
        return *this;
    }

    if (data.sci.Selections() != 1) { 
        MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.", L"Columns++ rectangular selection", MB_OK);
        _size = 0;
        return *this;
    }

    if (data.settings.elasticEnabled) data.setTabstops(*data.getDocument(), 0, -1);

    Corner topLeft = corner(0, 0);

    Corner bottomLeft = corner(data.sci.PositionFromLine(data.sci.LineCount() - 1), 0);
    if (bottomLeft.st == bottomLeft.en && bottomLeft.ln > 0)
        bottomLeft = corner(data.sci.PositionFromLine(bottomLeft.ln - 1), 0);
    int maxExtentAll = 0;

    for (Scintilla::Line i = 0; i <= bottomLeft.ln; ++i) {
        int extent = data.sci.PointXFromPosition(data.sci.LineEndPosition(i));
        if (extent > maxExtentAll) maxExtentAll = extent;
    }

    Corner topRight = corner(topLeft.en, (2 * (maxExtentAll - data.sci.PointXFromPosition(topLeft.en)) + blankWidth) / (2 * blankWidth));
    Corner bottomRight = corner(bottomLeft.en, (2 * (maxExtentAll - data.sci.PointXFromPosition(bottomLeft.en)) + blankWidth) / (2 * blankWidth));

    Corner& start  = _anchor.cp < _caret.cp ? _anchor : _caret ;
    Corner& end    = _anchor.cp < _caret.cp ? _caret  : _anchor;
    bool hasWidth  = _anchor.cp != _caret.cp || _anchor.vs != _caret.vs;
    bool hasHeight = _anchor.ln != _caret.ln;

    if (!hasWidth && !hasHeight) {

        intptr_t choice;
        if ((_caret.px == 0 || _caret.px >= bottomRight.px) && (_caret.ln == 0 || _caret.ln >= bottomRight.ln)) {
            if (MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.\n\n"
                L"Enclose the entire document in a rectangular selection?",
                L"Columns++ rectangular selection", MB_OKCANCEL) != IDOK) {
                _size = 0;
                return *this;
            }
            choice = 5;
        }
        else if (_caret.px == 0 || _caret.px >= bottomRight.px) {
            ChooseRectangularSelectionInfo crsi(data, false, false, _caret.px == 0 ? 4 : 6);
            choice = DialogBoxParam(data.dllInstance, MAKEINTRESOURCE(IDD_CHOOSE_RECTANGULAR_SELECTION_VERTICAL), data.nppData._nppHandle,
                chooseRectangularSelectionDialogProc, reinterpret_cast<LPARAM>(&crsi));
            if (!choice) { _size = 0; return *this; }
        }
        else if (_caret.ln == 0 || _caret.ln >= bottomRight.ln) {
            ChooseRectangularSelectionInfo crsi(data, false, false, _caret.ln == 0 ? 8 : 2);
            choice = DialogBoxParam(data.dllInstance, MAKEINTRESOURCE(IDD_CHOOSE_RECTANGULAR_SELECTION_HORIZONTAL), data.nppData._nppHandle,
                chooseRectangularSelectionDialogProc, reinterpret_cast<LPARAM>(&crsi));
            if (!choice) { _size = 0; return *this; }
        }
        else {
            ChooseRectangularSelectionInfo crsi(data, false, false);
            choice = DialogBoxParam(data.dllInstance, MAKEINTRESOURCE(IDD_CHOOSE_RECTANGULAR_SELECTION), data.nppData._nppHandle,
                chooseRectangularSelectionDialogProc, reinterpret_cast<LPARAM>(&crsi));
            if (!choice) { _size = 0; return *this; }
        }

        Corner anchor, caret;

        switch (choice) {
        case 1: anchor = bottomLeft ; caret = _caret ; break;
        case 3: anchor = bottomRight; caret = _caret ; break;
        case 5: anchor = bottomRight; caret = topLeft; break;
        case 7: anchor = topLeft    ; caret = _caret ; break;
        case 9: anchor = topRight   ; caret = _caret ; break;
        case 2:
        case 8:
            anchor = choice == 2 ? bottomRight : topRight;
            caret.cp = _caret.st;
            caret.vs = 0;
            caret.ln = _caret.ln;
            caret.st = _caret.st;
            caret.en = _caret.en;
            caret.sx = _caret.sx;
            caret.px = _caret.sx;
            break;
        case 4:
        case 6:
            anchor = choice == 4 ? bottomLeft : bottomRight;
            caret.ln = topLeft.ln;
            caret.st = topLeft.st;
            caret.en = topLeft.en;
            caret.sx = topLeft.sx;
            int px = _caret.px + _caret.sx;
            int ex = data.sci.PointXFromPosition(caret.en);
            if (px > ex) {
                caret.cp = caret.en;
                caret.vs = (2 * (px - ex) + blankWidth) / (2 * blankWidth);
            }
            else {
                caret.cp = data.positionFromLineAndPointX(caret.ln, px);
                caret.vs = 0;
            }
            caret.px = ex - caret.sx + blankWidth * static_cast<int>(caret.vs);
            break;
        }

        Scintilla::Position size = std::max(anchor.ln, caret.ln) - std::min(anchor.ln, caret.ln) + 1;
        if (size > std::numeric_limits<int>::max()) { _size = 0; return *this; }
        _size    = static_cast<int>(size);
        _mode    = Scintilla::SelectionMode::Rectangle;
        _anchor  = anchor;
        _caret   = caret;
        _reverse = _anchor.ln > _caret.ln;;
        data.sci.SetRectangularSelectionAnchor(_anchor.cp);
        data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
        data.sci.SetRectangularSelectionCaret(_caret.cp);
        data.sci.SetRectangularSelectionCaretVirtualSpace(_caret.vs);
        return *this;
    }

    if (hasWidth && !hasHeight) {
        intptr_t choice;
        if (data.extendSingleLine && _caret.ln < bottomLeft.ln) choice = 2;
        else if (_caret.ln == 0) {
            if (bottomLeft.ln == 0) {
                MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.", L"Columns++ rectangular selection", MB_OK);
                _size = 0;
                return *this;
            }
            if (MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.\n\n"
                L"Extend selection to the last line of the document?",
                L"Columns++ rectangular selection", MB_OKCANCEL) != IDOK) {
                _size = 0;
                return *this;
            }
            choice = 2;
        }
        else if (_caret.ln >= bottomLeft.ln) {
            if (MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.\n\n"
                L"Extend selection to the first line of the document?",
                L"Columns++ rectangular selection", MB_OKCANCEL) != IDOK) {
                _size = 0;
                return *this;
            }
            choice = 8;
        }
        else {
            ChooseRectangularSelectionInfo crsi(data, true, false);
            choice = DialogBoxParam(data.dllInstance, MAKEINTRESOURCE(IDD_CHOOSE_RECTANGULAR_SELECTION_VERTICAL),
                data.nppData._nppHandle, chooseRectangularSelectionDialogProc, reinterpret_cast<LPARAM>(&crsi));
            if (!choice) { _size = 0; return *this; }
        }
        Corner top = _caret;
        Corner bottom = _caret;
        if (choice == 8) {
            top.st = 0;
            top.ln = 0;
            top.en = data.sci.LineEndPosition(0);
            int endX = data.sci.PointXFromPosition(top.en);
            int anchorX = _anchor.px + _anchor.sx;
            if (anchorX > endX) {
                top.cp = top.en;
                top.vs = (2 * (anchorX - endX) + blankWidth) / (2 * blankWidth);
            }
            else {
                top.cp = data.positionFromLineAndPointX(top.ln, anchorX);
                top.vs = 0;
            }
            top.sx = data.sci.PointXFromPosition(top.st);
            top.px = anchorX - top.sx + blankWidth * static_cast<int>(top.vs);
        }
        else {
            if (choice == 5) {
                top.st = 0;
                top.ln = 0;
                top.en = data.sci.LineEndPosition(0);
            }
            bottom.en = data.sci.Length();
            bottom.ln = data.sci.LineFromPosition(bottom.en);
            bottom.st = data.sci.PositionFromLine(bottom.ln);
            if (bottom.st == bottom.en && bottom.ln > 0) {
                bottom.ln--;
                bottom.st = data.sci.PositionFromLine(bottom.ln);
                bottom.en = data.sci.LineEndPosition(bottom.ln);
            }
            if (bottom.ln == top.ln) { _size = 0; return *this; }
            if (choice == 5) {
                int endX = data.sci.PointXFromPosition(top.en);
                int caretX = _caret.px + _caret.sx;
                if (caretX > endX) {
                    top.cp = top.en;
                    top.vs = (2 * (caretX - endX) + blankWidth) / (2 * blankWidth);
                }
                else {
                    top.cp = data.positionFromLineAndPointX(top.ln, caretX);
                    top.vs = 0;
                }
                top.sx = data.sci.PointXFromPosition(top.st);
                top.px = caretX - top.sx + blankWidth * static_cast<int>(top.vs);

            }
            int endX = data.sci.PointXFromPosition(bottom.en);
            int anchorX = _anchor.px + _anchor.sx;
            if (anchorX > endX) {
                bottom.cp = bottom.en;
                bottom.vs = (2 * (anchorX - endX) + blankWidth) / (2 * blankWidth);
            }
            else {
                bottom.cp = data.positionFromLineAndPointX(bottom.ln, anchorX);
                bottom.vs = 0;
            }
            bottom.sx = data.sci.PointXFromPosition(bottom.st);
            bottom.px = anchorX - bottom.sx + blankWidth * static_cast<int>(bottom.vs);
        }
        Scintilla::Position size = bottom.ln - top.ln + 1;
        if (size > std::numeric_limits<int>::max()) { _size = 0; return *this; }
        _size    = static_cast<int>(size);
        _mode    = Scintilla::SelectionMode::Rectangle;
        if (choice == 5) _caret = top;
        _anchor  = choice == 8 ? top : bottom;
        _reverse = choice == 8 ? false : true;
        data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
        data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
        data.sci.SetRectangularSelectionCaret             (_caret.cp );
        data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
        return *this;
    }

    if (hasWidth && hasHeight && start.cp == start.st && (end.cp == end.st || end.cp == end.en)) {
        if (!data.extendFullLines) {
            if (MessageBox(data.nppData._nppHandle,
                start.cp == 0 && end.ln >= bottomRight.ln
                ? L"This command requires a rectangular selection.\n\n"
                  L"Convert to a rectangular selection enclosing the entire document?"
                : L"This command requires a rectangular selection.\n\n"
                  L"Convert to a rectangular selection enclosing the selected lines?",
                L"Columns++ rectangular selection", MB_OKCANCEL) != IDOK) {
                _size = 0;
                return *this;
            }
        }
    }
    else {
        MessageBox(data.nppData._nppHandle, L"This command requires a rectangular selection.", L"Columns++ rectangular selection", MB_OK);
        _size = 0;
        return *this;
    }

    if (end.cp == end.st) --end.ln;

    Scintilla::Position size = end.ln - start.ln + 1;
    if (size > std::numeric_limits<int>::max()) { _size = 0; return *this; }

    int maxRight = 0;
    for (Scintilla::Line i = start.ln; i <= end.ln; ++i) {
        int right = data.sci.PointXFromPosition(data.sci.LineEndPosition(i));
        if (right > maxRight) maxRight = right;
    }

    if (end.cp != end.st) {
        int pxEOL = data.sci.PointXFromPosition(end.en);
        int pxVirtual = maxRight - pxEOL;
        end.vs = (2 * pxVirtual + blankWidth) / (2 * blankWidth);
        end.px = pxEOL - end.sx + blankWidth * static_cast<int>(end.vs);
    }
    else if (_anchor.cp < _caret.cp) {
        _anchor.cp = _anchor.en;
        int pxEOL = data.sci.PointXFromPosition(_anchor.en);
        int pxVirtual = maxRight - pxEOL;
        _anchor.vs = (2 * pxVirtual + blankWidth) / (2 * blankWidth);
        _anchor.px = pxEOL - _anchor.sx + blankWidth * static_cast<int>(_anchor.vs);
        _caret .st = data.sci.PositionFromLine  (_caret.ln);
        _caret .sx = data.sci.PointXFromPosition(_caret.st);
        _caret .en = data.sci.LineEndPosition   (_caret.ln);
        _caret .cp = _caret.st;
        _caret .vs = 0;
        _caret .px = 0;
    }
    else {
        _anchor.st = data.sci.PositionFromLine  (_anchor.ln);
        _anchor.sx = data.sci.PointXFromPosition(_anchor.st);
        _anchor.en = data.sci.LineEndPosition   (_anchor.ln);
        _anchor.cp = _anchor.en;
        int pxEOL = data.sci.PointXFromPosition(_anchor.en);
        int pxVirtual = maxRight - pxEOL;
        _anchor.vs = (2 * pxVirtual + blankWidth) / (2 * blankWidth);
        _anchor.px = pxEOL - _anchor.sx + blankWidth * static_cast<int>(_anchor.vs);
    }

    data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
    data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
    data.sci.SetRectangularSelectionCaret             (_caret.cp );
    data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
    _mode = Scintilla::SelectionMode::Rectangle;
    _size = static_cast<int>(size);
    _reverse = _anchor.ln > _caret.ln;
    return *this;

}
