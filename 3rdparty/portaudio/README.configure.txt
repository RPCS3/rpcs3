PortAudio uses "autoconf" tools to generate Makefiles for Linux and Mac platforms.
The source for these are configure.in and Makefile.in
If you modify either of these files then please run this command before
testing and checking in your changes.

   autoreconf -if

Then test a build by doing:
   
   ./configure
   make clean
   make
   sudo make install

then check in the related files that are modified.
These might include files like:

   configure
   config.guess
   depcomp
   install.sh
   
