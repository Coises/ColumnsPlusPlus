// This file is part of Columns++ for Notepad++.
// Copyright 2025 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

// The Columns++ source code contained in this file is independent of Notepad++ code.
// It is released under the MIT (Expat) license:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial 
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "UnicodeRegexTraits.h"

const utf32_regex_traits::char_class_type utf32_regex_traits::categoryMasks[] = {
    CatMask_Cn,
    CatMask_Cc | mask_cntrl,
    CatMask_Cf,
    CatMask_Co,
    CatMask_Cs,
    CatMask_Ll | mask_graph | mask_word | mask_alpha | mask_lower,
    CatMask_Lm | mask_graph | mask_word | mask_alpha,
    CatMask_Lo | mask_graph | mask_word | mask_alpha,
    CatMask_Lt | mask_graph | mask_word | mask_alpha,
    CatMask_Lu | mask_graph | mask_word | mask_alpha | mask_upper,
    CatMask_Mc | mask_graph,
    CatMask_Me | mask_graph,
    CatMask_Mn | mask_graph,
    CatMask_Nd | mask_graph | mask_word | mask_digit,
    CatMask_Nl | mask_graph,
    CatMask_No | mask_graph,
    CatMask_Pc | mask_graph | mask_punct,
    CatMask_Pd | mask_graph | mask_punct,
    CatMask_Pe | mask_graph | mask_punct,
    CatMask_Pf | mask_graph | mask_punct,
    CatMask_Pi | mask_graph | mask_punct,
    CatMask_Po | mask_graph | mask_punct,
    CatMask_Ps | mask_graph | mask_punct,
    CatMask_Sc | mask_graph | mask_punct,
    CatMask_Sk | mask_graph | mask_punct,
    CatMask_Sm | mask_graph | mask_punct,
    CatMask_So | mask_graph | mask_punct,
    CatMask_Zl | mask_vertical,
    CatMask_Zp | mask_vertical,
    CatMask_Zs | mask_horizontal
};

const utf32_regex_traits::char_class_type utf32_regex_traits::asciiMasks[] = {
    /* 00 NUL   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 01 SOH   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 02 STX   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 03 ETX   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 04 EOT   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 05 ENQ   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 06 ACK   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 07 BEL   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 08 BS    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 09 HT    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_horizontal,
    /* 0A LF    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
    /* 0B VT    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
    /* 0C FF    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
    /* 0D CR    */ CatMask_Cc | mask_ascii | mask_cntrl | mask_vertical,
    /* 0E SO    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 0F SI    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 10 DLE   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 11 DC1   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 12 DC2   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 13 DC3   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 14 DC4   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 15 NAK   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 16 SYN   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 17 ETB   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 18 CAN   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 19 EM    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 1A SUB   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 1B ESC   */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 1C FS    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 1D GS    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 1E RS    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 1F US    */ CatMask_Cc | mask_ascii | mask_cntrl,
    /* 20 Space */ CatMask_Zs | mask_ascii | mask_horizontal,
    /* 21 !     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 22 "     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 23 #     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 24 $     */ CatMask_Sc | mask_ascii | mask_graph | mask_punct,
    /* 25 %     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 26 &     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 27 '     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 28 (     */ CatMask_Ps | mask_ascii | mask_graph | mask_punct,
    /* 29 )     */ CatMask_Pe | mask_ascii | mask_graph | mask_punct,
    /* 2A *     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 2B +     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
    /* 2C ,     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 2D -     */ CatMask_Pd | mask_ascii | mask_graph | mask_punct,
    /* 2E .     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 2F /     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 30 0     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 31 1     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 32 2     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 33 3     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 34 4     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 35 5     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 36 6     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 37 7     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 38 8     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 39 9     */ CatMask_Nd | mask_ascii | mask_graph | mask_word | mask_digit | mask_xdigit,
    /* 3A :     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 3B ;     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 3C <     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
    /* 3D =     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
    /* 3E >     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
    /* 3F ?     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 40 @     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 41 A     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
    /* 42 B     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
    /* 43 C     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
    /* 44 D     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
    /* 45 E     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
    /* 46 F     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper | mask_xdigit,
    /* 47 G     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 48 H     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 49 I     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 4A J     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 4B K     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 4C L     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 4D M     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 4E N     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 4F O     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 50 P     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 51 Q     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 52 R     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 53 S     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 54 T     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 55 U     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 56 V     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 57 W     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 58 X     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 59 Y     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 5A Z     */ CatMask_Lu | mask_ascii | mask_graph | mask_word | mask_alpha | mask_upper,
    /* 5B [     */ CatMask_Ps | mask_ascii | mask_graph | mask_punct,
    /* 5C \     */ CatMask_Po | mask_ascii | mask_graph | mask_punct,
    /* 5D ]     */ CatMask_Pe | mask_ascii | mask_graph | mask_punct,
    /* 5E ^     */ CatMask_Sk | mask_ascii | mask_graph | mask_punct,
    /* 5F _     */ CatMask_Pc | mask_ascii | mask_graph | mask_punct | mask_word,
    /* 60 `     */ CatMask_Sk | mask_ascii | mask_graph | mask_punct,
    /* 61 a     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
    /* 62 b     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
    /* 63 c     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
    /* 64 d     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
    /* 65 e     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
    /* 66 f     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower | mask_xdigit,
    /* 67 g     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 68 h     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 69 i     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 6A j     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 6B k     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 6C l     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 6D m     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 6E n     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 6F o     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 70 p     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 71 q     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 72 r     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 73 s     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 74 t     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 75 u     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 76 v     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 77 w     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 78 x     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 79 y     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 7A z     */ CatMask_Ll | mask_ascii | mask_graph | mask_word | mask_alpha | mask_lower,
    /* 7B {     */ CatMask_Ps | mask_ascii | mask_graph | mask_punct,
    /* 7C |     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
    /* 7D }     */ CatMask_Pe | mask_ascii | mask_graph | mask_punct,
    /* 7E ~     */ CatMask_Sm | mask_ascii | mask_graph | mask_punct,
    /* 7F DEL   */ CatMask_Cc | mask_ascii | mask_cntrl,
};
        
