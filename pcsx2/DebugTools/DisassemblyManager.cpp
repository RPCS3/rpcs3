#include "PrecompiledHeader.h"

#include <string>
#include <algorithm>
#include <map>

#include "DisassemblyManager.h"
#include "Memory.h"
#include "Debug.h"
#include "MIPSAnalyst.h"

int DisassemblyManager::maxParamChars = 29;

bool isInInterval(u32 start, u32 size, u32 value)
{
	return start <= value && value <= (start+size-1);
}

static u32 computeHash(u32 address, u32 size)
{
	u32 end = address+size;
	u32 hash = 0xBACD7814;
	while (address < end)
	{
		hash += memRead32(address);
		address += 4;
	}
	return hash;
}


void parseDisasm(const char* disasm, char* opcode, char* arguments, bool insertSymbols)
{
	if (*disasm == '(')
	{
		while (*disasm != ')' && *disasm != 0)
			disasm++;
		if (*disasm == ')')
			disasm++;
	}

	// copy opcode
	while (*disasm != 0 && *disasm != '\t')
	{
		*opcode++ = *disasm++;
	}
	*opcode = 0;

	if (*disasm++ == 0)
	{
		*arguments = 0;
		return;
	}

	const char* jumpAddress = strstr(disasm,"->$");
	const char* jumpRegister = strstr(disasm,"->");
	while (*disasm != 0)
	{
		// parse symbol
		if (disasm == jumpAddress)
		{
			u32 branchTarget;
			sscanf(disasm+3,"0x%08x",&branchTarget);

			const std::string addressSymbol = symbolMap.GetLabelString(branchTarget);
			if (!addressSymbol.empty() && insertSymbols)
			{
				arguments += sprintf(arguments,"%s",addressSymbol.c_str());
			} else {
				arguments += sprintf(arguments,"0x%08X",branchTarget);
			}

			disasm += 3+2+8;
			continue;
		}

		if (disasm == jumpRegister)
			disasm += 2;

		if (*disasm == ' ')
		{
			disasm++;
			continue;
		}
		*arguments++ = *disasm++;
	}

	*arguments = 0;
}

std::map<u32,DisassemblyEntry*>::iterator findDisassemblyEntry(std::map<u32,DisassemblyEntry*>& entries, u32 address, bool exact)
{
	if (exact)
		return entries.find(address);

	if (entries.size() == 0)
		return entries.end();

	// find first elem that's >= address
	auto it = entries.lower_bound(address);
	if (it != entries.end())
	{
		// it may be an exact match
		if (isInInterval(it->second->getLineAddress(0),it->second->getTotalSize(),address))
			return it;

		// otherwise it may point to the next
		if (it != entries.begin())
		{
			it--;
			if (isInInterval(it->second->getLineAddress(0),it->second->getTotalSize(),address))
				return it;
		}
	}

	// check last entry manually
	auto rit = entries.rbegin();
	if (isInInterval(rit->second->getLineAddress(0),rit->second->getTotalSize(),address))
	{
		return (++rit).base();
	}

	// no match otherwise
	return entries.end();
}

void DisassemblyManager::analyze(u32 address, u32 size = 1024)
{
	if (cpu->isAlive() == false)
		return;

	u32 end = address+size;

	address &= ~3;
	u32 start = address;

	while (address < end && start <= address)
	{
		auto it = findDisassemblyEntry(entries,address,false);
		if (it != entries.end())
		{
			DisassemblyEntry* entry = it->second;
			entry->recheck();
			address = entry->getLineAddress(0)+entry->getTotalSize();
			continue;
		}

		SymbolInfo info;
		if (!symbolMap.GetSymbolInfo(&info,address,ST_ALL))
		{
			if (address % 4)
			{
				u32 next = std::min<u32>((address+3) & ~3,symbolMap.GetNextSymbolAddress(address,ST_ALL));
				DisassemblyData* data = new DisassemblyData(cpu,address,next-address,DATATYPE_BYTE);
				entries[address] = data;
				address = next;
				continue;
			}

			u32 next = symbolMap.GetNextSymbolAddress(address,ST_ALL);

			if ((next % 4) && next != -1)
			{
				u32 alignedNext = next & ~3;

				if (alignedNext != address)
				{
					DisassemblyOpcode* opcode = new DisassemblyOpcode(cpu,address,(alignedNext-address)/4);
					entries[address] = opcode;
				}

				DisassemblyData* data = new DisassemblyData(cpu,address,next-alignedNext,DATATYPE_BYTE);
				entries[alignedNext] = data;
			} else {
				DisassemblyOpcode* opcode = new DisassemblyOpcode(cpu,address,(next-address)/4);
				entries[address] = opcode;
			}

			address = next;
			continue;
		}

		switch (info.type)
		{
		case ST_FUNCTION:
			{
				DisassemblyFunction* function = new DisassemblyFunction(cpu,info.address,info.size);
				entries[info.address] = function;
				address = info.address+info.size;
			}
			break;
		case ST_DATA:
			{
				DisassemblyData* data = new DisassemblyData(cpu,info.address,info.size,symbolMap.GetDataType(info.address));
				entries[info.address] = data;
				address = info.address+info.size;
			}
			break;
		}
	}

}

