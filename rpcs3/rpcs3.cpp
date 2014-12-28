#include "stdafx_gui.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rpcs3.h"
#include "Ini.h"
#include "Utilities/Log.h"
#include "Gui/ConLogFrame.h"
#include "Emu/GameInfo.h"

#include "Emu/Io/Keyboard.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Windows/WindowsKeyboardHandler.h"

#include "Emu/Io/Mouse.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/Windows/WindowsMouseHandler.h"

#include "Emu/Io/Pad.h"
#include "Emu/Io/Null/NullPadHandler.h"
#include "Emu/Io/Windows/WindowsPadHandler.h"
#if defined(_WIN32)
#include "Emu/Io/XInput/XInputPadHandler.h"
#endif

#include "Emu/SysCalls/Modules/cellMsgDialog.h"
#include "Gui/MsgDialog.h"

#include "Gui/GLGSFrame.h"
#include <wx/stdpaths.h>

#ifdef _WIN32
#include <wx/msw/wrapwin.h>
#endif

#ifdef __unix__
#include <X11/Xlib.h>
#endif

wxDEFINE_EVENT(wxEVT_DBG_COMMAND, wxCommandEvent);

IMPLEMENT_APP(Rpcs3App)
Rpcs3App* TheApp;

std::string simplify_path(const std::string& path, bool is_dir);

typedef be_t<uint> CGprofile;
typedef int CGbool;
typedef be_t<uint> CGresource;
typedef be_t<uint> CGenum;
typedef be_t<uint> CGtype;

typedef be_t<unsigned int>              CgBinaryOffset;
typedef CgBinaryOffset                  CgBinaryEmbeddedConstantOffset;
typedef CgBinaryOffset                  CgBinaryFloatOffset;
typedef CgBinaryOffset                  CgBinaryStringOffset;
typedef CgBinaryOffset                  CgBinaryParameterOffset;

// a few typedefs
typedef struct CgBinaryParameter        CgBinaryParameter;
typedef struct CgBinaryEmbeddedConstant CgBinaryEmbeddedConstant;
typedef struct CgBinaryVertexProgram    CgBinaryVertexProgram;
typedef struct CgBinaryFragmentProgram  CgBinaryFragmentProgram;
typedef struct CgBinaryProgram          CgBinaryProgram;

// fragment programs have their constants embedded in the microcode
struct CgBinaryEmbeddedConstant
{
	be_t<unsigned int> ucodeCount;       // occurances
	be_t<unsigned int> ucodeOffset[1];   // offsets that need to be patched follow
};

// describe a binary program parameter (CgParameter is opaque)
struct CgBinaryParameter
{
	CGtype                          type;          // cgGetParameterType()
	CGresource                      res;           // cgGetParameterResource()
	CGenum                          var;           // cgGetParameterVariability()
	be_t<int>                       resIndex;      // cgGetParameterResourceIndex()
	CgBinaryStringOffset            name;          // cgGetParameterName()
	CgBinaryFloatOffset             defaultValue;  // default constant value
	CgBinaryEmbeddedConstantOffset  embeddedConst; // embedded constant information
	CgBinaryStringOffset            semantic;      // cgGetParameterSemantic()
	CGenum                          direction;     // cgGetParameterDirection()
	be_t<int>                       paramno;       // 0..n: cgGetParameterIndex() -1: globals
	CGbool                          isReferenced;  // cgIsParameterReferenced()
	CGbool                          isShared;	   // cgIsParameterShared()
};

// attributes needed for vshaders
struct CgBinaryVertexProgram
{
	be_t<unsigned int>  instructionCount;         // #instructions
	be_t<unsigned int>  instructionSlot;          // load address (indexed reads!)
	be_t<unsigned int>  registerCount;            // R registers count
	be_t<unsigned int>  attributeInputMask;       // attributes vs reads from
	be_t<unsigned int>  attributeOutputMask;      // attributes vs writes (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
	be_t<unsigned int>  userClipMask;             // user clip plane enables (for SET_USER_CLIP_PLANE_CONTROL)
};

