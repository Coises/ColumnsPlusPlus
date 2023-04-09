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


void ColumnsPlusPlusData::alignLeft() {
    auto rs = getRectangularSelection();
    if (!rs.size()) return;
    sci.BeginUndoAction();
    for (auto row : rs) {
        std::string r;
        for (const auto& cell : row) {
            r += cell.trim();
            if (!settings.elasticEnabled || (cell.isLastInRow() && !cell.isEndOfLine())) {
                r += std::string(cell.leading() + cell.trailing(), ' ');
            }
            r += cell.terminator();
        }
        if (r != row.text()) row.replace(r);
    }
    sci.EndUndoAction();
    rs.refit();
}


void ColumnsPlusPlusData::alignRight() {
    auto rs = getRectangularSelection();
    if (!rs.size()) return;
    sci.BeginUndoAction();
    for (auto row : rs) {
        std::string r;
        for (const auto& cell : row) {
            if (cell.trimLength() == 0)  r += cell.text() + cell.terminator();
            else if (cell.isLastInRow()) r += std::string(row.vsMax() - row.vsMin() + cell.leading() + cell.trailing(), ' ') + cell.trim();
            else {
                int padWidth = sci.PointXFromPosition(cell.end() + 1) - sci.PointXFromPosition(cell.end());
                padWidth = (2 * padWidth + rs.blankWidth) / (2 * rs.blankWidth) - (settings.elasticEnabled ? settings.minimumSpaceBetweenColumns : 1);
                if (padWidth < 0) padWidth = 0;
                r += std::string(padWidth + cell.leading() + cell.trailing(), ' ') + cell.trim() + '\t';
            }
        }
        if (r != row.text()) row.replace(r);
    }
    sci.EndUndoAction();
    rs.refit();
}


void ColumnsPlusPlusData::alignNumeric() {

    struct Item {
        std::string text;
        intptr_t extentLeft;
        intptr_t extentRight;
        bool     align;
    };

    struct Line : std::vector<Item> {
        std::string line;
        bool eol;
    };

    struct Column {
        intptr_t extentLeft  = 0;
        intptr_t extentRight = 0;
        intptr_t width     = 0;
        intptr_t pad       = 0;
    };

    auto rs = getRectangularSelection();
    if (rs.size() < 2) return;

    std::vector<Line> item(rs.size());
    std::vector<Column> column;

    for (auto row : rs) {
        Line& cLine = item[row.index];
        cLine.line  = row.text();
        cLine.eol   = row.isEndOfLine();
        for (const auto& cell : row) {
            cLine.emplace_back();
            Item& cItem = cLine.back();
            if (cLine.size() > column.size()) column.emplace_back();
            Column& cColumn = column[cLine.size() - 1];
            intptr_t width = sci.PointXFromPosition(cell.end()) - sci.PointXFromPosition(cell.start());
            if (cell.isLastInRow()) width += rs.blankWidth * (row.vsMax() - row.vsMin());
            if (width > cColumn.width) cColumn.width = width;
            size_t dp = cell.trimLength() > 0 ? findDecimal(cell.trim()) : std::string::npos;
            if (dp == std::string::npos) {
                cItem.text = cell.text();
                cItem.align = false;
                continue;
            }
            cItem.extentLeft  = sci.PointXFromPosition(cell.left() + dp) - sci.PointXFromPosition(cell.left()     );
            cItem.extentRight = sci.PointXFromPosition(cell.right()    ) - sci.PointXFromPosition(cell.left() + dp);
            cItem.text = cell.trim();
            cItem.align = true;
            if (cItem.extentLeft  > cColumn.extentLeft ) cColumn.extentLeft  = cItem.extentLeft;
            if (cItem.extentRight > cColumn.extentRight) cColumn.extentRight = cItem.extentRight;
        }
    }

    // At this point, the selection is represented as a vector of rows, each containing a vector
    //     of items which are separated by tabs in the source.
    // item.align is true if the item can be numeric aligned, false if it is to be left as is.
    // Each item.text to be aligned has been trimmed of leading and trailing blanks.
    // column.width is the maximum number of pixels in any item in the column before trimming,
    //     including virtual space.
    // item.extentLeft is the number of pixels left of the (possibly implicit) decimal separator
    // item.extentRight is the number of pixels right of the (possibly implicit) ones digit,
    //     including the decimal separator if there is one.
    // column.extentLeft and .extentRight are the maximums of the extentLeft and extentRight values
    //     of all items in the column.

    for (Column& col : column) {
        intptr_t pad = col.width - col.extentLeft - col.extentRight;
        col.pad = pad > 0 ? (2 * pad + rs.blankWidth) / (2 * rs.blankWidth) : (2 * pad - rs.blankWidth) / (2 * rs.blankWidth);
    }

    // A positive pad value means once leading and trailing blanks are removed from all items in a column
    //     and the text is numerically aligned, the result is not as wide as the original column,
    //     so extra padding will be added to the left of aligned items to avoid changing the column width.
    // A negative pad value means the original column isn't wide enough to hold the column
    //     after numeric alignment, so the width of all items in the column will be increased.

    sci.BeginUndoAction();

    for (auto row : rs) {
        const auto& cells = item[row.index];
        bool notEOL = !cells.eol;
        intptr_t lastCell = cells.size() - 1;
        std::string r;
        for (int j = 0;;++j) {
            const Item& cell = cells[j];
            const Column& col = column[j];
            if (cell.align) {
                if (col.pad > 0) r += std::string(col.pad, ' ');
                r += std::string((2 * (col.extentLeft - cell.extentLeft) + rs.blankWidth) / (2 * rs.blankWidth), ' ');
                r += cell.text;
                if (j == lastCell ? notEOL : !settings.elasticEnabled)
                    r += std::string((2 * (col.extentRight - cell.extentRight) + rs.blankWidth) / (2 * rs.blankWidth), ' ');
            }
            else {
                r += cell.text;
                if (col.pad < 0 && (j == lastCell ? notEOL : !settings.elasticEnabled)) r += std::string(-col.pad, ' ');
            }
            if (j == lastCell) break;
            r += '\t';
        }
        if (r != cells.line) row.replace(r);
    }

    sci.EndUndoAction();
    rs.refit();

 }