std::vector<BranchLine> DisassemblyManager::getBranchLines(u32 start, u32 size)
{
	std::vector<BranchLine> result;
	
	auto it = findDisassemblyEntry(entries,start,false);
	if (it != entries.end())
	{
		do 
		{
			it->second->getBranchLines(start,size,result);
			it++;
		} while (it != entries.end() && start+size > it->second->getLineAddress(0));
	}

	return result;
}

void DisassemblyManager::getLine(u32 address, bool insertSymbols, DisassemblyLineInfo& dest)
{
	auto it = findDisassemblyEntry(entries,address,false);
	if (it == entries.end())
	{
		analyze(address);
		it = findDisassemblyEntry(entries,address,false);

		if (it == entries.end())
		{
			if (address % 4)
				dest.totalSize = ((address+3) & ~3)-address;
			else
				dest.totalSize = 4;
			dest.name = "ERROR";
			dest.params = "Disassembly failure";
			return;
		}
	}

	DisassemblyEntry* entry = it->second;
	if (entry->disassemble(address,dest,insertSymbols))
		return;
	
	if (address % 4)
		dest.totalSize = ((address+3) & ~3)-address;
	else
		dest.totalSize = 4;
	dest.name = "ERROR";
	dest.params = "Disassembly failure";
}

u32 DisassemblyManager::getStartAddress(u32 address)
{
	auto it = findDisassemblyEntry(entries,address,false);
	if (it == entries.end())
	{
		analyze(address);
		it = findDisassemblyEntry(entries,address,false);
		if (it == entries.end())
			return address;
	}
	
	DisassemblyEntry* entry = it->second;
	int line = entry->getLineNum(address,true);
	return entry->getLineAddress(line);
}

u32 DisassemblyManager::getNthPreviousAddress(u32 address, int n)
{
	while (cpu->isValidAddress(address))
	{
		auto it = findDisassemblyEntry(entries,address,false);
	
		while (it != entries.end())
		{
			DisassemblyEntry* entry = it->second;
			int oldLineNum = entry->getLineNum(address,true);
			int oldNumLines = entry->getNumLines();
			if (n <= oldLineNum)
			{
				return entry->getLineAddress(oldLineNum-n);
			}

			address = entry->getLineAddress(0)-1;
			n -= oldLineNum+1;
			it = findDisassemblyEntry(entries,address,false);
		}
	
		analyze(address-127,128);
	}
	
	return address-n*4;
}

u32 DisassemblyManager::getNthNextAddress(u32 address, int n)
{
	while (cpu->isValidAddress(address))
	{
		auto it = findDisassemblyEntry(entries,address,false);
	
		while (it != entries.end())
		{
			DisassemblyEntry* entry = it->second;
			int oldLineNum = entry->getLineNum(address,true);
			int oldNumLines = entry->getNumLines();
			if (oldLineNum+n < oldNumLines)
			{
				return entry->getLineAddress(oldLineNum+n);
			}

			address = entry->getLineAddress(0)+entry->getTotalSize();
			n -= (oldNumLines-oldLineNum);
			it = findDisassemblyEntry(entries,address,false);
		}

		analyze(address);
	}

	return address+n*4;
}

void DisassemblyManager::clear()
{
	for (auto it = entries.begin(); it != entries.end(); it++)
	{
		delete it->second;
	}
	entries.clear();
}

DisassemblyFunction::DisassemblyFunction(DebugInterface* _cpu, u32 _address, u32 _size): address(_address), size(_size)
{
	cpu = _cpu;
	hash = computeHash(address,size);
	load();
}

