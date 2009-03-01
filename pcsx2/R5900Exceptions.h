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

	//////////////////////////////////////////////////////////////////////////////////
	// Abstract base class for R5900 exceptions; contains the cpuRegs instance at the
	// time the exception is raised.
	//
	class BaseExcept : public Ps2Generic
	{
	public:
		const cpuRegisters cpuState;

	public:
		virtual ~BaseExcept() throw()=0;

		explicit BaseExcept( const std::string& msg ) :
			Exception::Ps2Generic( "(EE) " + msg ),
			cpuState( cpuRegs )
		{
		}
		
		u32 GetPc() const { return cpuState.pc; }
		bool IsDelaySlot() const { return !!cpuState.IsDelaySlot; }
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class AddressError : public BaseExcept
	{
	public:
		const bool OnWrite;
		const u32 Address;

	public:
		virtual ~AddressError() throw() {}

		explicit AddressError( u32 ps2addr, bool onWrite ) :
			BaseExcept( fmt_string( "Address error, addr=0x%x [%s]", ps2addr, onWrite ? "store" : "load" ) ),
			OnWrite( onWrite ),
			Address( ps2addr )
		{}
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	//
	class TLBMiss : public BaseExcept
	{
	public:
		const bool OnWrite;
		const u32 Address;

	public:
		virtual ~TLBMiss() throw() {}

		explicit TLBMiss( u32 ps2addr, bool onWrite ) :
			BaseExcept( fmt_string( "Tlb Miss, addr=0x%x [%s]", ps2addr, onWrite ? "store" : "load" ) ),
			OnWrite( onWrite ),
			Address( ps2addr )
		{}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class BusError : public BaseExcept
	{
	public:
		const bool OnWrite;
		const u32 Address;

	public:
		virtual ~BusError() throw() {}

		//
		explicit BusError( u32 ps2addr, bool onWrite ) :
			BaseExcept( fmt_string( "Bus Error, addr=0x%x [%s]", ps2addr, onWrite ? "store" : "load" ) ),
			OnWrite( onWrite ),
			Address( ps2addr )
		{}
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	//
	class SystemCall : public BaseExcept
	{
	public:
		virtual ~SystemCall() throw() {}

		explicit SystemCall() :
			BaseExcept( "SystemCall [SYSCALL]" )
		{}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class Trap : public BaseExcept
	{
	public:
		const u16 TrapCode;

	public:
		virtual ~Trap() throw() {}

		// Generates a trap for immediate-style Trap opcodes
		explicit Trap() :
			BaseExcept( "Trap" ),
			TrapCode( 0 )
		{}

		// Generates a trap for register-style Trap instructions, which contain an
		// error code in the opcode
		explicit Trap( u16 trapcode ) :
			BaseExcept( "Trap" ),
			TrapCode( trapcode )
		{}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class Break : public BaseExcept
	{
	public:
		virtual ~Break() throw() {}

		explicit Break() :
			BaseExcept( "Break Instruction" )
		{}
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	//
	class Overflow : public BaseExcept
	{
	public:
		virtual ~Overflow() throw() {}

		explicit Overflow() :
			BaseExcept( "Overflow" )
		{}
	};

	//////////////////////////////////////////////////////////////////////////////////
	//
	class DebugBreakpoint : public BaseExcept
	{
	public:
		virtual ~DebugBreakpoint() throw() {}

		explicit DebugBreakpoint() :
			BaseExcept( "Debug Breakpoint" )
		{}
	};
}


#endif
