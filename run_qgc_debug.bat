@echo off
cd /d E:\drone\QGC-debug\qgroundcontrol\build\Release
set QT_DEBUG_PLUGINS=1
set QT_LOGGING_RULES=qt.qml.*=true;qt.quick.*=true
set QML_IMPORT_TRACE=1
E:\drone\QGC-debug\qgroundcontrol\build\Release\QGroundControl.exe 2>&1 | powershell -Command "$input | Tee-Object -FilePath 'E:\drone\QGC-debug\qgc_stdout.log'"
echo Exit code: %errorlevel%
