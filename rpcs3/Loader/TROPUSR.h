#pragma once
#include "stdafx.h"
#include "Utilities/rXml.h"
#include "Utilities/File.h"

struct TROPUSRHeader
{
	be_t<u32> magic;         // 81 8F 54 AD
	be_t<u32> unk1;
	be_t<u32> tables_count;
	be_t<u32> unk2;
	char reserved[32];
};

struct TROPUSRTableHeader
{
	be_t<u32> type;
	be_t<u32> entries_size;
	be_t<u32> unk1;          // Seems to be 1
	be_t<u32> entries_count;
	be_t<u64> offset;
	be_t<u64> reserved;
};

struct TROPUSREntry4
{
	// Entry Header
	be_t<u32> entry_type;    // Always 0x4
	be_t<u32> entry_size;    // Always 0x50
	be_t<u32> entry_id;      // Entry ID
	be_t<u32> entry_unk1;    // Just zeroes?

	// Entry Contents
	be_t<u32> trophy_id;     // Trophy ID
	be_t<u32> trophy_grade;  // This seems interesting
	be_t<u32> trophy_pid;    // (Assuming that this is the platinum link id) FF FF FF FF (-1) = SCE_NP_TROPHY_INVALID_TROPHY_ID
	char unk6[68];           // Just zeroes?
};

struct TROPUSREntry6
{
	// Entry Header
	be_t<u32> entry_type;    // Always 6
	be_t<u32> entry_size;    // Always 0x60
	be_t<u32> entry_id;      // Entry ID
	be_t<u32> entry_unk1;    // Just zeroes?

	// Entry Contents
	be_t<u32> trophy_id;     // Trophy ID
	be_t<u32> trophy_state;  // Wild guess: 00 00 00 00 = Locked, 00 00 00 01 = Unlocked
	be_t<u32> unk4;          // This seems interesting
	be_t<u32> unk5;          // Just zeroes?
	be_t<u64> timestamp1;
	be_t<u64> timestamp2;
	char unk6[64];           // Just zeroes?

	// Note: One of the fields should hold a flag showing whether the trophy is hidden or not
};

struct trophy_xml_document : public rXmlDocument
{
	trophy_xml_document() : rXmlDocument() {}
	std::shared_ptr<rXmlNode> GetRoot() override;
};

class TROPUSRLoader
{
	enum trophy_grade : u32
	{
		unknown  = 0, // SCE_NP_TROPHY_GRADE_UNKNOWN
		platinum = 1, // SCE_NP_TROPHY_GRADE_PLATINUM
		gold     = 2, // SCE_NP_TROPHY_GRADE_GOLD
		silver   = 3, // SCE_NP_TROPHY_GRADE_SILVER
		bronze   = 4  // SCE_NP_TROPHY_GRADE_BRONZE
	};

	fs::file m_file;
	TROPUSRHeader m_header{};
	std::vector<TROPUSRTableHeader> m_tableHeaders;

	std::vector<TROPUSREntry4> m_table4;
	std::vector<TROPUSREntry6> m_table6;

	[[nodiscard]] bool Generate(std::string_view filepath, std::string_view configpath);
	[[nodiscard]] bool LoadHeader();
	[[nodiscard]] bool LoadTableHeaders();
	[[nodiscard]] bool LoadTables();

public:
	virtual ~TROPUSRLoader() = default;

	struct load_result
	{
		bool discarded_existing;
		bool success;
	};

	[[nodiscard]] load_result Load(std::string_view filepath, std::string_view configpath);
	[[nodiscard]] bool Save(std::string_view filepath);

	[[nodiscard]] u32 GetTrophiesCount() const;
	[[nodiscard]] u32 GetUnlockedTrophiesCount() const;

	[[nodiscard]] u32 GetUnlockedPlatinumID(u32 trophy_id, const std::string& config_path);

	[[nodiscard]] u32 GetTrophyGrade(u32 id) const;
	[[nodiscard]] u32 GetTrophyUnlockState(u32 id) const;
	[[nodiscard]] u64 GetTrophyTimestamp(u32 id) const;

	[[nodiscard]] bool UnlockTrophy(u32 id, u64 timestamp1, u64 timestamp2);
	[[nodiscard]] bool LockTrophy(u32 id);
};
