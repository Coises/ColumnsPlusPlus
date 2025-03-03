# Columns++

Columns++ is a plugin for [Notepad++](https://github.com/notepad-plus-plus/notepad-plus-plus) which offers features for working with text and data arranged in columns, including an implementation of elastic tabstops, enhanced searching and sorting, column alignment and numeric calulations.

Like Notepad++, Columns++ is released under the GNU General Public License (either version 3 of the License, or, at your option, any later version). Some original source code files which are not dependent on Notepad++ are released under the [MIT (Expat) license](https://www.opensource.org/licenses/MIT): see individual files for details.

Columns++ uses the [C++ Mathematical Expression Toolkit Library
(ExprTk)](https://github.com/ArashPartow/exprtk) by Arash Partow (https://www.partow.net/), which is released under the [MIT License](https://www.opensource.org/licenses/MIT).

Columns++ uses [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann (https://nlohmann.me), which is released under the [MIT License](https://www.opensource.org/licenses/MIT).

Columns++ uses the [Boost.Regex library](https://github.com/boostorg/regex/), which is released under the [Boost Software License, Version 1.0](https://www.boost.org/LICENSE_1_0.txt).

Columns++ uses some files from the [Unicode Character Database](https://www.unicode.org/ucd/), which is released under the [Unicode License](https://www.unicode.org/license.txt).

## Purpose

Columns++ is designed to provide some helpful functions for editing text or data that is lined up visually in columns, so that you can make a rectangular selection of the column(s) you want to process.

The integrated implementation of __Elastic tabstops__ works to line up columns when tabs are used as logical separators, including tab-separated values data files as well as any ordinary text or code document containing sections in which you want to line up columns easily using tabs. You can use this feature on its own or with the other functions in Columns++.

## Main Features

* __Elastic tabstops:__ Columns++ includes a new implementation of Nick Gravgaard's [Elastic tabstops](https://nickgravgaard.com/elastic-tabstops/). _(Please note that as of this writing I have not communicated with Mr. Gravgaard about my implementation of his proposal, and no endorsement on his part is implied.)_
* __Find and replace:__ Columns++ supports find and replace within regions, which can be defined by rectangular selections or multiple selections. Numeric formulas are supported in regular expression replacement strings. Regular expressions in Unicode documents support code points beyond the basic multiligual plane, the Unicode General Category property, recognition of grapheme clusters and finding invalid UTF-8 sequences.
* __Calculations:__ There are commands to add or average numbers in one or more columns and to insert the results of a calculation into each line of a rectangular selection.
* __Alignment:__ You can left- or right-align text, or line up numbers.
* __Sorting:__ Columns++ includes sort commands that work correctly with rectangular selections in files that use tabs. Sorts can be based on multiple columns or on regular expression capture groups.
* __Conversion:__ Commands to convert tabs to spaces and to convert between comma (or other delimiter) separated values and tabbed presentation are included.

There is a [help file](https://coises.github.io/ColumnsPlusPlus/help.htm).

## Limitations

Columns++ is optimized for use with Elastic tabstops. It also works with files that use traditional, fixed tabs for alignment, or no tabs at all; however, you should ordinarily select only one column at a time in files that don't use Elastic tabstops.

Columns++ is generally not helpful when columns do not line up visually, such as in comma-separated values files. However, Columns++ can convert between delimiter-separated values and tabbed presentation; and there are some features, particularly search using numeric formulas in regular expression replacement strings and sorting with custom criteria, which may be useful in documents that are not column-oriented.

Elastic tabstops can cause loading and editing to be slow for large files. Performance can be significantly improved by avoiding proportionally-spaced fonts. By default, Elastic tabstops is automatically turned off for files over 1000 KB or 5000 lines. You can change these limits.

## Installation

Columns++ is available through the Notepad++ Plugins Admin interface, though the version found there will not always be the newest.

You can use the Quick Installer for the [latest stable release](https://github.com/Coises/ColumnsPlusPlus/releases/latest/) or the [newest (possibly less stable) release](https://github.com/Coises/ColumnsPlusPlus/releases) if you have Notepad++ (either 32-bit, 64-bit or both) installed in the default location(s).

Otherwise, download an x86 or x64 zip file from the Releases section, depending on whether you're using 32-bit or 64-bit Notepad++. Unzip the file to a folder named __ColumnsPlusPlus__ (the name must be exactly this, or Notepad++ will not load the plugin) and copy that folder into the plugins directory where Notepad++ is installed (usually __C:\Program Files (x86)\Notepad++\plugins__ for 32-bit versions or __C:\Program Files\Notepad++\plugins__ for 64-bit versions).
