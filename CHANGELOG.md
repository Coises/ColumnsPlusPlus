# Columns++ for Notepad++ -- Pre-releases

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