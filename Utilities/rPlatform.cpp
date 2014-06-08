#include "stdafx.h"

#include <wx/glcanvas.h>
#include "Gui/GLGSFrame.h"

#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Windows/WindowsKeyboardHandler.h"

#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/Windows/WindowsMouseHandler.h"

#include "Emu/Io/Null/NullPadHandler.h"
#include "Emu/Io/Windows/WindowsPadHandler.h"
#if defined(_WIN32)
#include "Emu/Io/XInput/XInputPadHandler.h"
#endif


rCanvas::rCanvas(void *parent)
{
	handle = static_cast<void*>(new wxGLCanvas(static_cast<wxWindow *>(parent),wxID_ANY,NULL));
}

rCanvas::~rCanvas()
{
	delete static_cast<wxGLCanvas*>(handle);
}

//void *rCanvas::GetCurrent()
//{
//	static_cast<wxGLCanvas*>(handle)->GetCur;
//}

bool rCanvas::SetCurrent(void *ctx)
{
	return static_cast<wxGLCanvas*>(handle)->SetCurrent(*static_cast<wxGLContext *>(ctx));
}




rGLFrame::rGLFrame()
{
	handle = static_cast<void*>(new GLGSFrame());
}

rGLFrame::~rGLFrame()
{
	delete static_cast<GLGSFrame*>(handle);
}

void rGLFrame::Close()
{
	static_cast<GLGSFrame*>(handle)->Close();
}

bool rGLFrame::IsShown()
{
	return static_cast<GLGSFrame*>(handle)->IsShown();
}

void rGLFrame::Hide()
{
	static_cast<GLGSFrame*>(handle)->Hide();
}

void rGLFrame::Show()
{
	static_cast<GLGSFrame*>(handle)->Show();
}


void *rGLFrame::GetNewContext()
{
	return static_cast<void *>(new wxGLContext(
		static_cast<GLGSFrame*>(handle)->GetCanvas()
		));
}

void rGLFrame::Flip(void *ctx)
{
	static_cast<GLGSFrame*>(handle)->Flip(
		static_cast<wxGLContext*>(ctx));
}

void rGLFrame::SetCurrent(void *ctx)
{
	static_cast<GLGSFrame*>(handle)->GetCanvas()->SetCurrent(*static_cast<wxGLContext*>(ctx));
}


rImage::rImage()
{
	handle = static_cast<void*>(new wxImage());
}

rImage::~rImage()
{
	delete static_cast<wxImage*>(handle);
}

void rImage::Create(int width, int height, void *data, void *alpha)
{
	static_cast<wxImage*>(handle)->Create(width, height, static_cast<unsigned char*>(data), static_cast<unsigned char*>(alpha));
}
void rImage::SaveFile(const std::string& name, rImageType type)
{
	if (type == rBITMAP_TYPE_PNG)
	{
		static_cast<wxImage*>(handle)->SaveFile(fmt::FromUTF8(name),wxBITMAP_TYPE_PNG);
	}
	else
	{
		throw std::string("unsupported type");
	}
}

int rPlatform::getKeyboardHandlerCount()
{
	return 2;
}

KeyboardHandlerBase *rPlatform::getKeyboardHandler(int i)
{
	switch (i)
	{
		case 0:
			return new NullKeyboardHandler();
			break;
		case 1:
			return new WindowsKeyboardHandler();
			break;
		default:
			return new NullKeyboardHandler();
	}
}

int rPlatform::getMouseHandlerCount()
{
	return 2;
}


MouseHandlerBase *rPlatform::getMouseHandler(int i)
{
	switch (i)
	{
	case 0:
		return new NullMouseHandler();
		break;
	case 1:
		return new WindowsMouseHandler();
		break;
	default:
		return new NullMouseHandler();
	}
}

int rPlatform::getPadHandlerCount()
{
#if defined(_WIN32)
	return 3;
#else
	return 2;
#endif
}


PadHandlerBase *rPlatform::getPadHandler(int i)
{
	switch (i)
	{
	case 0:
		return new NullPadHandler();
		break;
	case 1:
		return new WindowsPadHandler();
		break;
#if defined(_WIN32)
	case 2:
		return new XInputPadHandler();
		break;
#endif
	default:
		return new NullPadHandler();
	}
}