@echo off
setlocal

cd /d "%~dp0"

for /f "delims=" %%I in ('where cl.exe 2^>nul') do (
    set "CL_EXE=%%I"
    goto :found_cl
)
:found_cl
if "%CL_EXE%"=="" (
    where cl >nul 2>nul
)
if "%CL_EXE%"=="" (
    set "CL_EXE=cl"
)
if "%CL_EXE%"=="cl" (
    where cl >nul 2>nul
    if errorlevel 1 (
        echo Run this from an x64 Visual Studio developer command prompt.
        exit /b 1
    )
)
if not "%CL_EXE%"=="cl" if not exist "%CL_EXE%" (
    echo Run this from an x64 Visual Studio developer command prompt.
    exit /b 1
)

set "OUT=Work\build"
mkdir "%OUT%" 2>nul

set "EFI_CL=/nologo /c /O1 /Oi /GS- /GR- /EHs-c- /Zl /W4"
set "EFI_LD=/nologo /dll /nodefaultlib /machine:x64 /subsystem:EFI_BOOT_SERVICE_DRIVER /fixed:no /dynamicbase:no /nxcompat:no"

"%CL_EXE%" %EFI_CL% /Fo:%OUT%\DxeBridge.obj DxeBridge.c || exit /b 1
link %EFI_LD% /entry:DxeBridgeEntry /out:%OUT%\DxeBridge.efi %OUT%\DxeBridge.obj || exit /b 1

"%CL_EXE%" %EFI_CL% /Fo:%OUT%\SmmHost.obj SmmHost.c || exit /b 1
link %EFI_LD% /entry:SmmHostEntry /out:%OUT%\SmmHost.efi %OUT%\SmmHost.obj || exit /b 1

"%CL_EXE%" %EFI_CL% /Fo:%OUT%\Payload.obj Payload.c || exit /b 1
link %EFI_LD% /entry:PayloadEntry /out:%OUT%\Payload.efi %OUT%\Payload.obj || exit /b 1

"%CL_EXE%" /nologo /W4 /O2 /DUNICODE /D_UNICODE /Fo:%OUT%\SmmClient.obj /Fe:%OUT%\SmmClient.exe SmmClient.c Advapi32.lib || exit /b 1

echo Built:
echo   %OUT%\DxeBridge.efi
echo   %OUT%\SmmHost.efi
echo   %OUT%\Payload.efi
echo   %OUT%\SmmClient.exe
