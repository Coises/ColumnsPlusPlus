$version = (Get-Command "vs.proj\bin\Win32\Release\ColumnsPlusPlus.dll").FileVersionInfo.ProductVersion
Compress-Archive -DestinationPath ColumnsPlusPlus-$version-x86.zip -Path "vs.proj\bin\Win32\Release\ColumnsPlusPlus.dll","CHANGELOG.md","help.htm","LICENSE.txt","source.txt"
$version = (Get-Command "vs.proj\bin\x64\Release\ColumnsPlusPlus.dll").FileVersionInfo.ProductVersion
Compress-Archive -DestinationPath ColumnsPlusPlus-$version-x64.zip -Path "vs.proj\bin\x64\Release\ColumnsPlusPlus.dll","CHANGELOG.md","help.htm","LICENSE.txt","source.txt"
Read-Host -Prompt "Press Enter to exit"
