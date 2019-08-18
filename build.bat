if not exist build md build
cd build
git clean -fdx && cmake .. -DDEFINE_WIN=ON && "c:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" fatcat.sln  /p:OutDir=bin
cd ..
pause