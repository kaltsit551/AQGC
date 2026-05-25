@echo off
cd /d E:\drone\QGC-debug\qgroundcontrol\build\Release
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.qml.*=true
E:\drone\QGC-debug\qgroundcontrol\build\Release\QGroundControl.exe > E:\drone\QGC-debug\qgc_stdout.log 2>&1
echo Exit code: %errorlevel% >> E:\drone\QGC-debug\qgc_stdout.log
