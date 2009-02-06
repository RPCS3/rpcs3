# builds the GUI C classes
mkdir temp
cp zerogs.glade temp/
cd temp
glade-2 --write-source zerogs.glade
rm src/main.c
cp src/*.h src/*.c ../
cd ..
/bin/rm -rf temp
