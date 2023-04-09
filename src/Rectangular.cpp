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


RectangularSelection& RectangularSelection::refit() {

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

    for (Corner* corner : {&_anchor, &_caret}) {
        Scintilla::Position cpEnd = data.sci.LineEndPosition(corner->ln);
        Scintilla::Position pxEnd = data.sci.PointXFromPosition(cpEnd);
        corner->cp = corner->px <= pxEnd ? data.positionFromLineAndPointX(corner->ln, corner->px) : cpEnd;
        corner->vs = corner->px <= pxEnd ? 0 : (2 * (corner->px - pxEnd) + blankWidth) / (2 * blankWidth);
        corner->st = data.sci.PositionFromLine(corner->ln);
        corner->px -= data.sci.PointXFromPosition(corner->st);
    }
    data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
    data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
    data.sci.SetRectangularSelectionCaret             (_caret.cp );
    data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
    return *this;

}


RectangularBounds RectangularSelection::getBounds() const {
    RectangularBounds rb;
    rb.pxAnchor = _anchor.px;
    rb.pxCaret  = _caret.px;
    if (_anchor.cp > _caret.cp || (_anchor.cp == _caret.cp && _anchor.px > _caret.px)) {
        rb.lineAnchor = _anchor.ln - data.sci.LineCount();
        rb.lineCaret = _caret.ln;
    }
    else  {
        rb.lineAnchor = _anchor.ln;
        rb.lineCaret = _caret.ln - data.sci.LineCount();
    }
    return rb;
}


bool RectangularSelection::loadBounds(const RectangularBounds& rb) {
    Scintilla::Line lineCount = data.sci.LineCount();
    Corner anchor, caret;
    for (auto [corner, rbLine, rbPx] : { std::tuple(&anchor, rb.lineAnchor, rb.pxAnchor),
                                         std::tuple(&caret , rb.lineCaret , rb.pxCaret ) }) {
        if (rbLine < 0) rbLine += lineCount;
        if (rbLine < 0 || rbLine >= lineCount) return false;
        corner->ln = rbLine;
        corner->px = rbPx;
        corner->st = data.sci.PositionFromLine(corner->ln);
        corner->en = data.sci.LineEndPosition(corner->ln);
        corner->sx = data.sci.PointXFromPosition(corner->st);
        int px     = corner->px + corner->sx;
        int pxEnd  = data.sci.PointXFromPosition(corner->en);
        corner->cp = px <= pxEnd ? data.positionFromLineAndPointX(corner->ln, px) : corner->en;
        corner->vs = px <= pxEnd ? 0 : (2 * (px - pxEnd) + blankWidth) / (2 * blankWidth);
    }
    bool reverse = anchor.ln > caret.ln;
    Scintilla::Position size = reverse ? anchor.ln - caret.ln + 1 : caret.ln - anchor.ln + 1;
    if (size > std::numeric_limits<int>::max()) return false;
    _anchor  = anchor;
    _caret   = caret;
    _mode    = Scintilla::SelectionMode::Rectangle;
    _size    = static_cast<int>(size);
    _reverse = reverse;
    data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
    data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
    data.sci.SetRectangularSelectionCaret             (_caret.cp );
    data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
    return true;
}


RectangularSelection::RectangularSelection(ColumnsPlusPlusData& data)
    : data(data), blankWidth(data.sci.TextWidth(0, " ")), tabWidth(data.sci.TabWidth()) {
    _mode       = data.sci.SelectionMode();
    _size       = data.sci.Selections();
    _anchor.cp  = data.sci.SelectionNAnchor(0);
    _caret .cp  = data.sci.SelectionNCaret(_size - 1);
    _anchor.vs  = data.sci.SelectionNAnchorVirtualSpace(0);
    _caret .vs  = data.sci.SelectionNCaretVirtualSpace(_size - 1);
    _anchor.ln  = data.sci.LineFromPosition  (_anchor.cp);
    _caret .ln  = data.sci.LineFromPosition  (_caret .cp);
    _anchor.st  = data.sci.PositionFromLine  (_anchor.ln);
    _caret .st  = data.sci.PositionFromLine  (_caret .ln);
    _anchor.en  = data.sci.LineEndPosition   (_anchor.ln);
    _caret .en  = data.sci.LineEndPosition   (_caret .ln);
    _anchor.sx  = data.sci.PointXFromPosition(_anchor.st);
    _caret .sx  = data.sci.PointXFromPosition(_caret .st);
    _anchor.px  = data.sci.PointXFromPosition(_anchor.cp) - _anchor.sx + blankWidth * static_cast<int>(_anchor.vs);
    _caret .px  = data.sci.PointXFromPosition(_caret .cp) - _caret .sx + blankWidth * static_cast<int>(_caret .vs);
    _reverse    = _anchor.ln > _caret.ln;
    if (_mode != Scintilla::SelectionMode::Rectangle && _mode != Scintilla::SelectionMode::Thin) _size = 0;
}


