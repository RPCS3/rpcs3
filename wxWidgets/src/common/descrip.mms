#*****************************************************************************
#                                                                            *
# Make file for VMS                                                          *
# Author : J.Jansen (joukj@hrem.nano.tudelft.nl)                             *
# Date : 1 December 2006                                                     *
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
.ifdef __WXGTK2__
CXX_DEFINE = /define=(__WXGTK__=1,VMS_GTK2=1)/float=ieee/name=(as_is,short)/ieee=denorm\
	   /assume=(nostdnew,noglobal_array_new)
CC_DEFINE = /define=(__WXGTK__=1,VMS_GTK2=1)/float=ieee/name=(as_is,short)/ieee=denorm
.else
.ifdef __WXX11__
CXX_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)/assume=(nostdnew,noglobal_array_new)
CC_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)
.else
CXX_DEFINE =
CC_DEFINE =
.endif
.endif
.endif
.endif

YACC=bison/yacc

SED=gsed

LEX=flex

.suffixes : .cpp

.cpp.obj :
	cxx $(CXXFLAGS)$(CXX_DEFINE) $(MMS$TARGET_NAME).cpp
.c.obj :
	cc $(CFLAGS)$(CC_DEFINE) $(MMS$TARGET_NAME).c

OBJECTS = \
		anidecod.obj,\
		animatecmn.obj,\
		appbase.obj,\
		appcmn.obj,\
		artprov.obj,\
		artstd.obj,\
		bmpbase.obj,\
		bookctrl.obj,\
		choiccmn.obj,\
		clipcmn.obj,\
		clntdata.obj,\
		cmdline.obj,\
		cmdproc.obj,\
		cmndata.obj,\
		config.obj,\
		containr.obj,\
		convauto.obj,\
		colourcmn.obj,\
		cshelp.obj,\
		ctrlcmn.obj,\
		ctrlsub.obj,\
		datacmn.obj,\
		datetime.obj,\
		datstrm.obj,\
		db.obj,\
		dbgrid.obj,\
		dbtable.obj,\
		dcbase.obj,\
		dcbufcmn.obj,\
		dircmn.obj,\
		dlgcmn.obj,\
		dobjcmn.obj,\
		docmdi.obj,\
		docview.obj,\
		dpycmn.obj,\
		dynarray.obj,\
		dynlib.obj,\
		encconv.obj,\
		event.obj,\
		evtloopcmn.obj,\
		extended.obj,\
		fddlgcmn.obj,\
		ffile.obj,\
		file.obj,\
		fileback.obj,\
		fileconf.obj,\
		filename.obj,\
		filefn.obj,\
		filesys.obj,\
		fldlgcmn.obj,\
		fmapbase.obj,\
		fontcmn.obj,\
		fontenumcmn.obj,\
		fontmap.obj,\
		framecmn.obj

OBJECTS1=fs_inet.obj,\
		ftp.obj,\
		gaugecmn.obj,\
		gbsizer.obj,\
		gdicmn.obj,\
		gifdecod.obj,\
		hash.obj,\
		hashmap.obj,\
		helpbase.obj,\
		http.obj,\
		iconbndl.obj,\
		init.obj,\
		imagall.obj,\
		imagbmp.obj,\
		image.obj,\
		imagfill.obj,\
		imaggif.obj,\
		imagiff.obj,\
		imagjpeg.obj,\
		imagpcx.obj,\
		imagpng.obj,\
		imagpnm.obj,\
		imagtga.obj,\
		imagtiff.obj,\
		imagxpm.obj,\
		intl.obj,\
		ipcbase.obj,\
		layout.obj,\
		lboxcmn.obj,\
		list.obj,\
		log.obj,\
		longlong.obj,\
		memory.obj,\
		menucmn.obj,\
		mimecmn.obj,\
		module.obj,\
		msgout.obj,\
		mstream.obj,\
		nbkbase.obj,\
		object.obj,\
		paper.obj,\
		platinfo.obj,\
		popupcmn.obj,\
		prntbase.obj,\
		process.obj,\
		protocol.obj,\
		quantize.obj,\
		radiocmn.obj,\
		rendcmn.obj,\
		sckaddr.obj,\
		sckfile.obj,\
		sckipc.obj,\
		sckstrm.obj,\
		sizer.obj,\
		socket.obj,\
		settcmn.obj,\
		statbar.obj,\
		stdpbase.obj,\
		stockitem.obj,\
		stopwatch.obj,\
		strconv.obj,\
		stream.obj,\
		string.obj,\
		sysopt.obj

