$x86Version = (Get-Command "vs.proj\bin\Win32\Release\ColumnsPlusPlus.dll").FileVersionInfo.ProductVersion
Compress-Archive -DestinationPath "ColumnsPlusPlus-$x86Version-x86.zip" -Path "vs.proj\bin\Win32\Release\ColumnsPlusPlus.dll","CHANGELOG.md","help.htm","LICENSE.txt","source.txt"
$x64Version = (Get-Command "vs.proj\bin\x64\Release\ColumnsPlusPlus.dll").FileVersionInfo.ProductVersion
Compress-Archive -DestinationPath "ColumnsPlusPlus-$x64Version-x64.zip" -Path "vs.proj\bin\x64\Release\ColumnsPlusPlus.dll","CHANGELOG.md","help.htm","LICENSE.txt","source.txt"
$x86Hash = (Get-FileHash "ColumnsPlusPlus-$x86Version-x86.zip" -Algorithm SHA256).Hash
$x64Hash = (Get-FileHash "ColumnsPlusPlus-$x64Version-x64.zip"   -Algorithm SHA256).Hash
$Hashes =  "SHA-256 Hash (ColumnsPlusPlus-$x64Version-x64.zip): " + $x64Hash + "`nSHA-256 Hash (ColumnsPlusPlus-$x86Version-x86.zip): " + $x86Hash
Write-Output $Hashes
Write-Output ""
Set-Clipboard -Value $Hashes
Read-Host -Prompt "Press Enter to exit"
