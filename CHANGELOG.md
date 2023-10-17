# Columns++ for Notepad++ -- Pre-releases

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