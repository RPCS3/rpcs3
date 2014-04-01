#include "stdafx.h"
#include "PKG.h"
#include "../Crypto/unpkg.h"

PKGLoader::PKGLoader(wxFile& f) : pkg_f(f)
{
}

bool PKGLoader::Install(std::string dest)
{
	// Initial checks
	if (!pkg_f.IsOpened())
		return false;

	dest = fmt::ToUTF8(wxGetCwd()) + dest;
	if (!dest.empty() && dest.back() != '/')
		dest += '/';

	// Fetch title ID from the header.
	char title_id[48];
	pkg_f.Seek(48);
	pkg_f.Read(title_id, 48);
	
	std::string titleID = std::string(title_id).substr(7, 9);

	if (wxDirExists(fmt::FromUTF8(dest+titleID))) {
		wxMessageDialog d_overwrite(NULL, "Another installation was found. Do you want to overwrite it?", "PKG Decrypter / Installer", wxYES_NO|wxCENTRE);
		if (d_overwrite.ShowModal() != wxID_YES) {
			ConLog.Error("PKG Loader: Another installation found in: %s", titleID.c_str());
			return false;
		}
		// TODO: Remove the following two lines and remove the folder dest+titleID
		ConLog.Error("PKG Loader: Another installation found in: %s", titleID.c_str());
		return false;
	}
	if (!wxMkdir(fmt::FromUTF8(dest+titleID))) {
		ConLog.Error("PKG Loader: Could not make the installation directory: %s", titleID.c_str());
		return false;
	}

	// Decrypt and unpack the PKG file.
	if (Unpack(pkg_f, titleID, dest) < 0)
	{
		ConLog.Error("PKG Loader: Failed to install package!");
		return false;
	}
	else
	{
		ConLog.Write("PKG Loader: Package successfully installed in: /dev_hdd0/game/%s", titleID.c_str());
		return true;
	}
}

bool PKGLoader::Close()
{
	return pkg_f.Close();
}