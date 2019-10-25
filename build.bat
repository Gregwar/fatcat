@echo off

if exist build rmdir /S /Q build
mkdir build
pushd build

set MINGW_BIN=C:\MinGW\bin
if not exist %MINGW_BIN% goto _msvc

set PATH=%MINGW_BIN%;%PATH%
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=MINSIZEREL && mingw32-make
goto _the_end

:_msvc
cmake .. && start "" fatcat.sln

:_the_end
popd
pause
