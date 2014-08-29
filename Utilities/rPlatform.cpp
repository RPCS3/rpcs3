#include "stdafx.h"
#include <wx/image.h>

#ifndef _WIN32
#include <dirent.h>
#endif

#include "rPlatform.h"

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
		mkdir(dir.c_str(), 0777);
#endif
	}
	return dir;
}
