@set version=%1
@if .%version%==. set /p version="Version?"
@if .%version%==. goto :skipit
copy /b ..\vs.proj\bin\Win32\Release\ColumnsPlusPlus.dll ColumnsPlusPlus32.dll
copy /b ..\vs.proj\bin\x64\Release\ColumnsPlusPlus.dll   ColumnsPlusPlus64.dll
7zr a ColumnsPlusPlus.7z ..\CHANGELOG.md ..\help.htm ..\LICENSE.txt ..\source.txt ColumnsPlusPlus32.dll ColumnsPlusPlus64.dll setup.bat
del ColumnsPlusPlus32.dll
del ColumnsPlusPlus64.dll
copy /b 7zS2.sfx + ColumnsPlusPlus.7z ..\ColumnsPlusPlus-%version%.exe
del ColumnsPlusPlus.7z
:skipit
@pause