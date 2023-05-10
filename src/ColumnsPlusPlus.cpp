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
#include <regex>
#include <string.h>
#include "resource.h"
#include "commctrl.h"


int unwrappedWidth(ColumnsPlusPlusData& data, Scintilla::Position from, Scintilla::Position to) {
    auto& sci = data.sci;
    int width = 0;
    int xLoc = sci.PointXFromPosition(from);
    int xEnd = sci.PointXFromPosition(to);
    if (sci.WrapMode() != Scintilla::Wrap::None && sci.WrapCount(sci.LineFromPosition(from)) > 1) {
        int yLoc = sci.PointYFromPosition(from);
        int yEnd = sci.PointYFromPosition(to);
        for (Scintilla::Position next = from + 1; yLoc != yEnd; ++next) {
            int yNext = sci.PointYFromPosition(next);
            while (yNext == yLoc) {
                ++next;
                yNext = sci.PointYFromPosition(next);
            }
            width += sci.PointXFromPosition(next - 1) - xLoc;
            char lastBeforeWrap = sci.CharacterAt(next - 1);
            char cStringBeforeWrap[] = " ";
            if (lastBeforeWrap) cStringBeforeWrap[0] = lastBeforeWrap;
            width += sci.TextWidth(sci.StyleIndexAt(next - 1), cStringBeforeWrap);
            xLoc = sci.PointXFromPosition(next);
            yLoc = yNext;
        }
    }
    return xEnd - xLoc;
}


void ColumnsPlusPlusData::setTabstops(DocumentData& dd, Scintilla::Line firstNeeded, Scintilla::Line lastNeeded, bool secondTime) {
    const int leadingIndent         = dd.blankWidth * (dd.settings.overrideTabSize ? dd.settings.minimumOrLeadingTabSize : sci.TabWidth());
    const int tabGap                = dd.blankWidth * dd.settings.minimumSpaceBetweenColumns;
    const Scintilla::Line lineCount = sci.LineCount();
    if (firstNeeded == -1) {
        firstNeeded = sci.FirstVisibleLine();
        lastNeeded  = firstNeeded + sci.LinesOnScreen();
    }
    if (lastNeeded < 0 || lastNeeded >= lineCount) lastNeeded = lineCount - 1;
    Scintilla::Line lastMonospaceFail = -1;
    size_t tlbIndex = 0;
    for (Scintilla::Line lineNum = firstNeeded; lineNum <= lastNeeded; ++lineNum) {
        while (tlbIndex < dd.tabLayouts.size() && dd.tabLayouts[tlbIndex].lastLine < lineNum) ++tlbIndex;
        if (tlbIndex >= dd.tabLayouts.size() || dd.tabLayouts[tlbIndex].firstLine > lineNum) {
            sci.ClearTabStops(lineNum);
            continue;
        }
        Scintilla::Position lineStarts = sci.PositionFromLine(lineNum);
        Scintilla::Position lineEnds   = sci.LineEnd(lineNum);
        if (lineStarts == lineEnds) continue;
        std::string line = sci.StringOfRange(Scintilla::Span(lineStarts, lineEnds));
        std::vector<int> tabs;
        std::vector<size_t> tabOffsets;
        if (dd.settings.leadingTabsIndent) for (unsigned int i = 0; i < line.length() && line[i] == '\t'; ++i) {
            tabs.push_back((i + 1) * leadingIndent);
            tabOffsets.push_back(i);
        }
        size_t leadingTabCount = tabs.size();
        int eolLimit = 0;
        {
            int tabstop = 0;
            const TabLayoutBlock* tlb = &dd.tabLayouts[tlbIndex];
            for (size_t tabchar = leadingTabCount; tabchar = line.find_first_of('\t', tabchar), tabchar != std::string::npos; ++tabchar) {
                tabstop += tlb->width;
                tabs.push_back(tabstop);
                tabOffsets.push_back(tabchar);
                size_t i = 0;
                while (i < tlb->right.size() && tlb->right[i].lastLine < lineNum) ++i;
                if (i >= tlb->right.size() || tlb->right[i].firstLine > lineNum) break;
                tlb = &tlb->right[i];
            }
            if (dd.settings.treatEolAsTab && tlb->right.size()) eolLimit = tabstop + tlb->width;
        }
        if (leadingTabCount == tabs.size()) {
            sci.ClearTabStops(lineNum);
            if (!eolLimit) continue;
        }
        else {
            bool unchanged = true;
            int position = 0;
            for (size_t i = 0; i < tabs.size(); ++i) {
                position = sci.GetNextTabStop(lineNum, position);
                if (position != tabs[i]) {
                    unchanged = false;
                    break;
                }
            }
            if (!unchanged) {
                sci.ClearTabStops(lineNum);
                for (size_t i = 0; i < tabs.size(); ++i) sci.AddTabStop(lineNum, tabs[i]);
            }
        }
        if (!secondTime && dd.assumeMonospace) {
            int pxStart = sci.PointXFromPosition(lineStarts);
            if ( (tabs.size() > leadingTabCount && tabs.back() < sci.PointXFromPosition(lineStarts + tabOffsets.back() + 1) - pxStart)
              || (eolLimit                      && eolLimit < sci.PointXFromPosition(lineEnds) - pxStart + tabGap) ) {
                lastMonospaceFail = lineNum;
                if (eolLimit) tabOffsets.push_back(line.length());
                size_t tabIndex = leadingTabCount;
                size_t from = 0;
                for (TabLayoutBlock* tlb = &dd.tabLayouts[tlbIndex]; tabIndex < tabOffsets.size(); ++tabIndex) {
                    int width = unwrappedWidth(*this, lineStarts + from, lineStarts + tabOffsets[tabIndex]) + tabGap;
                    if (width > tlb->width) tlb->width = width;
                    size_t i = 0;
                    if (i < tlb->right.size() && tlb->right[i].lastLine < lineNum) ++i;
                    if (i >= tlb->right.size() || tlb->right[i].firstLine > lineNum) break;
                    tlb = &tlb->right[i];
                    from = tabOffsets[tabIndex] + 1;
                }
            }
        }
    }
    if (lastMonospaceFail >= 0) setTabstops(dd, firstNeeded, lastMonospaceFail, true);
}


