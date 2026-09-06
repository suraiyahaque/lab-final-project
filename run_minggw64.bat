@echo off
echo ========================================
echo  Image Manipulation Software - Builder
echo ========================================
echo.

taskkill /F /IM app.exe >nul 2>&1

echo [1] Compiling with 32-bit MinGW...
C:\MinGW\bin\gcc.exe -g main.c bmp.c gui.c image.c operation.c compat.c -I third_party/iup/include -L third_party/iup -liupcontrols -liupcd -liup -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32 -o app.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo [2] Build successful!
echo [3] Launching app.exe...
echo.
start "" app.exe
