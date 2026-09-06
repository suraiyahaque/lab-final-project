@echo off
echo ===================================================
echo  Image Manipulation Software - Builder (Standard GCC)
echo ===================================================
echo.

:: Close any existing instance of the app before rebuilding
taskkill /F /IM app.exe >nul 2>&1

echo [1] Compiling with standard system 32-bit GCC (MinGW)...
gcc -g main.c bmp.c gui.c image.c operation.c compat.c -I third_party/iup/include -L third_party/iup -liupcontrols -liupcd -liup -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32 -o app.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    echo Please make sure your system's default GCC is 32-bit MinGW.
    echo.
    pause
    exit /b 1
)

echo [2] Build successful!
echo [3] Launching app.exe...
echo.
start "" app.exe
