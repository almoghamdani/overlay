@echo off

SET ARCH=x64

::If the build folder doesn't exist, create it
IF NOT EXIST build mkdir build

:: Initialize the visual studio developer command line with x64 if it's not initialized yet
IF NOT DEFINED DevEnvDir (
    call "%VS140COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%
    call "%VS150COMCOMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%
)

:: Create the cmake project
if "%1"=="-R" (
    cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET=x64-windows-static
) else (
    cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug -DVCPKG_TARGET_TRIPLET=x64-windows-static
)

:: Build the project
cmake --build build