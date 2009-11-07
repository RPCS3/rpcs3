#pragma once

#include <vector>
#include <string>

#include "IsoFileDescriptor.h"
#include "IsoFile.h"

class IsoDirectory 
{
public:
	std::vector<IsoFileDescriptor> files;

	SectorSource* internalReader;

protected:
	void Init(SectorSource* r, IsoFileDescriptor directoryEntry);

public:

	// Used to load the Root directory from an image
	IsoDirectory(SectorSource* r);

	// Used to load a specific directory from a file descriptor
	IsoDirectory(SectorSource* r, IsoFileDescriptor directoryEntry);

	IsoFileDescriptor GetEntry(std::string fileName);
	IsoFileDescriptor GetEntry(int index);

	int GetIndexOf(std::string fileName);

	// Tool. For OpenFile.
	IsoFileDescriptor FindFile(std::string filePath);

	IsoFile OpenFile(std::string filePath);

	~IsoDirectory(void);
};
