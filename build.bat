@echo off

SET ARCH=%1

:: Initialize the visual studio developer command line with x64 if it's not initialized yet
setlocal
IF NOT DEFINED DevEnvDir (
    call "%VS140COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%
    call "%VS150COMCOMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%
)

:: Create the cmake project
if "%2"=="-R" (
    cmake -G Ninja -B build\%ARCH% -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET=%ARCH%-windows-static -Wno-dev
) else (
    cmake -G Ninja -B build\%ARCH% -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=%ARCH%-windows-static -Wdev
)

:: Build the project
cmake --build build\%ARCH%
endlocal