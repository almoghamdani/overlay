@echo off

call build_arch.bat x86 %1
if %errorlevel% == 0 call build_arch.bat x64 %1