void DisassemblyFunction::recheck()
{
	u32 newHash = computeHash(address,size);
	if (hash != newHash)
	{
		hash = newHash;
		clear();
		load();
	}
}

int DisassemblyFunction::getNumLines()
{
	return (int) lineAddresses.size();
}

int DisassemblyFunction::getLineNum(u32 address, bool findStart)
{
	if (findStart)
	{
		int last = (int)lineAddresses.size() - 1;
		for (int i = 0; i < last; i++)
		{
			u32 next = lineAddresses[i + 1];
			if (lineAddresses[i] <= address && next > address)
				return i;
		}
		if (lineAddresses[last] <= address && this->address + this->size > address)
			return last;
	}
	else
	{
		int last = (int)lineAddresses.size() - 1;
		for (int i = 0; i < last; i++)
		{
			u32 next = lineAddresses[i + 1];
			if (lineAddresses[i] == address)
				return i;
		}
		if (lineAddresses[last] == address)
			return last;
	}

	return 0;
}

u32 DisassemblyFunction::getLineAddress(int line)
{
	return lineAddresses[line];
}

bool DisassemblyFunction::disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols)
{
	auto it = findDisassemblyEntry(entries,address,false);
	if (it == entries.end())
		return false;

	return it->second->disassemble(address,dest,insertSymbols);
}

void DisassemblyFunction::getBranchLines(u32 start, u32 size, std::vector<BranchLine>& dest)
{
	u32 end = start+size;

	for (size_t i = 0; i < lines.size(); i++)
	{
		BranchLine& line = lines[i];

		u32 first = line.first;
		u32 second = line.second;

		// skip branches that are entirely before or entirely after the window
		if ((first < start && second < start) ||
			(first > end && second > end))
			continue;

		dest.push_back(line);
	}
}

#define NUM_LANES 16

void DisassemblyFunction::generateBranchLines()
{
	struct LaneInfo
	{
		bool used;
		u32 end;
	};

	LaneInfo lanes[NUM_LANES];
	for (int i = 0; i < NUM_LANES; i++)
		lanes[i].used = false;

	u32 end = address+size;

	for (u32 funcPos = address; funcPos < end; funcPos += 4)
	{
		MIPSAnalyst::MipsOpcodeInfo opInfo = MIPSAnalyst::GetOpcodeInfo(cpu,funcPos);

		bool inFunction = (opInfo.branchTarget >= address && opInfo.branchTarget < end);
		if (opInfo.isBranch && !opInfo.isBranchToRegister && !opInfo.isLinkedBranch && inFunction)
		{
			BranchLine line;
			if (opInfo.branchTarget < funcPos)
			{
				line.first = opInfo.branchTarget;
				line.second = funcPos;
				line.type = LINE_UP;
			} else {
				line.first = funcPos;
				line.second = opInfo.branchTarget;
				line.type = LINE_DOWN;
			}

			lines.push_back(line);
		}
	}
			
	std::sort(lines.begin(),lines.end());
	for (size_t i = 0; i < lines.size(); i++)
	{
		for (int l = 0; l < NUM_LANES; l++)
		{
			if (lines[i].first > lanes[l].end)
				lanes[l].used = false;
		}

		int lane = -1;
		for (int l = 0; l < NUM_LANES; l++)
		{
			if (lanes[l].used == false)
			{
				lane = l;
				break;
			}
		}

		if (lane == -1)
		{
			// error
			continue;
		}

		lanes[lane].end = lines[i].second;
		lanes[lane].used = true;
		lines[i].laneIndex = lane;
	}
}

void DisassemblyFunction::addOpcodeSequence(u32 start, u32 end)
{
	DisassemblyOpcode* opcode = new DisassemblyOpcode(cpu,start,(end-start)/4);
	entries[start] = opcode;
	for (u32 pos = start; pos < end; pos += 4)
	{
		lineAddresses.push_back(pos);
	}
}

