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


// =================================================================================================
// THE CODE IN THIS FILE USES TEMPLATE SPECIALIZATION TO REPLACE SOME INTERNAL BOOST::REGEX ROUTINES
// THESE ARE NOT DOCUMENTED INTERFACES AND THIS CODE MAY FAIL IF BOOST::REGEX IS UPDATED
// =================================================================================================


// Make the \X escape work properly for Unicode grapheme segmentation
// reference: https://unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries

enum class PairClusters {yes, no, maybe};

inline PairClusters pairCluster(const char32_t c1, const char32_t c2) {
    using enum PairClusters;
    if (c1 < unicode_first_GraphBreak_complex && c2 < unicode_first_GraphBreak_complex)
        return c1 == '\r' && c2 == '\n' ? yes : no;
    const auto gb1 = unicodeGCB(c1);
    const auto gb2 = unicodeGCB(c2);
    if (gb1 == GraphBreak_CR || gb1 == GraphBreak_LF || gb1 == GraphBreak_Control) return no;
    if (gb2 == GraphBreak_CR || gb2 == GraphBreak_LF || gb2 == GraphBreak_Control) return no;
    if ( (gb1 == GraphBreak_L && (gb2 == GraphBreak_L || gb2 == GraphBreak_V || gb2 == GraphBreak_LV || gb2 == GraphBreak_LVT))
     || ((gb1 == GraphBreak_LV || gb1 == GraphBreak_V) && (gb2 == GraphBreak_V || gb2 == GraphBreak_T))
     || ((gb1 == GraphBreak_LVT || gb1 == GraphBreak_T) && gb2 == GraphBreak_T) )
        return yes;
    if (gb2 == GraphBreak_Extend || gb2 == GraphBreak_ZWJ) return yes;
    if (gb1 == GraphBreak_Prepend || gb2 == GraphBreak_SpacingMark) return yes;
    if (gb1 == GraphBreak_Regional_Indicator && gb2 == GraphBreak_Regional_Indicator) return maybe;
    if (gb1 == GraphBreak_ZWJ && unicodeExtPict(c2)) return maybe;
    const auto ib1 = unicodeICB(c1);
    const auto ib2 = unicodeICB(c2);
    if ((ib1 == IndicBreak_Extend || ib1 == IndicBreak_Linker) && ib2 == IndicBreak_Consonant) return maybe;
    return no;
}

inline bool multipleCluster(const RegularExpressionU::DocumentIterator& position,
                            const RegularExpressionU::DocumentIterator& backstop) {

    const char32_t c = *position;

    if (unicodeGCB(c) == GraphBreak_Regional_Indicator) {
        auto p = position;
        size_t count = 0;
        while (p != backstop && unicodeGCB(*--p) == GraphBreak_Regional_Indicator) ++count;
        return count & 1;
    }

    if (unicodeExtPict(c)) {
        auto p = position;
        if (p == backstop || *--p != 0x200D) return false;
        while (p != backstop && unicodeGCB(*--p) == GraphBreak_Extend);
        return unicodeExtPict(*p);
    }

    if (unicodeICB(c) == IndicBreak_Consonant) {
        bool foundLinker = false;
        auto p = position;
        while (p != backstop) {
            --p;
            switch (unicodeICB(*p)) {
            case IndicBreak_Consonant :
                return foundLinker;
            case IndicBreak_Extend:
                break;
            case IndicBreak_Linker:
                foundLinker = true;
                break;
            case IndicBreak_None:
                return false;
            }
        }
        return false;
    }

    return false;
}

template <>
bool boost::BOOST_REGEX_DETAIL_NS::perl_matcher<RegularExpressionU::DocumentIterator,
    std::allocator<boost::sub_match<RegularExpressionU::DocumentIterator>>, utf32_regex_traits>::match_combining() {

    if (position == last) return false;

    char32_t c1 = *position;

    if (position != backstop) /* we could be within a grapheme; if so, we must return false */ {
        auto prior = position;
        --prior;
        char32_t c0 = *prior;
        PairClusters pc01 = pairCluster(c0, c1);
        if (pc01 == PairClusters::yes) return false;
        if (pc01 == PairClusters::maybe && multipleCluster(position, backstop)) return false;
    }

    for (char32_t c2; ++position != last; c1 = c2) {
        c2 = *position;
        PairClusters pc12 = pairCluster(c1, c2);
        if (pc12 == PairClusters::no) break;
        if (pc12 == PairClusters::yes) continue;
        if (!multipleCluster(position, backstop)) break;
    }

    pstate = pstate->next.p;
    return true;

}


// This is the logic used to test for matches to [...] and \p{...} expressions
// Most of it is copied verbatim from \boost\regex\v5\perl_matcher.hpp lines 118 to 288;
// but the named class matching is altered to ignore case insensitivity.
//
// It is used once with our iterators and once with char32_t*, so we must specialize it twice.

