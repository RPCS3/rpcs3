#pragma once

class IsoDirectory 
{
public:
	SectorSource& internalReader;
	std::vector<IsoFileDescriptor> files;

public:
	IsoDirectory(SectorSource& r);
	IsoDirectory(SectorSource& r, IsoFileDescriptor directoryEntry);
	~IsoDirectory() throw();

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
