ucd = open("UnicodeData.txt")
ucf = open("CaseFolding.txt")
out = open("UnicodeCharacterData.cpp", "w")
hdr = open("UnicodeCharacterData.h", "w")

excludeFrom = 0X32400
excludeTo   = 0XE0000
stopAt      = 0XF0000

codeCategory = []
caseFold     = []
caseLower    = []
caseUpper    = []
foldPoints   = set()

def codeEntry(codept, fields) :
    global codeCategory
    global caseLower
    global caseUpper
    global foldPoints
    tag = " + unicode_has_fold" if codept in foldPoints else ""
    if (fields[13] != "") :
        caseLower.append("{" + f"{codept:#X}, 0X{fields[13]}" + "},\n")
        tag += " + unicode_has_lower"
    if (fields[12] != "") :
        caseUpper.append("{" + f"{codept:#X}, 0X{fields[12]}" + "},\n")
        tag += " + unicode_has_upper"
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

last = -1;

for line in ucd:
    fields = line.split(";")
    codept = int(fields[0], 16)
    if codept >= stopAt :
        break
    if fields[1].find(", Last>") >= 0 :
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

out.write('#include "UnicodeCharacterData.h"\n\nconst uint8_t unicode_category[] = {\n')
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
hdr.write(f"constexpr char32_t unicode_exclude_from = {excludeFrom:#X};\n")
hdr.write(f"constexpr char32_t unicode_exclude_to   = {excludeTo:#X};\n")
hdr.write(f"constexpr char32_t unicode_last_codept  = {last:#X};\n\n")
hdr.write('#include "UnicodeCharacterDataFixed.h"\n')
hdr.close()
