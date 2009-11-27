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
