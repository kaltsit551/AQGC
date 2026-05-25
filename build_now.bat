@echo off
chcp 65001 >nul
call "D:\tool\visual_stadio\product\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    echo VCVARS FAILED
    exit /b 1
)
cd /d "E:\drone\QGC-debug\qgroundcontrol"
echo === Starting Build ===
"C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe" --build build --config Release --parallel
echo === EXIT CODE: %errorlevel% ===
