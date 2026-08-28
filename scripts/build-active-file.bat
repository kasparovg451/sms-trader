@echo off
setlocal

set "VS_DEV_CMD=C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat"
set "SOURCE_FILE=%~1"
set "BUILD_DIR=%~dp0..\build"
set "JSON_INCLUDE_DIR=%BUILD_DIR%\cmake\_deps\nlohmann_json-src\include"

if not exist "%SOURCE_FILE%" (
    echo Source file not found: "%SOURCE_FILE%"
    exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

call "%VS_DEV_CMD%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

for %%F in ("%SOURCE_FILE%") do set "FILE_STEM=%%~nF"

cl.exe /nologo /std:c++20 /EHsc /W4 /Zi /I"%JSON_INCLUDE_DIR%" "%SOURCE_FILE%" /Fo:"%BUILD_DIR%\%FILE_STEM%.obj" /Fd:"%BUILD_DIR%\%FILE_STEM%.pdb" /Fe:"%BUILD_DIR%\%FILE_STEM%.exe"
exit /b %errorlevel%
