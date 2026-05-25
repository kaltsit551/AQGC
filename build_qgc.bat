@echo off
call "D:\tool\visual_stadio\product\VC\Auxiliary\Build\vcvarsall.bat" x64
echo === MSVC Environment Ready ===
cl.exe 2>&1
echo === CMake Version ===
C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe --version
echo === Ninja Version ===
C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\ninja.exe --version
echo === Configure QGC ===
cd /d E:\drone\QGC-debug\qgroundcontrol
C:\Users\20949\AppData\Local\Programs\Python\Python314\Scripts\cmake.exe -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=E:/Qt/6.10.3/msvc2022_64 -DCMAKE_MAKE_PROGRAM=C:/Users/20949/AppData/Local/Programs/Python/Python314/Scripts/ninja.exe
