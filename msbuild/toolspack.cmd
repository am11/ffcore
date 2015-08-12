@echo off
setlocal

set _HERE=%~dp0
"%_HERE%NuGet.exe" pack "%_HERE%tools.nuspec" -NoPackageAnalysis