void ColumnsPlusPlusData::analyzeTabstops(DocumentData& dd) {
    dd.elasticAnalysisRequired = false;
    dd.deleteWithoutLayoutChange = false;
    int digitWidth = sci.TextWidth(STYLE_DEFAULT, "0");
    int tabGap = dd.blankWidth * dd.settings.minimumSpaceBetweenColumns;
    int tabInd = dd.blankWidth * (dd.settings.overrideTabSize ? dd.settings.minimumOrLeadingTabSize : sci.TabWidth());
    int tabMin = dd.settings.leadingTabsIndent ? 0 : tabInd;
    Scintilla::Line lineCount = sci.LineCount();
    dd.tabLayouts.clear();
    for (Scintilla::Line lineNum = 0; lineNum < lineCount; ++lineNum) {
        Scintilla::Position begin = sci.PositionFromLine(lineNum);
        Scintilla::Position end = sci.LineEnd(lineNum);
        if (begin == end) continue;
        std::string lineText = sci.StringOfRange(Scintilla::Span(begin, end));
        if (dd.settings.treatEolAsTab) lineText += "\t";
        std::vector<TabLayoutBlock>* layouts = &dd.tabLayouts;
        size_t from = dd.settings.leadingTabsIndent ? lineText.find_first_not_of('\t') : 0;
        if (from == std::string::npos) continue;
        int indentSize = static_cast<int>(from) * tabInd;
        for (size_t tab; tab = lineText.find_first_of('\t', from), tab != std::string::npos;) {
            if (!layouts->size() || (!dd.settings.lineUpAll && layouts->back().lastLine < lineNum - 1)) layouts->emplace_back(lineNum, tabMin);
            TabLayoutBlock& tlb = layouts->back();
            tlb.lastLine = lineNum;
            int width = dd.assumeMonospace ? static_cast<int>(sci.CountCharacters(begin + from, begin + tab)) * digitWidth
                                           : unwrappedWidth(*this, begin + from, begin + tab);
            width += tabGap + indentSize;
            indentSize = 0;
            if (width > tlb.width) tlb.width = width;
            from = tab + 1;
            layouts = &tlb.right;
        }
    }
}


