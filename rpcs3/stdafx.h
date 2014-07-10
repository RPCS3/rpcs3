#pragma once

#ifdef MSVC_CRT_MEMLEAK_DETECTION
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX
#ifndef QT_UI
#include <wx/wxprec.h>
#include <wx/config.h>
#include <wx/string.h>
#include <wx/propdlg.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/dcclient.h>

#include <wx/wfstream.h>
#include <wx/dir.h>
#include <wx/spinctrl.h>
#include <wx/datetime.h>
#include <wx/filepicker.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include "wx/gauge.h"
#include <wx/stattext.h>
#include "wx/scrolbar.h"
#include "wx/frame.h"
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include "wx/app.h"
#include <wx/timer.h>
#include <wx/listctrl.h>
#include <wx/aui/auibook.h>
#endif

#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && !defined(DBG_NEW)
	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#define new DBG_NEW
#endif

// This header should be frontend-agnostic, so don't assume wx includes everything
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>

typedef unsigned int uint;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#include "Utilities/StrFmt.h"
#include "Utilities/Log.h"
#include "Utilities/BEType.h"
#include "Utilities/rFile.h"
#include "Utilities/rTime.h"
#include "Utilities/rXml.h"
#include "Utilities/rConcurrency.h"
#include "Utilities/rMsgBox.h"
#include "Utilities/rPlatform.h"
#include "Utilities/Thread.h"
#include "Utilities/Array.h"
#include "Utilities/Timer.h"
#include "Utilities/IdManager.h"

#include "Emu/SysCalls/Callback.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"

#include "Emu/FS/vfsDirBase.h"
#include "Emu/FS/vfsFileBase.h"
#include "Emu/FS/vfsLocalDir.h"
#include "Emu/FS/vfsLocalFile.h"
#include "Emu/FS/vfsStream.h"
#include "Emu/FS/vfsStreamMemory.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDir.h"

#define _PRGNAME_ "RPCS3"
#define _PRGVER_ "0.0.0.4"
