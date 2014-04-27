/*  PCSX2 - PS2 Emulator for PCs
*  Copyright (C) 2002-2014  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "AsyncFileReader.h"

#include "zlib_indexed.c"

/////////// Some complementary utilities for zlib_indexed.c //////////

#include <fstream>

static s64 fsize(const wxString& filename) {
	if (!wxFileName::FileExists(filename))
		return -1;

	std::ifstream f(filename.ToUTF8(), std::ifstream::binary);
	f.seekg(0, f.end);
	s64 size = f.tellg();
	f.close();

	return size;
}

#define GZIP_ID "PCSX2.index.gzip.v1|"
#define GZIP_ID_LEN (sizeof(GZIP_ID) - 1)	/* sizeof includes the \0 terminator */

// File format is:
// - [GZIP_ID_LEN] GZIP_ID (no \0)
// - [sizeof(Access)] index (should be allocated, contains various sizes)
// - [rest] the indexed data points (should be allocated, index->list should then point to it)
static Access* ReadIndexFromFile(const wxString& filename) {
	s64 size = fsize(filename);
	if (size <= 0) {
		Console.Error("Error: Can't open index file: '%s'", filename.To8BitData());
		return 0;
	}
	std::ifstream infile(filename.ToUTF8(), std::ifstream::binary);

	char fileId[GZIP_ID_LEN + 1] = { 0 };
	infile.read(fileId, GZIP_ID_LEN);
	if (wxString::From8BitData(GZIP_ID) != wxString::From8BitData(fileId)) {
		Console.Error("Error: Incompatible gzip index, please delete it manually: '%s'", filename.To8BitData());
		infile.close();
		return 0;
	}

	Access* index = (Access*)malloc(sizeof(Access));
	infile.read((char*)index, sizeof(Access));

	s64 datasize = size - GZIP_ID_LEN - sizeof(Access);
	if (datasize != index->have * sizeof(Point)) {
		Console.Error("Error: unexpected size of gzip index, please delete it manually: '%s'.", filename.To8BitData());
		infile.close();
		free(index);
		return 0;
	}

	char* buffer = (char*)malloc(datasize);
	infile.read(buffer, datasize);
	infile.close();
	index->list = (Point*)buffer; // adjust list pointer
	return index;
}

static void WriteIndexToFile(Access* index, const wxString filename) {
	if (wxFileName::FileExists(filename)) {
		Console.Warning("WARNING: Won't write index - file name exists (please delete it manually): '%s'", filename.To8BitData());
		return;
	}

	std::ofstream outfile(filename.ToUTF8(), std::ofstream::binary);
	outfile.write(GZIP_ID, GZIP_ID_LEN);

	Point* tmp = index->list;
	index->list = 0; // current pointer is useless on disk, normalize it as 0.
	outfile.write((char*)index, sizeof(Access));
	index->list = tmp;

	outfile.write((char*)index->list, sizeof(Point) * index->have);
	outfile.close();

	// Verify
	if (fsize(filename) != (s64)GZIP_ID_LEN + sizeof(Access) + sizeof(Point) * index->have) {
		Console.Warning("Warning: Can't write index file to disk: '%s'", filename.To8BitData());
	} else {
		Console.WriteLn(Color_Green, "OK: Gzip quick access index file saved to disk: '%s'", filename.To8BitData());
	}
}

/////////// End of complementary utilities for zlib_indexed.c //////////


static wxString iso2indexname(const wxString& isoname) {
	return isoname + L".pindex.tmp";
}

static void WarnOldIndex(const wxString& filename) {
	wxString oldName = filename + L".pcsx2.index.tmp";
	if (wxFileName::FileExists(oldName)) {
		Console.Warning("Note: Unused old index detected, please delete it manually: '%s'", oldName.To8BitData());
	}
}


class GzippedFileReader : public AsyncFileReader
{
	DeclareNoncopyableObject(GzippedFileReader);
public:
	GzippedFileReader(void) :
		m_pIndex(0)
	{	m_blocksize = 2048; };

	virtual ~GzippedFileReader(void) { Close(); };

	static  bool CanHandle(const wxString& fileName);
	virtual bool Open(const wxString& fileName);

	virtual int ReadSync(void* pBuffer, uint sector, uint count);

	virtual void BeginRead(void* pBuffer, uint sector, uint count);
	virtual int FinishRead(void);
	virtual void CancelRead(void) {};

