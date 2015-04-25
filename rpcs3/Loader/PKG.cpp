#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rMsgBox.h"
#include "Utilities/File.h"
#include "PKG.h"
#include "../Crypto/unpkg.h"

bool PKGLoader::Install(const fs::file& pkg_f, std::string dest)
{
	// Initial checks
	if (!pkg_f)
	{
		return false;
	}

	// TODO: This shouldn't use current dir
	dest.insert(0, 1, '.');
	if (!dest.empty() && dest.back() != '/')
	{
		dest += '/';
	}

	// Fetch title ID from the header.
	char title_id[48];
	pkg_f.seek(48);
	pkg_f.read(title_id, 48);
	
	std::string titleID = std::string(title_id).substr(7, 9);

	if (fs::is_dir(dest + titleID))
	{
		if (rMessageDialog(NULL, "Another installation found. Do you want to overwrite it?", "PKG Decrypter / Installer", rYES_NO | rCENTRE).ShowModal() != rID_YES)
		{
			LOG_ERROR(LOADER, "PKG Loader: Another installation found in: %s", titleID.c_str());
			return false;
		}
	}
	else if (!fs::create_dir(dest + titleID))
	{
		LOG_ERROR(LOADER, "PKG Loader: Could not create the installation directory: %s", titleID.c_str());
		return false;
	}

	// Decrypt and unpack the PKG file.
	if (Unpack(pkg_f, titleID, dest) < 0)
	{
		LOG_ERROR(LOADER, "PKG Loader: Failed to install package!");
		return false;
	}
	else
	{
		LOG_NOTICE(LOADER, "PKG Loader: Package successfully installed in: /dev_hdd0/game/%s", titleID.c_str());
		return true;
	}
}
