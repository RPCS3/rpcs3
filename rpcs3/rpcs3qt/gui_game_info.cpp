#include "stdafx.h"
#include "gui_game_info.h"
#include "localized.h"

std::string gui_game_info::GetGameVersion() const
{
	if (info.app_ver == Localized().category.unknown.toStdString())
	{
		// Fall back to Disc/Pkg Revision
		return info.version;
	}

	return info.app_ver;
}
