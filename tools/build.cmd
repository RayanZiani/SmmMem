@echo off
setlocal

where cl >nul 2>nul
if errorlevel 1 (
    echo Run this from an x64 Visual Studio developer command prompt.
    exit /b 1
)

mkdir "%~dp0Work" 2>nul
del /q "%~dp0Work\WmiPingBench.obj" "%~dp0Work\WmiPingBench.exe" "%~dp0Work\WmiInventory.obj" "%~dp0Work\WmiInventory.exe" 2>nul
cl /nologo /W4 /O2 /DUNICODE /D_UNICODE /Fo:"%~dp0Work\WmiPingBench.obj" /Fe:"%~dp0Work\WmiPingBench.exe" "%~dp0WmiPingBench.c"
if errorlevel 1 exit /b 1
cl /nologo /W4 /O2 /EHsc /DUNICODE /D_UNICODE /Fo:"%~dp0Work\WmiInventory.obj" /Fe:"%~dp0Work\WmiInventory.exe" "%~dp0WmiInventory.cpp"
exit /b %errorlevel%
