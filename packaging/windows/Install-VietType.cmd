@echo off
setlocal
set "VIETTYPE_SOURCE=%~dp0"
set "VIETTYPE_DEST=%LOCALAPPDATA%\Programs\VietType"
if not exist "%VIETTYPE_SOURCE%VietType.exe" (
    echo Extract the entire VietType package before starting installation.
    pause
    exit /b 1
)
tasklist /FI "IMAGENAME eq VietType.exe" /NH | find /I "VietType.exe" >nul
if not errorlevel 1 (
    echo Exit VietType from the system tray and run this installer again.
    pause
    exit /b 1
)
robocopy "%VIETTYPE_SOURCE%." "%VIETTYPE_DEST%" /E /R:1 /W:1 /XD "*.WebView2" /XF "Install-VietType.cmd" /NFL /NDL /NJH /NJS /NP
if errorlevel 8 goto :failed
powershell -NoProfile -Command "$ErrorActionPreference='Stop'; $menu=[Environment]::GetFolderPath('Programs'); $shell=New-Object -ComObject WScript.Shell; $link=$shell.CreateShortcut((Join-Path $menu 'VietType.lnk')); $link.TargetPath=Join-Path $env:VIETTYPE_DEST 'VietType.exe'; $link.WorkingDirectory=$env:VIETTYPE_DEST; $link.Save()"
if errorlevel 1 goto :failed
echo VietType is installed for this Windows user.
echo Open VietType from the Start menu.
pause
exit /b 0

:failed
echo VietType installation could not finish. Review the error above and retry.
pause
exit /b 1