OBJECTS2=tbarbase.obj,\
		textbuf.obj,\
		textcmn.obj,\
		textfile.obj,\
		timercmn.obj,\
		tokenzr.obj,\
		toplvcmn.obj,\
		treebase.obj,\
		txtstrm.obj,\
		url.obj,\
		utilscmn.obj,\
		rgncmn.obj,\
		uri.obj,\
		valgen.obj,\
		validate.obj,\
		valtext.obj,\
		variant.obj,\
		wfstream.obj,\
		wxchar.obj,\
		wincmn.obj,\
		xpmdecod.obj,\
		zipstrm.obj,\
		zstream.obj,\
		clrpickercmn.obj,\
		filepickercmn.obj,\
		fontpickercmn.obj,\
		pickerbase.obj,\
		listctrlcmn.obj

OBJECTS_MOTIF=radiocmn.obj,combocmn.obj

OBJECTS_X11=accesscmn.obj,dndcmn.obj,dpycmn.obj,dseldlg.obj,\
	dynload.obj,effects.obj,fddlgcmn.obj,fs_mem.obj,\
	gbsizer.obj,geometry.obj,matrix.obj,radiocmn.obj,\
	regex.obj,taskbarcmn.obj,xti.obj,xtistrm.obj,xtixml.obj,\
	combocmn.obj

OBJECTS_X11_2=socketevtdispatch.obj

