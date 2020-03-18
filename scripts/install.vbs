' This VBScript should run after all files have been copied onto the system.

Set sh = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")

Dim installer : Set installer = Nothing
Set installer = CreateObject("WindowsInstaller.Installer")
Dim sOutFile, outs, sScrFile

' Find out where our files were installed 
If(IsEmpty(Session)) Then
   'Not running from within installer.  Source path is current directory.
   InstallLoc = sh.CurrentDirectory
Else
   'Running inside the installer, use CustomActionData "[TARGETDIR]".
   ' InstallLoc = installer.ProductInfo(productCode, "InstallLocation")
   InstallLoc = Session.Property("CustomActionData")
   If(IsEmpty(InstallLoc)) Then
      InstallLoc = "C:\Program Files\sourceforge\ipmiutil"
   End If
End If
' wscript.echo "InstallLoc=" & InstallLoc

' Find System Folder (usually c:\windows\system32).
key = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\SystemRoot"
SysLoc = sh.RegRead(key) & "\system32"

' Add InstallLoc to the Environment Path
key3 = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\Path"
EnvPath = sh.RegRead(key3) & ";" & InstallLoc 
sh.RegWrite key3, EnvPath, "REG_EXPAND_SZ" 

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

' Schedule the checksel.cmd to run
sScrFile = InstallLoc & "\checksel.cmd"
sh.Run "at 23.30 /every:m,t,w,th,f,s,su """ & sScrFile & """ ", 0, True

