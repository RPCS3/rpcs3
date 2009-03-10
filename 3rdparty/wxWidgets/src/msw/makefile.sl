#
# File:		makefile.wat
# Author:	Julian Smart
# Created:	1998
#
# Makefile : Builds wxWindows library for Salford C++, WIN32

include ..\makesl.env

LIBTARGET   = $(WXLIB)\wx.lib
EXTRATARGETS = # xpm png zlib
EXTRATARGETSCLEAN = # clean_xpm clean_png clean_zlib
GENDIR=$(WXDIR)\src\generic
COMMDIR=$(WXDIR)\src\common
XPMDIR=$(WXDIR)\src\xpm
OLEDIR=ole
MSWDIR=$(WXDIR)\src\msw

GENERICOBJS= choicdgg.obj \
  dirdlgg.obj \
  gridg.obj \
  laywin.obj \
  panelg.obj \
  prop.obj \
  progdlgg.obj \
  propform.obj \
  proplist.obj \
  sashwin.obj \
  scrolwin.obj \
  splitter.obj \
  statusbr.obj \
  tabg.obj \
  textdlgg.obj

# These are generic things that don't need to be compiled on MSW,
# but sometimes it's useful to do so for testing purposes.
NONESSENTIALOBJS= printps.obj \
  prntdlgg.obj \
  msgdlgg.obj \
  colrdlgg.obj \
  fontdlgg.obj

COMMONOBJS = cmndata.obj \
  config.obj \
  dcbase.obj \
  docview.obj \
  docmdi.obj \
  dynarray.obj \
  dynlib.obj \
  event.obj \
  file.obj \
  filefn.obj \
  fileconf.obj \
  framecmn.obj \
  gdicmn.obj \
  image.obj \
  intl.obj \
  ipcbase.obj \
  helpbase.obj \
  layout.obj \
  log.obj \
  memory.obj \
  module.obj \
  object.obj \
  prntbase.obj \
  resource.obj \
  tbarbase.obj \
  tbarsmpl.obj \
  textfile.obj \
  timercmn.obj \
  utilscmn.obj \
  validate.obj \
  valgen.obj \
  valtext.obj \
  date.obj \
  hash.obj \
  list.obj \
  paper.obj \
  string.obj \
  socket.obj \
  sckint.obj \
  sckaddr.obj \
  sckfile.obj \
  sckipc.obj \
  sckstrm.obj \
  url.obj \
  http.obj \
  protocol.obj \
  time.obj \
  tokenzr.obj \
  wxexpr.obj \
  y_tab.obj \
  extended.obj \
  process.obj \
  wfstream.obj \
  mstream.obj \
  zstream.obj \
  stream.obj \
  datstrm.obj \
  objstrm.obj \
  variant.obj \
  wincmn.obj \
  wxchar.obj

# Can't compile these yet under Salford C++
#  mimetype.obj \
#  db.obj \
#  dbtable.obj \

MSWOBJS = \
  accel.obj \
  app.obj \
  bitmap.obj \
  bmpbuttn.obj \
  brush.obj \
  button.obj \
  checkbox.obj \
  checklst.obj \
  caret.obj \
  choice.obj \
  clipbrd.obj \
  colordlg.obj \
  colour.obj \
  combobox.obj \
  control.obj \
  curico.obj \
  cursor.obj \
  data.obj \
  dc.obj \
  dcmemory.obj \
  dcclient.obj \
  dcprint.obj \
  dcscreen.obj \
  dde.obj \
  dialog.obj \
  dib.obj \
  dibutils.obj \
  filedlg.obj \
  font.obj \
  fontdlg.obj \
  frame.obj \
  gauge95.obj \
  gdiobj.obj \
  helpwin.obj \
  icon.obj \
  imaglist.obj \
  iniconf.obj \
  joystick.obj \
  listbox.obj \
  listctrl.obj \
  main.obj \
  mdi.obj \
  menu.obj \
  menuitem.obj \
  metafile.obj \
  minifram.obj \
  msgdlg.obj \
  nativdlg.obj \
  notebook.obj \
  ownerdrw.obj \
  palette.obj \
  pen.obj \
  penwin.obj \
  printdlg.obj \
  printwin.obj \
  radiobox.obj \
  radiobut.obj \
  region.obj \
  registry.obj \
  regconf.obj \
  scrolbar.obj \
  settings.obj \
  slidrmsw.obj \
  slider95.obj \
  spinbutt.obj \
  statbmp.obj \
  statbox.obj \
  statbr95.obj \
  stattext.obj \
  tabctrl.obj \
  taskbar.obj \
  tbar95.obj \
  tbarmsw.obj \
  textctrl.obj \
  thread.obj \
  timer.obj \
  tooltip.obj \
  treectrl.obj \
  utils.obj \
  utilsexc.obj \
  wave.obj \
  window.obj

