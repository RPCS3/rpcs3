#pragma once
#include "Emu/CPU/CPUThread.h"

class ARMv7Thread : public CPUThread
{
public:
	ARMv7Thread();

	union
	{
		u32 GPR[15];

		struct
		{
			u32 pad[13];

			union
			{
				u32 SP;

				struct { u16 SP_main, SP_process; };
			};

			u32 LR;
		};
	};

	union
	{
		struct
		{
			u32 N : 1; //Negative condition code flag
			u32 Z : 1; //Zero condition code flag
			u32 C : 1; //Carry condition code flag
			u32 V : 1; //Overflow condition code flag
			u32 Q : 1; //Set to 1 if an SSAT or USAT instruction changes (saturates) the input value for the signed or unsigned range of the result
			u32 : 27;
		};

		u32 APSR;
	} APSR;

	union
	{
		struct
		{
			u32 : 24;
			u32 exception : 8;
		};

		u32 IPSR;
	} IPSR;

	void write_gpr(u8 n, u32 value)
	{
		assert(n < 16);

		if(n < 15)
		{
			GPR[n] = value;
		}
		else
		{
			SetBranch(value);
		}
	}

	u32 read_gpr(u8 n)
	{
		assert(n < 16);

		if(n < 15)
		{
			return GPR[n];
		}
		
		return PC;
	}

public:
	virtual void InitRegs(); 
	virtual void InitStack();
	virtual u64 GetFreeStackSize() const;
	virtual void SetArg(const uint pos, const u64 arg);

public:
	virtual std::string RegsToString();
	virtual std::string ReadRegString(const std::string& reg);
	virtual bool WriteRegString(const std::string& reg, std::string value);

protected:
	virtual void DoReset();
	virtual void DoRun();
	virtual void DoPause();
	virtual void DoResume();
	virtual void DoStop();

	virtual void DoCode();
};