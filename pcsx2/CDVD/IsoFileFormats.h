/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "CDVD.h"
#include "wx/wfstream.h"


enum isoType
{
	ISOTYPE_ILLEGAL = 0,
	ISOTYPE_CD,
	ISOTYPE_DVD,
	ISOTYPE_AUDIO,
	ISOTYPE_DVDDL
};

enum isoFlags
{
	ISOFLAGS_BLOCKDUMP_V2 =	0x0004,
	ISOFLAGS_BLOCKDUMP_V3 =	0x0020
};

static const int CD_FRAMESIZE_RAW	= 2448;

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

// --------------------------------------------------------------------------------------
//  isoFile
// --------------------------------------------------------------------------------------
class isoFile
{
	DeclareNoncopyableObject( isoFile );

protected:
	static const uint MaxSplits = 8;

protected:
	wxString	m_filename;
	uint		m_numparts;
	_IsoPart	m_parts[MaxSplits];

	isoType		m_type;
	u32			m_flags;

	s32			m_offset;
	s32			m_blockofs;
	u32			m_blocksize;

	// total number of blocks in the ISO image (including all parts)
	u32			m_blocks;

	// dtable / dtablesize are used when reading blockdumps
	ScopedArray<u32>	m_dtable;
	int					m_dtablesize;

	ScopedPtr<wxFileOutputStream>	m_outstream;

	// Currently unused internal buffer (it was used for compressed
	// iso support, before it was removed).
	//ScopedArray<u8>		m_buffer;
	//int					m_buflsn;
			
public:	
	isoFile();
	virtual ~isoFile() throw();

	bool IsOpened() const;
	
	isoType GetType() const		{ return m_type; }

	// Returns the number of blocks in the ISO image.
	uint GetBlockCount() const	{ return m_blocks; }
	
	int GetBlockOffset() const	{ return m_blockofs; }
	
	const wxString& GetFilename() const
	{
		return m_filename;
	}

	bool Test( const wxString& srcfile );
	void Open( const wxString& srcfile );
	void Create(const wxString& filename, int mode);
	void Close();
	bool Detect( bool readType=true );

	void WriteFormat(int blockofs, uint blocksize, uint blocks);

	void ReadBlock(u8* dst, uint lsn);
	void WriteBlock(const u8* src, uint lsn);
	
protected:
	bool detect();
	void _init();
	void _ReadDtable();
	void _ReadBlock(u8* dst, uint lsn);
	void _ReadBlockD(u8* dst, uint lsn);

	void _WriteBlock(const u8* src, uint lsn);
	void _WriteBlockD(const u8* src, uint lsn);

	bool tryIsoType(u32 _size, s32 _offset, s32 _blockofs);
	void FindParts();
	
	void outWrite( const void* src, size_t size );

	template< typename T >
	void outWrite( const T& data )
	{
		outWrite( &data, sizeof(data) );
	}
};