void DisassemblyFunction::load()
{
	generateBranchLines();

	// gather all branch targets
	std::set<u32> branchTargets;
	for (size_t i = 0; i < lines.size(); i++)
	{
		switch (lines[i].type)
		{
		case LINE_DOWN:
			branchTargets.insert(lines[i].second);
			break;
		case LINE_UP:
			branchTargets.insert(lines[i].first);
			break;
		}
	}
	
	u32 funcPos = address;
	u32 funcEnd = address+size;

	u32 nextData = symbolMap.GetNextSymbolAddress(funcPos-1,ST_DATA);
	u32 opcodeSequenceStart = funcPos;
	while (funcPos < funcEnd)
	{
		if (funcPos == nextData)
		{
			if (opcodeSequenceStart != funcPos)
				addOpcodeSequence(opcodeSequenceStart,funcPos);

			DisassemblyData* data = new DisassemblyData(cpu,funcPos,symbolMap.GetDataSize(funcPos),symbolMap.GetDataType(funcPos));
			entries[funcPos] = data;
			lineAddresses.push_back(funcPos);
			funcPos += data->getTotalSize();

			nextData = symbolMap.GetNextSymbolAddress(funcPos-1,ST_DATA);
			opcodeSequenceStart = funcPos;
			continue;
		}

		// force align
		if (funcPos % 4)
		{
			u32 nextPos = (funcPos+3) & ~3;

			DisassemblyComment* comment = new DisassemblyComment(cpu,funcPos,nextPos-funcPos,".align","4");
			entries[funcPos] = comment;
			lineAddresses.push_back(funcPos);
			
			funcPos = nextPos;
			opcodeSequenceStart = funcPos;
			continue;
		}

		MIPSAnalyst::MipsOpcodeInfo opInfo = MIPSAnalyst::GetOpcodeInfo(cpu,funcPos);
		u32 opAddress = funcPos;
		funcPos += 4;
		
		// skip branches and their delay slots
		if (opInfo.isBranch)
		{
			funcPos += 4;
			continue;
		}
		
		// lui
		if (MIPS_GET_OP(opInfo.encodedOpcode) == 0x0F && funcPos < funcEnd && funcPos != nextData)
		{
			u32 next = cpu->read32(funcPos);

			u32 immediate = ((opInfo.encodedOpcode & 0xFFFF) << 16) + (s16)(next & 0xFFFF);
			u32 immediateOr = ((opInfo.encodedOpcode & 0xFFFF) << 16) | (u16)(next & 0xFFFF);
			int rt = MIPS_GET_RT(opInfo.encodedOpcode);

			int nextRs = MIPS_GET_RS(next);
			int nextRt = MIPS_GET_RT(next);

			// both rs and rt of the second op have to match rt of the first,
			// otherwise there may be hidden consequences if the macro is displayed.
			// also, don't create a macro if something branches into the middle of it
			if (nextRs == rt && nextRt == rt && branchTargets.find(funcPos) == branchTargets.end())
			{
				DisassemblyMacro* macro = NULL;
				switch (MIPS_GET_OP(next))
				{
				case 0x09:	// addiu
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroLi(immediate,rt);
					funcPos += 4;
					break;
				case 0x0D:	// ori
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroLi(immediateOr,rt);
					funcPos += 4;
					break;
				case 0x20:	// lb
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("lb",immediate,rt,1);
					funcPos += 4;
					break;
				case 0x21:	// lh
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("lh",immediate,rt,2);
					funcPos += 4;
					break;
				case 0x23:	// lw
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("lw",immediate,rt,4);
					funcPos += 4;
					break;
				case 0x24:	// lbu
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("lbu",immediate,rt,1);
					funcPos += 4;
					break;
				case 0x25:	// lhu
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("lhu",immediate,rt,2);
					funcPos += 4;
					break;
				case 0x28:	// sb
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("sb",immediate,rt,1);
					funcPos += 4;
					break;
				case 0x29:	// sh
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("sh",immediate,rt,2);
					funcPos += 4;
				case 0x2B:	// sw
					macro = new DisassemblyMacro(cpu,opAddress);
					macro->setMacroMemory("sw",immediate,rt,4);
					funcPos += 4;
					break;
				}

				if (macro != NULL)
				{
					if (opcodeSequenceStart != opAddress)
						addOpcodeSequence(opcodeSequenceStart,opAddress);

					entries[opAddress] = macro;
					for (int i = 0; i < macro->getNumLines(); i++)
					{
						lineAddresses.push_back(macro->getLineAddress(i));
					}

					opcodeSequenceStart = funcPos;
					continue;
				}
			}
		}

		// just a normal opcode
	}

	if (opcodeSequenceStart != funcPos)
		addOpcodeSequence(opcodeSequenceStart,funcPos);
}

void DisassemblyFunction::clear()
{
	for (auto it = entries.begin(); it != entries.end(); it++)
	{
		delete it->second;
	}

	entries.clear();
	lines.clear();
	lineAddresses.clear();
	hash = 0;
}

