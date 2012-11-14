REM
REM replace c:\MINGW with whatever your installation root may be.
REM GCC_EXEC_PREFIX is optional, and hardly ever needs to be set (read:
REM leave it alone).
REM
SET MINGWDIR=c:\gcc-2.95
PATH=%MINGWDIR%\bin;%PATH%
set BISON_SIMPLE=%MINGWDIR%\share\bison.simple
set BISON_HAIRY=%MINGWDIR%\share\bison.hairy

REM SET GCC_EXEC_PREFIX=%MINGWDIR%\lib\gcc-lib\
REM set LIBRARY_PATH=%MINGWDIR%\lib;%MINGWDIR%\lib\gcc-lib\i386-mingw32\2.8.1

rem 4DOS users only...
unalias make
alias makeming make -f makefile.g95
