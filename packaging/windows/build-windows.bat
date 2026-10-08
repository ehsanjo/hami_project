@echo off
REM Build and package PriceTracker on Windows.
REM Run from the project root in a "Qt 6.x (MinGW)" or "x64 Native Tools" prompt.
REM
REM Before running, set these two (edit here or set them in the shell):
REM   QT_DIR  = your Qt kit folder, e.g. C:\Qt\6.8.0\mingw_64   (or ...\msvc2022_64)
REM   ISCC    = path to Inno Setup compiler
REM
REM Output: dist\windows\PriceTracker-<version>-setup.exe

setlocal
if "%QT_DIR%"=="" set QT_DIR=C:\Qt\6.8.0\mingw_64
if "%ISCC%"=="" set ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe

set ROOT=%~dp0..\..
set BUILD=%ROOT%\build-release
set APP=%ROOT%\dist\windows\app

echo == 1/5 Configure
cmake -S "%ROOT%" -B "%BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%" || goto :fail

echo == 2/5 Build
cmake --build "%BUILD%" || goto :fail

echo == 3/5 Tests
ctest --test-dir "%BUILD%" --output-on-failure || goto :fail

echo == 4/5 Collect app + Qt DLLs
if exist "%APP%" rmdir /s /q "%APP%"
cmake --install "%BUILD%" --prefix "%APP%" || goto :fail
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations "%APP%\PriceTracker.exe" || goto :fail

echo == 5/5 Build installer
"%ISCC%" "%~dp0PriceTracker.iss" || goto :fail

echo.
echo Done. Installer is in dist\windows
exit /b 0

:fail
echo.
echo FAILED. See the messages above.
exit /b 1