SOURCES = \
		anidecod.cpp,\
		animatecmn.cpp,\
		appbase.cpp,\
		appcmn.cpp,\
		artprov.cpp,\
		artstd.cpp,\
		bmpbase.cpp,\
		bookctrl.cpp,\
		choiccmn.cpp,\
		clipcmn.cpp,\
		clntdata.cpp,\
		cmdline.cpp,\
		cmdproc.cpp,\
		cmndata.cpp,\
		config.cpp,\
		containr.cpp,\
		convauto.cpp,\
		colourcmn.cpp,\
		cshelp.cpp,\
		ctrlcmn.cpp,\
		ctrlsub.cpp,\
		datacmn.cpp,\
		datetime.cpp,\
		datstrm.cpp,\
		db.cpp,\
		dbgrid.cpp,\
		dbtable.cpp,\
		dcbase.cpp,\
		dcbufcmn.cpp,\
		dircmn.cpp,\
		dlgcmn.cpp,\
		dobjcmn.cpp,\
		docmdi.cpp,\
		docview.cpp,\
		dpycmn.cpp,\
		dynarray.cpp,\
		dynlib.cpp,\
		encconv.cpp,\
		event.cpp,\
		evtloopcmn.cpp,\
		extended.c,\
		ffile.cpp,\
		fddlgcmn.cpp,\
		file.cpp,\
		fileback.cpp,\
		fileconf.cpp,\
		filename.cpp,\
		filefn.cpp,\
		filesys.cpp,\
		fldlgcmn.cpp,\
		fmapbase.cpp,\
		fontcmn.cpp,\
		fontenumcmn.cpp,\
		fontmap.cpp,\
		framecmn.cpp,\
		fs_inet.cpp,\
		ftp.cpp,\
		gaugecmn.cpp,\
		gbsizer.cpp,\
		gdicmn.cpp,\
		gifdecod.cpp,\
		hash.cpp,\
		hashmap.cpp,\
		helpbase.cpp,\
		http.cpp,\
		iconbndl.cpp,\
		init.cpp,\
		imagall.cpp,\
		imagbmp.cpp,\
		image.cpp,\
		imagfill.cpp,\
		imaggif.cpp,\
		imagiff.cpp,\
		imagjpeg.cpp,\
		imagpcx.cpp,\
		imagpng.cpp,\
		imagpnm.cpp,\
		imagtga.cpp,\
		imagtiff.cpp,\
		imagxpm.cpp,\
		intl.cpp,\
		ipcbase.cpp,\
		layout.cpp,\
		lboxcmn.cpp,\
		list.cpp,\
		listctrlcmn.cpp,\
		log.cpp,\
		longlong.cpp,\
		memory.cpp,\
		menucmn.cpp,\
		mimecmn.cpp,\
		module.cpp,\
		msgout.cpp,\
		mstream.cpp,\
		nbkbase.cpp,\
		object.cpp,\
		paper.cpp,\
		platinfo.cpp,\
		popupcmn.cpp,\
		prntbase.cpp,\
		process.cpp,\
		protocol.cpp,\
		quantize.cpp,\
		radiocmn.cpp,\
		rendcmn.cpp,\
		rgncmn.cpp,\
		sckaddr.cpp,\
		sckfile.cpp,\
		sckipc.cpp,\
		sckstrm.cpp,\
		sizer.cpp,\
		socket.cpp,\
		socketevtdispatch.cpp,\
		settcmn.cpp,\
		statbar.cpp,\
		stdpbase.cpp,\
		stockitem.cpp,\
		stopwatch.cpp,\
		strconv.cpp,\
		stream.cpp,\
		sysopt.cpp,\
		string.cpp,\
		tbarbase.cpp,\
		textbuf.cpp,\
		textcmn.cpp,\
		textfile.cpp,\
		timercmn.cpp,\
		tokenzr.cpp,\
		toplvcmn.cpp,\
		treebase.cpp,\
		txtstrm.cpp,\
		url.cpp,\
		utilscmn.cpp,\
		valgen.cpp,\
		validate.cpp,\
		valtext.cpp,\
		variant.cpp,\
		wfstream.cpp,\
		wincmn.cpp,\
		wxchar.cpp,\
		xpmdecod.cpp,\
		zipstrm.cpp,\
		zstream.cpp,\
		clrpickercmn.cpp,\
		filepickercmn.cpp,\
		fontpickercmn.cpp,\
		pickerbase.cpp,\
		accesscmn.cpp,\
		dndcmn.cpp,\
		dpycmn.cpp,\
		dseldlg.cpp,\
		dynload.cpp,\
		effects.cpp,\
		fddlgcmn.cpp,\
		fs_mem.cpp,\
		gbsizer.cpp,\
		geometry.cpp,\
		matrix.cpp,\
		radiocmn.cpp,\
		regex.cpp,\
		taskbarcmn.cpp,\
		uri.cpp,\
		xti.cpp,\
		xtistrm.cpp,\
		xtixml.cpp

all : $(SOURCES)
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS)
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS1)
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS2)
.ifdef __WXMOTIF__
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS_MOTIF)
	library [--.lib]libwx_motif.olb $(OBJECTS)
	library [--.lib]libwx_motif.olb $(OBJECTS1)
	library [--.lib]libwx_motif.olb $(OBJECTS2)
	library [--.lib]libwx_motif.olb $(OBJECTS_MOTIF)
.else
.ifdef __WXGTK__
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS_X11)
	library [--.lib]libwx_gtk.olb $(OBJECTS)
	library [--.lib]libwx_gtk.olb $(OBJECTS1)
	library [--.lib]libwx_gtk.olb $(OBJECTS2)
	library [--.lib]libwx_gtk.olb $(OBJECTS_X11)