bool DisassemblyOpcode::disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols)
{
	char opcode[64],arguments[256];
	
	std::string dis = cpu->disasm(address);
	parseDisasm(dis.c_str(),opcode,arguments,insertSymbols);
	dest.type = DISTYPE_OPCODE;
	dest.name = opcode;
	dest.params = arguments;
	dest.totalSize = 4;
	dest.info = MIPSAnalyst::GetOpcodeInfo(cpu,address);
	return true;
}

void DisassemblyOpcode::getBranchLines(u32 start, u32 size, std::vector<BranchLine>& dest)
{
	if (start < address)
	{
		size = start+size-address;
		start = address;
	}

	if (start+size > address+num*4)
		size = address+num*4-start;

	int lane = 0;
	for (u32 pos = start; pos < start+size; pos += 4)
	{
		MIPSAnalyst::MipsOpcodeInfo info = MIPSAnalyst::GetOpcodeInfo(cpu,pos);
		if (info.isBranch && !info.isBranchToRegister && !info.isLinkedBranch)
		{
			BranchLine line;
			line.laneIndex = lane++;

			if (info.branchTarget < pos)
			{
				line.first = info.branchTarget;
				line.second = pos;
				line.type = LINE_UP;
			} else {
				line.first = pos;
				line.second = info.branchTarget;
				line.type = LINE_DOWN;
			}

			dest.push_back(line);
		}
	}
}


void DisassemblyMacro::setMacroLi(u32 _immediate, u8 _rt)
{
	type = MACRO_LI;
	name = "li";
	immediate = _immediate;
	rt = _rt;
	numOpcodes = 2;
}

void DisassemblyMacro::setMacroMemory(std::string _name, u32 _immediate, u8 _rt, int _dataSize)
{
	type = MACRO_MEMORYIMM;
	name = _name;
	immediate = _immediate;
	rt = _rt;
	dataSize = _dataSize;
	numOpcodes = 2;
}

bool DisassemblyMacro::disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols)
{
	char buffer[64];
	dest.type = DISTYPE_MACRO;
	dest.info = MIPSAnalyst::GetOpcodeInfo(cpu,address);

	std::string addressSymbol;
	switch (type)
	{
	case MACRO_LI:
		dest.name = name;
		
		addressSymbol = symbolMap.GetLabelString(immediate);
		if (!addressSymbol.empty() && insertSymbols)
		{
			sprintf(buffer,"%s,%s",cpu->getRegisterName(0,rt),addressSymbol.c_str());
		} else {
			sprintf(buffer,"%s,0x%08X",cpu->getRegisterName(0,rt),immediate);
		}

		dest.params = buffer;
		
		dest.info.hasRelevantAddress = true;
		dest.info.releventAddress = immediate;
		break;
	case MACRO_MEMORYIMM:
		dest.name = name;

		addressSymbol = symbolMap.GetLabelString(immediate);
		if (!addressSymbol.empty() && insertSymbols)
		{
			sprintf(buffer,"%s,%s",cpu->getRegisterName(0,rt),addressSymbol.c_str());
		} else {
			sprintf(buffer,"%s,0x%08X",cpu->getRegisterName(0,rt),immediate);
		}

		dest.params = buffer;

		dest.info.isDataAccess = true;
		dest.info.dataAddress = immediate;
		dest.info.dataSize = dataSize;

		dest.info.hasRelevantAddress = true;
		dest.info.releventAddress = immediate;
		break;
	default:
		return false;
	}

	dest.totalSize = getTotalSize();
	return true;
}


DisassemblyData::DisassemblyData(DebugInterface* _cpu, u32 _address, u32 _size, DataType _type): address(_address), size(_size), type(_type)
{
	cpu = _cpu;
	hash = computeHash(address,size);
	createLines();
}

void DisassemblyData::recheck()
{
	u32 newHash = computeHash(address,size);
	if (newHash != hash)
	{
		hash = newHash;
		createLines();
	}
}

bool DisassemblyData::disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols)
{
	dest.type = DISTYPE_DATA;

	switch (type)
	{
	case DATATYPE_BYTE:
		dest.name = ".byte";
		break;
	case DATATYPE_HALFWORD:
		dest.name = ".half";
		break;
	case DATATYPE_WORD:
		dest.name = ".word";
		break;
	case DATATYPE_ASCII:
		dest.name = ".ascii";
		break;
	default:
		return false;
	}

	auto it = lines.find(address);
	if (it == lines.end())
		return false;

	dest.params = it->second.text;
	dest.totalSize = it->second.size;
	return true;
}

