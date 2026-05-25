@echo off
call "D:\tool\visual_stadio\product\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d E:\drone\QGC-debug\qgroundcontrol
C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe --build build --config Release --parallel > E:\drone\QGC-debug\build_perf_log.txt 2>&1
echo EXIT_CODE=%errorlevel% >> E:\drone\QGC-debug\build_perf_log.txt