# No OLE functions for wxDirDialog: use generic one instead
#  dirdlg.obj \
#  pnghand.obj \
#  xpmhand.obj \

OLEOBJS = \
  droptgt.obj \
  dropsrc.obj \
  dataobj.obj \
  oleutils.obj \
  uuid.obj \
  automtn.obj

# Add $(NONESSENTIALOBJS) if wanting generic dialogs, PostScript etc.
OBJECTS = $(COMMONOBJS) $(GENERICOBJS) $(MSWOBJS) # $(OLEOBJS)

all:        $(OBJECTS) $(LIBTARGET) $(EXTRATARGETS)

$(LIBTARGET) : $(OBJECTS)
    slink $$salford.lnk

clean:   $(EXTRATARGETSCLEAN)
    -erase *.obj
    -erase $(LIBTARGET)
    -erase *.pch
    -erase *.err

cleanall:   clean

test:   test.obj

test.obj:     test.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) test.cpp

accel.obj:     $(MSWDIR)\accel.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\accel.cpp /BINARY accel.obj

app.obj:     $(MSWDIR)\app.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\app.cpp /BINARY app.obj

bitmap.obj:     $(MSWDIR)\bitmap.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\bitmap.cpp /BINARY bitmap.obj

bmpbuttn.obj:     $(MSWDIR)\bmpbuttn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\bmpbuttn.cpp /BINARY bmpbuttn.obj

brush.obj:     $(MSWDIR)\brush.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\brush.cpp /BINARY brush.obj

button.obj:     $(MSWDIR)\button.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\button.cpp /BINARY button.obj

caret.obj:     $(MSWDIR)\caret.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\caret.cpp /BINARY caret.obj

choice.obj:     $(MSWDIR)\choice.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\choice.cpp /BINARY choice.obj

checkbox.obj:     $(MSWDIR)\checkbox.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\checkbox.cpp /BINARY checkbox.obj

checklst.obj:     $(MSWDIR)\checklst.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\checklst.cpp /BINARY checklst.obj

clipbrd.obj:     $(MSWDIR)\clipbrd.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\clipbrd.cpp /BINARY clipbrd.obj

colordlg.obj:     $(MSWDIR)\colordlg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\colordlg.cpp /BINARY colordlg.obj

colour.obj:     $(MSWDIR)\colour.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\colour.cpp /BINARY colour.obj

combobox.obj:     $(MSWDIR)\combobox.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\combobox.cpp /BINARY combobox.obj

control.obj:     $(MSWDIR)\control.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\control.cpp /BINARY control.obj

curico.obj:     $(MSWDIR)\curico.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\curico.cpp /BINARY curico.obj

cursor.obj:     $(MSWDIR)\cursor.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\cursor.cpp /BINARY cursor.obj

data.obj:     $(MSWDIR)\data.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\data.cpp /BINARY data.obj

dde.obj:     $(MSWDIR)\dde.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dde.cpp /BINARY dde.obj

dc.obj:     $(MSWDIR)\dc.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dc.cpp /BINARY dc.obj

dcmemory.obj:     $(MSWDIR)\dcmemory.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dcmemory.cpp /BINARY dcmemory.obj

dcclient.obj:     $(MSWDIR)\dcclient.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dcclient.cpp /BINARY dcclient.obj

dcprint.obj:     $(MSWDIR)\dcprint.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dcprint.cpp /BINARY dcprint.obj

dcscreen.obj:     $(MSWDIR)\dcscreen.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dcscreen.cpp /BINARY dcscreen.obj

dialog.obj:     $(MSWDIR)\dialog.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dialog.cpp /BINARY dialog.obj

dib.obj:     $(MSWDIR)\dib.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dib.cpp /BINARY dib.obj

dibutils.obj:     $(MSWDIR)\dibutils.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dibutils.cpp /BINARY dibutils.obj

dirdlg.obj:     $(MSWDIR)\dirdlg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\dirdlg.cpp /BINARY dirdlg.obj

