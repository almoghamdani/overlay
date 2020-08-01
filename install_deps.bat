@echo off

:: Build and install Vcpkg
echo Installing Vcpkg..
IF NOT EXIST .\vendor\vcpkg\vcpkg.exe call .\vendor\vcpkg\bootstrap-vcpkg.bat

:: Install deps
echo Installing dependencies..
vendor\vcpkg\vcpkg.exe install @vcpkg_win.txt
echo Finished installing dependencies!