template <>
RegularExpressionU::DocumentIterator boost::BOOST_REGEX_DETAIL_NS::re_is_set_member(
    RegularExpressionU::DocumentIterator next,
    RegularExpressionU::DocumentIterator last,
    const boost::BOOST_REGEX_DETAIL_NS::re_set_long<utf32_regex_traits::char_class_type>* set_,
    const boost::BOOST_REGEX_DETAIL_NS::regex_data<char32_t, utf32_regex_traits>& e, bool icase) {

    typedef RegularExpressionU::DocumentIterator iterator;     // ADDED: typedef template parameters
    typedef char32_t                             charT;        // ADDED: typedef template parameters
    typedef utf32_regex_traits                   traits_type;  // ADDED: typedef template parameters
    typedef utf32_regex_traits::char_class_type  char_classT;  // ADDED: typedef template parameters

    const charT* p = reinterpret_cast<const charT*>(set_ + 1);
    iterator ptr;
    unsigned int i;
    //bool icase = e.m_flags & regex_constants::icase;

    if (next == last) return next;

    typedef typename traits_type::string_type traits_string_type;
    const ::boost::regex_traits_wrapper<traits_type>& traits_inst = *(e.m_ptraits);

    // dwa 9/13/00 suppress incorrect MSVC warning - it claims this is never
    // referenced
    (void)traits_inst;

    // try and match a single character, could be a multi-character
    // collating element...
    for (i = 0; i < set_->csingles; ++i)
    {
        ptr = next;
        if (*p == static_cast<charT>(0))
        {
            // treat null string as special case:
            if (traits_inst.translate(*ptr, icase))
            {
                ++p;
                continue;
            }
            return set_->isnot ? next : (ptr == next) ? ++next : ptr;
        }
        else
        {
            while (*p && (ptr != last))
            {
                if (traits_inst.translate(*ptr, icase) != *p)
                    break;
                ++p;
                ++ptr;
            }

            if (*p == static_cast<charT>(0)) // if null we've matched
                return set_->isnot ? next : (ptr == next) ? ++next : ptr;

            p = re_skip_past_null(p);     // skip null
        }
    }

    charT col = traits_inst.translate(*next, icase);


    if (set_->cranges || set_->cequivalents)
    {
        traits_string_type s1;
        //
        // try and match a range, NB only a single character can match
        if (set_->cranges)
        {
            if ((e.m_flags & regex_constants::collate) == 0)
                s1.assign(1, col);
            else
            {
                charT a[2] = { col, charT(0), };
                s1 = traits_inst.transform(a, a + 1);
            }
            for (i = 0; i < set_->cranges; ++i)
            {
                if (STR_COMP(s1, p) >= 0)
                {
                    do { ++p; } while (*p);
                    ++p;
                    if (STR_COMP(s1, p) <= 0)
                        return set_->isnot ? next : ++next;
                }
                else
                {
                    // skip first string
                    do { ++p; } while (*p);
                    ++p;
                }
                // skip second string
                do { ++p; } while (*p);
                ++p;
            }
        }
        //
        // try and match an equivalence class, NB only a single character can match
        if (set_->cequivalents)
        {
            charT a[2] = { col, charT(0), };
            s1 = traits_inst.transform_primary(a, a + 1);
            for (i = 0; i < set_->cequivalents; ++i)
            {
                if (STR_COMP(s1, p) == 0)
                    return set_->isnot ? next : ++next;
                // skip string
                do { ++p; } while (*p);
                ++p;
            }
        }
    }
    if (traits_inst.isctype(*next, set_->cclasses) == true)                                // CHANGE: col => *next
        return set_->isnot ? next : ++next;
    if ((set_->cnclasses != 0) && (traits_inst.isctype(*next, set_->cnclasses) == false))  // CHANGE: col => *next
        return set_->isnot ? next : ++next;
    return set_->isnot ? ++next : next;
}