// Given a Scintilla position and length, find the tab layout block within which the characters occur.
// If found, true is returned and tlb points to the tab layout block and width is the pixel width of the text in the block on the line containing the characters.
// If the characters occur after the last tab on a line and treatEolAsTab is not set, true is returned and tlb is null.
// If treatEolAsTab is set, lineUpAll is not set, and the characters comprise the entire line, false is returned.
// If the characters fall on multiple lines, the characters include a tab,
// adding or removing the characters would change which tabs are leading tabs and leadingTabsIndent is true,
// or an error has occurred, false is returned.  False implies the tab layout cannot be updated and a new full analysis is required.

bool ColumnsPlusPlusData::findTabLayoutBlock(DocumentData& dd, Scintilla::Position position, Scintilla::Position length, TabLayoutBlock*& tlb, int& width) {
    Scintilla::Line lineNum = sci.LineFromPosition(position);
    Scintilla::Position endLine = sci.LineEnd(lineNum);
    if (position + length > endLine) /* string goes past end of line */ {
        tlb = 0;
        width = -1;
        return false;
    }
    Scintilla::Position beginLine = sci.PositionFromLine(lineNum);
    std::string line_buffer = sci.StringOfRange(Scintilla::Span(beginLine, endLine));
    intptr_t lineLength = line_buffer.length();
    char* line = line_buffer.data();
    char* positionInString = line + position - beginLine;
    char* tabAfter = (char*)memchr(positionInString, '\t', lineLength - (positionInString - line));
    if (!tabAfter) /* no tabs follow this position */ {
        if (dd.settings.treatEolAsTab) {
            if (!dd.settings.lineUpAll && length == lineLength) /* was or will be empty line: merge or break tlb */ {
                tlb = 0;
                width = -4;
                return false;
            }
            tabAfter = line + lineLength;
        }
        else {
            tlb = 0;
            width = 0;
            return true;
        }
    }
    if (tabAfter - positionInString < length) /* string includes a tab character */ {
        tlb = 0;
        width = -2;
        return false;
    }
    int tabCount = 0;
    int indentCount = 0;
    char* p = line;
    char* tabStopBefore = line;
    if (dd.settings.leadingTabsIndent) {
        while (p < positionInString && *p == '\t') ++p;
        if (positionInString == p && tabAfter - positionInString == length) /* changes which tabs are leading tabs */ {
            tlb = 0;
            width = -3;
            return false;
        }
        indentCount = static_cast<int>(p - line);
        tabStopBefore = p;
    }
    while (p = (char*)memchr(p, '\t', positionInString - p), p) {
        ++tabCount;
        ++p;
        tabStopBefore = p;
        indentCount = 0;
    }
    width = sci.PointXFromPosition(beginLine + (tabAfter - line)) - sci.PointXFromPosition(beginLine + (tabStopBefore - line));
    if (indentCount) width += indentCount * sci.TextWidth(STYLE_DEFAULT, " ")
                                          * (dd.settings.overrideTabSize ? dd.settings.minimumOrLeadingTabSize : dd.tabOriginal);
    std::vector<TabLayoutBlock>* layouts = &dd.tabLayouts;
    for (int n = 0; n <= tabCount; ++n) {
        for (size_t i = 0;; ++i) /* find TabLayoutBlock at level n for line lineNum */ {
            if (i >= layouts->size()) /* no such TabLayoutBlock */ {
                tlb = 0;
                width = -999;
                return false;
            }
            tlb = &(*layouts)[i];
            if (lineNum >= tlb->firstLine && lineNum <= tlb->lastLine) {
                layouts = &tlb->right;
                break;
            }
        }
    }
    return true;
}


