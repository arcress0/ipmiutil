' This VBScript should run after all files have been copied onto the system.

Set sh = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")

Dim installer : Set installer = Nothing
Set installer = CreateObject("WindowsInstaller.Installer")
Dim sOutFile, outs

' Find out where our files were installed 
If(IsEmpty(Session)) Then
   'Not running from within installer.  Source path is current directory.
   InstallLoc = sh.CurrentDirectory
Else
   'Running inside the installer, use CustomActionData "[TARGETDIR]".
   ' InstallLoc = installer.ProductInfo(productCode, "InstallLocation")
   InstallLoc = Session.Property("CustomActionData")
   If(IsEmpty(InstallLoc)) Then
      InstallLoc = "C:\Program Files\Intel\IPMIrasTools"
   End If
End If
' wscript.echo "InstallLoc=" & InstallLoc

' Find System Folder (usually c:\windows\system32).
key = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\SystemRoot"
SysLoc = sh.RegRead(key) & "\system32"

sOutFile = SysLoc & "\autoexnt.bat"
Set outs = fso.OpenTextFile(sOutFile, 2, True, 0) 
outs.WriteLine "REM autoexnt.bat for sdiskmon " & vbCrlf 
outs.WriteLine "cd " & InstallLoc  & vbCrlf 
outs.WriteLine "sdiskmon -b "      & vbCrlf 
outs.Close

' Set up the showsel EventLog source
' showselmsg.dll is copied by the installer already
keybase = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\showsel\"
key1 = keybase & "EventMessageFile"
key2 = keybase & "TypesSupported"
sh.RegWrite key1, "%SystemRoot%\system32\showselmsg.dll", "REG_EXPAND_SZ"
sh.RegWrite key2, 7, "REG_DWORD"
'val1 = sh.RegRead(key1)
'val2 = sh.RegRead(key2)
'wscript.echo "showsel: msgfile " & val1 & " types " & val2

' Set up the sdiskmon service
' instexnt.exe, autoexnt.exe, and autoexnt.bat are copied to SysLoc already.
sh.Run "instexnt install", 0, True
sh.Run "net start autoexnt", 0, True

