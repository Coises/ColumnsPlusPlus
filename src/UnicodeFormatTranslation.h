// This file is part of "NppCppMSVS: Visual Studio Project Template for a Notepad++ C++ Plugin"
// Copyright 2025, 2026 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

// The source code contained in this file is independent of Notepad++ code.
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

// This header provides inline functions for conversion between between Unicode formats utf-8, utf-16 and utf-32.
// It also contains inline functions in the utf8byte namespace for interpreting individual bytes in a utf-8 stream.

#pragma once
#include <string>
#include <string_view>

enum class InvalidUnicode {
    Substitute  = 0,  // Use substitution character when transcoding invalid Unicode
    Preserve_8  = 1,  // Use Python surrogate escape to round-trip invalid UTF-8
    Preserve_16 = 2   // Use WTF-8 to round-trip invalid UTF-16
};

namespace utf8byte {
    inline bool isASCII(char c) { return (c & 0x80) == 0x00; }
    inline bool isTrail(char c) { return (c & 0xC0) == 0x80; }
    inline bool isLead2(char c) { return (c & 0xE0) == 0xC0 && (c & 0xFE) != 0xC0; }
    inline bool isLead3(char c) { return (c & 0xF0) == 0xE0; }
    inline bool isLead4(char c) { return (c & 0xFC) == 0xF0 || c == 0xF4; }
    inline bool isTrash(char c) { return (c & 0xFE) == 0xC0 || ((c & 0xF0) == 0xF0 && (c & 0x0C) != 0x00 && c != 0xF4); }
    inline bool badPair(unsigned char c1, unsigned char c2) {
        // checks first two of 3 or 4 byte sequences; does not validate 1 or 2 byte sequences
        return ((c1 == 0xE0 && c2 < 0xA0) || (c1 == 0xED && c2 > 0x9F) || (c1 == 0xF0 && c2 < 0x90) || (c1 == 0xF4 && c2 > 0x8F));
    }
    inline size_t implicit_length(char c) {
        return isASCII(c) ? 1
             : isLead2(c) ? 2
             : isLead3(c) ? 3
             : isLead4(c) ? 4
                          : 0;
    }
    inline bool valid_trail(char c1, char c2, char c3)          { return !badPair(c1, c2) && isTrail(c2) && isTrail(c3); }
    inline bool valid_trail(char c1, char c2, char c3, char c4) { return !badPair(c1, c2) && isTrail(c2) && isTrail(c3) && isTrail(c4); }
    inline char32_t to32(char c1, char c2)                   { return (((c1 & 0x1F) <<  6) |  (c2 & 0x3F)); }
    inline char32_t to32(char c1, char c2, char c3)          { return (((c1 & 0x0F) << 12) | ((c2 & 0x3F) <<  6) |  (c3 & 0x3F)); }
    inline char32_t to32(char c1, char c2, char c3, char c4) { return (((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)); }
}


// Translation between utf-8, utf-16 and utf-32

inline std::u32string utf16to32(const std::wstring_view w) {
    std::u32string u;
    for (size_t i = 0; i < w.length(); ++i) {
        if (w[i] >= 0xD800 && w[i] < 0xDC00 && i + 1 < w.length() && w[i + 1] >= 0xDC00 && w[i + 1] <= 0xDFFF) {
            u += (static_cast<char32_t>(w[i] & 0x7FF) << 10 | (w[i + 1] & 0x03FF)) + 0x10000;
            ++i;
        }
        else u += w[i];
    }
    return u;
}

inline std::wstring utf32to16(const std::u32string_view u) {
    std::wstring w;
    for (size_t i = 0; i < u.length(); ++i) {
        if (u[i] >= 0x10000) {
            w += static_cast<wchar_t>(0xD800 + ((u[i] - 0x10000) >> 10));
            w += static_cast<wchar_t>(0xDC00 + (u[i] & 0x03FF));
        }
        else w += static_cast<wchar_t>(u[i]);
    }
    return w;
}

inline std::u32string utf8to32(const std::string_view s, InvalidUnicode errs = InvalidUnicode::Substitute) {
    std::u32string u;
    for (size_t i = 0; i < s.length(); ++i) {
        switch (utf8byte::implicit_length(s[i])) {
        case 1:
            u += s[i];
            continue;
        case 2:
            if (i + 1 >= s.length() || !utf8byte::isTrail(s[i + 1])) break;
            u += utf8byte::to32(s[i], s[i + 1]);
            i += 1;
            continue;
        case 3:
            if (i + 2 >= s.length() || !utf8byte::isTrail(s[i + 1]) || !utf8byte::isTrail(s[i + 2])) break;
            if ((errs != InvalidUnicode::Preserve_16 || s[i] != 0xED) && utf8byte::badPair(s[i], s[i + 1])) break;
            u += static_cast<wchar_t>(utf8byte::to32(s[i], s[i + 1], s[i + 2]));
            i += 2;
            continue;
        case 4:
            if (i + 3 >= s.length() || !utf8byte::valid_trail(s[i], s[i + 1], s[i + 2], s[i + 3])) break;
            u += utf8byte::to32(s[i], s[i + 1], s[i + 2], s[i + 3]);
            i += 3;
            continue;
        }
        u += (errs == InvalidUnicode::Preserve_8) ? 0xDC00 | s[i] : 0xFFFD;
    }
    return u;
}

inline std::string utf32to8(const std::u32string_view u, InvalidUnicode errs = InvalidUnicode::Substitute) {
    std::string s;
    for (auto c : u) {
        if (c < 0x80) s += static_cast<char>(c);
        else if (c < 0x800) {
            s += static_cast<char>((c >> 6) | 0xC0);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else if (c >= 0xD800 && c <= 0xDFFF && errs != InvalidUnicode::Preserve_16) {
            if (errs == InvalidUnicode::Preserve_8 && (c >= 0xDC80 && c <= 0xDCFF)) s += static_cast<char>(0xFF & c);
            else s += "\xEF\xBF\xBD";
        }
        else if (c <= 0x10000) {
            s += static_cast<char>((c >> 12) | 0xE0);
            s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else if (c <= 0x110000){
            s += static_cast<char>((c >> 18) | 0xF0);
            s += static_cast<char>(((c >> 12) & 0x3F) | 0x80);
            s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else s += "\xEF\xBF\xBD";
    }
    return s;
}

inline std::wstring utf8to16(const std::string_view s, InvalidUnicode errs = InvalidUnicode::Substitute) {
    std::wstring w;
    for (size_t i = 0; i < s.length(); ++i) {
        switch (utf8byte::implicit_length(s[i])) {
        case 1:
            w += s[i];
            continue;
        case 2:
            if (i + 1 >= s.length() || !utf8byte::isTrail(s[i + 1])) break;
            w += static_cast<wchar_t>(utf8byte::to32(s[i], s[i + 1]));
            i += 1;
            continue;
        case 3:
            if (i + 2 >= s.length() || !utf8byte::isTrail(s[i + 1]) || !utf8byte::isTrail(s[i + 2])) break;
            if ((errs != InvalidUnicode::Preserve_16 || s[i] != 0xED) && utf8byte::badPair(s[i], s[i + 1])) break;
            w += static_cast<wchar_t>(utf8byte::to32(s[i], s[i + 1], s[i + 2]));
            i += 2;
            continue;
        case 4:
            if (i + 3 >= s.length() || !utf8byte::valid_trail(s[i], s[i + 1], s[i + 2], s[i + 3])) break;
            char32_t u = utf8byte::to32(s[i], s[i + 1], s[i + 2], s[i + 3]);
            w += static_cast<wchar_t>(0xD800 + ((u - 0x10000) >> 10));
            w += static_cast<wchar_t>(0xDC00 + (u & 0x03FF));
            i += 3;
            continue;
        }
        w += (errs == InvalidUnicode::Preserve_8) ? 0xDC00 | s[i] : 0xFFFD;
    }
    return w;
}

inline std::string utf16to8(const std::wstring_view w, InvalidUnicode errs = InvalidUnicode::Substitute) {
    std::string s;
    for (size_t i = 0; i < w.length(); ++i) {
        wchar_t c = w[i];
        if (c < 0x80) s += static_cast<char>(c);
        else if (c < 0x800) {
            s += static_cast<char>((c >> 6) | 0xC0);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else if (c < 0xD800 || c > 0xDFFF) {
            s += static_cast<char>((c >> 12) | 0xE0);
            s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
            s += static_cast<char>((c & 0x3F) | 0x80);
        }
        else {
            if (i + 1 < w.length() && c < 0xDC00 && w[i + 1] >= 0xDC00 && w[i + 1] <= 0xDFFF) {
                char32_t u = ((static_cast<char32_t>(c & 0x3FF) << 10) | (w[i + 1] & 0x03FF)) + 0x10000;
                s += static_cast<char>((u >> 18) | 0xF0);
                s += static_cast<char>(((u >> 12) & 0x3F) | 0x80);
                s += static_cast<char>(((u >> 6) & 0x3F) | 0x80);
                s += static_cast<char>((u & 0x3F) | 0x80);
                ++i;
            }
            else if (errs == InvalidUnicode::Preserve_8 && (c >= 0xDC80 && c <= 0xDCFF)) s += static_cast<char>(0xFF & c);
            else if (errs == InvalidUnicode::Preserve_16) {
                s += static_cast<char>(0xED);
                s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
                s += static_cast<char>((c & 0x3F) | 0x80);
            }
            else s += "\xEF\xBF\xBD";
        }
    }
    return s;
}
