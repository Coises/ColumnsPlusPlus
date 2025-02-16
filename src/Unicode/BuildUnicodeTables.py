ucd = open("UnicodeData.txt")
out = open("UnicodeCharacterData.cpp", "w")
hdr = open("UnicodeCharacterData.h", "w")

excludeFrom = 0X32400
excludeTo   = 0XE0000
stopAt      = 0XF0000

codeCategory = []
codePartner  = []
codeParNotes = []

def codeEntry(codept, fields) :
    global codeCategory
    global codePartner 
    global codeParNotes
    codeCategory.append(f"/* {codept:>5X} */ Category_{fields[2]},\n")
    if (fields[12] == "") :
        if (fields[13] == "") :
            codePartner.append(f"/* {codept:>5X} */ {codept:#X},\n")
        else:
            codePartner.append(f"/* {codept:>5X} */ 0X{fields[13]},\n")
    elif (fields[13] == "") :
        codePartner.append(f"/* {codept:>5X} */ 0X{fields[12]},\n")
    else :
        codePartner.append(f"/* {codept:>5X} */ 0X200000,\n")
        codeParNotes.append(f"// Code point {codept:>5X} has upper case {fields[12]} and lower case {fields[13]}\n")

last = -1;

for line in ucd:
    fields = line.split(";")
    codept = int(fields[0], 16)
    if codept >= stopAt :
        codeCategory[-1] = codeCategory[-1][0:-2]
        codePartner[-1] = codePartner[-1][0:-2]
        break
    if fields[1].find(", Last>") >= 0 :
        for i in range(last + 1, codept) :
            codeEntry(i, fields)
    else :
        if (last + 1 != codept) :
            if codept >= excludeFrom and last <= excludeFrom :
                if codept < excludeTo :
                    out.write(f"** ERROR: Code point {codept:X} within exclusion zone; table will be broken! **\n");
                codeCategory.append(("0," * (codept - last - 1 - (excludeTo - excludeFrom))) + "\n")
                codePartner.append(("0," * (codept - last - 1 - (excludeTo - excludeFrom))) + "\n")
            else :
                codeCategory.append(("0," * (codept - last - 1)) + "\n")
                codePartner.append(("0," * (codept - last - 1)) + "\n")
    codeEntry(codept, fields)
    last = codept
    
ucd.close()

out.write('#include "UnicodeCharacterData.h"\n\n')
out.write("// Partner special cases:\n")
out.writelines(codeParNotes)
out.write("\nuint8_t unicode_category[] = {\n")
out.writelines(codeCategory)
out.write("\n};\n\nchar32_t unicode_partner[] = {\n")
out.writelines(codePartner)
out.write("\n};\n")
out.close()

hdr.write("#pragma once\n\n")
hdr.write(f"constexpr char32_t unicode_exclude_from = {excludeFrom:#X};\n")
hdr.write(f"constexpr char32_t unicode_exclude_to   = {excludeTo:#X};\n")
hdr.write(f"constexpr char32_t unicode_last_codept  = {last:#X};\n\n")
hdr.write('#include "UnicodeCharacterDataFixed.h"\n')
hdr.close()
