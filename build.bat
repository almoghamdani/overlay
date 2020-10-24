@echo off

call build_32.bat %1
if %errorlevel% == 0 call build_64.bat %1