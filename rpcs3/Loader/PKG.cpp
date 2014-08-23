#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rMsgBox.h"
#include "PKG.h"
#include "../Crypto/unpkg.h"

PKGLoader::PKGLoader(rFile& f) : pkg_f(f)
{
}

bool PKGLoader::Install(std::string dest)
{
	// Initial checks
	if (!pkg_f.IsOpened())
		return false;

	// TODO: This shouldn't use current dir
	dest.insert(0, 1, '.');
	if (!dest.empty() && dest.back() != '/')
		dest += '/';

	// Fetch title ID from the header.
	char title_id[48];
	pkg_f.Seek(48);
	pkg_f.Read(title_id, 48);
	
	std::string titleID = std::string(title_id).substr(7, 9);

	if (rExists(dest + titleID)) {
		rMessageDialog d_overwrite(NULL, "Another installation was found. Do you want to overwrite it?", "PKG Decrypter / Installer", rYES_NO|rCENTRE);
		if (d_overwrite.ShowModal() != rID_YES) {
			LOG_ERROR(LOADER, "PKG Loader: Another installation found in: %s", titleID.c_str());
			return false;
		}
		
		rRmdir(dest + titleID);
	}
	if (!rMkdir(dest + titleID)) {
		LOG_ERROR(LOADER, "PKG Loader: Could not make the installation directory: %s", titleID.c_str());
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

bool PKGLoader::Close()
{
	return pkg_f.Close();
}
