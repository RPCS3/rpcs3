mkdir build
mkdir build\rpcs3
copy bin\rpcs3-*.exe build\rpcs3
copy bin\soft-oal.dll build\rpcs3
copy bin\make_fself.cmd build\rpcs3

mkdir build\rpcs3\dev_hdd1
xcopy /e bin\dev_hdd1 build\rpcs3\dev_hdd1

mkdir build\rpcs3\dev_hdd0
xcopy /e bin\dev_hdd0 build\rpcs3\dev_hdd0

mkdir build\rpcs3\dev_flash
xcopy /e bin\dev_flash build\rpcs3\dev_flash

mkdir build\rpcs3\dev_usb000
xcopy /e bin\dev_usb000 build\rpcs3\dev_usb000

for /f "delims=" %%a in ('git describe') do @set gitrev=%%a

cd build
7z a -mx9 ..\rpcs3-%gitrev%-windows-x86_64.7z rpcs3
cd ..
