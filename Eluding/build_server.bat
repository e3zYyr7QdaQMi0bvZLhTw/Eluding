@echo off

SET SFML_DIR=C:\SFML
SET VS_DIR="C:\Program Files\Microsoft Visual Studio\2022\Community"
SET VCVARS=%VS_DIR%\VC\Auxiliary\Build\vcvarsall.bat

if exist %VCVARS% (
    echo Found Visual Studio 2022
) else (
    echo Visual Studio 2022 not found at %VS_DIR%
    echo Please update the VS_DIR variable in this script
    exit /b 1
)

if exist %SFML_DIR% (
    echo Found SFML at %SFML_DIR%
) else (
    echo SFML not found at %SFML_DIR%
    echo Please download SFML from https://www.sfml-dev.org/download.php
    echo and extract it to %SFML_DIR% or update the SFML_DIR variable in this script
    exit /b 1
)

call %VCVARS% x64
if %ERRORLEVEL% NEQ 0 (
    echo Failed to setup Visual Studio environment
    exit /b 1
)

mkdir build_server 2>nul
cd build_server

echo Building server...
cl.exe /EHsc /std:c++17 /O2 /Fe:server_game.exe /I%SFML_DIR%\include /I..\server\include /I..\server\include\Entities /I..\shared\include ..\server\src\*.cpp ..\server\src\Entities\*.cpp ..\shared\src\*.cpp
if %ERRORLEVEL% NEQ 0 (
    echo Failed to build server
    exit /b 1
)

echo Creating maps directory...
mkdir maps 2>nul
copy ..\maps\*.json maps\ 2>nul

cd ..

echo Server build successful!
echo.
echo To start the server: .\build_server\server_game.exe [port]
echo.
echo Example: .\build_server\server_game.exe 12345 