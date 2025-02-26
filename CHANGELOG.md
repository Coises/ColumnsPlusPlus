# Columns++ for Notepad++ -- Releases

## Version 1.2 -- February 26th, 2025

This version is focused on improving the behavior of regular expressions for Unicode documents:

* Matching is now based on Unicode code points rather than UTF-16 code units. Each Unicode code point is a single regular expression “character” — surrogate pairs are not used.

* The hexadecimal representation for code points beyond the basic multiligual plane can be entered directly (e.g., `\x{1F642}` for 🙂) in both find and replace fields.

* The [character classes documented for Unicode](https://www.boost.org/doc/libs/release/libs/regex/doc/html/boost_regex/syntax/character_classes/optional_char_class_names.html) work, with the exception of Cs/Surrogate. (Unpaired surrogates cannot yield valid UTF-8; Scintilla displays attepts to encode them — aka [WTF-8](https://en.wikipedia.org/wiki/UTF-8#WTF-8) — as three invalid bytes, and this regular expression implementation treats them the same way.)

* These escapes are added:
    +  `\i` - matches invalid UTF-8 bytes. (You can also use `[[:invalid:]]`.)
    +  `\o` - matches ASCII characters (code points 0-127).
    +  `\y` - matches defined characters (all except unassigned, invalid and private use — you can also use `[[:defined:]]`).
    +  `\I`, `\O` and `\Y` match the complements of those classes.

* `\X` reliably matches a “grapheme cluster” (what normal people call a character) regardless of how many Unicode code points (what the regular expression engine sees as a “character”) comprise it.

* The Unicode character classes, named character classes and the `\l` and `\u` escapes are always case-sensitive, even when **Match case** is not checked or the `(?i)` modifier is used.

* All the control character and non-printing character abbreviations that are shown (depending on **View** | **Show Symbol** settings) in reverse colors can be used as symbolic character names: e.g., `[[.NBSP.]]` will find non-breaking spaces.

In the **Search in indicated region** dialog, for all documents and search modes:

* Columns++ shows a progress dialog if the estimated time for a multiple search action (Count, Select, Replace All/Before/After) exceeds about two seconds.

* When nothing is selected, no search region is set, and a stepwise find or replace is initiated with **Auto set** checked — causing the search region to be set to the entire document — the search now starts from the caret position instead of from the beginning or end of the document.

## Version 1.1.5 -- February 1st, 2025

* Due to a change in Notepad++ 8.7.6, responsiveness is degraded when deleting character by character (e.g., using the backspace or delete key) in large files with elastic tabstops enabled. Notepad++ version 8.7.7 provides a way to mitigate this, which Columns++ 1.1.5 employs. (Send NPPM_ADDSCNMODIFIEDFLAGS when elastic tabstops is first enabled.)

## Version 1.1.4 -- January 22nd, 2025

* Changed elastic tabstops processing to work correctly with Notepad++ version 8.7.6.

* Changed About dialog to show build date and time rather than file date and time.

## Version 1.1.3 -- October 13th, 2024

* Corrected an error which caused the column position to be lost when navigating with cursor up or down keys over short lines with Elastic tabstops enabled. Addresses issue #25.

* Updated Notepad++ include files to version 8.7, Scintilla include files to version 5.5.2, Boost.Regex to 1.86 and nlohmann/json to 3.11.3.

* Updated the help to clarify how numbers are processed internally for calculations and what limits this places on accuracy of calculations.

## Version 1.1.2 -- May 27th, 2024

* Fixed an error that sometimes caused elastic tabstops progress dialogs to be raised when not needed, and to run much more slowly than necessary.

## Version 1.1.1 -- May 23rd, 2024

* Fixed an error that caused elastic tabstop processing to be applied after using **Replace All** or **Replace All in All Opened Documents** in the Notepad++ **Search** dialog when elastic tabstops is not enabled. Expected to resolve issue #24. (Note: This error has been present since Columns++ version 1.0.4 when used with Notepad++ versions 8.6.5 or greater.)

* Fixed some errors that could cause elastic tabstops layout to fail to update properly in the inactive view when the same document is visible in both views.

## Version 1.1 -- May 21st, 2024

### Fixes and improvements:

* When DirectWrite is enabled in Notepad++ settings, "Best estimate" for monospaced font optimization will always be "no"; monospaced font optimizations don't appear to have any advantage with DirectWrite, and they can cause problems.

* Improvements to the way tab settings are tracked can sometimes significantly speed up, or completely avoid, the "Setting tabstops" message box that interrupts editing when working with large rectangular selections in large files.

* Fixed an error that caused settings for a document to be lost when it was moved from one view to the other or closed in one view while open in the other.

* Fixed an omission that sometimes caused pasting a rectangular selection or a multiple selection to paste into the wrong columns in lines that were not visible on the screen. *This fix only works on Notepad++ versions 8.6 and higher. The work-around for pre-8.6 versions is to make a zero-width rectangular selection deep enough to cover all the lines into which you will paste before pasting.*

### New features:

* Added the [Selection](https://coises.github.io/ColumnsPlusPlus/help.htm#selectionmenu) submenu, which contains options to extend a rectangular section to the edges of the document (up, left, right or down), to enclose a selection (which can be a single or a multiple selection) in a rectangular selection, and to extend a selection either left to right or top to bottom. Default keyboard shortcuts are established, which can be changed or removed in the Shortcut Mapper in Notepad++.

* Added the [Timestamps](https://coises.github.io/ColumnsPlusPlus/help.htm#timestamps) command and dialog. *This command may not be available on Windows versions older than Windows 10 version 1903/19H1. Columns++ will attempt to detect when it will not work and remove it from the menu, but it is possible that it will appear but be non-functional on some older systems.*

### Notes:

* Some new features and fixes are probably incompatible with the [BetterMultiSelection](https://github.com/dail8859/BetterMultiSelection) plugin. I believe that most if not all of that plugin's features are now incorporated natively in current versions of Notepad++; if there are specific problems you cannot solve without that plugin and you observe conflicts with Columns++, please open an issue in the GitHub repository for Columns++ and I will see if there is anything I can do about it.


## Version 1.0.6 -- March 26th, 2024

* Avoid display errors that could occur when editing a wrapped line containing elastic tabstops. Under certain circumstances, an empty magenta line might appear following the edited line, or text at the end of the line might not be wrapped properly.

## Version 1.0.5 -- February 29th, 2024

* Correction to support for the new notification planned in Notepad++ 8.6.5.
* Fix inaccurate Convert tabs to spaces processing when elastic tabstops are used with DirectWrite enabled.

## Version 1.0.4 -- February 23rd, 2024

* Support planned changes to notifications in Notepad++ versions greater than 8.6.4. (Columns++ has problems with Notepad++ versions 8.6.3 and 8.6.4; it is recommended to skip these versions if you use Columns++.)
* Improved how Columns++ manages elastic tabstops layout when DirectWrite is enabled in Notepad++.

## Version 1.0.3 -- February 17th, 2024

* Fixed an omission that can cause odd cursor positioning when pressing arrow up or down immediately after inserting a tab when elastic tabstops is enabled.

## Version 1.0.2 -- January 4th, 2024

* **All users of versions 0.8 through 1.0.1 should update to this version as soon as possible.** This version fixes a serious bug in Search | Replace All/Before/After which can cause the entire application to hang (thus necessitating a force close of Notepad++ and loss of unsaved changes in all open tabs).

## Version 1.0.1 -- December 14th, 2023

* Added Width as an option for custom sorts; addresses Issue #15 feature request.

## Version 1.0 -- November 17th, 2023

* This will be the first version submitted for inclusion in Plugins Admin.

* Added ability to check GitHub for new releases of Columns++ and to show an indication on the Columns++ menu when an update is available. It is not possible to update Plugins Admin until a new version of Notepad++ is released, but this will make it easier to update manually, if desired, before the next release of Notepad++.

## Version 0.8-alpha -- October 28th, 2023

* References in formulas and sort keys to regular expression capture groups beyond 9 work now.

* When matching regular expressions in column selections or search regions, lookbehind assertions do not recognize any text beyond the boundary of the selection within the row or the segment of the search region in which a match is attempted. (In previous versions of Columns++, as in Notepad++, the text potentially examined by lookbehind assertions always extended to the beginning of the document.)

* Changed the way Search in indicated region behaves when no region is indicated, nothing is selected and a search is initiated; if Auto set is checked (the default), the search region is set to the entire document. The old behavior (raising a dialog requesting a rectangular selection) may be obtained by unchecking Auto set.

* Made some refinements in how find and replace strings in Extended search mode are processed, including fixes for potential bugs and adding \U*xxxxxx* (note: capital U) accepting up to six hexadecimal digits to specify any valid Unicode code point.

* Added dropdown arrows to the Count and Replace All buttons in Search in indicated region to support additional operations: Select All, Count Before, Count After, Select Before, Select After, Replace Before, Replace After and Clear History.

* Regular expressions using \K will work for incremental find and replace in Search in indicated region providing focus does not leave the dialog between finding a match and replacing it.

* Added an Align... command to the Columns++ menu to support aligning column text on any character, character string or regular expression.

## Version 0.7.5-alpha  -- October 17th, 2023

* When the user attempts to initiate a search with a multiple selection in which all selections are empty, show a message that a search region could not be constructed from the selection. Previous behavior was to become unresponsive, requiring the user to force-close Notepad++. Addresses issue #13.

## Version 0.7.4-alpha  -- October 10th, 2023

* The **Align numeric** command and the **Numeric aligned** option of the **Calculate...** command now take into account numbers formatted as times, based on the settings in the **Time formats** dialog.

## Version 0.7.3-alpha -- October 4th, 2023

* Improved exception handling: Exceptions from the Scintilla C++ interface will now be passed to Notepad++ in a way that allows it to report them as such, and uncleared error status codes from outside Columns++ will no longer cause exceptions in Columns++.

* An error in handling parentheses in regular expression replacement strings was fixed.

* A design flaw that could cause formula substitutions in regular expression replacement strings to be misinterpreted as part of a special sequence (such as a $*n* capture group reference) was fixed.

## Version 0.7.2-alpha -- September 20th, 2023

* Replaced a Windows API call that caused Version 0.7.1-alpha to fail to load on versions of Windows prior to Windows 10 with one that works on Vista and later versions.

## Verions 0.7.1-alpha -- September 18th, 2023

* Added a progress dialog for long-running Elastic tabstops operations. The operation can be cancelled using a link in the dialog or the Escape key. A control to set the minimum estimated time remaining to trigger the progress dialog was added to the Options dialog.

**Known problems:**

* Numeric alignment does not work consistently with numbers formatted as times.

## Version 0.7-alpha -- September 3rd, 2023

* Added the ability to use formulas in regular expression replacements in the **Search** dialog.

* Made setting and modifying the search region in the **Search** dialog more flexible.

* Made the use of time formats for numbers more controllable and predictable, and added the **Time formats** dialog.

* Made all dialogs that use formulas and/or regular expressions validate them and show balloon tips for errors.

* Improved handling of zero-length regular expression matches.

* Made the caret assertion in regular expressions match the beginning of each contiguous segment in the search region (e.g., the left edge of the selection in each row of a rectangular selection) as well as its standard match to the beginning of lines in multi-line segments.

* Updated help to reflect these changes.

* Fixed an error which caused sluggish scrolling when a rectangular selection covered a large number of lines.

**Known problems:**

* If a tab has Elastic tabstops enabled and has a very large number of lines in a rectangular selection, switching from another tab to that tab is slow: on the author's system, a selection spanning 20,000 lines with 65 tabs per line causes a delay of fifteen to twenty seconds, during which Notepad++ is unresponsive.

* **Serious:** The delay described above is magnified many times, possibly to the point where force-closing Notepad++ is the only reasonable option, if the tab from which one is switching is showing a Markdown Panel (from the MarkdownPanel plugin). At this time, I do not know the reason for this, nor whether other plugins that show side panels could cause a similar effect. (The built-in Document Map does not cause it.) ***Don't leave work unsaved when you are using Elastic tabstops with large files (over a few thousand lines).***

* Numeric alignment does not work consistently with numbers formatted as times.


## Version 0.6.1-alpha -- August 16th, 2023

* Make use of NPPM_ALLOCATEINDICATOR introduced in Notepad++ 8.5.6 while remaining compatible with older versions.

* Fixed a few minor cosmetic errors.

## Version 0.6-alpha -- August 14th, 2023

* Added **Sort...** command and dialog supporting custom sorts, including selection of locale for locale sorts, sorting within the column selection only (leaving data surrounding the selection in place), and sort keys derived from a regular expression match.

* Made **Convert Tabs to Spaces** significantly faster for large files.

## Version 0.5.1-alpha -- May 27th, 2023

* Corrected an error that caused numeric alignment to be ignored when skip unmatched lines was checked in the dialog for the **Calculate** command.
* Fixed a minor error in the help file.
* Fixed incorrect GPL version (2 instead of 3) in the version info block.

## Version 0.5-alpha -- May 19th, 2023

* Added the **Calculate** command.
* Updated help.htm, README.md and source.txt.

## Version 0.4.1-alpha -- May 10th, 2023

* Fixed a regression in performance of Undo/Redo with **Elastic tabstops** enabled.

## Version 0.4-alpha -- May 8th, 2023

* Improved performance with **Elastic tabstops** enabled, especially when the fonts in use are monospaced.

* Added settings to the Elastic tabstops profile dialog to control application of monospaced fonts optimization and whether to use a single exclamation point, instead of the standard multi-character mnemonic, to represent control, non-printing and invalid characters when in monospaced mode.

* Fixed an error in saving the settings of user-created elastic tabstops profiles and an error which caused the default profile to be randomly changed.

## Version 0.3-alpha -- April 30th, 2023

* Implemented **Convert separated values to tabs...** and **Convert tabs to separated values...** commands and added appropriate documentation to help.htm.

## Version 0.2.2-alpha -- April 25th, 2023

* Attempt to fix failure to apply elastic tabstops when first opening a file on some systems (issue #9).

## Version 0.2.1.7-alpha -- April 18th, 2023

* Changed search to place caret after (or before, if searching backward) the replacement text instead of selecting it. Addresses issue #7.

* Added option **Replace: Don't move to the following occurrence** which has the same effect for Columns++ search as the option with the same name in Preferences|Search in Notepad++ for built-in search. Default is unchecked (like the Notepad++ option), which is a change in behavior for Columns++ search. Addresses issue #8.

## Version 0.2.0.6-alpha -- April 17th, 2023

* Changed to search in "indicated region" instead of "rectangular selection." This is more flexible, makes it easier to see what is happening, and solves the problem of replacements causing which text is included in the selection to change (Issue #6) -- but it is a little harder to explain how it works to end users. It is possible to use the existing Notepad++ Styles (1st-5th or Find Mark Style), or to use a custom indicator just for Columns++. The indicator can be chosen on the Search dialog; settings for the custom indicator number and color are in Options.

* A known loose end is that backward regular expression searches are not disabled, but they also are not done "zig-zag" (backward by lines but forward in each line) as before, because that concept is not so straightforward when using indicators instead of rectangular selections. Whether to disable them, allow them (with whatever unexpected results it is that can happen), or re-implement the "zig-zag" approach remains to be decided.

* There is no way that I know of to ensure that the indicator number used for custom selections is not also used by some other plugin. (DSpellCheck, for example, just hard-codes number 19.) For that reason, I made it configurable; but there's no easy way to explain to the end user what's going on, since we don't really know what's going on. For now, I left it default enabled using indicator number 18; if experience shows this conflicts with some other plugin, it can either be changed to another number or initially disabled.

## Version 0.1.0.5-alpha - April 14th, 2023

* Added new prompts when a command needs a rectangular selection and the current selection is not rectangular or is zero-width.
* Changed default behavior not to make any implicit selections, so the prompts will be seen; settings from older versions are ignored and reset to the new defaults. You can still enable the same implicit selections by going to the Options dialog; they're just not the defaults anymore.

## Version 0.0.3.4-alpha - April 11th, 2023

* Correct dialog text "Copy these results to the clipboard" truncated on some systems.

## Version 0.0.2.3-alpha - April 11th, 2023

* Change __Add numbers__ and __Average numbers__ to open a dialog allowing user to decide whether or not to copy results to the clipboard and whether or not to paste results into an empty line at the end of the selection. Also adds ability to add an empty line into which to paste results if none exists, and to choose a thousands separator.

## Version 0.0.1.2-alpha - April 9th, 2023

* Attempt to fix old text in search status bar not always cleared (issue #1).
* Reworked About dialog a bit to add x86/x64 and ensure date/time text won't overflow.

## Version 0.0.0.1-alpha - April 8th, 2023

* First release.