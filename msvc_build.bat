@echo off
call "D:\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 exit /b %errorlevel%
msbuild lua.sln /p:Configuration=Debug /p:Platform=x64
exit /b %errorlevel%