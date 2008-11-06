ZeroGSShaders.exe ps2hw.fx ps2fx.dat
del Win32\ps2fx.dat Win32\Release\*.res Win32\Debug\*.res
move /y ps2fx.dat Win32\ps2fx.dat