typedef enum {
	CgBinaryPTTNone = 0,
	CgBinaryPTT2x16 = 1,
	CgBinaryPTT1x32 = 2,
} CgBinaryPartialTexType;

// attributes needed for pshaders
struct CgBinaryFragmentProgram
{
	be_t<unsigned int>  instructionCount;         // #instructions
	be_t<unsigned int>  attributeInputMask;       // attributes fp reads (uses SET_VERTEX_ATTRIB_OUTPUT_MASK bits)
	be_t<unsigned int>  partialTexType;           // texid 0..15 use two bits each marking whether the texture format requires partial load: see CgBinaryPartialTexType
	be_t<unsigned short> texCoordsInputMask;      // tex coords used by frag prog. (tex<n> is bit n)
	be_t<unsigned short> texCoords2D;             // tex coords that are 2d        (tex<n> is bit n)
	be_t<unsigned short> texCoordsCentroid;       // tex coords that are centroid  (tex<n> is bit n)
	unsigned char registerCount;            // R registers count
	unsigned char outputFromH0;             // final color from R0 or H0
	unsigned char depthReplace;             // fp generated z epth value
	unsigned char pixelKill;                // fp uses kill operations
};

#include "Emu/RSX/GL/GLFragmentProgram.h"
#include "Emu/RSX/GL/GLVertexProgram.h"
// defines a binary program -- *all* address/offsets are relative to the begining of CgBinaryProgram
struct CgBinaryProgram
{
	// vertex/pixel shader identification (BE/LE as well)
	CGprofile profile;

	// binary revision (used to verify binary and driver structs match)
	be_t<unsigned int> binaryFormatRevision;

	// total size of this struct including profile and totalSize field!
	be_t<unsigned int> totalSize;

	// parameter usually queried using cgGet[First/Next]LeafParameter
	be_t<unsigned int> parameterCount;
	CgBinaryParameterOffset parameterArray;

	// depending on profile points to a CgBinaryVertexProgram or CgBinaryFragmentProgram struct
	CgBinaryOffset program;

	// raw ucode data
	be_t<unsigned int>    ucodeSize;
	CgBinaryOffset  ucode;

	// variable length data follows
	unsigned char data[1];
};

void compile_shader(std::string path)
{
	ShaderVar var("r0.yz.x");
	var.symplify();
	LOG_ERROR(GENERAL, var.get().c_str());

	u32 ptr;
	{
		wxFile f(path);

		if (!f.IsOpened())
			return;

		size_t size = f.Length();
		vm::ps3::init();
		ptr = vm::alloc(size);
		f.Read(vm::get_ptr(ptr), size);
		f.Close();
	}

	CgBinaryProgram& prog = vm::get_ref<CgBinaryProgram>(ptr);
	LOG_ERROR(GENERAL, "%d - %x", (u32)prog.profile, (u32)prog.binaryFormatRevision);

	std::string shader;
	GLParamArray param_array;
	u32 size;

	if (prog.profile == 7004)
	{
		CgBinaryFragmentProgram& fprog = vm::get_ref<CgBinaryFragmentProgram>(ptr + prog.program);

		u32 ctrl = (fprog.outputFromH0 ? 0 : 0x40) | (fprog.depthReplace ? 0xe : 0);

		GLFragmentDecompilerThread(shader, param_array, ptr + prog.ucode, size, ctrl).Task();
	}
	else
	{
		CgBinaryVertexProgram& vprog = vm::get_ref<CgBinaryVertexProgram>(ptr + prog.program);

		std::vector<u32> data;
		be_t<u32>* vdata = vm::get_ptr<be_t<u32>>(ptr + prog.ucode);
		for (u32 i = 0; i < prog.ucodeSize; ++i, ++vdata)
		{
			data.push_back(vdata[i]);
		}

		GLVertexDecompilerThread(data, shader, param_array).Task();
	}

	LOG_ERROR(GENERAL, shader.c_str());
	vm::close();
}