void ColumnsPlusPlusData::scnModified(const Scintilla::NotificationData* scnp) {
    using Scintilla::FlagSet;
    if ( !FlagSet( scnp->modificationType,
                   ( Scintilla::ModificationFlags::InsertText | Scintilla::ModificationFlags::BeforeDelete
                   | Scintilla::ModificationFlags::DeleteText | Scintilla::ModificationFlags::ChangeStyle ) ) ) return;
    DocumentData* ddp = getDocument(scnp);
    if (!ddp) return;
    DocumentData& ctd = *ddp;
    if (!ctd.settings.elasticEnabled || ctd.elasticAnalysisRequired) return;
    int blankWidth = sci.TextWidth(STYLE_DEFAULT, " ");
    int tabGap = blankWidth * ctd.settings.minimumSpaceBetweenColumns;
    if (FlagSet(scnp->modificationType, Scintilla::ModificationFlags::InsertText)) {
        ctd.deleteWithoutLayoutChange = false;
        if (scnp->linesAdded == 0) /* Unless the number of lines is unchanged, we need full analysis. */ {
            TabLayoutBlock* tlb;
            int width;
            if (findTabLayoutBlock(ctd, scnp->position, scnp->length, tlb, width)) {
                if (tlb) {
                    width += tabGap;
                    if (width > tlb->width) tlb->width = width;
                }
                return;
            }
        }
    }
    else if (FlagSet(scnp->modificationType, Scintilla::ModificationFlags::BeforeDelete)) {
        TabLayoutBlock* tlb;
        int width;
        if (findTabLayoutBlock(ctd, scnp->position, scnp->length, tlb, width) && (!tlb || width + tabGap < tlb->width)) {
            ctd.deleteWithoutLayoutChange         = true;
            ctd.deleteWithoutLayoutChangePosition = scnp->position;
            ctd.deleteWithoutLayoutChangeLength   = scnp->length;
        }
        else ctd.deleteWithoutLayoutChange = false;
        return;
    }
    else if (FlagSet(scnp->modificationType, Scintilla::ModificationFlags::DeleteText)) {
        if (ctd.deleteWithoutLayoutChange && scnp->position == ctd.deleteWithoutLayoutChangePosition && scnp->length == ctd.deleteWithoutLayoutChangeLength) {
            ctd.deleteWithoutLayoutChange = false;
            return;
        }
    }
    ctd.elasticAnalysisRequired = true;
}


void ColumnsPlusPlusData::scnUpdateUI(const Scintilla::NotificationData* scnp) {
    DocumentData* ddp = getDocument(scnp);
    if (!ddp) return;
    if (Scintilla::FlagSet(scnp->updated, Scintilla::Update::Selection)) syncFindButton();
    if (!ddp->settings.elasticEnabled) return;
    ddp->deleteWithoutLayoutChange = false;
    if (fontSpacingChange(*ddp)) {
        setSpacing(*ddp);
        analyzeTabstops(*ddp);
    }
    else if (ddp->elasticAnalysisRequired) analyzeTabstops(*ddp);
    reselectRectangularSelectionAndControlCharSymbol(*ddp, false);
    setTabstops(*ddp);
}


void ColumnsPlusPlusData::scnZoom(const Scintilla::NotificationData* scnp) {
    DocumentData* ddp = getDocument(scnp);
    if (!ddp || !ddp->settings.elasticEnabled) return;
    ddp->deleteWithoutLayoutChange = false;
    setSpacing(*ddp);
    analyzeTabstops(*ddp);
    setTabstops(*ddp);
}


