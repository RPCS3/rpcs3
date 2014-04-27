@echo off
call parsedate
set TIMESTAMP=%month%%day%%year:~2,3%
winrar u ZeroGS%TIMESTAMP%.zip *.h *.cpp Win32\*.sln Win32\*.vcproj Win32\*.suo Win32\*.bat Win32\*.cpp Win32\*.h Win32\*.def *.asm *.fx *.bat zerogs.psd ZeroGSShaders.exe x86\*.* Win32\*.rc Win32\*.bmp Win32\*.dat ..\ZeroGSShaders\*.h ..\ZeroGSShaders\*.cpp