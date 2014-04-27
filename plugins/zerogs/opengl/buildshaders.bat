ZeroGSShaders.exe ps2hw.fx ps2hw.dat
del Win32\ps2hw.dat Win32\Release\*.res Win32\Debug\*.res
move /y ps2hw.dat Win32\ps2hw.dat