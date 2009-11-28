/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

enum IsoFS_Type
{
	FStype_ISO9660	= 1,
	FStype_Joliet	= 2,
};

class IsoDirectory 
{
public:
	SectorSource&					internalReader;
	std::vector<IsoFileDescriptor>	files;
	IsoFS_Type						m_fstype;

public:
	IsoDirectory(SectorSource& r);
	IsoDirectory(SectorSource& r, IsoFileDescriptor directoryEntry);
	virtual ~IsoDirectory() throw();

	wxString FStype_ToString() const;
	SectorSource& GetReader() const { return internalReader; }

	bool Exists(const wxString& filePath) const;
	bool IsFile(const wxString& filePath) const;
	bool IsDir(const wxString& filePath) const;
	
	u32 GetFileSize( const wxString& filePath ) const;

	IsoFileDescriptor FindFile(const wxString& filePath) const;
	
protected:
	const IsoFileDescriptor& GetEntry(const wxString& fileName) const;
	const IsoFileDescriptor& GetEntry(int index) const;

	void Init(const IsoFileDescriptor& directoryEntry);
	int GetIndexOf(const wxString& fileName) const;
};
