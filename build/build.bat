@echo off

SET vcvars=

:: Detect which Visual Studio version is installed
SET dir_10=c:\Program Files (x86)\Microsoft Visual Studio 10.0\
SET dir_17=c:\Program Files (x86)\Microsoft Visual Studio\2017\

IF EXIST %dir_10%\NUL(
    SET vcvars="%dir_10%VC\vcvarsall.bat"
)
IF EXIST %dir_17%\NUL(
    SET vcvars="%dir_17%Community\VC\Auxiliary\Build\vcvarsall.bat"
)

SET version_h=".\version.h"
SET resource_rc="..\src\resource\WinSpy.rc"

:: update the version build count, and 
:: the resource-file prior to building the solution
incbuild.rb %version_h% %resource_rc%

:: path to the visual-studio commandline environment variables

:: spawn another shell and run 'build0' with the VCVARS
%comspec% /c "%vcvars% x86_amd64 && build0.bat"

:: package everything together
buildzip.rb x86
buildzip.rb amd64
