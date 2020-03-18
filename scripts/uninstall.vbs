' This VBScript should run before any files are actually removed

On Error Resume Next

Set sh = CreateObject("WScript.Shell")
' Set fso = CreateObject("Scripting.FileSystemObject")

Dim installer : Set installer = Nothing
Set installer = CreateObject("WindowsInstaller.Installer")

'key = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\SystemRoot"
'SysLoc = sh.RegRead(key) & "\system32"

' remove the showsel EventLog source entries
keybase = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\showsel"
key1 = keybase & "\EventMessageFile"
key2 = keybase & "\TypesSupported"
sh.RegDelete key1
sh.RegDelete key2
sh.RegDelete keybase

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

' Remove the InstallLoc from the Environment Path
key3 = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\Path"
EnvPath = sh.RegRead(key3) 
' wscript.echo "EnvPath= " & EnvPath

' Calculates strStartAt which cuts the strSearchFor value from the path
' strSearchFor = ";C:\program files\intel\ipmirastools"
strSearchFor = ";" & InstallLoc 
newPath = EnvPath 
iStartAt = "0"
strSearchForLen=len(strSearchFor)
strStartAt = inStr(lCase(newPath), lCase(strSearchFor))
If (strStartAt > "0") then
  iStartAt = (strStartAt-1)
' wscript.echo "strStartAt= " & strStartAt & " iStartAt= " & iStartAt

' Builds the new Path
' Joins the Values to the left and right of strSearchFor
  strTotalLen=len(newPath)
  newPathLeft=left(newPath,iStartAt)
  newPathRight=right(newPath, (strTotalLen-strSearchForLen-iStartAt))
  newPath=newPathLeft + newPathRight 
end if

' wscript.echo "newPath= " & newPath
sh.RegWrite key3, newPath, "REG_EXPAND_SZ" 