filedlg.obj:     $(MSWDIR)\filedlg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\filedlg.cpp /BINARY filedlg.obj

font.obj:     $(MSWDIR)\font.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\font.cpp /BINARY font.obj

fontdlg.obj:     $(MSWDIR)\fontdlg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\fontdlg.cpp /BINARY fontdlg.obj

frame.obj:     $(MSWDIR)\frame.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\frame.cpp /BINARY frame.obj

gauge95.obj:     $(MSWDIR)\gauge95.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\gauge95.cpp /BINARY gauge95.obj

gdiobj.obj:     $(MSWDIR)\gdiobj.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\gdiobj.cpp /BINARY gdiobj.obj

helpwin.obj:     $(MSWDIR)\helpwin.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\helpwin.cpp /BINARY helpwin.obj

icon.obj:     $(MSWDIR)\icon.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\icon.cpp /BINARY icon.obj

imaglist.obj:     $(MSWDIR)\imaglist.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\imaglist.cpp /BINARY imaglist.obj

iniconf.obj:     $(MSWDIR)\iniconf.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\iniconf.cpp /BINARY iniconf.obj

joystick.obj:     $(MSWDIR)\joystick.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\joystick.cpp /BINARY joystick.obj

listbox.obj:     $(MSWDIR)\listbox.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\listbox.cpp /BINARY listbox.obj

listctrl.obj:     $(MSWDIR)\listctrl.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\listctrl.cpp /BINARY listctrl.obj

main.obj:     $(MSWDIR)\main.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\main.cpp /BINARY main.obj

mdi.obj:     $(MSWDIR)\mdi.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\mdi.cpp /BINARY mdi.obj

menu.obj:     $(MSWDIR)\menu.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\menu.cpp /BINARY menu.obj

menuitem.obj:     $(MSWDIR)\menuitem.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\menuitem.cpp /BINARY menuitem.obj

metafile.obj:     $(MSWDIR)\metafile.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\metafile.cpp /BINARY metafile.obj

minifram.obj:     $(MSWDIR)\minifram.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\minifram.cpp /BINARY minifram.obj

msgdlg.obj:     $(MSWDIR)\msgdlg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\msgdlg.cpp /BINARY msgdlg.obj

nativdlg.obj:     $(MSWDIR)\nativdlg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\nativdlg.cpp /BINARY nativdlg.obj

notebook.obj:     $(MSWDIR)\notebook.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\notebook.cpp /BINARY notebook.obj

ownerdrw.obj:     $(MSWDIR)\ownerdrw.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\ownerdrw.cpp /BINARY ownerdrw.obj

palette.obj:     $(MSWDIR)\palette.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\palette.cpp /BINARY palette.obj

pen.obj:     $(MSWDIR)\pen.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\pen.cpp /BINARY pen.obj

penwin.obj:     $(MSWDIR)\penwin.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\penwin.cpp /BINARY penwin.obj

printdlg.obj:     $(MSWDIR)\printdlg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\printdlg.cpp /BINARY printdlg.obj

printwin.obj:     $(MSWDIR)\printwin.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\printwin.cpp /BINARY printwin.obj

radiobox.obj:     $(MSWDIR)\radiobox.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\radiobox.cpp /BINARY radiobox.obj

radiobut.obj:     $(MSWDIR)\radiobut.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\radiobut.cpp /BINARY radiobut.obj

region.obj:     $(MSWDIR)\region.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\region.cpp /BINARY region.obj

registry.obj:     $(MSWDIR)\registry.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\registry.cpp /BINARY registry.obj

regconf.obj:     $(MSWDIR)\regconf.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\regconf.cpp /BINARY regconf.obj

scrolbar.obj:     $(MSWDIR)\scrolbar.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\scrolbar.cpp /BINARY scrolbar.obj

settings.obj:     $(MSWDIR)\settings.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\settings.cpp /BINARY settings.obj

slidrmsw.obj:     $(MSWDIR)\slidrmsw.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\slidrmsw.cpp /BINARY slidrmsw.obj

slider95.obj:     $(MSWDIR)\slider95.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\slider95.cpp /BINARY slider95.obj

spinbutt.obj:     $(MSWDIR)\spinbutt.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\spinbutt.cpp /BINARY spinbutt.obj

statbmp.obj:     $(MSWDIR)\statbmp.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\statbmp.cpp /BINARY statbmp.obj

