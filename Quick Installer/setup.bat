@echo off
if not exist "%PROGRAMFILES(x86)%\Notepad++\plugins" goto :done32
if exist "%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus" (set inup=Update) else (set inup=Install)
choice /m "%inup% Columns++ extension for 32-bit Notepad++?"
if errorlevel 2 goto :done32
echo Copying files to "%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus"
if %inup%==Install mkdir "%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus"
copy "%~dp0\ColumnsPlusPlus32.dll"	"%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus\ColumnsPlusPlus.dll"
copy "%~dp0\CHANGELOG.md"	"%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus\"
copy "%~dp0\help.htm"	"%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus\"
copy "%~dp0\LICENSE.txt"	"%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus\"
copy "%~dp0\source.txt"	"%PROGRAMFILES(x86)%\Notepad++\plugins\ColumnsPlusPlus\"

:done32

if not exist "%PROGRAMW6432%\Notepad++\plugins" goto :done64
if exist "%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus" (set inup=Update) else (set inup=Install)
choice /m "%inup% Columns++ extension for 64-bit Notepad++?"
if errorlevel 2 goto :done64
echo Copying files to "%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus"
if %inup%==Install mkdir "%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus"
copy "%~dp0\ColumnsPlusPlus64.dll"	"%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus\ColumnsPlusPlus.dll"
copy "%~dp0\CHANGELOG.md"	"%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus\"
copy "%~dp0\help.htm"	"%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus\"
copy "%~dp0\LICENSE.txt"	"%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus\"
copy "%~dp0\source.txt"	"%PROGRAMW6432%\Notepad++\plugins\ColumnsPlusPlus\"

:done64

@pause