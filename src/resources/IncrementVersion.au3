FileChangeDir(@ScriptDir)

; Autoupdate enabled?
$auto = IniRead("AutoIt_VersionInfo.ini", "VERSION", "AutoIncrement", "ERROR")
if $auto = "ERROR" Then Exit

; Get version
$text = IniRead("AutoIt_VersionInfo.ini", "VERSION", "VersionNumber", "ERROR")
if $text = "ERROR" Then Exit

$version = StringSplit($text, ".")

; Delete existing version info
FileDelete("AutoIt_VersionInfo.h")

; Increment version info and write back to the ini file if required
If $auto = "1" Then
	$version[4] = $version[4] + 1
	IniWrite("AutoIt_VersionInfo.ini", "VERSION", "VersionNumber", $version[1] & "." & $version[2] & "." & $version[3] & "." & $version[4])
EndIf

$numericversion = $version[1] & "," & $version[2] & "," & $version[3] & "," & $version[4]
$stringversion = """" & $version[1] & ", " & $version[2] & ", " & $version[3] & ", " & $version[4] & "\0"""

$f = "AutoIt_VersionInfo.h"
FileWriteLine($f, "#define MYFILEVER        " & $numericversion)
FileWriteLine($f, "#define MYPRODUCTVER     " & $numericversion)
FileWriteLine($f, "#define MYSTRFILEVER     " & $stringversion)
FileWriteLine($f, "#define MYSTRPRODUCTVER  " & $stringversion)
