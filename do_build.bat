@echo off
call "D:\tool\visual_stadio\product\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d E:\drone\QGC-debug\qgroundcontrol
echo === Configuring ===
C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=E:/Qt/6.10.3/msvc2022_64 -DCMAKE_MAKE_PROGRAM=C:/Users/20949/AppData/Local/Programs/Python/Python314/Scripts/ninja.exe -DQGC_ENABLE_GST_VIDEOSTREAMING=OFF
if %errorlevel% neq 0 (
    echo === Configure FAILED ===
    exit /b %errorlevel%
)
echo === Building ===
C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe --build build --config Release --parallel
if %errorlevel% neq 0 (
    echo === Build FAILED ===
    exit /b %errorlevel%
)
echo === Build SUCCESS ===