RectangularSelection& RectangularSelection::extend() {

    if (_mode == Scintilla::SelectionMode::Rectangle || _mode == Scintilla::SelectionMode::Thin) {
        if (_size == 0) return *this;
        if (_mode == Scintilla::SelectionMode::Rectangle) for (int i = 0; i < _size; ++i) {
            Scintilla::Position a = data.sci.SelectionNAnchor(i);
            Scintilla::Position c = data.sci.SelectionNCaret(i);
            if (a != c) return *this;
            a = data.sci.SelectionNAnchorVirtualSpace(i);
            c = data.sci.SelectionNCaretVirtualSpace(i);
            if (a != c) return *this;
        }
        if (!data.extendZeroWidth) { _size = 0; return *this; }
        int maxExtent = 0;
        Scintilla::Line firstLine = std::min(_anchor.ln, _caret.ln);
        for (Scintilla::Line i = firstLine; i < firstLine + _size; ++i) {
            int extent = data.sci.PointXFromPosition(data.sci.LineEndPosition(i));
            if (extent > maxExtent) maxExtent = extent;
        }
        _anchor.cp    = _anchor.en;
        int pxEOL     = data.sci.PointXFromPosition(_anchor.en);
        int pxVirtual = maxExtent - pxEOL;
        _anchor.vs    = (2 * pxVirtual + blankWidth) / (2 * blankWidth);
        _anchor.px    = pxEOL - _anchor.sx + blankWidth * static_cast<int>(_anchor.vs);
        data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
        data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
        data.sci.SetRectangularSelectionCaret             (_caret.cp );
        data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
        _mode = Scintilla::SelectionMode::Rectangle;
        return *this;
    }

    if (data.sci.Selections() != 1) { _size = 0; return *this; }

    if (_anchor.ln == _caret.ln) {
        if (!data.extendSingleLine) { _size = 0; return *this; }
        if (_anchor.cp == _caret.cp && _anchor.vs == _caret.vs) { _size = 0; return *this; }
        Corner bottom;
        bottom.en = data.sci.Length();
        bottom.ln = data.sci.LineFromPosition(bottom.en);
        bottom.st = data.sci.PositionFromLine(bottom.ln);
        if (bottom.st == bottom.en && bottom.ln > 0) {
            bottom.ln--;
            bottom.st = data.sci.PositionFromLine(bottom.ln);
            bottom.en = data.sci.LineEndPosition(bottom.ln);
        }
        if (bottom.ln == _caret.ln) { _size = 0; return *this; }
        if (data.settings.elasticEnabled) data.setTabstops(*data.getDocument(), _caret.ln);
        int endX = data.sci.PointXFromPosition(bottom.en);
        int anchorX = _anchor.px + _anchor.sx;
        if (anchorX > endX) {
            bottom.cp = bottom.en;
            bottom.vs = (2 * (anchorX - endX) + blankWidth) / (2 * blankWidth);
        }
        else {
            bottom.cp = anchorX <= endX ? data.positionFromLineAndPointX(bottom.ln, anchorX) : bottom.en;
            bottom.vs = anchorX <= endX ? 0 : (2 * (anchorX - endX) + blankWidth) / (2 * blankWidth);
        }
        Scintilla::Position size = bottom.ln - _caret.ln + 1;
        if (size > std::numeric_limits<int>::max()) { _size = 0; return *this; }
        _size    = static_cast<int>(size);
        _mode    = Scintilla::SelectionMode::Rectangle;
        _reverse = true;
        _anchor  = bottom;
        _anchor.sx = data.sci.PointXFromPosition(_anchor.st);
        _anchor.px = data.sci.PointXFromPosition(_anchor.cp) - _anchor.sx + blankWidth * static_cast<int>(_anchor.vs);
        data.sci.SetRectangularSelectionAnchor            (_anchor.cp);
        data.sci.SetRectangularSelectionAnchorVirtualSpace(_anchor.vs);
        data.sci.SetRectangularSelectionCaret             (_caret.cp );
        data.sci.SetRectangularSelectionCaretVirtualSpace (_caret.vs );
        return *this;
    }

    if (!data.extendFullLines) { _size = 0; return *this; }

    Corner& start = _anchor.cp < _caret.cp ? _anchor : _caret;
    Corner& end   = _anchor.cp < _caret.cp ? _caret : _anchor;

    if (start.cp != start.st || (end.cp != end.st && end.cp != end.en)) { _size = 0; return *this; }
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
