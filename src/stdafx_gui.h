#pragma once

#include "restore_new.h"

#ifndef QT_UI

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/config.h>
#include <wx/string.h>
#include <wx/propdlg.h>
#include <wx/dcclient.h>

#include <wx/wfstream.h>
#include <wx/spinctrl.h>
#include <wx/datetime.h>
#include <wx/filepicker.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/scrolbar.h>
#include <wx/frame.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/clipbrd.h>
#include <wx/notebook.h>
#include <wx/radiobox.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>
#include <wx/dataobj.h>
#include <wx/imaglist.h>
#include <wx/log.h>
#include <wx/dynlib.h>
#include <wx/progdlg.h>
#include <wx/busyinfo.h>
#include <wx/dnd.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

#include "define_new_memleakdetect.h"
