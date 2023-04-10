# Columns++

Columns++ is a plugin for [Notepad++](https://github.com/notepad-plus-plus/notepad-plus-plus) offering various features for working with text and data arranged in columns.

Like Notepad++, Columns++ is released under the GNU General Public License (either version 2 of the License, or, at your option, any later version).

## Purpose

Columns++ is designed to provide some helpful functions for editing text or data that is lined up visually in columns, so that you can make a rectangular selection of the column(s) you want to process.

The integrated implementation of __Elastic tabstops__ works to line up columns when tabs are used as logical separators, including tab-separated values data files as well as any ordinary text or code document containing sections in which you want to line up columns easily using tabs. You can use this feature on its own or with the other functions in Columns++.

## Main Features

* __Elastic tabstops:__ Columns++ includes a new implementation of Nick Gravgaard's [Elastic tabstops](https://nickgravgaard.com/elastic-tabstops/). _(Please note that as of this writing I have not communicated with Mr. Gravgaard about my implementation of his proposal, and no endorsement on his part is implied.)_
* __Find and replace in rectangular selections__
* __Calculations:__ There are commands to add or average numbers in one or more columns.
* __Alignment:__ You can left- or right-align text, or line up numbers.
* __Sorting:__ Columns++ includes  sort commands that work correctly with rectangular selections in files that use tabs.

There is a [help file](https://coises.github.io/ColumnsPlusPlus/help.htm).

## Limitations

Columns++ is not optimized for files that use traditional, fixed tabs for alignment. The  commands offered can still be useful for these files, but it is recommended to select only one column at at a time.

Columns++ is generally not helpful when columns do not line up visually, such as in comma-separated values files.

Elastic tabstops can cause loading and editing to be slow for large files. By default, elastic tabstops is automatically turned off for files over 1000 KB or 5000 lines. You can change these limits.

## Status

Columns++ is currently pre-release, meaning it hasn't been broadly tested, and anything can happen. For me, it's already useful; but it will remain "use at your own risk" until I have enough feedback from others to warrant more confidence.

## Installation

For now, while Columns++ is in pre-release status, it must be installed manually:

Download an x86 or x64 zip file from the Releases section, depending on whether you're using 32-bit or 64-bit Notepad++. Unzip the file to a folder named __ColumnsPlusPlus__ (the name must be exactly this, or Notepad++ will not load the plugin) and copy that folder into the plugins directory where Notepad++ is installed (usually __C:\Program Files (x86)\Notepad++\plugins__ for 32-bit versions or __C:\Program Files\Notepad++\plugins__ for 64-bit versions).
