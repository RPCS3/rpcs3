#
# File:		makeprog.sl
# Author:	Julian Smart
# Created:	1998
#
# Makefile : Include file for wxWindows samples, Salford C++/WIN32

include ..\..\src\makesl.env

TARGET   = $(PROGRAM).exe

ALLOBJECTS = $(OBJECTS) $(PROGRAM)_resources.obj

$(TARGET) : $(ALLOBJECTS)
    echo slink $(ALLOBJECTS) $(WXDIR)\src\msw\main.obj $(WXLIB)\wx.lib -subsystem:windows -file:$(TARGET) 
    slink $(ALLOBJECTS) $(WXDIR)\src\msw\main.obj $(WXLIB)\wx.lib -subsystem:windows -file:$(TARGET) 

$(PROGRAM)_resources.obj: $(PROGRAM).rc
    src /ERROR_NUMBERS /DELETE_OBJ_ON_ERROR /DEFINE __SALFORDC__ /DEFINE __WXMSW__ /DEFINE __WIN32__ /DEFINE __WIN95__ /DEFINE WXINCDIR=$(RESOURCEDIR) /INCLUDE $(WXDIR)\include /INCLUDE $(WXDIR)\include\wx\msw $(PROGRAM).rc /BINARY $(PROGRAM)_resources.obj

clean:
    -erase *.obj
    -erase $(TARGET)
    -erase *.res

cleanall:   clean