template <>
char32_t* boost::BOOST_REGEX_DETAIL_NS::re_is_set_member(
    char32_t* next,
    char32_t* last,
    const boost::BOOST_REGEX_DETAIL_NS::re_set_long<utf32_regex_traits::char_class_type>* set_,
    const boost::BOOST_REGEX_DETAIL_NS::regex_data<char32_t, utf32_regex_traits>& e, bool icase) {

    typedef char32_t*                            iterator;     // ADDED: typedef template parameters
    typedef char32_t                             charT;        // ADDED: typedef template parameters
    typedef utf32_regex_traits                   traits_type;  // ADDED: typedef template parameters
    typedef utf32_regex_traits::char_class_type  char_classT;  // ADDED: typedef template parameters

    const charT* p = reinterpret_cast<const charT*>(set_ + 1);
    iterator ptr;
    unsigned int i;
    //bool icase = e.m_flags & regex_constants::icase;

    if (next == last) return next;

    typedef typename traits_type::string_type traits_string_type;
    const ::boost::regex_traits_wrapper<traits_type>& traits_inst = *(e.m_ptraits);

    // dwa 9/13/00 suppress incorrect MSVC warning - it claims this is never
    // referenced
    (void)traits_inst;

    // try and match a single character, could be a multi-character
    // collating element...
    for (i = 0; i < set_->csingles; ++i)
    {
        ptr = next;
        if (*p == static_cast<charT>(0))
        {
            // treat null string as special case:
            if (traits_inst.translate(*ptr, icase))
            {
                ++p;
                continue;
            }
            return set_->isnot ? next : (ptr == next) ? ++next : ptr;
        }
        else
        {
            while (*p && (ptr != last))
            {
                if (traits_inst.translate(*ptr, icase) != *p)
                    break;
                ++p;
                ++ptr;
            }

            if (*p == static_cast<charT>(0)) // if null we've matched
                return set_->isnot ? next : (ptr == next) ? ++next : ptr;

            p = re_skip_past_null(p);     // skip null
        }
    }

    charT col = traits_inst.translate(*next, icase);


    if (set_->cranges || set_->cequivalents)
    {
        traits_string_type s1;
        //
        // try and match a range, NB only a single character can match
        if (set_->cranges)
        {
            if ((e.m_flags & regex_constants::collate) == 0)
                s1.assign(1, col);
            else
            {
                charT a[2] = { col, charT(0), };
                s1 = traits_inst.transform(a, a + 1);
            }
            for (i = 0; i < set_->cranges; ++i)
            {
                if (STR_COMP(s1, p) >= 0)
                {
                    do { ++p; } while (*p);
                    ++p;
                    if (STR_COMP(s1, p) <= 0)
                        return set_->isnot ? next : ++next;
                }
                else
                {
                    // skip first string
                    do { ++p; } while (*p);
                    ++p;
                }
                // skip second string
                do { ++p; } while (*p);
                ++p;
            }
        }
        //
        // try and match an equivalence class, NB only a single character can match
        if (set_->cequivalents)
        {
            charT a[2] = { col, charT(0), };
            s1 = traits_inst.transform_primary(a, a + 1);
            for (i = 0; i < set_->cequivalents; ++i)
            {
                if (STR_COMP(s1, p) == 0)
                    return set_->isnot ? next : ++next;
                // skip string
                do { ++p; } while (*p);
                ++p;
            }
        }
    }
    if (traits_inst.isctype(*next, set_->cclasses) == true)                                // CHANGE: col => *next
        return set_->isnot ? next : ++next;
    if ((set_->cnclasses != 0) && (traits_inst.isctype(*next, set_->cnclasses) == false))  // CHANGE: col => *next
        return set_->isnot ? next : ++next;
    return set_->isnot ? ++next : next;
}


// This is the constructor for the regular expression creator.
// Most of it is copied verbatim from boost\regex\v5\basic_regex_creator.hpp lines 260 to 283;
// but the lower, upper and alpha masks, which are used only to modify the lower and upper masks
// when case insensitive matching is in effect, are left as zeros.
// (This is hacky, but simpler than overriding the places where these values are used.)

template <>
boost::BOOST_REGEX_DETAIL_NS::basic_regex_creator<char32_t, utf32_regex_traits>::basic_regex_creator
    (regex_data<char32_t, utf32_regex_traits>* data)
    : m_pdata(data), m_traits(*(data->m_ptraits)), m_last_state(0), m_icase(false), m_repeater_id(0),
    m_has_backrefs(false), m_bad_repeats(0), m_has_recursions(false), m_word_mask(0),
    m_mask_space(0), m_lower_mask(0), m_upper_mask(0), m_alpha_mask(0) {
    typedef char32_t           charT;                             // ADDED: typedef template parameters
    typedef utf32_regex_traits traits;                            // ADDED: typedef template parameters
    m_pdata->m_data.clear();
    m_pdata->m_status = ::boost::regex_constants::error_ok;
    static const charT w = 'w';
    static const charT s = 's';
    // static const charT l[5] = { 'l', 'o', 'w', 'e', 'r', };    // CHANGE: removed
    // static const charT u[5] = { 'u', 'p', 'p', 'e', 'r', };    // CHANGE: removed
    // static const charT a[5] = { 'a', 'l', 'p', 'h', 'a', };    // CHANGE: removed
    m_word_mask = m_traits.lookup_classname(&w, &w +1);
    m_mask_space = m_traits.lookup_classname(&s, &s +1);
    // m_lower_mask = m_traits.lookup_classname(l, l + 5);        // CHANGE: removed
    // m_upper_mask = m_traits.lookup_classname(u, u + 5);        // CHANGE: removed
    // m_alpha_mask = m_traits.lookup_classname(a, a + 5);        // CHANGE: removed
    m_pdata->m_word_mask = m_word_mask;
    BOOST_REGEX_ASSERT(m_word_mask != 0); 
    BOOST_REGEX_ASSERT(m_mask_space != 0); 
    // BOOST_REGEX_ASSERT(m_lower_mask != 0);                     // CHANGE: removed
    // BOOST_REGEX_ASSERT(m_upper_mask != 0);                     // CHANGE: removed
    // BOOST_REGEX_ASSERT(m_alpha_mask != 0);                     // CHANGE: removed
}
