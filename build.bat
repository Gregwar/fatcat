@echo off
set PATH=C:\MinGW\bin;%PATH%
if exist build rmdir /S /Q build
mkdir build
pushd build
cmake .. -G "MinGW Makefiles" && mingw32-make
popd
pause
