#pragma once

#include <vector>
#include <set>
#include <map>
#include <string>

#include "Utilities/Threading.h"
#include "Pcsx2Types.h"

enum SymbolType {
	ST_NONE     = 0,
	ST_FUNCTION = 1,
	ST_DATA     = 2,
	ST_ALL      = 3,
};

struct SymbolInfo {
	SymbolType type;
	u32 address;
	u32 size;
};

struct SymbolEntry {
	std::string name;
	u32 address;
	u32 size;
};

struct LoadedModuleInfo {
	std::string name;
	u32 address;
	u32 size;
	bool active;
};

enum DataType {
	DATATYPE_NONE, DATATYPE_BYTE, DATATYPE_HALFWORD, DATATYPE_WORD, DATATYPE_ASCII
};

class SymbolMap {
public:
	SymbolMap() {}
	void Clear();
	void SortSymbols();

	bool LoadNocashSym(const char *ilename);

	SymbolType GetSymbolType(u32 address) const;
	bool GetSymbolInfo(SymbolInfo *info, u32 address, SymbolType symmask = ST_FUNCTION) const;
	u32 GetNextSymbolAddress(u32 address, SymbolType symmask);
	std::string GetDescription(unsigned int address) const;
	std::vector<SymbolEntry> GetAllSymbols(SymbolType symmask);

	void AddModule(const char *name, u32 address, u32 size);
	void UnloadModule(u32 address, u32 size);
	u32 GetModuleRelativeAddr(u32 address, int moduleIndex = -1) const;
	u32 GetModuleAbsoluteAddr(u32 relative, int moduleIndex) const;
	int GetModuleIndex(u32 address) const;
	bool IsModuleActive(int moduleIndex) const;
	std::vector<LoadedModuleInfo> getAllModules() const;

	void AddFunction(const char* name, u32 address, u32 size, int moduleIndex = -1);
	u32 GetFunctionStart(u32 address) const;
	int GetFunctionNum(u32 address) const;
	u32 GetFunctionSize(u32 startAddress) const;
	bool SetFunctionSize(u32 startAddress, u32 newSize);
	bool RemoveFunction(u32 startAddress, bool removeName);

	void AddLabel(const char* name, u32 address, int moduleIndex = -1);
	std::string GetLabelString(u32 address) const;
	void SetLabelName(const char* name, u32 address, bool updateImmediately = true);
	bool GetLabelValue(const char* name, u32& dest);

	void AddData(u32 address, u32 size, DataType type, int moduleIndex = -1);
	u32 GetDataStart(u32 address) const;
	u32 GetDataSize(u32 startAddress) const;
	DataType GetDataType(u32 startAddress) const;

	static const u32 INVALID_ADDRESS = (u32)-1;

	void UpdateActiveSymbols();

private:
	void AssignFunctionIndices();
	const char *GetLabelName(u32 address) const;
	const char *GetLabelNameRel(u32 relAddress, int moduleIndex) const;

	struct FunctionEntry {
		u32 start;
		u32 size;
		int index;
		int module;
	};

	struct LabelEntry {
		u32 addr;
		int module;
		char name[128];
	};

	struct DataEntry {
		DataType type;
		u32 start;
		u32 size;
		int module;
	};

	struct ModuleEntry {
		// Note: this index is +1, 0 matches any for backwards-compat.
		int index;
		u32 start;
		u32 size;
		char name[128];
	};

	// These are flattened, read-only copies of the actual data in active modules only.
	std::map<u32, const FunctionEntry> activeFunctions;
	std::map<u32, const LabelEntry> activeLabels;
	std::map<u32, const DataEntry> activeData;

	// This is indexed by the end address of the module.
	std::map<u32, const ModuleEntry> activeModuleEnds;

	typedef std::pair<int, u32> SymbolKey;

	// These are indexed by the module id and relative address in the module.
	std::map<SymbolKey, FunctionEntry> functions;
	std::map<SymbolKey, LabelEntry> labels;
	std::map<SymbolKey, DataEntry> data;
	std::vector<ModuleEntry> modules;

	mutable Threading::MutexRecursive lock_;
};

extern SymbolMap symbolMap;

