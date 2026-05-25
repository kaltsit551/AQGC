@echo off
call "D:\tool\visual_stadio\product\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    echo VCVARS FAILED > "E:\drone\QGC-debug\build_log.txt"
    exit /b 1
)
cd /d "E:\drone\QGC-debug\qgroundcontrol"
echo === Starting Build === > "E:\drone\QGC-debug\build_log.txt"
"C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe" --build build --config Release --parallel >> "E:\drone\QGC-debug\build_log.txt" 2>&1
echo === EXIT CODE: %errorlevel% === >> "E:\drone\QGC-debug\build_log.txt"