int DisassemblyData::getLineNum(u32 address, bool findStart)
{
	auto it = lines.upper_bound(address);
	if (it != lines.end())
	{
		if (it == lines.begin())
			return 0;
		it--;
		return it->second.lineNum;
	}

	return lines.rbegin()->second.lineNum;
}

void DisassemblyData::createLines()
{
	lines.clear();
	lineAddresses.clear();

	u32 pos = address;
	u32 end = address+size;
	u32 maxChars = DisassemblyManager::getMaxParamChars();
	
	std::string currentLine;
	u32 currentLineStart = pos;

	int lineCount = 0;
	if (type == DATATYPE_ASCII)
	{
		bool inString = false;
		while (pos < end)
		{
			u8 b = memRead8(pos++);
			if (b >= 0x20 && b <= 0x7F)
			{
				if (currentLine.size()+1 >= maxChars)
				{
					if (inString == true)
						currentLine += "\"";

					DataEntry entry = {currentLine,pos-1-currentLineStart,lineCount++};
					lines[currentLineStart] = entry;
					lineAddresses.push_back(currentLineStart);
					
					currentLine = "";
					currentLineStart = pos-1;
					inString = false;
				}

				if (inString == false)
					currentLine += "\"";
				currentLine += (char)b;
				inString = true;
			} else {
				char buffer[64];
				if (pos == end && b == 0)
					strcpy(buffer,"0");
				else
					sprintf(buffer,"0x%02X",b);

				if (currentLine.size()+strlen(buffer) >= maxChars)
				{
					if (inString == true)
						currentLine += "\"";
					
					DataEntry entry = {currentLine,pos-1-currentLineStart,lineCount++};
					lines[currentLineStart] = entry;
					lineAddresses.push_back(currentLineStart);
					
					currentLine = "";
					currentLineStart = pos-1;
					inString = false;
				}

				bool comma = false;
				if (currentLine.size() != 0)
					comma = true;

				if (inString)
					currentLine += "\"";

				if (comma)
					currentLine += ",";

				currentLine += buffer;
				inString = false;
			}
		}

		if (inString == true)
			currentLine += "\"";

		if (currentLine.size() != 0)
		{
			DataEntry entry = {currentLine,pos-currentLineStart,lineCount++};
			lines[currentLineStart] = entry;
			lineAddresses.push_back(currentLineStart);
		}
	} else {
		while (pos < end)
		{
			char buffer[64];
			u32 value;

			u32 currentPos = pos;

			switch (type)
			{
			case DATATYPE_BYTE:
				value = memRead8(pos);
				sprintf(buffer,"0x%02X",value);
				pos++;
				break;
			case DATATYPE_HALFWORD:
				value = memRead16(pos);
				sprintf(buffer,"0x%04X",value);
				pos += 2;
				break;
			case DATATYPE_WORD:
				{
					value = memRead32(pos);
					const std::string label = symbolMap.GetLabelString(value);
					if (!label.empty())
						sprintf(buffer,"%s",label.c_str());
					else
						sprintf(buffer,"0x%08X",value);
					pos += 4;
				}
				break;
			}

			size_t len = strlen(buffer);
			if (currentLine.size() != 0 && currentLine.size()+len >= maxChars)
			{
				DataEntry entry = {currentLine,currentPos-currentLineStart,lineCount++};
				lines[currentLineStart] = entry;
				lineAddresses.push_back(currentLineStart);

				currentLine = "";
				currentLineStart = currentPos;
			}

			if (currentLine.size() != 0)
				currentLine += ",";
			currentLine += buffer;
		}

		if (currentLine.size() != 0)
		{
			DataEntry entry = {currentLine,pos-currentLineStart,lineCount++};
			lines[currentLineStart] = entry;
			lineAddresses.push_back(currentLineStart);
		}
	}
}


DisassemblyComment::DisassemblyComment(DebugInterface* _cpu, u32 _address, u32 _size, std::string _name, std::string _param)
	: cpu(_cpu), address(_address), size(_size), name(_name), param(_param)
{

}

bool DisassemblyComment::disassemble(u32 address, DisassemblyLineInfo& dest, bool insertSymbols)
{
	dest.type = DISTYPE_OTHER;
	dest.name = name;
	dest.params = param;
	dest.totalSize = size;
	return true;
}
