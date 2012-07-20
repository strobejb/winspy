@echo off

cd ..

REM Win32 Release
msbuild WinSpy.sln /p:Configuration="Release" /p:Platform=Win32

REM Win64 Release
msbuild WinSpy.sln /p:Configuration="Release" /p:Platform=x64

cd build
