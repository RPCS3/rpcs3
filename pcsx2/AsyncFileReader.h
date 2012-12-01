#pragma once
/*
// --------------------------------------------------------------------------------------
//  MultiPartIso
// --------------------------------------------------------------------------------------
// An encapsulating class for array boundschecking and easy ScopedPointer behavior.
//
class _IsoPart
{
	DeclareNoncopyableObject( _IsoPart );

public:
	// starting block index of this part of the iso.
	u32			slsn;
	// ending bock index of this part of the iso.
	u32			elsn;

	wxString						filename;
	ScopedPtr<wxFileInputStream>	handle;

public:	
	_IsoPart() {}
	~_IsoPart() throw();
	
	void Read( void* dest, size_t size );

	void Seek(wxFileOffset pos, wxSeekMode mode = wxFromStart);
	void SeekEnd(wxFileOffset pos=0);
	wxFileOffset Tell() const;
	uint CalculateBlocks( uint startBlock, uint blocksize );

	template< typename T >
	void Read( T& dest )
	{
		Read( &dest, sizeof(dest) );
	}
};
*/
#ifdef WIN32
#	include <Windows.h>
#	undef Yield
#endif

class AsyncFileReader
{	
protected:
	AsyncFileReader(void) {}

	wxString m_filename;
	
	uint m_blocksize;

public:
	virtual ~AsyncFileReader(void) {};

	virtual bool Open(const wxString& fileName)=0;

	virtual int ReadSync(void* pBuffer, uint sector, uint count)=0;

	virtual void BeginRead(void* pBuffer, uint sector, uint count)=0;
	virtual int FinishRead(void)=0;
	virtual void CancelRead(void)=0;

	virtual void Close(void)=0;

	virtual int GetBlockCount(void) const=0;
	
	virtual void SetBlockSize(uint bytes) {}	
	
	uint GetBlockSize() const { return m_blocksize; }

	const wxString& GetFilename() const
	{
		return m_filename;
	}
};

class FlatFileReader : public AsyncFileReader
{
#ifdef WIN32
	HANDLE hOverlappedFile;

	OVERLAPPED asyncOperationContext;
	bool asyncInProgress;

	HANDLE hEvent;
#else
#	error Not implemented
#endif

public:
	FlatFileReader(void);
	virtual ~FlatFileReader(void);

	virtual bool Open(const wxString& fileName);

	virtual int ReadSync(void* pBuffer, uint sector, uint count);

	virtual void BeginRead(void* pBuffer, uint sector, uint count);
	virtual int FinishRead(void);
	virtual void CancelRead(void);

	virtual void Close(void);

	virtual int GetBlockCount(void) const;
	
	void SetBlockSize(uint bytes) { m_blocksize = bytes; }
};

class MultipartFileReader : public AsyncFileReader
{
	DeclareNoncopyableObject( MultipartFileReader );

	static const int MaxParts = 8;
	
	struct Part {
		uint start;
		uint end; // exclusive
		bool isReading;
		AsyncFileReader* reader;
	} m_parts[MaxParts];
	uint m_numparts;
	
	uint GetFirstPart(uint lsn);
	void FindParts();

public:
	MultipartFileReader(AsyncFileReader* firstPart);
	virtual ~MultipartFileReader(void);

	virtual bool Open(const wxString& fileName);

	virtual int ReadSync(void* pBuffer, uint sector, uint count);

	virtual void BeginRead(void* pBuffer, uint sector, uint count);
	virtual int FinishRead(void);
	virtual void CancelRead(void);

	virtual void Close(void);

	virtual int GetBlockCount(void) const;
	
	void SetBlockSize(uint bytes);

	static AsyncFileReader* DetectMultipart(AsyncFileReader* reader);
};

class BlockdumpFileReader : public AsyncFileReader
{
	DeclareNoncopyableObject( BlockdumpFileReader );

	wxFileInputStream* m_file;
	
	// total number of blocks in the ISO image (including all parts)
	u32			m_blocks;	
	s32			m_blockofs;

	// dtable / dtablesize are used when reading blockdumps
	ScopedArray<u32>	m_dtable;
	int					m_dtablesize;

	int m_lresult;

public:
	BlockdumpFileReader(void);
	virtual ~BlockdumpFileReader(void);

	virtual bool Open(const wxString& fileName);

	virtual int ReadSync(void* pBuffer, uint sector, uint count);

	virtual void BeginRead(void* pBuffer, uint sector, uint count);
	virtual int FinishRead(void);
	virtual void CancelRead(void);

	virtual void Close(void);

	virtual int GetBlockCount(void) const;

	static bool DetectBlockdump(AsyncFileReader* reader);

	int GetBlockOffset() { return m_blockofs; }
};