statbox.obj:     $(MSWDIR)\statbox.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\statbox.cpp /BINARY statbox.obj

statbr95.obj:     $(MSWDIR)\statbr95.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\statbr95.cpp /BINARY statbr95.obj

stattext.obj:     $(MSWDIR)\stattext.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\stattext.cpp /BINARY stattext.obj

tabctrl.obj:     $(MSWDIR)\tabctrl.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\tabctrl.cpp /BINARY tabctrl.obj

taskbar.obj:     $(MSWDIR)\taskbar.cpp
        cl @<<
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\taskbar.cpp /BINARY taskbar.obj

tbar95.obj:     $(MSWDIR)\tbar95.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\tbar95.cpp /BINARY tbar95.obj

tbarmsw.obj:     $(MSWDIR)\tbarmsw.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\tbarmsw.cpp /BINARY tbarmsw.obj

textctrl.obj:     $(MSWDIR)\textctrl.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\textctrl.cpp /BINARY textctrl.obj

thread.obj:     $(MSWDIR)\thread.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\thread.cpp /BINARY thread.obj

timer.obj:     $(MSWDIR)\timer.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\timer.cpp /BINARY timer.obj

tooltip.obj:     $(MSWDIR)\tooltip.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\tooltip.cpp /BINARY tooltip.obj

treectrl.obj:     $(MSWDIR)\treectrl.cpp
        cl @<<
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\treectrl.cpp /BINARY treectrl.obj

utils.obj:     $(MSWDIR)\utils.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\utils.cpp /BINARY utils.obj

utilsexc.obj:     $(MSWDIR)\utilsexc.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\utilsexc.cpp /BINARY utilsexc.obj

wave.obj:     $(MSWDIR)\wave.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\wave.cpp /BINARY wave.obj

window.obj:     $(MSWDIR)\window.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\window.cpp /BINARY window.obj

xpmhand.obj:     $(MSWDIR)\xpmhand.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(MSWDIR)\xpmhand.cpp /BINARY xpmhand.obj

droptgt.obj:     $(OLEDIR)\droptgt.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(OLEDIR)\droptgt.cpp /BINARY droptgt.obj

dropsrc.obj:     $(OLEDIR)\dropsrc.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(OLEDIR)\dropsrc.cpp /BINARY dropsrc.obj

dataobj.obj:     $(OLEDIR)\dataobj.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(OLEDIR)\dataobj.cpp /BINARY dataobj.obj

oleutils.obj:     $(OLEDIR)\oleutils.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(OLEDIR)\oleutils.cpp /BINARY oleutils.obj

uuid.obj:     $(OLEDIR)\uuid.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(OLEDIR)\uuid.cpp /BINARY uuid.obj

automtn.obj:     $(OLEDIR)\automtn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(OLEDIR)\automtn.cpp /BINARY automtn.obj

########################################################
# Common objects (always compiled)

cmndata.obj:     $(COMMDIR)\cmndata.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\cmndata.cpp /BINARY cmndata.obj

config.obj:     $(COMMDIR)\config.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\config.cpp /BINARY config.obj

dcbase.obj:     $(COMMDIR)\dcbase.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\dcbase.cpp /BINARY dcbase.obj

db.obj:     $(COMMDIR)\db.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\db.cpp /BINARY db.obj

dbtable.obj:     $(COMMDIR)\dbtable.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\dbtable.cpp /BINARY dbtable.obj

docview.obj:     $(COMMDIR)\docview.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\docview.cpp /BINARY docview.obj

docmdi.obj:     $(COMMDIR)\docmdi.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\docmdi.cpp /BINARY docmdi.obj

dynarray.obj:     $(COMMDIR)\dynarray.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\dynarray.cpp /BINARY dynarray.obj

dynlib.obj:     $(COMMDIR)\dynlib.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\dynlib.cpp /BINARY dynlib.obj

event.obj:     $(COMMDIR)\event.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\event.cpp /BINARY event.obj

file.obj:     $(COMMDIR)\file.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\file.cpp /BINARY file.obj

fileconf.obj:     $(COMMDIR)\fileconf.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\fileconf.cpp /BINARY fileconf.obj

filefn.obj:     $(COMMDIR)\filefn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\filefn.cpp /BINARY filefn.obj

framecmn.obj:     $(COMMDIR)\framecmn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\framecmn.cpp /BINARY framecmn.obj

