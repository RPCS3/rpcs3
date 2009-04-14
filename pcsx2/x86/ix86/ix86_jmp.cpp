/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/*
 * ix86 core v0.9.0
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.0:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

#include "PrecompiledHeader.h"

#include "System.h"
#include "ix86_internal.h"


// Another Work-in-Progress!!


/*
emitterT void x86SetPtr( u8* ptr ) 
{
	x86Ptr = ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////
// x86Ptr Label API
//

class x86Label
{
public:
	class Entry
	{
	protected:
		u8* (*m_emit)( u8* emitTo, u8* label_target, int cc );	// callback for the instruction to emit (cc = comparison type)
		u8* m_base;			// base address of the instruction (passed to the instruction)
		int m_cc;			// comparison type of the instruction
		
	public:
		explicit Entry( int cc ) :
			m_base( x86Ptr )
		,	m_writebackpos( writebackidx )
		{
		}

		void Commit( const u8* target ) const
		{
			//uptr reltarget = (uptr)m_base - (uptr)target;
			//*((u32*)&m_base[m_writebackpos]) = reltarget;
			jASSUME( m_emit != NULL );
			jASSUME( m_base != NULL );
			return m_emit( m_base, target, m_cc );
		}
	};

protected:
	u8* m_target;		// x86Ptr target address of this label
	Entry m_writebacks[8];
	int m_writeback_curpos;

public:
	// creates a label list with no valid target.
	// Use x86LabelList::Set() to set a target prior to class destruction.
	x86Label() : m_target()
	{
	}

	x86Label( EmitPtrCache& src ) : m_target( src.GetPtr() )
	{
	}
	
	// Performs all address writebacks on destruction.
	virtual ~x86Label()
	{
		IssueWritebacks();
	}

	void SetTarget() { m_address = x86Ptr; }
	void SetTarget( void* addr ) { m_address = (u8*)addr; }

	void Clear()
	{
		m_writeback_curpos = 0;
	}
	
	// Adds a jump/call instruction to this label for writebacks.
	void AddWriteback( void* emit_addr, u8* (*instruction)(), int cc )
	{
		jASSUME( m_writeback_curpos < MaxWritebacks );
		m_writebacks[m_writeback_curpos] = Entry( (u8*)instruction, addrpart ) );
		m_writeback_curpos++;
	}
	
	void IssueWritebacks() const
	{
		const std::list<Entry>::const_iterator& start = m_list_writebacks.
		for( ; start!=end; start++ )
		{
			Entry& current = *start;
			u8* donespot = current.Commit();
			
			// Copy the data from the m_nextinst to the current location,
			// and update any additional writebacks (but what about multiple labels?!?)

		}
	}
};
#endif

void JMP( x86Label& dest )
{
	dest.AddWriteback( x86Ptr, emitJMP, 0 );
}

void JLE( x86Label& dest )
{
	dest.AddWriteback( x86Ptr, emitJCC, 0 );
}

void x86SetJ8( u8* j8 )
{
	u32 jump = ( x86Ptr - j8 ) - 1;

	if ( jump > 0x7f ) {
		Console::Error( "j8 greater than 0x7f!!" );
		assert(0);
	}
	*j8 = (u8)jump;
}

void x86SetJ8A( u8* j8 )
{
	u32 jump = ( x86Ptr - j8 ) - 1;

	if ( jump > 0x7f ) {
		Console::Error( "j8 greater than 0x7f!!" );
		assert(0);
	}

	if( ((uptr)x86Ptr&0xf) > 4 ) {

		uptr newjump = jump + 16-((uptr)x86Ptr&0xf);

		if( newjump <= 0x7f ) {
			jump = newjump;
			while((uptr)x86Ptr&0xf) *x86Ptr++ = 0x90;
		}
	}
	*j8 = (u8)jump;
}

emitterT void x86SetJ32( u32* j32 ) 
{
	*j32 = ( x86Ptr - (u8*)j32 ) - 4;
}

emitterT void x86SetJ32A( u32* j32 )
{
	while((uptr)x86Ptr&0xf) *x86Ptr++ = 0x90;
	x86SetJ32(j32);
}

emitterT void x86Align( int bytes ) 
{
	// forward align
	x86Ptr = (u8*)( ( (uptr)x86Ptr + bytes - 1) & ~( bytes - 1 ) );
}
*/
