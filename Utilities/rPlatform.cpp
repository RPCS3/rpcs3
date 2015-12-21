#include "stdafx.h"
#include "restore_new.h"
#include <common/Log.h>
#pragma warning(push)
#pragma message("TODO: remove wx dependency: <wx/image.h>")
#pragma warning(disable : 4996)
#include <wx/image.h>
#pragma warning(pop)
#include "define_new_memleakdetect.h"

#ifndef _WIN32
#include <dirent.h>
#include <errno.h>
#endif

#include "rPlatform.h"
#include "wxfmt.h"

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
		throw EXCEPTION("unsupported type");
	}
}