gdicmn.obj:     $(COMMDIR)\gdicmn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\gdicmn.cpp /BINARY gdicmn.obj

image.obj:     $(COMMDIR)\image.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\image.cpp /BINARY image.obj

intl.obj:     $(COMMDIR)\intl.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\intl.cpp /BINARY intl.obj

ipcbase.obj:     $(COMMDIR)\ipcbase.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\ipcbase.cpp /BINARY ipcbase.obj

helpbase.obj:     $(COMMDIR)\helpbase.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\helpbase.cpp /BINARY helpbase.obj

layout.obj:     $(COMMDIR)\layout.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\layout.cpp /BINARY layout.obj

log.obj:     $(COMMDIR)\log.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\log.cpp /BINARY log.obj

memory.obj:     $(COMMDIR)\memory.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\memory.cpp /BINARY memory.obj

mimetype.obj:     $(COMMDIR)\mimetype.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\mimetype.cpp /BINARY mimetype.obj

module.obj:     $(COMMDIR)\module.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\module.cpp /BINARY module.obj

object.obj:     $(COMMDIR)\object.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\object.cpp /BINARY object.obj

prntbase.obj:     $(COMMDIR)\prntbase.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\prntbase.cpp /BINARY prntbase.obj

resource.obj:     $(COMMDIR)\resource.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\resource.cpp /BINARY resource.obj

tbarbase.obj:     $(COMMDIR)\tbarbase.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\tbarbase.cpp /BINARY tbarbase.obj

tbarsmpl.obj:     $(COMMDIR)\tbarsmpl.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\tbarsmpl.cpp /BINARY tbarsmpl.obj

textfile.obj:     $(COMMDIR)\textfile.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\textfile.cpp /BINARY textfile.obj

timercmn.obj:     $(COMMDIR)\timercmn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\timercmn.cpp /BINARY timercmn.obj

utilscmn.obj:     $(COMMDIR)\utilscmn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\utilscmn.cpp /BINARY utilscmn.obj

validate.obj:     $(COMMDIR)\validate.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\validate.cpp /BINARY validate.obj

valgen.obj:     $(COMMDIR)\valgen.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\valgen.cpp /BINARY valgen.obj

valtext.obj:     $(COMMDIR)\valtext.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\valtext.cpp /BINARY valtext.obj

date.obj:     $(COMMDIR)\date.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\date.cpp /BINARY date.obj

wxexpr.obj:     $(COMMDIR)\wxexpr.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\wxexpr.cpp /BINARY wxexpr.obj

hash.obj:     $(COMMDIR)\hash.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\hash.cpp /BINARY hash.obj

list.obj:     $(COMMDIR)\list.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\list.cpp /BINARY list.obj

paper.obj:     $(COMMDIR)\paper.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\paper.cpp /BINARY paper.obj

string.obj:     $(COMMDIR)\string.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\string.cpp /BINARY string.obj

socket.obj:     $(COMMDIR)\socket.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\socket.cpp /BINARY socket.obj

sckint.obj:     $(COMMDIR)\sckint.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\sckint.cpp /BINARY sckint.obj

sckaddr.obj:     $(COMMDIR)\sckaddr.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\sckaddr.cpp /BINARY sckaddr.obj

sckfile.obj:     $(COMMDIR)\sckfile.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\sckfile.cpp /BINARY sckfile.obj

sckipc.obj:     $(COMMDIR)\sckipc.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\sckipc.cpp /BINARY sckipc.obj

sckstrm.obj:     $(COMMDIR)\sckstrm.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\sckstrm.cpp /BINARY sckstrm.obj

url.obj:     $(COMMDIR)\url.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\url.cpp /BINARY url.obj

http.obj:     $(COMMDIR)\http.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\http.cpp /BINARY http.obj

protocol.obj:     $(COMMDIR)\protocol.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\protocol.cpp /BINARY protocol.obj

tokenzr.obj:     $(COMMDIR)\tokenzr.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\tokenzr.cpp /BINARY tokenzr.obj

matrix.obj:     $(COMMDIR)\matrix.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\matrix.cpp /BINARY matrix.obj

time.obj:     $(COMMDIR)\time.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\time.cpp /BINARY time.obj

stream.obj:     $(COMMDIR)\stream.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\stream.cpp /BINARY stream.obj

