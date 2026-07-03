@echo off
rem Native Windows QEMU Launch Script 🏎️

set OUT_DIR=Build
set QEMU_PATH="qemu-system-x86_64"

echo === Launching Bare Metal Calculator in QEMU (Windows) ===

if not exist "%OUT_DIR%\boot.bin" (
    echo Error: %OUT_DIR%\boot.bin not found. Run Tools\Build\build.bat first! 💀
    exit /b 1
)

echo Starting QEMU emulation loop...
%QEMU_PATH% -drive format=raw,file=%OUT_DIR%\boot.bin -serial stdio

if %errorlevel% neq 0 (
    echo Trying fallback to default installation directory...
    "C:\Program Files\qemu\qemu-system-x86_64.exe" -drive format=raw,file=%OUT_DIR%\boot.bin -serial stdio
)
