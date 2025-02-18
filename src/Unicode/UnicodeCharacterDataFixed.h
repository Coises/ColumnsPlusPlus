#include <cstdint>
#include <map>
extern const uint8_t unicode_category[];
extern const std::map<char32_t, char32_t> unicode_fold;
extern const std::map<char32_t, char32_t> unicode_lower;
extern const std::map<char32_t, char32_t> unicode_upper;

enum Unicode_Category : uint8_t {
    Category_Cn, // Unassigned              a reserved unassigned code point or a noncharacter
    Category_Cc, // Control                 a C0 or C1 control code
    Category_Cf, // Format                  a format control character
    Category_Co, // Private_Use             a private-use character
    Category_Cs, // Surrogate               a surrogate code point
    Category_Ll, // Lowercase_Letter        a lowercase letter
    Category_Lm, // Modifier_Letter         a modifier letter
    Category_Lo, // Other_Letter            other letters, including syllables and ideographs
    Category_Lt, // Titlecase_Letter        a digraph encoded as a single character, with first part uppercase
    Category_Lu, // Uppercase_Letter        an uppercase letter
    Category_Mc, // Spacing_Mark            a spacing combining mark (positive advance width)
    Category_Me, // Enclosing_Mark          an enclosing combining mark
    Category_Mn, // Nonspacing_Mark         a nonspacing combining mark (zero advance width)
    Category_Nd, // Decimal_Number          a decimal digit
    Category_Nl, // Letter_Number           a letterlike numeric character
    Category_No, // Other_Number            a numeric character of other type
    Category_Pc, // Connector_Punctuation   a connecting punctuation mark, like a tie
    Category_Pd, // Dash_Punctuation        a dash or hyphen punctuation mark
    Category_Pe, // Close_Punctuation       a closing punctuation mark (of a pair)
    Category_Pf, // Final_Punctuation       a final quotation mark
    Category_Pi, // Initial_Punctuation     an initial quotation mark
    Category_Po, // Other_Punctuation       a punctuation mark of other type
    Category_Ps, // Open_Punctuation        an opening punctuation mark (of a pair)
    Category_Sc, // Currency_Symbol         a currency sign
    Category_Sk, // Modifier_Symbol         a non-letterlike modifier symbol
    Category_Sm, // Math_Symbol             a symbol of mathematical use
    Category_So, // Other_Symbol            a symbol of other type
    Category_Zl, // Line_Separator          U+2028 LINE SEPARATOR only
    Category_Zp, // Paragraph_Separator     U+2029 PARAGRAPH SEPARATOR only
    Category_Zs  // Space_Separator         a space character (of various non-zero widths)
};

constexpr uint64_t CatMask_Cn = 1Ui64 << (Category_Cn + 32);
constexpr uint64_t CatMask_Cc = 1Ui64 << (Category_Cc + 32);
constexpr uint64_t CatMask_Cf = 1Ui64 << (Category_Cf + 32);
constexpr uint64_t CatMask_Co = 1Ui64 << (Category_Co + 32);
constexpr uint64_t CatMask_Cs = 1Ui64 << (Category_Cs + 32);
constexpr uint64_t CatMask_Ll = 1Ui64 << (Category_Ll + 32);
constexpr uint64_t CatMask_Lm = 1Ui64 << (Category_Lm + 32);
constexpr uint64_t CatMask_Lo = 1Ui64 << (Category_Lo + 32);
constexpr uint64_t CatMask_Lt = 1Ui64 << (Category_Lt + 32);
constexpr uint64_t CatMask_Lu = 1Ui64 << (Category_Lu + 32);
constexpr uint64_t CatMask_Mc = 1Ui64 << (Category_Mc + 32);
constexpr uint64_t CatMask_Me = 1Ui64 << (Category_Me + 32);
constexpr uint64_t CatMask_Mn = 1Ui64 << (Category_Mn + 32);
constexpr uint64_t CatMask_Nd = 1Ui64 << (Category_Nd + 32);
constexpr uint64_t CatMask_Nl = 1Ui64 << (Category_Nl + 32);
constexpr uint64_t CatMask_No = 1Ui64 << (Category_No + 32);
constexpr uint64_t CatMask_Pc = 1Ui64 << (Category_Pc + 32);
constexpr uint64_t CatMask_Pd = 1Ui64 << (Category_Pd + 32);
constexpr uint64_t CatMask_Pe = 1Ui64 << (Category_Pe + 32);
constexpr uint64_t CatMask_Pf = 1Ui64 << (Category_Pf + 32);
constexpr uint64_t CatMask_Pi = 1Ui64 << (Category_Pi + 32);
constexpr uint64_t CatMask_Po = 1Ui64 << (Category_Po + 32);
constexpr uint64_t CatMask_Ps = 1Ui64 << (Category_Ps + 32);
constexpr uint64_t CatMask_Sc = 1Ui64 << (Category_Sc + 32);
constexpr uint64_t CatMask_Sk = 1Ui64 << (Category_Sk + 32);
constexpr uint64_t CatMask_Sm = 1Ui64 << (Category_Sm + 32);
constexpr uint64_t CatMask_So = 1Ui64 << (Category_So + 32);
constexpr uint64_t CatMask_Zl = 1Ui64 << (Category_Zl + 32);
constexpr uint64_t CatMask_Zp = 1Ui64 << (Category_Zp + 32);
constexpr uint64_t CatMask_Zs = 1Ui64 << (Category_Zs + 32);

constexpr uint8_t unicode_has_fold  = 0x80;
constexpr uint8_t unicode_has_lower = 0x40;
constexpr uint8_t unicode_has_upper = 0x20;

inline Unicode_Category unicodeGenCat(char32_t c) {
    if (c < unicode_exclude_from) return static_cast<Unicode_Category>(0x1fUi8 & unicode_category[c]);
    if (c < unicode_exclude_to) return Category_Cn;
    if (c > unicode_last_codept)
        return (c >= 0xF0000 && c <=0xFFFFD) || (c >= 0x100000 && c <= 0x10FFFD) ? Category_Co : Category_Cn;
    return static_cast<Unicode_Category>(0x1fUi8 & unicode_category[c - (unicode_exclude_to - unicode_exclude_from)]);
}

inline char32_t unicode_casing_operation(char32_t c, const std::map<char32_t, char32_t>& map, uint8_t flag) {
    if      (c < unicode_exclude_from) { if (!(flag & unicode_category[c])) return c; }
    else if (c < unicode_exclude_to || c > unicode_last_codept) return c;
    else if (!(flag & unicode_category[c - (unicode_exclude_to - unicode_exclude_from)])) return c;
    if (auto i = map.find(c); i != map.end()) return i->second;
    return c;
}

inline char32_t unicodeFold(char32_t c) {
    return unicode_casing_operation(c, unicode_fold, unicode_has_fold);
}

inline char32_t unicodeLower(char32_t c) {
    return unicode_casing_operation(c, unicode_lower, unicode_has_lower);
}

inline char32_t unicodeUpper(char32_t c) {
    return unicode_casing_operation(c, unicode_upper, unicode_has_upper);
}
