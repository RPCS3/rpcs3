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

// --------------------------------------------------------------------------------------
//  BaseR5900Exception
// --------------------------------------------------------------------------------------
// Abstract base class for R5900 exceptions; contains the cpuRegs instance at the
// time the exception is raised.
//
// Translation note: EE Emulation exceptions are untranslated only.  There's really no
// point in providing translations for this hardcore mess. :)
//
class BaseR5900Exception : public Exception::Ps2Generic
{
	DEFINE_EXCEPTION_COPYTORS(BaseR5900Exception, Exception::Ps2Generic)

public:
	cpuRegisters cpuState;

public:
	u32 GetPc() const { return cpuState.pc; }
	bool IsDelaySlot() const { return !!cpuState.IsDelaySlot; }

	wxString& Message() { return m_message; }
	wxString FormatMessage() const
	{
		return wxsFormat(L"(EE pc:%8.8X) ", cpuRegs.pc) + m_message;
	}

protected:
	void Init( const wxString& msg )
	{
		m_message = msg;;
		cpuState = cpuRegs;
	}

	void Init( const char* msg )
	{
		m_message = fromUTF8( msg );
		cpuState = cpuRegs;
	}
};

namespace R5900Exception
{
	// --------------------------------------------------------------------------------------
	//  BaseAddressError
	// --------------------------------------------------------------------------------------
	class BaseAddressError : public BaseR5900Exception
	{
		DEFINE_EXCEPTION_COPYTORS(BaseAddressError, BaseR5900Exception)

	public:
		bool OnWrite;
		u32 Address;

	protected:
		void Init( u32 ps2addr, bool onWrite, const wxString& msg )
		{
			_parent::Init( wxsFormat( msg+L", addr=0x%x [%s]", ps2addr, onWrite ? L"store" : L"load" ) );
			OnWrite = onWrite;
			Address = ps2addr;
		}
	};


	class AddressError : public BaseAddressError
	{
	public:
		AddressError( u32 ps2addr, bool onWrite )
		{
			BaseAddressError::Init( ps2addr, onWrite, L"Address error" );
		}
	};

	class TLBMiss : public BaseAddressError
	{
		DEFINE_EXCEPTION_COPYTORS(TLBMiss, BaseAddressError)

	public:
		TLBMiss( u32 ps2addr, bool onWrite )
		{
			BaseAddressError::Init( ps2addr, onWrite, L"TLB Miss" );
		}
	};

	class BusError : public BaseAddressError
	{
		DEFINE_EXCEPTION_COPYTORS(BusError, BaseAddressError)

	public:
		BusError( u32 ps2addr, bool onWrite )
		{
			BaseAddressError::Init( ps2addr, onWrite, L"Bus Error" );
		}
	};

	class Trap : public BaseR5900Exception
	{
		DEFINE_EXCEPTION_COPYTORS(Trap, BaseR5900Exception)

	public:
		u16 TrapCode;

	public:
		// Generates a trap for immediate-style Trap opcodes
		Trap()
		{
			_parent::Init( "Trap" );
			TrapCode = 0;
		}

		// Generates a trap for register-style Trap instructions, which contain an
		// error code in the opcode
		explicit Trap( u16 trapcode )
		{
			_parent::Init( "Trap" ),
			TrapCode = trapcode;
		}
	};

	class DebugBreakpoint : public BaseR5900Exception
	{
		DEFINE_EXCEPTION_COPYTORS(DebugBreakpoint, BaseR5900Exception)
		
	public:
		explicit DebugBreakpoint()
		{
			_parent::Init( "Debug Breakpoint" );
		}
	};
}
