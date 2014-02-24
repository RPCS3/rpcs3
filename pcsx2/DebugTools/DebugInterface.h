#pragma once
#include "MemoryTypes.h"
#include "ExpressionParser.h"

class DebugInterface
{
public:
	enum RegisterType { NORMAL, SPECIAL };

	virtual u32 read8(u32 address) = 0;
	virtual u32 read16(u32 address) = 0;
	virtual u32 read32(u32 address) = 0;
	virtual u64 read64(u32 address) = 0;
	virtual u128 read128(u32 address) = 0;
	virtual void write8(u32 address, u8 value) = 0;

	// register stuff
	virtual int getRegisterCategoryCount() = 0;
	virtual const char* getRegisterCategoryName(int cat) = 0;
	virtual int getRegisterSize(int cat) = 0;
	virtual int getRegisterCount(int cat) = 0;
	virtual RegisterType getRegisterType(int cat) = 0;
	virtual const char* getRegisterName(int cat, int num) = 0;
	virtual u128 getRegister(int cat, int num) = 0;
	virtual wxString getRegisterString(int cat, int num) = 0;
	virtual u128 getHI() = 0;
	virtual u128 getLO() = 0;
	virtual u32 getPC() = 0;
	virtual void setPc(u32 newPc) = 0;
	virtual void setRegister(int cat, int num, u128 newValue) = 0;
	
	virtual std::string disasm(u32 address) = 0;
	virtual bool isValidAddress(u32 address) = 0;
	
	bool initExpression(const char* exp, PostfixExpression& dest);
	bool parseExpression(PostfixExpression& exp, u64& dest);
	bool isAlive();
	bool isCpuPaused();
	void pauseCpu();
	void resumeCpu();
};

class R5900DebugInterface: public DebugInterface
{
public:
	virtual u32 read8(u32 address);
	virtual u32 read16(u32 address);
	virtual u32 read32(u32 address);
	virtual u64 read64(u32 address);
	virtual u128 read128(u32 address);
	virtual void write8(u32 address, u8 value);

	// register stuff
	virtual int getRegisterCategoryCount();
	virtual const char* getRegisterCategoryName(int cat);
	virtual int getRegisterSize(int cat);
	virtual int getRegisterCount(int cat);
	virtual RegisterType getRegisterType(int cat);
	virtual const char* getRegisterName(int cat, int num);
	virtual u128 getRegister(int cat, int num);
	virtual wxString getRegisterString(int cat, int num);
	virtual u128 getHI();
	virtual u128 getLO();
	virtual u32 getPC();
	virtual void setPc(u32 newPc);
	virtual void setRegister(int cat, int num, u128 newValue);

	virtual std::string disasm(u32 address);
	virtual bool isValidAddress(u32 address);
};


class R3000DebugInterface: public DebugInterface
{
public:
	virtual u32 read8(u32 address);
	virtual u32 read16(u32 address);
	virtual u32 read32(u32 address);
	virtual u64 read64(u32 address);
	virtual u128 read128(u32 address);
	virtual void write8(u32 address, u8 value);

	// register stuff
	virtual int getRegisterCategoryCount();
	virtual const char* getRegisterCategoryName(int cat);
	virtual int getRegisterSize(int cat);
	virtual int getRegisterCount(int cat);
	virtual RegisterType getRegisterType(int cat);
	virtual const char* getRegisterName(int cat, int num);
	virtual u128 getRegister(int cat, int num);
	virtual wxString getRegisterString(int cat, int num);
	virtual u128 getHI();
	virtual u128 getLO();
	virtual u32 getPC();
	virtual void setPc(u32 newPc);
	virtual void setRegister(int cat, int num, u128 newValue);

	virtual std::string disasm(u32 address);
	virtual bool isValidAddress(u32 address);
};

extern R5900DebugInterface r5900Debug;
extern R3000DebugInterface r3000Debug;