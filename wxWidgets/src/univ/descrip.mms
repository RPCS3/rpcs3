#*****************************************************************************
#                                                                            *
# Make file for VMS                                                          *
# Author : J.Jansen (joukj@hrem.nano.tudelft.nl)                             *
# Date : 22 September 2006                                                   *
#                                                                            *
#*****************************************************************************
.first
	define wx [--.include.wx]

CXX_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)/assume=(nostdnew,noglobal_array_new)
CC_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)

.suffixes : .cpp

.cpp.obj :
	cxx $(CXXFLAGS)$(CXX_DEFINE) $(MMS$TARGET_NAME).cpp
.c.obj :
	cc $(CFLAGS)$(CC_DEFINE) $(MMS$TARGET_NAME).c

OBJECTS = \
		bmpbuttn.obj,\
		button.obj,\
		checkbox.obj,\
		checklst.obj,\
		choice.obj,\
		colschem.obj,\
		control.obj,\
		dialog.obj,\
		framuniv.obj,\
		gauge.obj,\
		inpcons.obj,\
		inphand.obj,\
		listbox.obj,\
		menu.obj,\
		notebook.obj,\
		radiobut.obj,\
		scrarrow.obj,\
		scrolbar.obj,\
		scrthumb.obj,\
		slider.obj,\
		spinbutt.obj,\
		statbmp.obj,\
		statbox.obj,\
		statline.obj,\
		stattext.obj,\
		statusbr.obj,\
		stdrend.obj,\
		textctrl.obj,\
		theme.obj,\
		toolbar.obj,\
		topluniv.obj,\
		winuniv.obj,\
		combobox.obj,\
		ctrlrend.obj,\
		gtk.obj,\
		metal.obj,\
		radiobox.obj,\
		scrthumb.obj,\
		win32.obj

SOURCES =\
		bmpbuttn.cpp \
		button.cpp \
		checkbox.cpp \
		checklst.cpp \
		choice.cpp \
		colschem.cpp \
		control.cpp \
		dialog.cpp \
		framuniv.cpp \
		gauge.cpp \
		inpcons.cpp \
		inphand.cpp \
		listbox.cpp \
		menu.cpp \
		notebook.cpp \
		radiobut.cpp \
		scrarrow.cpp \
		scrolbar.cpp \
		scrthumb.cpp \
		slider.cpp \
		spinbutt.cpp \
		statbmp.cpp \
		statbox.cpp \
		statline.cpp \
		stattext.cpp \
		statusbr.cpp \
		stdrend.cpp,\
		textctrl.cpp \
		theme.cpp \
		toolbar.cpp \
		topluniv.cpp \
		winuniv.cpp \
		combobox.cpp \
		ctrlrend.cpp \
		radiobox.cpp \
		scrthumb.cpp \
		[.themes]gtk.cpp \
		[.themes]metal.cpp \
		[.themes]win32.cpp
   
all : $(SOURCES)
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS)
	library [--.lib]libwx_x11_univ.olb $(OBJECTS)

bmpbuttn.obj : bmpbuttn.cpp
button.obj : button.cpp
checkbox.obj : checkbox.cpp
checklst.obj : checklst.cpp
choice.obj : choice.cpp
colschem.obj : colschem.cpp
control.obj : control.cpp
dialog.obj : dialog.cpp
framuniv.obj : framuniv.cpp
gauge.obj : gauge.cpp
inpcons.obj : inpcons.cpp
inphand.obj : inphand.cpp
listbox.obj : listbox.cpp
menu.obj : menu.cpp
notebook.obj : notebook.cpp
radiobut.obj : radiobut.cpp
scrarrow.obj : scrarrow.cpp
scrolbar.obj : scrolbar.cpp
scrthumb.obj : scrthumb.cpp
slider.obj : slider.cpp
spinbutt.obj : spinbutt.cpp
statbmp.obj : statbmp.cpp
statbox.obj : statbox.cpp
statline.obj : statline.cpp
stattext.obj : stattext.cpp
statusbr.obj : statusbr.cpp
stdrend.obj : stdrend.cpp
textctrl.obj : textctrl.cpp
theme.obj : theme.cpp
toolbar.obj : toolbar.cpp
topluniv.obj : topluniv.cpp
winuniv.obj : winuniv.cpp
combobox.obj : combobox.cpp
ctrlrend.obj : ctrlrend.cpp
gtk.obj : [.themes]gtk.cpp
	cxx $(CXXFLAGS)$(CXX_DEFINE) [.themes]gtk.cpp
metal.obj : [.themes]metal.cpp
	cxx $(CXXFLAGS)$(CXX_DEFINE) [.themes]metal.cpp
radiobox.obj : radiobox.cpp
scrthumb.obj : scrthumb.cpp
win32.obj : [.themes]win32.cpp
	cxx $(CXXFLAGS)$(CXX_DEFINE) [.themes]win32.cpp