const std::map<std::string, utf32_regex_traits::char_class_type> utf32_regex_traits::classnames = {
    {"c*", CatMask_Cc | CatMask_Cf | CatMask_Cn | CatMask_Co},
    {"l*", CatMask_Ll | CatMask_Lm | CatMask_Lo | CatMask_Lt | CatMask_Lu},
    {"m*", CatMask_Mc | CatMask_Me | CatMask_Mn},
    {"n*", CatMask_Nd | CatMask_Nl | CatMask_No},
    {"p*", CatMask_Pc | CatMask_Pd | CatMask_Pe | CatMask_Pf | CatMask_Pi | CatMask_Po | CatMask_Ps},
    {"s*", CatMask_Sc | CatMask_Sk | CatMask_Sm | CatMask_So},
    {"z*", CatMask_Zl | CatMask_Zp | CatMask_Zs},
    {"cc", CatMask_Cc},
    {"cf", CatMask_Cf},
    {"cn", CatMask_Cn},
    {"co", CatMask_Co},
    {"ll", CatMask_Ll},
    {"lm", CatMask_Lm},
    {"lo", CatMask_Lo},
    {"lt", CatMask_Lt},
    {"lu", CatMask_Lu},
    {"mc", CatMask_Mc},
    {"me", CatMask_Me},
    {"mn", CatMask_Mn},
    {"nd", CatMask_Nd},
    {"nl", CatMask_Nl},
    {"no", CatMask_No},
    {"pc", CatMask_Pc},
    {"pd", CatMask_Pd},
    {"pe", CatMask_Pe},
    {"pf", CatMask_Pf},
    {"pi", CatMask_Pi},
    {"po", CatMask_Po},
    {"ps", CatMask_Ps},
    {"sc", CatMask_Sc},
    {"sk", CatMask_Sk},
    {"sm", CatMask_Sm},
    {"so", CatMask_So},
    {"zl", CatMask_Zl},
    {"zp", CatMask_Zp},
    {"zs", CatMask_Zs},
    {"ascii"                   , mask_ascii},
    {"any"                     , 0x3fffffff00000000U},
    {"assigned"                , 0x3fffffee00000000U},
    {"other"                   , CatMask_Cc | CatMask_Cf | CatMask_Cn | CatMask_Co},
    {"letter"                  , CatMask_Ll | CatMask_Lm | CatMask_Lo | CatMask_Lt | CatMask_Lu},
    {"mark"                    , CatMask_Mc | CatMask_Me | CatMask_Mn},
    {"number"                  , CatMask_Nd | CatMask_Nl | CatMask_No},
    {"punctuation"             , CatMask_Pc | CatMask_Pd | CatMask_Pe | CatMask_Pf | CatMask_Pi | CatMask_Po | CatMask_Ps},
    {"symbol"                  , CatMask_Sc | CatMask_Sk | CatMask_Sm | CatMask_So},
    {"separator"               , CatMask_Zl | CatMask_Zp | CatMask_Zs},
    {"control"                 , CatMask_Cc},
    {"format"                  , CatMask_Cf},
    {"not assigned"            , CatMask_Cn},
    {"private use"             , CatMask_Co},
    {"invalid"                 , CatMask_Cs},  // No surrogates in UTF-32, but we use some to hold invalid UTF-8 bytes
    {"lowercase letter"        , CatMask_Ll},
    {"modifier letter"         , CatMask_Lm},
    {"other letter"            , CatMask_Lo},
    {"titlecase"               , CatMask_Lt},
    {"uppercase letter"        , CatMask_Lu},
    {"spacing combining mark"  , CatMask_Mc},
    {"enclosing mark"          , CatMask_Me},
    {"non-spacing mark"        , CatMask_Mn},
    {"decimal digit number"    , CatMask_Nd},
    {"letter number"           , CatMask_Nl},
    {"other number"            , CatMask_No},
    {"connector punctuation"   , CatMask_Pc},
    {"dash punctuation"        , CatMask_Pd},
    {"close punctuation"       , CatMask_Pe},
    {"final punctuation"       , CatMask_Pf},
    {"initial punctuation"     , CatMask_Pi},
    {"other punctuation"       , CatMask_Po},
    {"open punctuation"        , CatMask_Ps},
    {"currency symbol"         , CatMask_Sc},
    {"modifier symbol"         , CatMask_Sk},
    {"math symbol"             , CatMask_Sm},
    {"other symbol"            , CatMask_So},
    {"line separator"          , CatMask_Zl},
    {"paragraph separator"     , CatMask_Zp},
    {"space separator"         , CatMask_Zs},
    {"alnum"   , mask_alnum     },
    {"alpha"   , mask_alpha     },
    {"blank"   , mask_blank     },
    {"cntrl"   , mask_cntrl     },
    {"d"       , mask_digit     },
    {"digit"   , mask_digit     },
    {"graph"   , mask_graph     },
    {"h"       , mask_horizontal},
    {"i"       , CatMask_Cs     },
    {"l"       , mask_lower     },
    {"lower"   , mask_lower     },
    {"m"       , CatMask_Mc | CatMask_Me | CatMask_Mn},
    {"o"       , mask_ascii     },
    {"print"   , mask_print     },
    {"punct"   , mask_punct     },
    {"s"       , mask_space     },
    {"space"   , mask_space     },
    {"u"       , mask_upper     },
    {"unicode" , mask_unicode   },
    {"upper"   , mask_upper     },
    {"v"       , mask_vertical  },
    {"w"       , mask_word      },
    {"word"    , mask_word      },
    {"xdigit"  , mask_xdigit    },
    {"y"       , 0x3fffffe600000000U},
    {"defined" , 0x3fffffe600000000U}
};

