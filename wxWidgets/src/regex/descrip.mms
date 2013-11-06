#*****************************************************************************
#                                                                            *
# Make file for VMS                                                          *
# Author : J.Jansen (joukj@hrem.nano.tudelft.nl)                             *
# Date : 14 December 2010                                                    *
#                                                                            *
#*****************************************************************************
.first
	define wx [--.include.wx]

.ifdef __WXMOTIF__
CXX_DEFINE = /define=(__WXMOTIF__=1)/name=(as_is,short)\
	   /assume=(nostdnew,noglobal_array_new)/include=[]
CC_DEFINE = /define=(__WXMOTIF__=1)/name=(as_is,short)/include=[]
.else
.ifdef __WXGTK__
CXX_DEFINE = /define=(__WXGTK__=1)/float=ieee/name=(as_is,short)/ieee=denorm\
	   /assume=(nostdnew,noglobal_array_new)/include=[]
CC_DEFINE = /define=(__WXGTK__=1)/float=ieee/name=(as_is,short)/ieee=denorm\
	/include=[]
.else
.ifdef __WXGTK2__
CXX_DEFINE = /define=(__WXGTK__=1,VMS_GTK2=1)/float=ieee/name=(as_is,short)/ieee=denorm\
	   /assume=(nostdnew,noglobal_array_new)/include=[]
CC_DEFINE = /define=(__WXGTK__=1,VMS_GTK2=1)/float=ieee/name=(as_is,short)\
	/ieee=denorm/include=[]
.else
.ifdef __WXX11__
CXX_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)/assume=(nostdnew,noglobal_array_new)/include=[]
CC_DEFINE = /define=(__WXX11__=1,__WXUNIVERSAL__==1)/float=ieee\
	/name=(as_is,short)/include=[]
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

OBJECTS=regcomp.obj,regerror.obj,regexec.obj,regfree.obj,\
	regfronts.obj,tclUniData.obj

SOURCES=regcomp.c,regc_color.c,regc_cvec.c,regc_lex.c,regc_locale.c,\
	regc_nfa.c,regerror.c,regexec.c,rege_dfa.c,regfree.c,regfronts.c,\
	tclUniData.c

all : $(SOURCES)
	$(MMS)$(MMSQUALIFIERS) $(OBJECTS)
.ifdef __WXMOTIF__
	library [--.lib]libwx_motif.olb $(OBJECTS)
.else
.ifdef __WXGTK__
	library [--.lib]libwx_gtk.olb $(OBJECTS)
.else
.ifdef __WXGTK2__
	library [--.lib]libwx_gtk2.olb $(OBJECTS)
.else
.ifdef __WXX11__
	library [--.lib]libwx_x11_univ.olb $(OBJECTS)
.endif
.endif
.endif
.endif

$(OBJECTS) : [--.include.wx]setup.h

regcomp.obj : regcomp.c
regerror.obj : regerror.c
regexec.obj : regexec.c
regfree.obj : regfree.c
regfronts.obj : regfronts.c
tclUniData.obj : tclUniData.c
