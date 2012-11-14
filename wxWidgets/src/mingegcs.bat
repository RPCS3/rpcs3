REM
REM replace k:\mingw32 with whatever your installation root may be.
REM
path C:\WINDOWS;C:\WINDOWS\COMMAND;k:\mingw32\bin;c:\bin

SET GCC_EXEC_PREFIX=k:\mingw32\lib\gcc-lib\
set BISON_SIMPLE=k:\mingw32\share\bison.simple
set BISON_HAIRY=k:\mingw32\share\bison.hairy
set C_INCLUDE_PATH=k:\MINGW32\include
set CPLUS_INCLUDE_PATH=k:\MINGW32\include\g++;g:\MINGW32\include
set LIBRARY_PATH=k:\MINGW32\lib
set GCC_EXEC_PREFIX=k:\MINGW32\lib\gcc-lib\

rem 4DOS users only...
unalias make
alias makeming make -f makefile.g95