void ColumnsPlusPlusData::bufferActivated() {
    void* docptr = sci.DocPointer();
    bool isNewDocument = !documents.contains(docptr);
    DocumentData& dd = documents[docptr];
    UINT_PTR nppBufferId = SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
    if (nppBufferId != dd.buffer) isNewDocument = true;
    if (isNewDocument) {
        dd.buffer = nppBufferId;
        std::wstring extension = getFileExtension(dd.buffer);
        std::wstring profileName = extensionToProfile.contains(extension) ? extensionToProfile[extension]
            : extension.length() && extensionToProfile.contains(L"*") ? extensionToProfile[L"*"]
            : L"*";
        if (profileName == L"") settings.elasticEnabled = false;
        else if (profileName != L"*" && profiles.contains(profileName)) {
            settings.profileName = profileName;
            static_cast<ElasticTabsProfile&>(settings) = profiles[profileName];
            settings.elasticEnabled = true;
        }
        if ( extension.length() && settings.elasticEnabled
          && ( disableOverSize > 0 && disableOverSize * 1024 < sci.Length()
            || disableOverLines > 0 && disableOverLines < sci.LineCount() ) )
            settings.elasticEnabled = false;
        dd.settings = settings;
    }
    else settings = dd.settings;
    SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, elasticEnabledMenuItem, settings.elasticEnabled);
    SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, decimalSeparatorMenuItem, settings.decimalSeparatorIsComma);
    dd.tabOriginal = sci.TabWidth();
    sci.SetTabIndents(!settings.elasticEnabled);
    if (!settings.elasticEnabled) {
        sci.SetControlCharSymbol(0);
        return;
    }
    if (settings.overrideTabSize) sci.SetTabWidth(settings.minimumOrLeadingTabSize);
    if (isNewDocument) {
        setSpacing(dd);
        analyzeTabstops(dd);
        setTabstops(dd);
    }
    else reselectRectangularSelectionAndControlCharSymbol(dd, true);
}


void ColumnsPlusPlusData::fileClosed(const NMHDR* nmhdr) {
    for (auto i = documents.begin(); i != documents.end();)
        if (i->second.buffer == nmhdr->idFrom) documents.erase(i++);
        else ++i;
}


void ColumnsPlusPlusData::fileOpened(const NMHDR* nmhdr) {
    for (auto i = documents.begin(); i != documents.end();)
        if (i->second.buffer == nmhdr->idFrom) documents.erase(i++);
        else ++i;
}


void ColumnsPlusPlusData::toggleElasticEnabled() {
    DocumentData* ddp = getDocument();
    ddp->settings.elasticEnabled = settings.elasticEnabled ^= true;
    SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, elasticEnabledMenuItem, settings.elasticEnabled);
    if (settings.elasticEnabled) {
        if (settings.overrideTabSize) {
            ddp->tabOriginal = sci.TabWidth();
            sci.SetTabWidth(settings.minimumOrLeadingTabSize);
        }
        sci.SetTabIndents(0);
        setSpacing(*ddp);
        analyzeTabstops(*ddp);
        setTabstops(*ddp);
    } else {
        if (settings.overrideTabSize) sci.SetTabWidth(ddp->tabOriginal);
        sci.SetTabIndents(1);
        if (sci.ControlCharSymbol()) sci.SetControlCharSymbol(0);
        Scintilla::Line lineCount = sci.LineCount();
        for (Scintilla::Line lineNum = 0; lineNum < lineCount; ++lineNum) sci.ClearTabStops(lineNum);
    }
}


void ColumnsPlusPlusData::toggleDecimalSeparator() {
    DocumentData* ddp = getDocument();
    ddp->settings.decimalSeparatorIsComma = settings.decimalSeparatorIsComma ^= true;
    SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, decimalSeparatorMenuItem, settings.decimalSeparatorIsComma);
}