wfstream.obj:     $(COMMDIR)\wfstream.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\wfstream.cpp /BINARY wfstream.obj

mstream.obj:     $(COMMDIR)\mstream.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\mstream.cpp /BINARY mstream.obj

zstream.obj:     $(COMMDIR)\zstream.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\zstream.cpp /BINARY zstream.obj

datstrm.obj:     $(COMMDIR)\datstrm.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\datstrm.cpp /BINARY datstrm.obj

objstrm.obj:     $(COMMDIR)\objstrm.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\objstrm.cpp /BINARY objstrm.obj

extended.obj:     $(COMMDIR)\extended.c
  $(CC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\extended.c /BINARY extended.obj

process.obj:     $(COMMDIR)\process.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\process.cpp /BINARY process.obj

variant.obj:     $(COMMDIR)\variant.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\variant.cpp /BINARY variant.obj

wincmn.obj:     $(COMMDIR)\wincmn.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\wincmn.cpp /BINARY wincmn.obj

wxchar.obj:     $(COMMDIR)\wxcharp.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(COMMDIR)\wxchar.cpp /BINARY wxchar.obj

########################################################
# Generic objects (not always compiled, depending on
# whether platforms have native implementations)

choicdgg.obj:     $(GENDIR)\choicdgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\choicdgg.cpp /BINARY choicdgg.obj

colrdlgg.obj:     $(GENDIR)\colrdgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\colrdgg.cpp /BINARY colordgg.obj

dirdlgg.obj:     $(GENDIR)\dirdlgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\dirdlgg.cpp /BINARY dirdlgg.obj

fontdlgg.obj:     $(GENDIR)\fontdlgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\fontdlgg.cpp /BINARY fontdlgg.obj

gridg.obj:     $(GENDIR)\gridg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\gridg.cpp /BINARY gridg.obj

laywin.obj:     $(GENDIR)\laywin.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\laywin.cpp /BINARY laywin.obj

msgdlgg.obj:     $(GENDIR)\msgdlgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\msgdlgg.cpp /BINARY msgdlgg.obj

panelg.obj:     $(GENDIR)\panelg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\panelg.cpp /BINARY panelg.obj

printps.obj:     $(GENDIR)\printps.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\printps.cpp /BINARY printps.obj

progdlgg.obj:     $(GENDIR)\progdlgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\progdlgg.cpp /BINARY progdlgg.obj

prop.obj:     $(GENDIR)\prop.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\prop.cpp /BINARY prop.obj

propform.obj:     $(GENDIR)\propform.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\propform.cpp /BINARY propform.obj

proplist.obj:     $(GENDIR)\proplist.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\proplist.cpp /BINARY proplist.obj

prntdlgg.obj:     $(GENDIR)\prntdlgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\prntdlgg.cpp /BINARY prntdlgg.obj

sashwin.obj:     $(GENDIR)\sashwin.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\sashwin.cpp /BINARY sashwin.obj

scrolwin.obj:     $(GENDIR)\scrolwin.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\scrolwin.cpp /BINARY scrolwin.obj

splitter.obj:     $(GENDIR)\splitter.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\splitter.cpp /BINARY splitter.obj

statusbr.obj:     $(GENDIR)\statusbr.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\statusbr.cpp /BINARY statusbr.obj

tabg.obj:     $(GENDIR)\tabg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\tabg.cpp /BINARY tabg.obj

textdlgg.obj: $(GENDIR)\textdlgg.cpp
  $(CCC) $(CPPFLAGS) $(IFLAGS) $(GENDIR)\textdlgg.cpp /BINARY textdlgg.obj

crbuffri.obj: $(XPMDIR)\crbuffri.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crbuffri.c

crbuffrp.obj: $(XPMDIR)\crbuffrp.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crbuffrp.c

crdatfri.obj: $(XPMDIR)\crdatfri.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crdatfri.c

crdatfrp.obj: $(XPMDIR)\crdatfrp.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crdatfrp.c

create.obj: $(XPMDIR)\create.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\create.c

crifrbuf.obj: $(XPMDIR)\crifrbuf.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crifrbuf.c

crifrdat.obj: $(XPMDIR)\crifrdat.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crifrdat.c

crpfrbuf.obj: $(XPMDIR)\crpfrbuf.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crpfrbuf.c

crpfrdat.obj: $(XPMDIR)\crpfrdat.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\crpfrdat.c

dataxpm.obj: $(XPMDIR)\data.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\data.c /BINARY dataxpm.obj

hashtab.obj: $(XPMDIR)\hashtab.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\hashtab.c

misc.obj: $(XPMDIR)\misc.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\misc.c

parse.obj: $(XPMDIR)\parse.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\parse.c

rdftodat.obj: $(XPMDIR)\rdftodat.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\rdftodat.c

rdftoi.obj: $(XPMDIR)\rdftoi.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\rdftoi.c

rdftop.obj: $(XPMDIR)\rdftop.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\rdftop.c

rgb.obj: $(XPMDIR)\rgb.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\rgb.c

scan.obj: $(XPMDIR)\scan.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\scan.c

simx.obj: $(XPMDIR)\simx.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\simx.c

wrffrdat.obj: $(XPMDIR)\wrffrdat.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\wrffrdat.c

wrffri.obj: $(XPMDIR)\wrffri.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\wrffri.c

wrffrp.obj: $(XPMDIR)\wrffrp.c
  *$(CC) $(CPPFLAGS) $(IFLAGS) $(XPMDIR)\wrffrp.c

OBJ1 = adler32$(O) compress$(O) crc32$(O) gzio$(O) uncompr$(O) deflate$(O) \
  trees$(O) 
OBJ2 = zutil$(O) inflate$(O) infblock$(O) inftrees$(O) infcodes$(O) \
  infutil$(O) inffast$(O) 

adler32.obj: adler32.c zutil.h zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c

compress.obj: compress.c zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c

crc32.obj: crc32.c zutil.h zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c

deflate.obj: deflate.c deflate.h zutil.h zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c

gzio.obj: gzio.c zutil.h zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c

infblock.obj: infblock.c zutil.h zlib.h zconf.h infblock.h inftrees.h\
   infcodes.h infutil.h
	$(CC) -c $(CFLAGS) $*.c

infcodes.obj: infcodes.c zutil.h zlib.h zconf.h inftrees.h infutil.h\
   infcodes.h inffast.h
	$(CC) -c $(CFLAGS) $*.c

inflate.obj: inflate.c zutil.h zlib.h zconf.h infblock.h
	$(CC) -c $(CFLAGS) $*.c

inftrees.obj: inftrees.c zutil.h zlib.h zconf.h inftrees.h
	$(CC) -c $(CFLAGS) $*.c

infutil.obj: infutil.c zutil.h zlib.h zconf.h inftrees.h infutil.h
	$(CC) -c $(CFLAGS) $*.c

inffast.obj: inffast.c zutil.h zlib.h zconf.h inftrees.h infutil.h inffast.h
	$(CC) -c $(CFLAGS) $*.c

trees.obj: trees.c deflate.h zutil.h zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c

uncompr.obj: uncompr.c zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c

zutil.obj: zutil.c zutil.h zlib.h zconf.h
	$(CC) -c $(CFLAGS) $*.c


y_tab.obj:     $(COMMDIR)\y_tab.c $(COMMDIR)\lex_yy.c
  $(CC) /ANSI_C $(CPPFLAGS) $(IFLAGS) /DEFINE USE_DEFINE $(COMMDIR)\y_tab.c /BINARY y_tab.obj

$(COMMDIR)\y_tab.c:     $(COMMDIR)\dosyacc.c
        copy $(COMMDIR)\dosyacc.c $(COMMDIR)\y_tab.c

$(COMMDIR)\lex_yy.c:    $(COMMDIR)\doslex.c
    copy $(COMMDIR)\doslex.c $(COMMDIR)\lex_yy.c

xpm:   
    cd $(WXDIR)\src\xpm
    wmake -f makefile.wat all
    cd $(WXDIR)\src\msw

clean_xpm:   
    cd $(WXDIR)\src\xpm
    wmake -f makefile.wat clean
    cd $(WXDIR)\src\msw

png:   
    cd $(WXDIR)\src\png
    wmake -f makefile.wat all
    cd $(WXDIR)\src\msw

clean_png:   
    cd $(WXDIR)\src\png
    wmake -f makefile.wat clean
    cd $(WXDIR)\src\msw

zlib:   
    cd $(WXDIR)\src\zlib
    wmake -f makefile.wat all
    cd $(WXDIR)\src\msw

clean_zlib:   
    cd $(WXDIR)\src\zlib
    wmake -f makefile.wat clean
    cd $(WXDIR)\src\msw


