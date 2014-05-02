#pragma once
#include <vector>
#include <string>

//struct rGLContext;
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/Null/NullPadHandler.h"

struct rCanvas
{
	rCanvas(void *parent);
	rCanvas(const rCanvas &) = delete;
	~rCanvas();
	/*rGLContext*/void *GetCurrent();
	bool SetCurrent(/*rGLContext &*/ void *ctx);

	void *handle;
};

//struct rGLContext
//{
//	rGLContext();
//	rGLContext(rGLContext &) = delete;
//	rGLContext(rCanvas *canvas);
//	~rGLContext();
//
//	void *handle;
//};

//struct rFrame
//{
//	rFrame();
//	rFrame(rFrame &) = delete;
//	~rFrame();
//
//	void Close();
//	bool IsShown();
//	void Hide();
//	void Show();
//
//	void *handle;
//};

struct rGLFrame/*: public rFrame*/
{
	rGLFrame();
	rGLFrame(const rGLFrame &) = delete;
	~rGLFrame();

	void Close();
	bool IsShown();
	void Hide();
	void Show();

	void *handle;

	void SetCurrent( void *ctx);
	//rCanvas *GetCanvas();
	void *GetNewContext();
	void Flip(/*rGLContext*/void *ctx);
};

struct rPlatform
{
	rGLFrame *getGLGSFrame();
	static rPlatform &getPlatform();

	static int getKeyboardHandlerCount();
	static KeyboardHandlerBase *getKeyboardHandler(int i);
	static int getMouseHandlerCount();
	static MouseHandlerBase *getMouseHandler(int i);
	static int getPadHandlerCount();
	static PadHandlerBase *getPadHandler(int i);
};

/**********************************************************************
*********** RSX Debugger
************************************************************************/

struct RSXDebuggerProgram
{
	u32 id;
	u32 vp_id;
	u32 fp_id;
	std::string vp_shader;
	std::string fp_shader;
	bool modified;

	RSXDebuggerProgram()
		: modified(false)
	{
	}
};

extern std::vector<RSXDebuggerProgram> m_debug_programs;

/**********************************************************************
*********** Image stuff
************************************************************************/
enum rImageType
{
	rBITMAP_TYPE_PNG
};
struct rImage
{
	rImage();
	rImage(const rImage &) = delete;
	~rImage();
	void Create(int width , int height, void *data, void *alpha);
	void SaveFile(const std::string& name, rImageType type);

	void *handle;
};
