@echo off
setlocal

cd /d "%~dp0"

where cl >nul 2>nul
if errorlevel 1 (
    echo Run this from an x64 Visual Studio developer command prompt.
    exit /b 1
)

set "OUT=Work\build"
mkdir "%OUT%" 2>nul
del /q "%OUT%\*.obj" "%OUT%\*.efi" "%OUT%\*.exe" 2>nul

set "EFI_CL=/nologo /c /O1 /Oi /GS- /GR- /EHs-c- /Zl /W4"
set "EFI_LD=/nologo /dll /nodefaultlib /machine:x64 /fixed:no /dynamicbase:no /nxcompat:no"

call cl %EFI_CL% /Fo:%OUT%\Smm.obj Smm.c || exit /b 1
call cl %EFI_CL% /Fo:%OUT%\Dxe.obj Dxe.c || exit /b 1

link %EFI_LD% /subsystem:EFI_BOOT_SERVICE_DRIVER /entry:SmmEntry /out:%OUT%\Smm.efi %OUT%\Smm.obj || exit /b 1
link %EFI_LD% /subsystem:EFI_BOOT_SERVICE_DRIVER /entry:DxeEntry /out:%OUT%\Dxe.efi %OUT%\Dxe.obj || exit /b 1

call cl /nologo /W4 /O2 /DUNICODE /D_UNICODE /Fo:%OUT%\Client.obj /Fe:%OUT%\Client.exe Client.c || exit /b 1
call cl /nologo /W4 /O2 /Fo:%OUT%\DbgRead.obj /Fe:%OUT%\DbgRead.exe DbgRead.c advapi32.lib || exit /b 1

echo Built:
echo   %OUT%\Smm.efi
echo   %OUT%\Dxe.efi
echo   %OUT%\Client.exe
echo   %OUT%\DbgRead.exe
