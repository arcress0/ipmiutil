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

' remove the sdiskmon service
sh.Run "net stop autoexnt", 0, True
sh.Run "instexnt remove", 0, True
' do not need to explicitly remove the autoexnt.bat file.


