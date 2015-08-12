@echo off

set DevRoot=%~dp0
set DevRoot=%DevRoot:~0,-1%
set EnableNuGetPackageRestore=true

title Dev - %DevRoot%

::
:: Reset the PATH
::
set VSINSTALLDIR=%VS140COMNTOOLS:~0,-14%
set path=%windir%\system32;%windir%\system32\wbem;%windir%
call :InsertPath %ProgramFiles(x86)%\Microsoft SDKs\Windows\v8.1A\bin\NETFX 4.5.1 Tools
call :InsertPath %ProgramFiles(x86)%\Windows Kits\8.1\bin\x64
call :InsertPath %windir%\Microsoft.NET\Framework\v4.0.30319
call :InsertPath %VSINSTALLDIR%Common7\Tools
call :InsertPath %VSINSTALLDIR%Common7\IDE
call :InsertPath %VSINSTALLDIR%VC\BIN
call :InsertPath %ProgramFiles(x86)%\MSBuild\14.0\bin

::
:: Add custom commands
::
doskey /MACROFILE="%DevRoot%\alias.txt"

::
:: Run user custom script
::
if exist "%DevRoot%\personal\setenv.bat" (
  call "%DevRoot%\personal\setenv.bat"
)

goto :DONE

::
:: Functions
::
:InsertPath
if exist "%*" set path=%*;%path%
goto :EOF

:DONE
