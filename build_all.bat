@echo off

call build_32.bat
call build_32.bat -R
call build_64.bat
call build_64.bat -R