.else
.ifdef __WXGTK2__
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS_X11)
	library [--.lib]libwx_gtk2.olb $(OBJECTS)
	library [--.lib]libwx_gtk2.olb $(OBJECTS1)
	library [--.lib]libwx_gtk2.olb $(OBJECTS2)
	library [--.lib]libwx_gtk2.olb $(OBJECTS_X11)
.else
.ifdef __WXX11__
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS_X11)
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS_X11_2)
	library [--.lib]libwx_x11_univ.olb $(OBJECTS)
	library [--.lib]libwx_x11_univ.olb $(OBJECTS1)
	library [--.lib]libwx_x11_univ.olb $(OBJECTS2)
	library [--.lib]libwx_x11_univ.olb $(OBJECTS_X11)
	library [--.lib]libwx_x11_univ.olb $(OBJECTS_X11_2)
.endif
.endif
.endif
.endif

anidecod.obj : anidecod.cpp
animatecmn.obj : animatecmn.cpp
appbase.obj : appbase.cpp
appcmn.obj : appcmn.cpp
artprov.obj : artprov.cpp
artstd.obj : artstd.cpp
bmpbase.obj : bmpbase.cpp
bookctrl.obj : bookctrl.cpp
choiccmn.obj : choiccmn.cpp
clipcmn.obj : clipcmn.cpp
clntdata.obj : clntdata.cpp
cmdline.obj : cmdline.cpp
cmdproc.obj : cmdproc.cpp
cmndata.obj : cmndata.cpp
config.obj : config.cpp
containr.obj : containr.cpp
convauto.obj : convauto.cpp
colourcmn.obj : colourcmn.cpp
cshelp.obj : cshelp.cpp
ctrlcmn.obj : ctrlcmn.cpp
ctrlsub.obj : ctrlsub.cpp
datacmn.obj : datacmn.cpp
datetime.obj : datetime.cpp
datstrm.obj : datstrm.cpp
db.obj : db.cpp
dbgrid.obj : dbgrid.cpp
dbtable.obj : dbtable.cpp
dcbase.obj : dcbase.cpp
dcbufcmn.obj : dcbufcmn.cpp
dircmn.obj : dircmn.cpp
dlgcmn.obj : dlgcmn.cpp
dobjcmn.obj : dobjcmn.cpp
docmdi.obj : docmdi.cpp
docview.obj : docview.cpp
dynarray.obj : dynarray.cpp
dynlib.obj : dynlib.cpp
encconv.obj : encconv.cpp
event.obj : event.cpp
evtloopcmn.obj : evtloopcmn.cpp
extended.obj : extended.c
ffile.obj : ffile.cpp
fddlgcmn.obj : fddlgcmn.cpp
file.obj : file.cpp
fileback.obj : fileback.cpp
fileconf.obj : fileconf.cpp
filefn.obj : filefn.cpp
filename.obj : filename.cpp
filesys.obj : filesys.cpp
fldlgcmn.obj : fldlgcmn.cpp
fmapbase.obj : fmapbase.cpp
fontcmn.obj : fontcmn.cpp
fontenumcmn.obj : fontenumcmn.cpp
fontmap.obj : fontmap.cpp
framecmn.obj : framecmn.cpp
fs_inet.obj : fs_inet.cpp
ftp.obj : ftp.cpp
gaugecmn.obj : gaugecmn.cpp
gbsizer.obj : gbsizer.cpp
gdicmn.obj : gdicmn.cpp
gifdecod.obj : gifdecod.cpp
hash.obj : hash.cpp
hashmap.obj : hashmap.cpp
helpbase.obj : helpbase.cpp
http.obj : http.cpp
iconbndl.obj : iconbndl.cpp
init.obj : init.cpp
imagall.obj : imagall.cpp
imagbmp.obj : imagbmp.cpp
image.obj : image.cpp
imagfill.obj : imagfill.cpp
imaggif.obj : imaggif.cpp
imagiff.obj : imagiff.cpp
imagjpeg.obj : imagjpeg.cpp
imagpcx.obj : imagpcx.cpp
imagpng.obj : imagpng.cpp
imagpnm.obj : imagpnm.cpp
imagtga.obj : imagtga.cpp
imagtiff.obj : imagtiff.cpp
imagxpm.obj : imagxpm.cpp
intl.obj : intl.cpp
ipcbase.obj : ipcbase.cpp
layout.obj : layout.cpp
lboxcmn.obj : lboxcmn.cpp
list.obj : list.cpp
log.obj : log.cpp
longlong.obj : longlong.cpp
memory.obj : memory.cpp
menucmn.obj : menucmn.cpp
mimecmn.obj : mimecmn.cpp
module.obj : module.cpp
msgout.obj : msgout.cpp
mstream.obj : mstream.cpp
nbkbase.obj : nbkbase.cpp
object.obj : object.cpp
paper.obj : paper.cpp
platinfo.obj : platinfo.cpp
popupcmn.obj : popupcmn.cpp
prntbase.obj : prntbase.cpp
process.obj : process.cpp
protocol.obj : protocol.cpp
quantize.obj : quantize.cpp
radiocmn.obj : radiocmn.cpp
rendcmn.obj : rendcmn.cpp
rgncmn.obj : rgncmn.cpp
sckaddr.obj : sckaddr.cpp
sckfile.obj : sckfile.cpp
sckipc.obj : sckipc.cpp
sckstrm.obj : sckstrm.cpp
sizer.obj : sizer.cpp
socket.obj : socket.cpp
socketevtdispatch.obj : socketevtdispatch.cpp
settcmn.obj : settcmn.cpp
statbar.obj : statbar.cpp
stdpbase.obj : stdpbase.cpp
stockitem.obj : stockitem.cpp
stopwatch.obj : stopwatch.cpp
strconv.obj : strconv.cpp
stream.obj : stream.cpp
sysopt.obj : sysopt.cpp
string.obj : string.cpp
tbarbase.obj : tbarbase.cpp
textbuf.obj : textbuf.cpp
textcmn.obj : textcmn.cpp
textfile.obj : textfile.cpp
timercmn.obj : timercmn.cpp
tokenzr.obj : tokenzr.cpp
toplvcmn.obj : toplvcmn.cpp
treebase.obj : treebase.cpp
txtstrm.obj : txtstrm.cpp
url.obj : url.cpp
utilscmn.obj : utilscmn.cpp
valgen.obj : valgen.cpp
validate.obj : validate.cpp
valtext.obj : valtext.cpp
variant.obj : variant.cpp
wfstream.obj : wfstream.cpp
wincmn.obj : wincmn.cpp
wxchar.obj : wxchar.cpp
xpmdecod.obj : xpmdecod.cpp
zipstrm.obj : zipstrm.cpp
zstream.obj : zstream.cpp
accesscmn.obj : accesscmn.cpp
dndcmn.obj : dndcmn.cpp
dpycmn.obj : dpycmn.cpp
dseldlg.obj : dseldlg.cpp
dynload.obj : dynload.cpp
effects.obj : effects.cpp
fddlgcmn.obj : fddlgcmn.cpp
fs_mem.obj : fs_mem.cpp
gbsizer.obj : gbsizer.cpp
geometry.obj : geometry.cpp
matrix.obj : matrix.cpp
radiocmn.obj : radiocmn.cpp
regex.obj : regex.cpp
taskbarcmn.obj : taskbarcmn.cpp
xti.obj : xti.cpp
xtistrm.obj : xtistrm.cpp
xtixml.obj : xtixml.cpp
uri.obj : uri.cpp
dpycmn.obj : dpycmn.cpp
combocmn.obj : combocmn.cpp
clrpickercmn.obj : clrpickercmn.cpp
filepickercmn.obj : filepickercmn.cpp
fontpickercmn.obj : fontpickercmn.cpp
pickerbase.obj : pickerbase.cpp
listctrlcmn.obj : listctrlcmn.cpp
