#*****************************************************************************
#                                                                            *
# Make file for VMS                                                          *
# Author : J.Jansen (joukj@hrem.nano.tudelft.nl)                             *
# Date : 31 October 2006                                                     *
#                                                                            *
#*****************************************************************************
.first
	define wx [--.include.wx]

.ifdef __WXMOTIF__
CXX_DEFINE = /define=(__WXMOTIF__=1)/name=(as_is,short)\
	   /assume=(nostdnew,noglobal_array_new)
CC_DEFINE = /define=(__WXMOTIF__=1)/name=(as_is,short)
.else
.ifdef __WXGTK__
CXX_DEFINE = /define=(__WXGTK__=1)/float=ieee/name=(as_is,short)/ieee=denorm\
	   /assume=(nostdnew,noglobal_array_new)
CC_DEFINE = /define=(__WXGTK__=1)/float=ieee/name=(as_is,short)/ieee=denorm
.else
.ifdef __WXX11__
CXX_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)/assume=(nostdnew,noglobal_array_new)
CC_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)
.else
.ifdef __WXGTK2__
CXX_DEFINE = /define=(__WXGTK__=1,VMS_GTK2==1)/float=ieee\
	/name=(as_is,short)/assume=(nostdnew,noglobal_array_new)
CC_DEFINE = /define=(__WX_GTK__=1,VMS_GTK2==1)/float=ieee\
	/name=(as_is,short)
.else
CXX_DEFINE =
CC_DEFINE =
.endif
.endif
.endif
.endif

.suffixes : .cpp

.cpp.obj :
	cxx $(CXXFLAGS)$(CXX_DEFINE) $(MMS$TARGET_NAME).cpp
.c.obj :
	cc $(CFLAGS)$(CC_DEFINE) $(MMS$TARGET_NAME).c

OBJECTS = \
		aboutdlgg.obj,\
		animateg.obj,\
		busyinfo.obj,\
		calctrl.obj,\
		caret.obj,\
		choicbkg.obj,\
		choicdgg.obj,\
		colrdlgg.obj,\
		clrpickerg.obj,\
		datectlg.obj,\
		dcpsg.obj,\
		dirctrlg.obj,\
		dirdlgg.obj,\
		fdrepdlg.obj,\
		fontdlgg.obj,\
		fontpickerg.obj,\
		grid.obj,\
		gridctrl.obj,\
		gridsel.obj,\
		helpext.obj,\
		htmllbox.obj,\
		imaglist.obj,\
		laywin.obj,\
		listbkg.obj,\
		listctrl.obj,\
		logg.obj,\
		msgdlgg.obj,\
		numdlgg.obj,\
		odcombo.obj,\
		panelg.obj,\
		printps.obj,\
		prntdlgg.obj,\
		propdlg.obj,\
		progdlgg.obj,\
		renderg.obj,\
		sashwin.obj,\
		scrlwing.obj,\
		selstore.obj,\
		splitter.obj,\
		tabg.obj,\
		textdlgg.obj,\
		tipdlg.obj,\
		tipwin.obj,\
		toolbkg.obj,\
		treebkg.obj,\
		treectlg.obj,\
		wizard.obj,\
		hyperlink.obj,\
		filepickerg.obj,\
		bmpcboxg.obj

SOURCES = \
		aboutdlgg.cpp,\
		accel.cpp,\
		animateg.cpp,\
		busyinfo.cpp,\
		calctrl.cpp,\
		caret.cpp,\
		choicbkg.cpp,\
		choicdgg.cpp,\
		collpaneg.cpp,\
		colrdlgg.cpp,\
		clrpickerg.cpp,\
		datectlg.cpp,\
		dcpsg.cpp,\
		dirctrlg.cpp,\
		dirdlgg.cpp,\
		filedlgg.cpp,\
		fdrepdlg.cpp,\
		fontdlgg.cpp,\
		fontpickerg.cpp,\
		grid.cpp,\
		gridctrl.cpp,\
		gridsel.cpp,\
		helpext.cpp,\
		htmllbox.cpp,\
		imaglist.cpp,\
		laywin.cpp,\
		listbkg.cpp,\
		listctrl.cpp,\
		logg.cpp,\
		msgdlgg.cpp,\
		notebook.cpp,\
		numdlgg.cpp,\
		odcombo.cpp,\
		paletteg.cpp,\
		panelg.cpp,\
		printps.cpp,\
		prntdlgg.cpp,\
		propdlg.cpp,\
		progdlgg.cpp,\
		renderg.cpp,\
		sashwin.cpp,\
		selstore.cpp,\
		splitter.cpp,\
		statline.cpp,\
		statusbr.cpp,\
		tabg.cpp,\
		textdlgg.cpp,\
		tipdlg.cpp,\
		tipwin.cpp,\
		toolbkg.cpp,\
		treebkg.cpp,\
		treectlg.cpp,\
		wizard.cpp,\
		dragimgg.cpp,\
		fdrepdlg.cpp,\
		htmllbox.cpp,\
		listbkg.cpp,\
		mdig.cpp,\
		scrlwing.cpp,\
		spinctlg.cpp,\
		splash.cpp,\
		timer.cpp,\
		vlbox.cpp,\
		hyperlink.cpp,\
		filepickerg.cpp,\
		vscroll.cpp,\
		icon.cpp,bmpcboxg.cpp

