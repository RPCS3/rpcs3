
:: Probably self-explanatory: This batch file compiles a single souce image into a
:: CPP header file for use by pcsx2.
::
::  bin2cpp.cmd   SrcImage
::
:: Parameters
::   SrcImage - Complete filename with extension.
::

@echo off

SETLOCAL ENABLEEXTENSIONS

cd "%~0\..\"
"..\..\..\tools\bin\bin2cpp.exe" %1

ENDLOCAL
