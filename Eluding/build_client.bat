@echo off

chcp 65001 >nul

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

REM setup visual studio environment
call %VCVARS% x64
if %ERRORLEVEL% NEQ 0 (
    echo Failed to setup Visual Studio environment
    exit /b 1
)

mkdir build_client 2>nul
cd build_client

REM delete existing client.res if it exists
if exist client.res del client.res

REM create shared/src directory if it doesn't exist
if not exist "..\shared\src" mkdir "..\shared\src"

REM create a super simple resource file locally
echo /* Simple resource file */ > temp.rc
echo #define IDI_ICON1 101 >> temp.rc
echo IDI_ICON1 ICON "..\client\src\icon.ico" >> temp.rc
echo 1 ICON "..\client\src\icon.ico" >> temp.rc


echo Compiling resource file...
rc.exe /nologo /c 65001 /fo client.res temp.rc
if %ERRORLEVEL% NEQ 0 (
    echo Failed to compile resource file
    exit /b 1
)

SET COMMON_FLAGS=/EHsc /std:c++17 /O2 /GL /MD /DNDEBUG /DWIN32_LEAN_AND_MEAN /I%SFML_DIR%\include

echo Compiling network code...
cl.exe %COMMON_FLAGS% /c ..\shared\src\map.cpp
cl.exe %COMMON_FLAGS% /c ..\client\src\client_network.cpp

echo Building client in release mode with dynamic linking...
cl.exe %COMMON_FLAGS% ^
..\client\src\client.cpp ^
..\client\src\client_renderer.cpp ^
..\client\src\client_input.cpp ^
client_network.obj ^
map.obj ^
client.res ^
/Fe:Evades.exe ^
/link /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF /LIBPATH:%SFML_DIR%\lib ^
sfml-graphics.lib sfml-window.lib sfml-system.lib sfml-network.lib ^
ws2_32.lib shell32.lib user32.lib gdi32.lib winmm.lib opengl32.lib advapi32.lib

if %ERRORLEVEL% NEQ 0 (
    echo Failed to build client
    exit /b 1
)

echo Copying SFML DLLs...
copy %SFML_DIR%\bin\sfml-graphics-3.dll . 2>nul
copy %SFML_DIR%\bin\sfml-window-3.dll . 2>nul
copy %SFML_DIR%\bin\sfml-system-3.dll . 2>nul
copy %SFML_DIR%\bin\sfml-network-3.dll . 2>nul

echo Copying icon file...
copy ..\client\src\icon.ico . 2>nul

echo Creating maps directory...
mkdir maps 2>nul
copy ..\maps\*.json maps\ 2>nul

del temp.rc

cd ..

echo Dynamic client build successful!
echo.
echo To start the client: .\build_client\Evades.exe [server_ip] [server_port]
echo.
echo Example: .\build_client\Evades.exe 127.0.0.1 12345
