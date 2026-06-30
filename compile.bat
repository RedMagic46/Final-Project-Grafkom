@echo off
title Compile Start Menu Clone
echo =========================================
echo  COMPILING WINDOWS START MENU CLONE...
echo =========================================
echo Source files: main.cpp, StartMenuApp.cpp, UIRenderer.cpp
echo Target: StartMenu.exe
echo.

g++ -O3 -mwindows src\main.cpp src\StartMenuApp.cpp src\UIRenderer.cpp -o StartMenu.exe -lgdi32 -lshell32 -luser32

if %errorlevel% neq 0 (
    echo.
    echo -----------------------------------------
    echo ERROR: Compilation failed!
    echo Please make sure MinGW is in your PATH.
    echo -----------------------------------------
    pause
    exit /b %errorlevel%
)

echo.
echo -----------------------------------------
echo SUCCESS: Compilation completed successfully!
echo Created: StartMenu.exe
echo -----------------------------------------
echo.
echo Double-click StartMenu.exe to run.
echo.
pause