.ifdef __WXMOTIF__
OBJECTS0=,statusbr.obj,statline.obj,notebook.obj,spinctlg.obj,collpaneg.obj,\
	combog.obj
.else
.ifdef __WXX11__
OBJECTS0=,accel.obj,filedlgg.obj,dragimgg.obj,fdrepdlg.obj,htmllbox.obj,\
	listbkg.obj,mdig.obj,spinctlg.obj,splash.obj,timer.obj,\
	vlbox.obj,vscroll.obj,combog.obj,icon.obj,collpaneg.obj
.else
.ifdef __WXGTK__
OBJECTS0=,accel.obj,statusbr.obj,filedlgg.obj,paletteg.obj,vlbox.obj,\
	vscroll.obj,combog.obj,icon.obj,collpaneg.obj
.else
OBJECTS0=,accel.obj,statusbr.obj,filedlgg.obj,paletteg.obj,vlbox.obj,\
	vscroll.obj,combog.obj,icon.obj
.endif
.endif
.endif

all : $(SOURCES)
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS)$(OBJECTS0)
.ifdef __WXMOTIF__
	library/crea [--.lib]libwx_motif.olb $(OBJECTS)$(OBJECTS0)
.else
.ifdef __WXGTK__
	library/crea [--.lib]libwx_gtk.olb $(OBJECTS)$(OBJECTS0)
.else
.ifdef __WXGTK2__
	library/crea [--.lib]libwx_gtk2.olb $(OBJECTS)$(OBJECTS0)
.else
.ifdef __WXX11__
	library/crea [--.lib]libwx_x11_univ.olb $(OBJECTS)$(OBJECTS0)
.endif
.endif
.endif
.endif

aboutdlgg.obj : aboutdlgg.cpp
accel.obj : accel.cpp
animateg.obj : animateg.cpp
busyinfo.obj : busyinfo.cpp
calctrl.obj : calctrl.cpp
caret.obj : caret.cpp
choicdgg.obj : choicdgg.cpp
clrpickerg.obj : clrpickerg.cpp
collpaneg.obj : collpaneg.cpp
colrdlgg.obj : colrdlgg.cpp
datectlg.obj : datectlg.cpp
dcpsg.obj : dcpsg.cpp
dirctrlg.obj : dirctrlg.cpp
dirdlgg.obj : dirdlgg.cpp
filedlgg.obj : filedlgg.cpp
fontdlgg.obj : fontdlgg.cpp
fdrepdlg.obj : fdrepdlg.cpp
grid.obj : grid.cpp
gridctrl.obj : gridctrl.cpp
gridsel.obj : gridsel.cpp
helpext.obj : helpext.cpp
htmllbox.obj : htmllbox.cpp
icon.obj : icon.cpp
imaglist.obj : imaglist.cpp
laywin.obj : laywin.cpp
listctrl.obj : listctrl.cpp
logg.obj : logg.cpp
msgdlgg.obj : msgdlgg.cpp
notebook.obj : notebook.cpp
numdlgg.obj : numdlgg.cpp
odcombo.obj : odcombo.cpp
paletteg.obj : paletteg.cpp
panelg.obj : panelg.cpp
printps.obj : printps.cpp
prntdlgg.obj : prntdlgg.cpp
progdlgg.obj : progdlgg.cpp
propdlg.obj : propdlg.cpp
scrlwing.obj : scrlwing.cpp
spinctlg.obj : spinctlg.cpp
renderg.obj : renderg.cpp
sashwin.obj : sashwin.cpp
selstore.obj : selstore.cpp
splitter.obj : splitter.cpp
statline.obj : statline.cpp
statusbr.obj : statusbr.cpp
tabg.obj : tabg.cpp
textdlgg.obj : textdlgg.cpp
tipdlg.obj : tipdlg.cpp
tipwin.obj : tipwin.cpp
treectlg.obj : treectlg.cpp
wizard.obj : wizard.cpp
dragimgg.obj : dragimgg.cpp
fdrepdlg.obj : fdrepdlg.cpp
htmllbox.obj : htmllbox.cpp
listbkg.obj : listbkg.cpp
mdig.obj : mdig.cpp
scrlwing.obj : scrlwing.cpp
spinctlg.obj : spinctlg.cpp
splash.obj : splash.cpp
timer.obj : timer.cpp
vlbox.obj : vlbox.cpp
vscroll.obj : vscroll.cpp
	cxx $(CXXFLAGS)$(CXX_DEFINE)/nowarn vscroll.cpp
listbkg.obj : listbkg.cpp
choicbkg.obj : choicbkg.cpp
toolbkg.obj : toolbkg.cpp
treebkg.obj : treebkg.cpp
combog.obj : combog.cpp
fontpickerg.obj : fontpickerg.cpp
hyperlink.obj : hyperlink.cpp
filepickerg.obj : filepickerg.cpp
bmpcboxg.obj : bmpcboxg.cpp
