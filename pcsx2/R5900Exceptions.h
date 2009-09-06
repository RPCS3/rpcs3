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

#ifndef _R5900_EXCEPTIONS_H_
#define _R5900_EXCEPTIONS_H_

namespace R5900Exception
{
	using Exception::Ps2Generic;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Abstract base class for R5900 exceptions; contains the cpuRegs instance at the
	// time the exception is raised.
	//
	// Translation note: EE Emulation exceptions are untranslated only.  There's really no
	// point in providing translations for this hardcore mess. :)
	//
	class BaseExcept : public virtual Ps2Generic
	{
	public:
		cpuRegisters cpuState;

	public:
		virtual ~BaseExcept() throw()=0;

		u32 GetPc() const { return cpuState.pc; }
		bool IsDelaySlot() const { return !!cpuState.IsDelaySlot; }

	protected:
		void Init( const wxString& msg )
		{
			m_message = L"(EE) " + msg;
			cpuState = cpuRegs;
		}

		void Init( const char*msg )
		{
			m_message = wxString::FromUTF8( msg );
			cpuState = cpuRegs;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class BaseAddressError : public BaseExcept
	{
	public:
		bool OnWrite;
		u32 Address;

	public:
		virtual ~BaseAddressError() throw() {}
		
	protected:
		void Init( u32 ps2addr, bool onWrite, const wxString& msg )
		{
			BaseExcept::Init( wxsFormat( msg+L", addr=0x%x [%s]", ps2addr, onWrite ? L"store" : L"load" ) );
			OnWrite = onWrite;
			Address = ps2addr;
		}
	};
	
	
	class AddressError : public BaseAddressError
	{
	public:
		virtual ~AddressError() throw() {}

		AddressError( u32 ps2addr, bool onWrite )
		{
			BaseAddressError::Init( ps2addr, onWrite, L"Address error" );
		}
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	//
	class TLBMiss : public BaseAddressError
	{
	public:
		virtual ~TLBMiss() throw() {}

		TLBMiss( u32 ps2addr, bool onWrite )
		{
			BaseAddressError::Init( ps2addr, onWrite, L"TLB Miss" );
		}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class BusError : public BaseAddressError
	{
	public:
		virtual ~BusError() throw() {}

		BusError( u32 ps2addr, bool onWrite )
		{
			BaseAddressError::Init( ps2addr, onWrite, L"Bus Error" );
		}
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	//
	class Trap : public BaseExcept
	{
	public:
		u16 TrapCode;

	public:
		virtual ~Trap() throw() {}

		// Generates a trap for immediate-style Trap opcodes
		Trap()
		{
			BaseExcept::Init( "Trap" );
			TrapCode = 0;
		}

		// Generates a trap for register-style Trap instructions, which contain an
		// error code in the opcode
		explicit Trap( u16 trapcode )
		{
			BaseExcept::Init( "Trap" ),
			TrapCode = trapcode;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class DebugBreakpoint : public BaseExcept
	{
	public:
		virtual ~DebugBreakpoint() throw() {}

		explicit DebugBreakpoint()
		{
			BaseExcept::Init( "Debug Breakpoint" );
		}
	};
}


#endif
