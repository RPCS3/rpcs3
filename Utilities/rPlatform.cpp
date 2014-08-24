#include "stdafx.h"

#include <wx/glcanvas.h>
#include "Gui/GLGSFrame.h"

#ifndef _WIN32
#include <dirent.h>
#endif

#include "rPlatform.h"

rCanvas::rCanvas(void *parent)
{
	handle = static_cast<void*>(new wxGLCanvas(static_cast<wxWindow *>(parent),wxID_ANY,NULL));
}

rCanvas::~rCanvas()
{
	delete static_cast<wxGLCanvas*>(handle);
}

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

std::string rPlatform::getConfigDir()
{
	static std::string dir = ".";
	if (dir == ".") {
#ifdef _WIN32
		dir = "";
		//mkdir(dir.c_str());
#else
		if (getenv("XDG_CONFIG_HOME") != NULL)
			dir = getenv("XDG_CONFIG_HOME");
		else if (getenv("HOME") != NULL)
			dir = getenv("HOME") + std::string("/.config");
		else // Just in case
			dir = "./config";
		dir = dir + "/rpcs3/";
		mkdir(dir.c_str());
#endif
	}
	return dir;
}
