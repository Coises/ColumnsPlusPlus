# This file is part of Columns++ for Notepad++.
# Copyright 2025 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>
#
# The Python program contained in this file uses the files in the UCD sub-directory
# to build the files:
#     UnicodeCharacterData.cpp
#     UnicodeCharacterData.h
# which are used in Columns++ to support regular expressions for Unicode files.
#
# This file is released under the MIT (Expat) license:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
# associated documentation files (the "Software"), to deal in the Software without restriction, 
# including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial 
# portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
# LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import re

ucd = open("UCD\\UnicodeData.txt"          , encoding='utf-8')
ucf = open("UCD\\CaseFolding.txt"          , encoding='utf-8')
ucg = open("UCD\\GraphemeBreakProperty.txt", encoding='utf-8')
dcp = open("UCD\\DerivedCoreProperties.txt", encoding='utf-8')
emj = open("UCD\\emoji-data.txt"           , encoding='utf-8')
out = open("UnicodeCharacterData.cpp", "w" , encoding='utf-8')
hdr = open("UnicodeCharacterData.h", "w"   , encoding='utf-8')

dcpParse = re.compile(r"([0-9a-fA-F]++)(?:\.\.([0-9a-fA-F]++))?+\s*+;\s*+InCB\s*+;\s*+(\w++)");
emojiParse = re.compile(r"([0-9a-fA-F]++)(?:\.\.([0-9a-fA-F]++))?+\s*+;\s*+Extended_Pictographic\b");
graphParse = re.compile(r"([0-9a-fA-F]++)(?:\.\.([0-9a-fA-F]++))?+\s*+;\s*+(\w++)");

excludeFrom = 0X33480
excludeTo   = 0XE0000
stopAt      = 0XF0000
firstGraphBreakComplex = 0x200000

codeCategory = []
caseFold     = []
caseLower    = []
caseUpper    = []
graphBreak   = dict()
indicBreak   = dict()
foldPoints   = set()
extPict      = set()

def codeEntry(codept, fields) :
    global codeCategory
    global caseLower
    global caseUpper
    global foldPoints
    tag = f" | (GraphBreak_{graphBreak[codept]} << GraphBreakShift)" if codept in graphBreak else ""
    if codept in indicBreak :
        tag += f" | (IndicBreak_{indicBreak[codept]} << IndicBreakShift)"
    if codept in foldPoints :
        tag += " | unicode_has_fold"
    if (fields[13] != "") :
        caseLower.append("{" + f"{codept:#X}, 0X{fields[13]}" + "},\n")
        tag += " | unicode_has_lower"
    if (fields[12] != "") :
        caseUpper.append("{" + f"{codept:#X}, 0X{fields[12]}" + "},\n")
        tag += " | unicode_has_upper"
    if codept in extPict :
        tag += " | unicode_extended_pictographic"
    codeCategory.append(f"/* {codept:>5X} */ Category_{fields[2]}{tag},\n")

for line in ucf:
    if line[0:1] == "#" :
        continue
    fields = line.split("; ")
    if len(fields) < 3 or (fields[1] != "C" and fields[1] != "S") :
        continue
    caseFold.append("{" + f"0X{fields[0]}, 0X{fields[2]}" + "},\n")
    foldPoints.add(int(fields[0], 16))

ucf.close()

for line in ucg:
    gm = graphParse.match(line)
    if not gm :
        continue
    cp1 = int(gm.group(1), 16)
    cp2 = cp1 if not gm.group(2) else int(gm.group(2), 16)
    for cp in range(cp1, cp2 + 1) :
        graphBreak[cp] = gm.group(3)
    if firstGraphBreakComplex > cp1 and gm.group(3) != "Control" and  gm.group(3) != "CR" and  gm.group(3) != "LF" :
        firstGraphBreakComplex = cp1
        
ucg.close()

for line in dcp:
    indic = dcpParse.match(line)
    if not indic :
        continue
    cp1 = int(indic.group(1), 16)
    cp2 = cp1 if not indic.group(2) else int(indic.group(2), 16)
    for cp in range(cp1, cp2 + 1) :
        indicBreak[cp] = indic.group(3)

dcp.close()

for line in emj:
    emoji = emojiParse.match(line)
    if not emoji :
        continue
    cp1 = int(emoji.group(1), 16)
    cp2 = cp1 if not emoji.group(2) else int(emoji.group(2), 16)
    for cp in range(cp1, cp2 + 1) :
        extPict.add(cp)

emj.close()

last = -1;

for line in ucd:
    fields = line.split(";")
    codept = int(fields[0], 16)
    if codept >= stopAt :
        break
    if fields[1].find(", Last>") >= 0 :
        if codept >= excludeFrom and last < excludeFrom :
            out.write(f"** ERROR: Code point {codept:X} within exclusion zone; table will be broken! **\n");
        for i in range(last + 1, codept) :
            codeEntry(i, fields)
    else :
        if (last + 1 != codept) :
            if codept >= excludeFrom and last <= excludeFrom :
                if codept < excludeTo :
                    out.write(f"** ERROR: Code point {codept:X} within exclusion zone; table will be broken! **\n");
                reps = codept - last - 1 - (excludeTo - excludeFrom)
            else :
                reps = codept - last - 1
            rep1 = reps // 40
            rep2 = reps % 40
            if (rep1 > 0) :
                codeCategory.append((("0," * 40)  + "\n") * rep1)
            if (rep2 > 0) :
                codeCategory.append(("0," * rep2) + "\n")
    codeEntry(codept, fields)
    last = codept
    
ucd.close()

out.write('#include "UnicodeCharacterData.h"\n\nconst uint16_t unicode_character_data[] = {\n')
out.writelines(codeCategory)
out.write("};\n\nconst std::map<char32_t, char32_t> unicode_fold = {\n")
out.writelines(caseFold)
out.write("};\n\nconst std::map<char32_t, char32_t> unicode_lower = {\n")
out.writelines(caseLower)
out.write("};\n\nconst std::map<char32_t, char32_t> unicode_upper = {\n")
out.writelines(caseUpper)
out.write("};\n")
out.close()

hdr.write("#pragma once\n\n")
hdr.write(f"constexpr char32_t unicode_exclude_from             = {excludeFrom:#X};\n")
hdr.write(f"constexpr char32_t unicode_exclude_to               = {excludeTo:#X};\n")
hdr.write(f"constexpr char32_t unicode_last_codept              = {last:#X};\n")
hdr.write(f"constexpr char32_t unicode_first_GraphBreak_complex = {firstGraphBreakComplex:>#7X};\n\n")
hdr.write('#include "UnicodeCharacterDataFixed.h"\n')
hdr.close()
