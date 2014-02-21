#pragma once

#include "SymbolMap.h"
#include "Utilities/Threading.h"
#include "Pcsx2Types.h"
#include "DebugInterface.h"
#include "MIPSAnalyst.h"

enum DisassemblyLineType { DISTYPE_OPCODE, DISTYPE_MACRO, DISTYPE_DATA, DISTYPE_OTHER };

struct DisassemblyLineInfo
{
	DisassemblyLineType type;
	MIPSAnalyst::MipsOpcodeInfo info;
	std::string name;
	std::string params;
	u32 totalSize;
};

enum LineType { LINE_UP, LINE_DOWN, LINE_RIGHT };

struct BranchLine
{
	u32 first;
	u32 second;
	LineType type;
	int laneIndex;

	bool operator<(const BranchLine& other) const
	{
		return first < other.first;
	}
};

class DisassemblyEntry
{
public:
	virtual ~DisassemblyEntry() { };
	virtual void recheck() = 0;
	virtual int getNumLines() = 0;
	virtual int getLineNum(u32 address, bool findStart) = 0;
	virtual u32 getLineAddress(int line) = 0;
	virtual u32 getTotalSize() = 0;
	virtual bool disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols) = 0;
	virtual void getBranchLines(u32 start, u32 size, std::vector<BranchLine>& dest) { };
};

class DisassemblyFunction: public DisassemblyEntry
{
public:
	DisassemblyFunction(DebugInterface* _cpu, u32 _address, u32 _size);
	virtual void recheck();
	virtual int getNumLines();
	virtual int getLineNum(u32 address, bool findStart);
	virtual u32 getLineAddress(int line);
	virtual u32 getTotalSize() { return size; };
	virtual bool disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols);
	virtual void getBranchLines(u32 start, u32 size, std::vector<BranchLine>& dest);
private:
	void generateBranchLines();
	void load();
	void clear();
	void addOpcodeSequence(u32 start, u32 end);

	DebugInterface* cpu;
	u32 address;
	u32 size;
	u32 hash;
	std::vector<BranchLine> lines;
	std::map<u32,DisassemblyEntry*> entries;
	std::vector<u32> lineAddresses;
};

class DisassemblyOpcode: public DisassemblyEntry
{
public:
	DisassemblyOpcode(DebugInterface* _cpu, u32 _address, int _num): cpu(_cpu), address(_address), num(_num) { };
	virtual ~DisassemblyOpcode() { };
	virtual void recheck() { };
	virtual int getNumLines() { return num; };
	virtual int getLineNum(u32 address, bool findStart) { return (address-this->address)/4; };
	virtual u32 getLineAddress(int line) { return address+line*4; };
	virtual u32 getTotalSize() { return num*4; };
	virtual bool disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols);
	virtual void getBranchLines(u32 start, u32 size, std::vector<BranchLine>& dest);
private:
	DebugInterface* cpu;
	u32 address;
	int num;
};


class DisassemblyMacro: public DisassemblyEntry
{
public:
	DisassemblyMacro(DebugInterface* _cpu, u32 _address): cpu(_cpu), address(_address) { };
	virtual ~DisassemblyMacro() { };
	
	void setMacroLi(u32 _immediate, u8 _rt);
	void setMacroMemory(std::string _name, u32 _immediate, u8 _rt, int _dataSize);
	
	virtual void recheck() { };
	virtual int getNumLines() { return 1; };
	virtual int getLineNum(u32 address, bool findStart) { return 0; };
	virtual u32 getLineAddress(int line) { return address; };
	virtual u32 getTotalSize() { return numOpcodes*4; };
	virtual bool disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols) ;
private:
	enum MacroType { MACRO_LI, MACRO_MEMORYIMM };
	
	DebugInterface* cpu;
	MacroType type;
	std::string name;
	u32 immediate;
	u32 address;
	u32 numOpcodes;
	u8 rt;
	int dataSize;
};


class DisassemblyData: public DisassemblyEntry
{
public:
	DisassemblyData(DebugInterface* _cpu, u32 _address, u32 _size, DataType _type);
	virtual ~DisassemblyData() { };
	
	virtual void recheck();
	virtual int getNumLines() { return (int)lines.size(); };
	virtual int getLineNum(u32 address, bool findStart);
	virtual u32 getLineAddress(int line) { return lineAddresses[line]; };
	virtual u32 getTotalSize() { return size; };
	virtual bool disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols);
private:
	void createLines();

	struct DataEntry
	{
		std::string text;
		u32 size;
		int lineNum;
	};
	
	DebugInterface* cpu;
	u32 address;
	u32 size;
	u32 hash;
	DataType type;
	std::map<u32,DataEntry> lines;
	std::vector<u32> lineAddresses;
};

class DisassemblyComment: public DisassemblyEntry
{
public:
	DisassemblyComment(DebugInterface* _cpu, u32 _address, u32 _size, std::string name, std::string param);
	virtual ~DisassemblyComment() { };
	
	virtual void recheck() { };
	virtual int getNumLines() { return 1; };
	virtual int getLineNum(u32 address, bool findStart) { return 0; };
	virtual u32 getLineAddress(int line) { return address; };
	virtual u32 getTotalSize() { return size; };
	virtual bool disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols);
private:
	DebugInterface* cpu;
	u32 address;
	u32 size;
	std::string name;
	std::string param;
};

class DebugInterface;

class DisassemblyManager
{
public:
	void clear();

	void setCpu(DebugInterface* _cpu) { cpu = _cpu; };
	void setMaxParamChars(int num) { maxParamChars = num; clear(); };
	void getLine(u32 address, bool insertSymbols, DisassemblyLineInfo& dest);
	void analyze(u32 address, u32 size);
	std::vector<BranchLine> getBranchLines(u32 start, u32 size);

	u32 getStartAddress(u32 address);
	u32 getNthPreviousAddress(u32 address, int n = 1);
	u32 getNthNextAddress(u32 address, int n = 1);

	static int getMaxParamChars() { return maxParamChars; };
private:
	DisassemblyEntry* getEntry(u32 address);
	std::map<u32,DisassemblyEntry*> entries;
	DebugInterface* cpu;
	static int maxParamChars;
};

bool isInInterval(u32 start, u32 size, u32 value);
