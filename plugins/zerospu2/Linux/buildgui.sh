# builds the GUI C classes
mkdir temp
cp zerospu2.glade temp/
cd temp
glade-2 --write-source zerospu2.glade
rm src/main.c
cp src/*.h src/*.c ../
cd ..
/bin/rm -rf temp
