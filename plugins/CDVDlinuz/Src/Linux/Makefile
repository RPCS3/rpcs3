
PLUGIN = libCDVDlinuz.so
PLUGINOBJS = CDVDlinuz.o ../buffer.o
PLUGINHEADERS = CDVDlinuz.h ../buffer.h
PLUGINFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -D_LARGEFILE64_SOURCE \
	-I.. -I. -I./Linux
PLUGINLIBS =

CFG = cfgCDVDlinuz
CFGOBJS = aboutbox.o mainbox.o interface.o
CFGHEADERS = aboutbox.h mainbox.h interface.h
CFGFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -D_LARGEFILE64_SOURCE \
	-I.. -I. -I./Linux
CFGLIBS =

SHAREDOBJS = actualfile.o conf.o logfile.o \
	device.o CD.o DVD.o \
	../convert.o ../ini.o ../version.o
SHAREDHEADERS = actualfile.h conf.h logfile.h \
	device.h CD.h DVD.h \
	../convert.h ../ini.h ../version.h


CC = gcc

GTKFLAGS = $(shell pkg-config --cflags gtk+-2.0)

//GTKFLAGS += -DG_DISABLE_DEPRECATED \
//	-DGDK_DISABLE_DEPRECATED \
//	-DGDK_PIXBUF_DISABLE_DEPRECATED \
//	-DGTK_DISABLE_DEPRECATED

GTKLIBS = $(shell pkg-config --libs gtk+-2.0)
# Do we need to remove "-rdynamic" as well? Or is that just the main program?


all: plugin cfg
install: all

release: plugin cfg
	cp $(PLUGIN) ../..
	cp $(CFG) ../..

plugin: $(PLUGINOBJS) $(SHAREDOBJS)
#	rm -f $(PLUGIN)
	$(CC) -shared -Wl,-soname,$(PLUGIN) $(PLUGINFLAGS) $(PLUGINLIBS) \
	$(PLUGINOBJS) $(SHAREDOBJS) -o $(PLUGIN)
	strip --strip-unneeded --strip-debug $(PLUGIN)

cfg: $(CFGOBJS) $(SHAREDOBJS)
#	rm -f $(CFG)
	$(CC) $(CFGFLAGS) $(GTKFLAGS) $(CFGLIBS) $(GTKLIBS) \
	$(CFGOBJS) $(SHAREDOBJS) -o $(CFG)
	strip $(CFG)

$(PLUGINOBJS) $(SHAREDOBJS): %.o: %.c
	$(CC) $(PLUGINFLAGS) -c $< -o $@

$(CFGOBJS): %.o: %.c
	$(CC) $(CFGFLAGS) $(GTKFLAGS) -c $< -o $@

.PHONY : clean allclean
clean:
	-rm -f $(PLUGINOBJS) $(PLUGIN) $(CFGOBJS) $(CFG) $(SHAREDOBJS)
	-rm -f *~ temp.txt ../*~ ../temp.txt ../../*~

allclean:
	-rm -f $(PLUGINOBJS) $(PLUGIN) $(CFGOBJS) $(CFG) $(SHAREDOBJS)
	-rm -f *~ temp.txt ../*~ ../temp.txt ../../*~
	-rm -f ../../$(PLUGIN) ../../$(CFG)

