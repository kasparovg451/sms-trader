@echo off
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "PROJECT_DIR=%~dp0.."
set "BUILD_DIR=%PROJECT_DIR%\build"

if not exist "%VSWHERE%" (
    echo Visual Studio Build Tools with the C++ workload were not found.
    exit /b 1
)

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VS_INSTALL=%%I"

if not defined VS_INSTALL if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VS_INSTALL=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"

if not defined VS_INSTALL (
    echo Visual Studio Build Tools with the C++ workload were not found.
    exit /b 1
)

call "%VS_INSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

cmake --preset default -S "%PROJECT_DIR%"
if errorlevel 1 exit /b 1

cmake --build --preset default
exit /b %errorlevel%