const std::map<std::string, utf32_regex_traits::char_type> utf32_regex_traits::character_names = {
    {"ht"    , 0x0009},  // Horizontal Tab
    {"lf"    , 0x000a},  // Line Feed
    {"cr"    , 0x000d},  // Carriage Return
    {"sflo"  , 0x1bca0}, // Shorthand Format Letter Overlap
    {"sfco"  , 0x1bca1}, // Shorthand Format Continuing Overlap
    {"sfds"  , 0x1bca2}, // Shorthand Format Down Step
    {"sfus"  , 0x1bca3}, // Shorthand Format Up Step
                         // from Notepad++ (ScintillaEditView.h):
    {"nul"   , 0x0000},  // Null
    {"soh"   , 0x0001},  // Start of Heading
    {"stx"   , 0x0002},  // Start of Text
    {"etx"   , 0x0003},  // End of Text
    {"eot"   , 0x0004},  // End of Transmission
    {"enq"   , 0x0005},  // Enquiry
    {"ack"   , 0x0006},  // Acknowledge
    {"bel"   , 0x0007},  // Bell
    {"bs"    , 0x0008},  // Backspace
    {"vt"    , 0x000b},  // Line Tabulation
    {"ff"    , 0x000c},  // Form Feed
    {"so"    , 0x000e},  // Shift Out
    {"si"    , 0x000f},  // Shift In
    {"dle"   , 0x0010},  // Data Link Escape
    {"dc1"   , 0x0011},  // Device Control One
    {"dc2"   , 0x0012},  // Device Control Two
    {"dc3"   , 0x0013},  // Device Control Three
    {"dc4"   , 0x0014},  // Device Control Four
    {"nak"   , 0x0015},  // Negative Acknowledge
    {"syn"   , 0x0016},  // Synchronous Idle
    {"etb"   , 0x0017},  // End of Transmission Block
    {"can"   , 0x0018},  // Cancel
    {"em"    , 0x0019},  // End of Medium
    {"sub"   , 0x001a},  // Substitute
    {"esc"   , 0x001b},  // Escape
    {"fs"    , 0x001c},  // Information Separator Four
    {"gs"    , 0x001d},  // Information Separator Three
    {"rs"    , 0x001e},  // Information Separator Two
    {"us"    , 0x001f},  // Information Separator One
    {"del"   , 0x007f},  // Delete
    {"pad"   , 0x0080},  // Padding Character
    {"hop"   , 0x0081},  // High Octet Preset
    {"bph"   , 0x0082},  // Break Permitted Here
    {"nbh"   , 0x0083},  // No Break Here
    {"ind"   , 0x0084},  // Index
    {"nel"   , 0x0085},  // Next Line
    {"ssa"   , 0x0086},  // Start of Selected Area
    {"esa"   , 0x0087},  // End of Selected Area
    {"hts"   , 0x0088},  // Character (Horizontal) Tabulation Set
    {"htj"   , 0x0089},  // Character (Horizontal) Tabulation With Justification
    {"lts"   , 0x008a},  // Line (Vertical) Tabulation Set
    {"pld"   , 0x008b},  // Partial Line Forward (Down)
    {"plu"   , 0x008c},  // Partial Line Backward (Up)
    {"ri"    , 0x008d},  // Reverse Line Feed (Index)
    {"ss2"   , 0x008e},  // Single-Shift Two
    {"ss3"   , 0x008f},  // Single-Shift Three
    {"dcs"   , 0x0090},  // Device Control String
    {"pu1"   , 0x0091},  // Private Use One
    {"pu2"   , 0x0092},  // Private Use Two
    {"sts"   , 0x0093},  // Set Transmit State
    {"cch"   , 0x0094},  // Cancel Character
    {"mw"    , 0x0095},  // Message Waiting
    {"spa"   , 0x0096},  // Start of Protected Area
    {"epa"   , 0x0097},  // End of Protected Area
    {"sos"   , 0x0098},  // Start of String
    {"sgci"  , 0x0099},  // Single Graphic Character Introducer
    {"sci"   , 0x009a},  // Single Character Introducer
    {"csi"   , 0x009b},  // Control Sequence Introducer
    {"st"    , 0x009c},  // String Terminator
    {"osc"   , 0x009d},  // Operating System Command
    {"pm"    , 0x009e},  // Private Message
    {"apc"   , 0x009f},  // Application Program Command
    {"nbsp"  , 0x00a0},  // no-break space
    {"shy"   , 0x00ad},  // soft hyphen
    {"alm"   , 0x061c},  // arabic letter mark
    {"sam"   , 0x070f},  // syriac abbreviation mark
    {"ospm"  , 0x1680},  // ogham space mark
    {"mvs"   , 0x180e},  // mongolian vowel separator
    {"nqsp"  , 0x2000},  // en quad
    {"mqsp"  , 0x2001},  // em quad
    {"ensp"  , 0x2002},  // en space
    {"emsp"  , 0x2003},  // em space
    {"3/msp" , 0x2004},  // three-per-em space
    {"4/msp" , 0x2005},  // four-per-em space
    {"6/msp" , 0x2006},  // six-per-em space
    {"fsp"   , 0x2007},  // figure space
    {"psp"   , 0x2008},  // punctation space
    {"thsp"  , 0x2009},  // thin space
    {"hsp"   , 0x200a},  // hair space
    {"zwsp"  , 0x200b},  // zero-width space
    {"zwnj"  , 0x200c},  // zero-width non-joiner
    {"zwj"   , 0x200d},  // zero-width joiner
    {"lrm"   , 0x200e},  // left-to-right mark
    {"rlm"   , 0x200f},  // right-to-left mark
    {"ls"    , 0x2028},  // Line Separator
    {"ps"    , 0x2029},  // Paragraph Separator
    {"lre"   , 0x202a},  // left-to-right embedding
    {"rle"   , 0x202b},  // right-to-left embedding
    {"pdf"   , 0x202c},  // pop directional formatting
    {"lro"   , 0x202d},  // left-to-right override
    {"rlo"   , 0x202e},  // right-to-left override
    {"nnbsp" , 0x202f},  // narrow no-break space
    {"mmsp"  , 0x205f},  // medium mathematical space
    {"wj"    , 0x2060},  // word joiner
    {"(fa)"  , 0x2061},  // function application
    {"(it)"  , 0x2062},  // invisible times
    {"(is)"  , 0x2063},  // invisible separator
    {"(ip)"  , 0x2064},  // invisible plus
    {"lri"   , 0x2066},  // left-to-right isolate
    {"rli"   , 0x2067},  // right-to-left isolate
    {"fsi"   , 0x2068},  // first strong isolate
    {"pdi"   , 0x2069},  // pop directional isolate
    {"iss"   , 0x206a},  // inhibit symmetric swapping
    {"ass"   , 0x206b},  // activate symmetric swapping
    {"iafs"  , 0x206c},  // inhibit arabic form shaping
    {"aafs"  , 0x206d},  // activate arabic form shaping
    {"nads"  , 0x206e},  // national digit shapes
    {"nods"  , 0x206f},  // nominal digit shapes
    {"idsp"  , 0x3000},  // ideographic space
    {"zwnbsp", 0xfeff},  // zero-width no-break space
    {"iaa"   , 0xfff9},  // interlinear annotation anchor
    {"ias"   , 0xfffa},  // interlinear annotation separator
    {"iat"   , 0xfffb},  // interlinear annotation terminator
                         // other POSIX names, from Boost (regex_traits_default.hpp):
    {"alert"               , 0x07},
    {"backspace"           , 0x08},
    {"tab"                 , 0x09},
    {"newline"             , 0x0a},
    {"vertical-tab"        , 0x0b},
    {"form-feed"           , 0x0c},
    {"carriage-return"     , 0x0d},
    {"IS4"                 , 0x1c},
    {"IS3"                 , 0x1d},
    {"IS2"                 , 0x1e},
    {"IS1"                 , 0x1f},
    {"space"               , 0x20},
    {"exclamation-mark"    , 0x21},
    {"quotation-mark"      , 0x22},
    {"number-sign"         , 0x23},
    {"dollar-sign"         , 0x24},
    {"percent-sign"        , 0x25},
    {"ampersand"           , 0x26},
    {"apostrophe"          , 0x27},
    {"left-parenthesis"    , 0x28},
    {"right-parenthesis"   , 0x29},
    {"asterisk"            , 0x2a},
    {"plus-sign"           , 0x2b},
    {"comma"               , 0x2c},
    {"hyphen"              , 0x2d},
    {"period"              , 0x2e},
    {"slash"               , 0x2f},
    {"zero"                , 0x30},
    {"one"                 , 0x31},
    {"two"                 , 0x32},
    {"three"               , 0x33},
    {"four"                , 0x34},
    {"five"                , 0x35},
    {"six"                 , 0x36},
    {"seven"               , 0x37},
    {"eight"               , 0x38},
    {"nine"                , 0x39},
    {"colon"               , 0x3a},
    {"semicolon"           , 0x3b},
    {"less-than-sign"      , 0x3c},
    {"equals-sign"         , 0x3d},
    {"greater-than-sign"   , 0x3e},
    {"question-mark"       , 0x3f},
    {"commercial-at"       , 0x40},
    {"left-square-bracket" , 0x5b},
    {"backslash"           , 0x5c},
    {"right-square-bracket", 0x5d},
    {"circumflex"          , 0x5e},
    {"underscore"          , 0x5f},
    {"grave-accent"        , 0x60},
    {"left-curly-bracket"  , 0x7b},
    {"vertical-line"       , 0x7c},
    {"right-curly-bracket" , 0x7d},
    {"tilde"               , 0x7e}
};

const std::set<utf32_regex_traits::string_type> utf32_regex_traits::digraphs = {  // from Boost
    U"ae", U"ch", U"dz", U"lj", U"ll", U"nj", U"ss",
    U"Ae", U"Ch", U"Dz", U"Lj", U"Ll", U"Nj", U"Ss",
    U"AE", U"CH", U"DZ", U"LJ", U"LL", U"NJ", U"SS"
};