	virtual void Close(void);

	virtual uint GetBlockCount(void) const {
		// type and formula copied from FlatFileReader
		// FIXME? : Shouldn't it be uint and (size - m_dataoffset) / m_blocksize ?
		return (int)((m_pIndex ? m_pIndex->uncompressed_size : 0) / m_blocksize);
	};

	// Same as FlatFileReader, but in case it changes
	virtual void SetBlockSize(uint bytes) { m_blocksize = bytes; }
	virtual void SetDataOffset(uint bytes) { m_dataoffset = bytes; }
private:
	bool	OkIndex();  // Verifies thatt we have an index, or try to create one
	int		mBytesRead; // Temp sync read result when simulating async read
	Access* m_pIndex;   // Quick access index
};


// TODO: do better than just checking existance and extension
bool GzippedFileReader::CanHandle(const wxString& fileName) {
	return wxFileName::FileExists(fileName) && fileName.Lower().EndsWith(L".gz");
}


#define SPAN_DEFAULT (1048576L * 2)   /* distance between access points when creating a new index */

bool GzippedFileReader::OkIndex() {
	if (m_pIndex)
		return true;

	// Try to read index from disk
	WarnOldIndex(m_filename);
	wxString indexfile = iso2indexname(m_filename);

	if (wxFileName::FileExists(indexfile) && (m_pIndex = ReadIndexFromFile(indexfile))) {
		Console.WriteLn(Color_Green, "OK: Gzip quick access index read from disk: '%s'", indexfile.To8BitData());
		if (m_pIndex->span != SPAN_DEFAULT) {
			Console.Warning("Note: This index has %1.1f MB intervals, while the current default for new indexes is %1.1f MB.", (float)m_pIndex->span / 1024 / 1024, (float)SPAN_DEFAULT / 1024 / 1024);
			Console.Warning("It will work fine, but if you want to generate a new index with default intervals, delete this index file.");
			Console.Warning("(smaller intervals mean bigger index file and quicker but more frequent decompressions)");
		}
		return true;
	}

	// No valid index file. Generate an index
	Console.Warning("This may take a while (but only once). Scanning compressed file to generate a quick access index...");

	Access *index;
	FILE* infile = fopen(m_filename.ToUTF8(), "rb");
	int len = build_index(infile, SPAN_DEFAULT, &index);
	printf("\n"); // build_index prints progress without \n's
	fclose(infile);

	if (len >= 0) {
		m_pIndex = index;
		WriteIndexToFile((Access*)m_pIndex, indexfile);
	} else {
		Console.Error("ERROR (%d): index could not be generated for file '%s'", len, m_filename.To8BitData());
		return false;
	}

	return true;
}

bool GzippedFileReader::Open(const wxString& fileName) {
	Close();
	m_filename = fileName;
	if (!CanHandle(fileName) || !OkIndex()) {
		Close();
		return false;
	};

	return true;
};

void GzippedFileReader::BeginRead(void* pBuffer, uint sector, uint count) {
	// No a-sync support yet, implement as sync
	mBytesRead = ReadSync(pBuffer, sector, count);
	return;
};

int GzippedFileReader::FinishRead(void) {
	int res = mBytesRead;
	mBytesRead = -1;
	return res;
};

int GzippedFileReader::ReadSync(void* pBuffer, uint sector, uint count) {
	if (!OkIndex())
		return -1;

	PX_off_t offset = (s64)sector * m_blocksize + m_dataoffset;
	int bytesToRead = count * m_blocksize;

	FILE* in = fopen(m_filename.ToUTF8(), "rb");
	int res = extract(in, m_pIndex, offset, (unsigned char*)pBuffer, bytesToRead);
	fclose(in);
	return res;
}

void GzippedFileReader::Close() {
	m_filename.Empty();
	if (m_pIndex) {
		free_index((Access*)m_pIndex);
		m_pIndex = 0;
	}
}


// CompressedFileReader factory - currently there's only GzippedFileReader

// Go through available compressed readers
bool CompressedFileReader::DetectCompressed(AsyncFileReader* pReader) {
	return GzippedFileReader::CanHandle(pReader->GetFilename());
}

// Return a new reader which can handle, or any reader otherwise (which will fail on open)
AsyncFileReader* CompressedFileReader::GetNewReader(const wxString& fileName) {
	//if (GzippedFileReader::CanHandle(pReader))
	return new GzippedFileReader();
}