bool Rpcs3App::OnInit()
{
	SetSendDbgCommandCallback([](DbgCommand id, CPUThread* t)
	{
		wxGetApp().SendDbgCommand(id, t);
	});

	SetCallAfterCallback([](std::function<void()> func)
	{
		wxGetApp().CallAfter(func);
	});

	SetGetKeyboardHandlerCountCallback([]()
	{
		return 2;
	});

	SetGetKeyboardHandlerCallback([](int i) -> KeyboardHandlerBase*
	{
		switch (i)
		{
		case 0: return new NullKeyboardHandler();
		case 1: return new WindowsKeyboardHandler();
		}

		assert(!"Invalid keyboard handler number");
		return new NullKeyboardHandler();
	});

	SetGetMouseHandlerCountCallback([]()
	{
		return 2;
	});

	SetGetMouseHandlerCallback([](int i) -> MouseHandlerBase*
	{
		switch (i)
		{
		case 0: return new NullMouseHandler();
		case 1: return new WindowsMouseHandler();
		}

		assert(!"Invalid mouse handler number");
		return new NullMouseHandler();
	});

	SetGetPadHandlerCountCallback([]()
	{
#if defined(_WIN32)
		return 3;
#else
		return 2;
#endif
	});

	SetGetPadHandlerCallback([](int i) -> PadHandlerBase*
	{
		switch (i)
		{
		case 0: return new NullPadHandler();
		case 1: return new WindowsPadHandler();
#if defined(_WIN32)
		case 2: return new XInputPadHandler();
#endif
		}

		assert(!"Invalid pad handler number");
		return new NullPadHandler();
	});

	SetGetGSFrameCallback([]() -> GSFrameBase*
	{
		return new GLGSFrame();
	});

	SetMsgDialogCallbacks(MsgDialogCreate, MsgDialogDestroy, MsgDialogProgressBarSetMsg, MsgDialogProgressBarReset, MsgDialogProgressBarInc);

	TheApp = this;
	SetAppName(_PRGNAME_);
	wxInitAllImageHandlers();

	// RPCS3 assumes the current working directory is the folder where it is contained, so we make sure this is true
	const wxString executablePath = wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
	wxSetWorkingDirectory(executablePath);

	main_thread = std::this_thread::get_id();

	Ini.Load();
	Emu.Init();
	Emu.SetEmulatorPath(executablePath.ToStdString());

	m_MainFrame = new MainFrame();
	SetTopWindow(m_MainFrame);
	m_MainFrame->Show();
	m_MainFrame->DoSettings(true);

	OnArguments();

	//compile_shader("compile_shader0.spo");
	//compile_shader("compile_shader1.spo");

	return true;
}

void Rpcs3App::OnArguments()
{
	// Usage:
	//   rpcs3-*.exe               Initializes RPCS3
	//   rpcs3-*.exe [(S)ELF]      Initializes RPCS3, then loads and runs the specified (S)ELF file.

	if (Rpcs3App::argc > 1) {
		Emu.SetPath(fmt::ToUTF8(argv[1]));
		Emu.Load();
		Emu.Run();
	}
}

void Rpcs3App::Exit()
{
	Emu.Stop();
	Ini.Save();

	wxApp::Exit();

#ifdef _WIN32
	timeEndPeriod(1);
#endif
}

void Rpcs3App::SendDbgCommand(DbgCommand id, CPUThread* thr)
{
	wxCommandEvent event(wxEVT_DBG_COMMAND, id);
	event.SetClientData(thr);
	AddPendingEvent(event);
}

Rpcs3App::Rpcs3App()
{
#ifdef _WIN32
	timeBeginPeriod(1);
#endif

#if defined(__unix__) && !defined(__APPLE__)
	XInitThreads();
#endif
}

GameInfo CurGameInfo;
