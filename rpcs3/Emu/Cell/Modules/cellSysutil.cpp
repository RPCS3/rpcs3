#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_cond.h"

#include "cellSysutil.h"

#include "Utilities/StrUtil.h"

#include <mutex>
#include <queue>

logs::channel cellSysutil("cellSysutil");

using CXmlAllocator = void(u32 allocType, vm::ptr<void> userData, vm::ptr<void> oldAddr, u32 requiredSize, vm::ptr<void> addr, vm::ptr<u32> size);

enum Cxml_AllocationType
{
    AllocationType_Alloc_Tree,
    AllocationType_Alloc_IDTable,
    AllocationType_Alloc_StringTable,
    AllocationType_Alloc_IntArrayTable,
    AllocationType_Alloc_FloatArrayTable,
    AllocationType_Alloc_FileTable,

    AllocationType_Free_Tree,
    AllocationType_Free_IDTable,
    AllocationType_Free_StringTable,
    AllocationType_Free_IntArrayTable,
    AllocationType_Free_FloatArrayTable,
    AllocationType_Free_FileTable,
};

enum AccessMode
{
    AccessMode_ReadWrite,
    AccessMode_ReadOnly_File,
    AccessMode_ReadOnly_Memory,
};

struct cxml_document
{
    be_t<u32> accessMode;                 // 0x00
    struct Header {
        char magic[4];                    // 0x04
        be_t<u32> version;                // 0x08
        be_t<u32> tree_offset;            // 0x0C
        be_t<u32> tree_size;              // 0x10
        be_t<u32> idtable_offset;         // 0x14
        be_t<u32> idtable_size;           // 0x18
        be_t<u32> stringtable_offset;     // 0x1C
        be_t<u32> stringtable_size;       // 0x20
        be_t<u32> intarraytable_offset;   // 0x24
        be_t<u32> intarraytable_size;     // 0x28
        be_t<u32> floatarraytable_offset; // 0x2C
        be_t<u32> floatarraytable_size;   // 0x30
        be_t<u32> filetable_offset;       // 0x34
        be_t<u32> filetable_size;         // 0x38
    } header;

    vm::bptr<u8> tree;               // 0x3C
    be_t<u32> tree_capacity;           // 0x40
    vm::bptr<u8> idtable;            // 0x44
    be_t<u32> idtable_capacity;        // 0x48
    vm::bptr<char> stringtable;        // 0x4C
    be_t<u32> stringtable_capacity;    // 0x50
    vm::bptr<u32> intarraytable;       // 0x54
    be_t<u32> intarraytable_capacity;  // 0x58
    vm::bptr<f32> floatarraytable;   // 0x5C
    be_t<u32> floatarraytable_capacity;// 0x60
    vm::bptr<u8> filetable;          // 0x64
    be_t<u32> filetable_capacity;      // 0x68
    vm::bptr<CXmlAllocator> allocator; // 0x6C
    vm::bptr<void> allocator_userdata; // 0x70
};

static_assert(sizeof(cxml_document) == 0x74, "Invalid CXml doc size!");

static_assert(sizeof(cxml_document::Header) == 0x38, "Invalid CXml header size!");

struct cxml_element
{
    vm::bptr<cxml_document> doc;
    be_t<u32> offset;
};

struct cxml_attribute
{
    vm::bptr<cxml_document> doc;
    be_t<u32> element_offset;
    be_t<u32> offset;
};

struct cxml_childelement_bin
{
    be_t<s32> name;        // 0x00
    be_t<s32> attr_num;    // 0x04
    be_t<s32> parent;      // 0x08
    be_t<s32> prev;        // 0x0C
    be_t<s32> next;        // 0x10
    be_t<s32> first_child; // 0x14
    be_t<s32> last_child;  // 0x18
};

static_assert(sizeof(cxml_childelement_bin) == 0x1C, "Invalid CXml child element bin size!");

struct SysutilPacketInfo {
    be_t<u32> slot;
    be_t<u32> evnt;
    be_t<u32> size;
};

// these structs are awful and im not sure exactly if they are right, but im just going with it

struct SysutilSharedMemInfo {
    be_t<u32> unk1;         // 0x00 // 0x400 default, some sort of offset between..packet headers?
    be_t<u32> size;         // 0x04 // 0x7C00 hardset ... probly should allocate this somewhere
    be_t<u32> unk3;         // 0x08 // only changes during 'write' pending maybe?
    be_t<u32> unk4;         // 0x0C // only changes during 'read'
    be_t<u32> bytesWritten; // 0x10 
    be_t<u32> unk6;         // 0x14 //status?
    be_t<u32> unk7;         // 0x18
    u8 unk8;                // 0x1C // event is 0? and in use
    u8 packetInUse;         // 0x1D // packet in use for non 0 event?
    u8 unk9;                // 0x1E // 0? this might just be an abort flag
    u8 unk10;               // 0x1F
};

static_assert(sizeof(SysutilSharedMemInfo) == 0x20, "Invalid SysutilSharedMemInfo size");

struct SysutilSlotSync {
    vm::bptr<SysutilSharedMemInfo> sharedMemInfo; // 0x0
    be_t<u32> shm_mutex{ 0 }; // 0x4
    be_t<u32> nem_cond{ 0 }; // 0x8 
    be_t<u32> nfl_cond{ 0 }; // 0xC
    be_t<u32> stf_cond{ 0 }; // 0x10
    be_t<u32> ctf_cond{ 0 }; // 0x14
    be_t<s32> evnt{ -1 };    //  0x18
    be_t<u32> sharedAddr{ 0 }; // ??? this doesnt go here but i dont know where else to put it...
};

//static_assert(sizeof(SysutilSlotSync) == 0x18, "Invalid SysutilSlotSync size");

class sysutil_service_manager final : public named_thread
{
    void on_task() override;

    std::string get_name() const override { return "Sysutil Service Manager"; }

    u32 internalPacketRead();

    vm::ptr<char> internalCxmlBuffer = vm::null;
    vm::ptr<cxml_document> internalCxmlDoc = vm::null;
    vm::ptr<cxml_attribute> internalCxmlAttr = vm::null;
    vm::ptr<cxml_element> internalCxmlElement = vm::null;

    std::shared_ptr<ppu_thread> internalThread;

public:
    // packet stuff may want to be vm::gvar instead 
    atomic_t<bool> packetMemInUse{ false };
    std::array<bool, 6> memoryInUse{ 0 };

    vm::ptr<char> cxml_packet_mem = vm::null;

    SysutilSlotSync slotSync; // todo: there should be 1 per slot

    void on_init(const std::shared_ptr<void>&) override;

    std::vector<u64> keys;

    semaphore<> mutex;

    sysutil_service_manager() = default;

    ~sysutil_service_manager()
    {
        vm::dealloc_verbose_nothrow(cxml_packet_mem.addr());
        vm::dealloc_verbose_nothrow(slotSync.sharedMemInfo.addr());
        vm::dealloc_verbose_nothrow(slotSync.sharedAddr);
        vm::dealloc_verbose_nothrow(internalCxmlBuffer.addr());
        vm::dealloc_verbose_nothrow(internalCxmlDoc.addr());
        vm::dealloc_verbose_nothrow(internalCxmlAttr.addr());
        vm::dealloc_verbose_nothrow(internalCxmlElement.addr());
    }
};

void sysutil_service_manager::on_init(const std::shared_ptr<void> &_this) {
    // this is needed for local sysutil packet
    cxml_packet_mem.set(vm::alloc(0x6000, vm::main));

    // internal use
    internalCxmlBuffer.set(vm::alloc(0x6000, vm::main));
    internalCxmlDoc.set(vm::alloc(sizeof(cxml_document), vm::main));
    internalCxmlAttr.set(vm::alloc(sizeof(cxml_attribute), vm::main));
    internalCxmlElement.set(vm::alloc(sizeof(cxml_element), vm::main));

    // this will be used as 'shared mem' between the sysutil service and sysutil;
    // these technically have names and ipc keys and all that jazz, but that's being ignored, as its not supported currently

    // also theres like double these for both slots of the packets

    slotSync.sharedMemInfo.set(vm::alloc(sizeof(SysutilSharedMemInfo), vm::main));

    memset(slotSync.sharedMemInfo.get_ptr(), 0, sizeof(SysutilSharedMemInfo));

    slotSync.sharedMemInfo->unk1 = 0x400;
    slotSync.sharedMemInfo->size = 0x7C00;
    slotSync.sharedAddr = vm::alloc(0x7C00, vm::main);

    vm::var<u32> mutexid;
    vm::var<sys_mutex_attribute_t> attr;
    attr->protocol = SYS_SYNC_PRIORITY;
    attr->recursive = SYS_SYNC_NOT_RECURSIVE;
    attr->pshared = SYS_SYNC_NOT_PROCESS_SHARED; // hack, this is supposed to be processes shared;
    attr->adaptive = SYS_SYNC_NOT_ADAPTIVE;
    attr->ipc_key = 0; // should be, 0x8006010000000020? or 0x8006010000000021 for slot 1
    attr->flags = 0; // flags are also something,
    strcpy_trunc(attr->name, "_s__shm");

    error_code ret = sys_mutex_create(mutexid, attr);
    if (ret != 0)
        fmt::throw_exception("sysutil-GetServiceManager failed to create cond");

    slotSync.shm_mutex = *mutexid;

    vm::var<u32> condid;
    vm::var<sys_cond_attribute_t> condAttr;
    condAttr->pshared = SYS_SYNC_NOT_PROCESS_SHARED; // hack, supposed to be shared
    condAttr->flags = 0;
    condAttr->ipc_key = 0; // 0x8006010000000030 ? 
    strcpy_trunc(condAttr->name, "_s__nem"); // sysutil looks like it makes multiple conds based on the same mutex

    ret = sys_cond_create(condid, *mutexid, condAttr);
    if (ret != CELL_OK)
        fmt::throw_exception("sysutil-GetServiceManager failed to create cond");

    slotSync.nem_cond = *condid;

    condAttr->ipc_key = 0; // 0x8006010000000040 ? 
    strcpy_trunc(condAttr->name, "_s__nfl");

    ret = sys_cond_create(condid, *mutexid, condAttr);
    if (ret != CELL_OK)
        fmt::throw_exception("sysutil-GetServiceManager failed to create cond");

    slotSync.nfl_cond = *condid;

    condAttr->ipc_key = 0; // 0x8006010000000050 ? 
    strcpy_trunc(condAttr->name, "_s__stf");

    ret = sys_cond_create(condid, *mutexid, condAttr);
    if (ret != CELL_OK)
        fmt::throw_exception("sysutil-GetServiceManager failed to create cond");

    slotSync.stf_cond = *condid;

    condAttr->ipc_key = 0; // 0x8006010000000060 ? 
    strcpy_trunc(condAttr->name, "_s__ctf");

    ret = sys_cond_create(condid, *mutexid, condAttr);
    if (ret != CELL_OK)
        fmt::throw_exception("sysutil-GetServiceManager failed to create cond");

    slotSync.ctf_cond = *condid;

    // this is just used to route the locks to, this is probly a 'bad idea' *tm
    internalThread = idm::make_ptr<ppu_thread>("SceVshSysutil", 1, 0x4000);
    internalThread->run();

    named_thread::on_init(_this);
}

// this is very similar to packetRead for now, it just ignores the event
u32 sysutil_service_manager::internalPacketRead() {
    u32 bufferLeft = 0x6000;

    u8 isDone = 0;

    u32 addr = internalCxmlBuffer.addr();
    while (bufferLeft != 0 && isDone == 0) {
        internalThread->cmd_list
        ({
            { ppu_cmd::set_args, 2 }, u64{ slotSync.shm_mutex }, u64{ 0 },
            { ppu_cmd::hle_call, FIND_FUNC(sys_mutex_lock) },
        });

        //if (ret != CELL_OK)
        //    fmt::throw_exception("internalPacketRead: lock failed1");

        while ((slotSync.sharedMemInfo->unk7 == 2) || slotSync.sharedMemInfo->bytesWritten == 0) {
            if (slotSync.sharedMemInfo->unk9 != 0) {
                internalThread->cmd_list
                ({
                    { ppu_cmd::set_args, 1 }, u64{ slotSync.shm_mutex },
                    { ppu_cmd::hle_call, FIND_FUNC(sys_mutex_unlock) },
                });
                fmt::throw_exception("internalPacketRead: unk9"); // no idea what this return signifiys 
            }

            internalThread->cmd_list
            ({
                { ppu_cmd::set_args, 2 }, u64{ slotSync.nem_cond }, u64{ 0 },
                { ppu_cmd::hle_call, FIND_FUNC(sys_cond_wait) },
            });

            //if (ret != CELL_OK)
            //   fmt::throw_exception("internalPacketRead: wait failed");
        }

        u32 rawr = slotSync.sharedMemInfo->bytesWritten;
        while (slotSync.sharedMemInfo->bytesWritten != 0) {
            s32 tmp = slotSync.sharedMemInfo->unk3 - slotSync.sharedMemInfo->unk4;

            if (slotSync.sharedMemInfo->unk3 <= slotSync.sharedMemInfo->unk4)
            {
                tmp = slotSync.sharedMemInfo->size - slotSync.sharedMemInfo->unk4;
                if (tmp > bufferLeft)
                    tmp = bufferLeft;
            }
            else if (slotSync.sharedMemInfo->unk3 - slotSync.sharedMemInfo->unk4 <= bufferLeft)
            {
                // purposely left empty
            }
            else {
                tmp = bufferLeft;
            }
            u32 addrSrc = slotSync.sharedAddr + slotSync.sharedMemInfo->unk4;
            std::memcpy(vm::base(addr), vm::base(addrSrc), tmp);

            bufferLeft -= tmp;
            addr += tmp;
            slotSync.sharedMemInfo->unk4 += tmp;

            if (slotSync.sharedMemInfo->unk4 >= slotSync.sharedMemInfo->size)
                slotSync.sharedMemInfo->unk4 = 0;

            slotSync.sharedMemInfo->bytesWritten -= tmp;

            if (bufferLeft == 0)
                break;
        }

        if (slotSync.sharedMemInfo->unk6 == 2 && slotSync.sharedMemInfo->bytesWritten == 0) {
            isDone = 1;
            slotSync.sharedMemInfo->unk6 = 0;
            slotSync.sharedMemInfo->unk7 = 0;
        }

        internalThread->cmd_list
        ({
            { ppu_cmd::set_args, 1 }, u64{ slotSync.nfl_cond },
            { ppu_cmd::hle_call, FIND_FUNC(sys_cond_signal) },
        });

        //if (ret != CELL_OK)
         //   fmt::throw_exception("internalPacketRead: unlock failed 1");

        internalThread->cmd_list
        ({
            { ppu_cmd::set_args, 1 }, u64{ slotSync.shm_mutex },
            { ppu_cmd::hle_call, FIND_FUNC(sys_mutex_unlock) },
        });
        
        //if (ret != CELL_OK)
        //   fmt::throw_exception("internalPacketRead: unlock failed2");
    }

    if (isDone == 0)
        fmt::throw_exception("internalPacketRead: not done..buffer full?");

    return 0x6000 - bufferLeft;
}

s32 _ZN4cxml8Document16CreateFromBufferEPKvjb(vm::ptr<cxml_document> cxmlDoc, vm::ptr<void> buf, u32 size, u8 accessMode);
vm::cptr<char> GetStringAddrFromNameOffset(vm::ptr<cxml_document> doc, s32 nameOffset);
u32 _ZN4cxml8Document18GetDocumentElementEv(vm::ptr<cxml_element> element, vm::ptr<cxml_document> doc);

void sysutil_service_manager::on_task() {

    u32 sizeOfPacket = internalPacketRead();
    // k we should hopefully have the packet now in our 'internal' memory
    
    s32 ret = _ZN4cxml8Document16CreateFromBufferEPKvjb(internalCxmlDoc, internalCxmlBuffer, sizeOfPacket, 0);
    if (ret != CELL_OK)
        fmt::throw_exception("sysutilService:createfrombuffer fail");

    if (std::memcmp(internalCxmlDoc->header.magic, "NPTR", 4) == 0) {
        _ZN4cxml8Document18GetDocumentElementEv(internalCxmlElement, internalCxmlDoc);
        if (internalCxmlElement->doc == vm::null)
            fmt::throw_exception("sysutilservice, no root element");

        const auto& element = vm::_ref<cxml_childelement_bin>(internalCxmlDoc->tree.addr() + internalCxmlElement->offset + sizeof(cxml_childelement_bin));
        
        auto str = GetStringAddrFromNameOffset(internalCxmlDoc, element.name);
        if (str == vm::null)
            fmt::throw_exception("sysutilservice: name is null");
        if (std::memcmp(str.get_ptr(), "f", 0) != 0)
            fmt::throw_exception("sysutilservice: wrong element, name=%s", str);
        
    }
    else
        fmt::throw_exception("sysutilService unsupported packet");
}

struct sysutil_cb_manager
{
	std::mutex mutex;

	std::array<std::pair<vm::ptr<CellSysutilCallback>, vm::ptr<void>>, 4> callbacks;

	std::queue<std::function<s32(ppu_thread&)>> registered;

	std::function<s32(ppu_thread&)> get_cb()
	{
		std::lock_guard<std::mutex> lock(mutex);

		if (registered.empty())
		{
			return nullptr;
		}

		auto func = std::move(registered.front());

		registered.pop();

		return func;
	}
};

extern void sysutil_register_cb(std::function<s32(ppu_thread&)>&& cb)
{
	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	std::lock_guard<std::mutex> lock(cbm->mutex);

	cbm->registered.push(std::move(cb));
}

extern void sysutil_send_system_cmd(u64 status, u64 param)
{
	if (const auto cbm = fxm::get<sysutil_cb_manager>())
	{
		for (auto& cb : cbm->callbacks)
		{
			if (cb.first)
			{
				std::lock_guard<std::mutex> lock(cbm->mutex);

				cbm->registered.push([=](ppu_thread& ppu) -> s32
				{
					// TODO: check it and find the source of the return value (void isn't equal to CELL_OK)
					cb.first(ppu, status, param, cb.second);
					return CELL_OK;
				});
			}
		}
	}
}

template <>
void fmt_class_string<CellSysutilLang>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellSysutilLang value)
	{
		switch (value)
		{
		case CELL_SYSUTIL_LANG_JAPANESE: return "Japanese";
		case CELL_SYSUTIL_LANG_ENGLISH_US: return "English (US)";
		case CELL_SYSUTIL_LANG_FRENCH: return "French";
		case CELL_SYSUTIL_LANG_SPANISH: return "Spanish";
		case CELL_SYSUTIL_LANG_GERMAN: return "German";
		case CELL_SYSUTIL_LANG_ITALIAN: return "Italian";
		case CELL_SYSUTIL_LANG_DUTCH: return "Dutch";
		case CELL_SYSUTIL_LANG_PORTUGUESE_PT: return "Portuguese (PT)";
		case CELL_SYSUTIL_LANG_RUSSIAN: return "Russian";
		case CELL_SYSUTIL_LANG_KOREAN: return "Korean";
		case CELL_SYSUTIL_LANG_CHINESE_T: return "Chinese (Trad.)";
		case CELL_SYSUTIL_LANG_CHINESE_S: return "Chinese (Simp.)";
		case CELL_SYSUTIL_LANG_FINNISH: return "Finnish";
		case CELL_SYSUTIL_LANG_SWEDISH: return "Swedish";
		case CELL_SYSUTIL_LANG_DANISH: return "Danish";
		case CELL_SYSUTIL_LANG_NORWEGIAN: return "Norwegian";
		case CELL_SYSUTIL_LANG_POLISH: return "Polish";
		case CELL_SYSUTIL_LANG_ENGLISH_GB: return "English (UK)";
		case CELL_SYSUTIL_LANG_PORTUGUESE_BR: return "Portuguese (BR)";
		case CELL_SYSUTIL_LANG_TURKISH: return "Turkish";
		}

		return unknown;
	});
}

// For test
enum systemparam_id_name : s32 {};

template <>
void fmt_class_string<systemparam_id_name>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG: return "ID_LANG";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN: return "ID_ENTER_BUTTON_ASSIGN";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT: return "ID_DATE_FORMAT";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT: return "ID_TIME_FORMAT";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE: return "ID_TIMEZONE";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME: return "ID_SUMMERTIME";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL: return "ID_GAME_PARENTAL_LEVEL";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT: return "ID_GAME_PARENTAL_LEVEL0_RESTRICT";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT: return "ID_CURRENT_USER_HAS_NP_ACCOUNT";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ: return "ID_CAMERA_PLFREQ";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE: return "ID_PAD_RUMBLE";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE: return "ID_KEYBOARD_TYPE";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD: return "ID_JAPANESE_KEYBOARD_ENTRY_METHOD";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD: return "ID_CHINESE_KEYBOARD_ENTRY_METHOD";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF: return "ID_PAD_AUTOOFF";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME: return "ID_NICKNAME";
		case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME: return "ID_CURRENT_USERNAME";
		}

		return unknown;
	});
}

s32 cellSysutilGetSystemParamInt(systemparam_id_name id, vm::ptr<s32> value)
{
	cellSysutil.warning("cellSysutilGetSystemParamInt(id=0x%x(%s), value=*0x%x)", id, id, value);

	// TODO: load this information from config (preferably "sys/" group)

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_LANG:
		*value = g_cfg.sys.language;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_ENTER_BUTTON_ASSIGN:
		*value = CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_DATE_FORMAT:
		*value = CELL_SYSUTIL_DATE_FMT_DDMMYYYY;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIME_FORMAT:
		*value = CELL_SYSUTIL_TIME_FMT_CLOCK24;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_TIMEZONE:
		*value = 180;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_SUMMERTIME:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL:
		*value = CELL_SYSUTIL_GAME_PARENTAL_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_GAME_PARENTAL_LEVEL0_RESTRICT:
		*value = CELL_SYSUTIL_GAME_PARENTAL_LEVEL0_RESTRICT_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USER_HAS_NP_ACCOUNT:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CAMERA_PLFREQ:
		*value = CELL_SYSUTIL_CAMERA_PLFREQ_DISABLED;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_RUMBLE:
		*value = CELL_SYSUTIL_PAD_RUMBLE_OFF;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_KEYBOARD_TYPE:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_JAPANESE_KEYBOARD_ENTRY_METHOD:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CHINESE_KEYBOARD_ENTRY_METHOD:
		*value = 0;
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_PAD_AUTOOFF:
		*value = 0;
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 cellSysutilGetSystemParamString(systemparam_id_name id, vm::ptr<char> buf, u32 bufsize)
{
	cellSysutil.trace("cellSysutilGetSystemParamString(id=0x%x(%s), buf=*0x%x, bufsize=%d)", id, id, buf, bufsize);

	memset(buf.get_ptr(), 0, bufsize);

	switch(id)
	{
	case CELL_SYSUTIL_SYSTEMPARAM_ID_NICKNAME:
		memcpy(buf.get_ptr(), "Unknown", 8); // for example
	break;

	case CELL_SYSUTIL_SYSTEMPARAM_ID_CURRENT_USERNAME:
		memcpy(buf.get_ptr(), "Unknown", 8);
	break;

	default:
		return CELL_EINVAL;
	}

	return CELL_OK;
}

s32 cellSysutilCheckCallback(ppu_thread& ppu)
{
	cellSysutil.trace("cellSysutilCheckCallback()");

	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	while (auto&& func = cbm->get_cb())
	{
		if (s32 res = func(ppu))
		{
			return res;
		}

		thread_ctrl::test();
	}

	return CELL_OK;
}

s32 cellSysutilRegisterCallback(s32 slot, vm::ptr<CellSysutilCallback> func, vm::ptr<void> userdata)
{
	cellSysutil.warning("cellSysutilRegisterCallback(slot=%d, func=*0x%x, userdata=*0x%x)", slot, func, userdata);

	if (slot >= 4)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	cbm->callbacks[slot] = std::make_pair(func, userdata);

	return CELL_OK;
}

s32 cellSysutilUnregisterCallback(u32 slot)
{
	cellSysutil.warning("cellSysutilUnregisterCallback(slot=%d)", slot);

	if (slot >= 4)
	{
		return CELL_SYSUTIL_ERROR_VALUE;
	}

	const auto cbm = fxm::get_always<sysutil_cb_manager>();

	cbm->callbacks[slot] = std::make_pair(vm::null, vm::null);

	return CELL_OK;
}

s32 cellSysCacheClear(void)
{
	cellSysutil.todo("cellSysCacheClear()");

	if (!fxm::check<CellSysCacheParam>())
	{
		return CELL_SYSCACHE_ERROR_NOTMOUNTED;
	}

	const std::string& local_path = vfs::get("/dev_hdd1/cache/");

	// TODO: Write tests to figure out, what is deleted.

	return CELL_SYSCACHE_RET_OK_CLEARED;
}

s32 cellSysCacheMount(vm::ptr<CellSysCacheParam> param)
{
	cellSysutil.warning("cellSysCacheMount(param=*0x%x)", param);

	const std::string& cache_id = param->cacheId;
	verify(HERE), cache_id.size() < sizeof(param->cacheId);
	
	const std::string& cache_path = "/dev_hdd1/cache/" + cache_id + '/';
	strcpy_trunc(param->getCachePath, cache_path);

	// TODO: implement (what?)
	fs::create_dir(vfs::get(cache_path));
	fxm::make_always<CellSysCacheParam>(*param);

	return CELL_SYSCACHE_RET_OK_RELAYED;
}

bool g_bgm_playback_enabled = true;

s32 cellSysutilEnableBgmPlayback()
{
	cellSysutil.warning("cellSysutilEnableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = true;

	return CELL_OK;
}

s32 cellSysutilEnableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.warning("cellSysutilEnableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = true; 

	return CELL_OK;
}

s32 cellSysutilDisableBgmPlayback()
{
	cellSysutil.warning("cellSysutilDisableBgmPlayback()");

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

s32 cellSysutilDisableBgmPlaybackEx(vm::ptr<CellSysutilBgmPlaybackExtraParam> param)
{
	cellSysutil.warning("cellSysutilDisableBgmPlaybackEx(param=*0x%x)", param);

	// TODO
	g_bgm_playback_enabled = false;

	return CELL_OK;
}

s32 cellSysutilGetBgmPlaybackStatus(vm::ptr<CellSysutilBgmPlaybackStatus> status)
{
	cellSysutil.warning("cellSysutilGetBgmPlaybackStatus(status=*0x%x)", status);

	// TODO
	status->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	status->enableState = g_bgm_playback_enabled ? CELL_SYSUTIL_BGMPLAYBACK_STATUS_ENABLE : CELL_SYSUTIL_BGMPLAYBACK_STATUS_DISABLE;
	status->currentFadeRatio = 0; // current volume ratio (0%)
	memset(status->contentId, 0, sizeof(status->contentId));
	memset(status->reserved, 0, sizeof(status->reserved));

	return CELL_OK;
}

s32 cellSysutilGetBgmPlaybackStatus2(vm::ptr<CellSysutilBgmPlaybackStatus2> status2)
{
	cellSysutil.warning("cellSysutilGetBgmPlaybackStatus2(status2=*0x%x)", status2);

	// TODO
	status2->playerState = CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP;
	memset(status2->reserved, 0, sizeof(status2->reserved));

	return CELL_OK;
}

s32 cellSysutilSetBgmPlaybackExtraParam()
{
	cellSysutil.todo("cellSysutilSetBgmPlaybackExtraParam()");
	return CELL_OK;
}

s32 cellSysutilRegisterCallbackDispatcher(u32 evnt, vm::ptr<CellSysutilCallback> callback)
{
    cellSysutil.todo("cellSysutilRegisterCallbackDispatcher(evnt=0x%x, callback=*0x%x)", evnt, callback);
    return CELL_OK;
}

s32 cellSysutilUnregisterCallbackDispatcher()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilPacketRead(ppu_thread& ppu, u32 slot, u32 evnt, u32 addr, u32 bufferSize, vm::ptr<u8> done)
{
    cellSysutil.error("cellSysutilPacketRead(slot=%d, event=0x%x, addr=*0x%x, bufferSize=0x%x, outVar=*0x%x)", slot, evnt, addr, bufferSize, done);

    if (slot > 1)
        return CELL_SYSUTIL_ERROR_INVALID_PACKET_SLOT;

    const auto m = fxm::get_always<sysutil_service_manager>();
    if (m->slotSync.evnt != evnt)
        return CELL_SYSUTIL_ERROR_NO_PACKET_FOR_EVENT;

    u32 bufferLeft = bufferSize;

    u8 isDone = 0;

    while (bufferLeft != 0 && isDone == 0) {
        error_code ret = sys_mutex_lock(ppu, m->slotSync.shm_mutex, 0);
        if (ret != CELL_OK)
            return ret;

        while ((((evnt == 0) && m->slotSync.sharedMemInfo->unk7 == 1) || ((evnt != 0) && m->slotSync.sharedMemInfo->unk7 == 2)) || m->slotSync.sharedMemInfo->bytesWritten == 0) {
            if (m->slotSync.sharedMemInfo->unk9 != 0) {
                sys_mutex_unlock(ppu, m->slotSync.shm_mutex);
                return CELL_SYSUTIL_ERROR_UNK_ERROR; // no idea what this return signifiys 
            }

            ret = sys_cond_wait(ppu, m->slotSync.nem_cond, 0);
            if (ret != CELL_OK)
                return ret;
        }

        u32 rawr = m->slotSync.sharedMemInfo->bytesWritten;
        while (m->slotSync.sharedMemInfo->bytesWritten != 0) {
            s32 tmp = m->slotSync.sharedMemInfo->unk3 - m->slotSync.sharedMemInfo->unk4;

            if (m->slotSync.sharedMemInfo->unk3 <= m->slotSync.sharedMemInfo->unk4)
            {
                tmp = m->slotSync.sharedMemInfo->size - m->slotSync.sharedMemInfo->unk4;
                if (tmp > bufferLeft)
                    tmp = bufferLeft;
            }
            else if (m->slotSync.sharedMemInfo->unk3 - m->slotSync.sharedMemInfo->unk4 <= bufferLeft)
            {
                // purposely left empty
            }
            else {
                tmp = bufferLeft;
            }
            u32 addrSrc = m->slotSync.sharedAddr + m->slotSync.sharedMemInfo->unk4;
            std::memcpy(vm::base(addr), vm::base(addrSrc), tmp);

            bufferLeft -= tmp;
            addr += tmp;
            m->slotSync.sharedMemInfo->unk4 += tmp;

            if (m->slotSync.sharedMemInfo->unk4 >= m->slotSync.sharedMemInfo->size)
                m->slotSync.sharedMemInfo->unk4 = 0;

            m->slotSync.sharedMemInfo->bytesWritten -= tmp;

            if (bufferLeft == 0)
                break;
        }

        if (m->slotSync.sharedMemInfo->unk6 == 2 && m->slotSync.sharedMemInfo->bytesWritten == 0) {
            isDone = 1;
            m->slotSync.sharedMemInfo->unk6 = 0;
            m->slotSync.sharedMemInfo->unk7 = 0;
        }

        ret = sys_cond_signal(ppu, m->slotSync.nfl_cond);
        if (ret != CELL_OK)
            return ret;

        ret = sys_mutex_unlock(ppu, m->slotSync.shm_mutex);
        if (ret != CELL_OK)
            return ret;
    }

    if (done != vm::null)
        *done = isDone;
    else if (isDone == 0)
        return CELL_SYSUTIL_ERROR_UNK_ERROR2;

    return bufferSize - bufferLeft;
}

s32 cellSysutilPacketWrite(ppu_thread& ppu, u32 slot, u32 evnt, u32 addr, u32 size, u32 end)
{
    cellSysutil.error("cellSysutilPacketWrite(slot=%d, event=0x%x, addr=*0x%x, size=0x%x, end=%d)", slot, evnt, addr, size, end);
    if (size == 0)
        return CELL_OK;

    if (slot > 1)
        return CELL_SYSUTIL_ERROR_INVALID_PACKET_SLOT;

    const auto m = fxm::get_always<sysutil_service_manager>();
    if (m->slotSync.evnt != evnt)
        return CELL_SYSUTIL_ERROR_NO_PACKET_FOR_EVENT;
    
    while (size != 0) {
        error_code ret = sys_mutex_lock(ppu, m->slotSync.shm_mutex, 0);
        if (ret != CELL_OK)
            return ret;

        while (m->slotSync.sharedMemInfo->unk6 == 2 || m->slotSync.sharedMemInfo->bytesWritten >= m->slotSync.sharedMemInfo->size) {
            if (m->slotSync.sharedMemInfo->unk9 != 0) {
                sys_mutex_unlock(ppu, m->slotSync.shm_mutex);
                return CELL_SYSUTIL_ERROR_UNK_ERROR; // no idea what this return signifiys 
            }

            ret = sys_cond_wait(ppu, m->slotSync.nfl_cond, 0);
            if (ret != CELL_OK)
                return ret;
        }

        m->slotSync.sharedMemInfo->unk6 = 1;

        // *i think* this just sets 2 if event is 0, or 1 if its not
        m->slotSync.sharedMemInfo->unk7 = evnt == 0 ? 2 : 1;

        while (m->slotSync.sharedMemInfo->bytesWritten < m->slotSync.sharedMemInfo->size) {
            s32 tmp = (s32)m->slotSync.sharedMemInfo->unk4 - m->slotSync.sharedMemInfo->unk3;
            if (m->slotSync.sharedMemInfo->unk4 <= m->slotSync.sharedMemInfo->unk3) {
                tmp = m->slotSync.sharedMemInfo->size - m->slotSync.sharedMemInfo->unk3;
                if (tmp > size)
                    tmp = size;
            }
            else if ((s32)m->slotSync.sharedMemInfo->unk4 - m->slotSync.sharedMemInfo->unk3 <= size) {
                // purposely empty for now
            }
            else {
                tmp = size;
            }

            u32 addrDst = m->slotSync.sharedAddr + m->slotSync.sharedMemInfo->unk3;
            memcpy(vm::base(addrDst), vm::base(addr), size);
            size -= tmp;
            addr += tmp;

            u32 tmp2 = tmp + m->slotSync.sharedMemInfo->unk3;
            m->slotSync.sharedMemInfo->unk3 = tmp2;
            if (tmp2 >= m->slotSync.sharedMemInfo->size) {
                m->slotSync.sharedMemInfo->unk3 = 0;
            }

            m->slotSync.sharedMemInfo->bytesWritten += tmp;

            if (size == 0)
                break;
        }

        if (end != 0 && size == 0)
            m->slotSync.sharedMemInfo->unk6 = 2;

        ret = sys_cond_signal(ppu, m->slotSync.nem_cond);
        if (ret != CELL_OK)
            return ret;

        ret = sys_mutex_unlock(ppu, m->slotSync.shm_mutex);
        if (ret != CELL_OK)
            return ret;
    }

    return CELL_OK;
}

// something special happens if a2 is 0, but w/e
s32 cellSysutilPacketBegin(ppu_thread& ppu, u32 slot, u32 evnt)
{
    cellSysutil.error("cellSysutilPacketBegin(slot=0x%x, evnt=0x%x)", slot, evnt);

    // only 0 and 1 are valid slots
    if (slot > 1)
        return CELL_SYSUTIL_ERROR_INVALID_PACKET_SLOT;

    // too lazy to deal with multiple slots for now, slot 0 only looks to be used by the actual sysutil.lib anyway
    if (slot == 0)
        fmt::throw_exception("unimplemented slot... deal with slot 0");

    const auto m = fxm::get_always<sysutil_service_manager>();

    error_code ret = sys_mutex_lock(ppu, m->slotSync.shm_mutex, 0);
    if (ret != CELL_OK)
        return ret;

    // this does something slightly different for some dumb reason
    if (evnt == 0)
        fmt::throw_exception("unimplemented event. deal with event 0");

    while (m->slotSync.sharedMemInfo->packetInUse == 1) {
        if (m->slotSync.sharedMemInfo->unk9 != 0) {
            sys_mutex_unlock(ppu, m->slotSync.shm_mutex);
            return CELL_SYSUTIL_ERROR_UNK_ERROR; // no idea what this return signifiys 
        }

        ret = sys_cond_wait(ppu, m->slotSync.ctf_cond, 0);
        if (ret != CELL_OK)
            return ret;
    }

    m->slotSync.sharedMemInfo->packetInUse = 1;
    m->slotSync.evnt = evnt;

    ret = sys_mutex_unlock(ppu, m->slotSync.shm_mutex);
    return ret;
}

// this returns some address inside of sysutil. tracing through calls, it looks like this is shoved into the 'allocator_userdata' of a the cxml document packet
// it also sets something to 1 at a memory location
u32 cellSysutil_B47470E1(u32 slot)
{
    cellSysutil.todo("cellSysutil_B47470E1(slot=0x%x)", slot);
    const auto m = fxm::get_always<sysutil_service_manager>();

    // todo: deal with the multiple spots
    if (m->packetMemInUse.compare_and_swap_test(false, true))
        return m->cxml_packet_mem.addr();
    return 0;
}

// stores something at the address given above based on slot
void cellSysutil_40719C8C(u32 slot)
{
    cellSysutil.todo("cellSysutil_40719C8C(slot=0x%x)", slot);
    const auto m = fxm::get_always<sysutil_service_manager>();

    m->packetMemInUse = false;
}

s32 cellSysutilPacketEnd(ppu_thread& ppu, u32 slot, u32 evnt)
{
    cellSysutil.error("cellSysutilPacketEnd(slot=0x%x, evnt=0x%x)", slot, evnt);
    if (slot > 1)
        return CELL_SYSUTIL_ERROR_INVALID_PACKET_SLOT;

    // too lazy to deal with multiple slots for now, slot 0 only looks to be used by the actual sysutil.lib anyway
    if (slot == 0)
        fmt::throw_exception("unimplemented slot... deal with slot 0");

    const auto m = fxm::get_always<sysutil_service_manager>();

    error_code ret = sys_mutex_lock(ppu, m->slotSync.shm_mutex, 0);
    if (ret != CELL_OK)
        return ret;

    // this does something slightly different for some dumb reason
    if (evnt == 0)
        fmt::throw_exception("unimplemented event. deal with event 0");

    ret = sys_cond_signal(ppu, m->slotSync.ctf_cond);
    if (ret != CELL_OK)
        return ret;

    m->slotSync.sharedMemInfo->packetInUse = 0;
    m->slotSync.evnt = -1;

    ret = sys_mutex_unlock(ppu, m->slotSync.shm_mutex);
    return ret;
}

s32 cellSysutilGameDataAssignVmc()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGameDataExit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGameExit_I()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGamePowerOff_I()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilGameReboot_I()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilSharedMemoryAlloc()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSysutilSharedMemoryFree()
{
	fmt::throw_exception("Unimplemented" HERE);
}

// TODO: Currently we are missing some special reloc info when loading an sprx (i've only seen them in sysutil_* libraries), but the allocator callback address ends up with a function pointer that isnt relocated
// there *seems* to be reloc info at the end of the fnid tables that should be parsed as elf32_rel or similar, but i cant figure out the exact method of figuring out where that data is from the segment tables
// even if it were reloc'd correctly as is, it looks like the callback points in the middle a function, which as far as i can see, would screw up the stack pointer
// so until whatever happening with those callback reloc's is figured out, this is just called manually instead, which looks to be the correct function anyway

void cellSysutil_B59872EF(u32 allocType, vm::ptr<void> userData, vm::ptr<void> oldAddr, u32 requiredSize, vm::ptr<u32> addr, vm::ptr<u32> size)
{
    cellSysutil.trace("cellSysutil_B59872EF(allocType=%d, userData=*0x%x, oldAddr=*0x%x, reqSize=%d, addr=*0x%x, size=*0x%x)", allocType, userData, oldAddr, requiredSize, addr, size);
    // todo: use userData memaddr
    const auto m = fxm::get_always<sysutil_service_manager>();
    if (size != vm::null)
        *size = 0;
    if (addr != vm::null)
        *addr = 0;

    switch (allocType) {
    case AllocationType_Alloc_Tree:
    case AllocationType_Alloc_IDTable:
    case AllocationType_Alloc_StringTable:
    case AllocationType_Alloc_IntArrayTable:
    case AllocationType_Alloc_FileTable:
        if (m->memoryInUse[allocType] == true || requiredSize > 0x1000)
            return;
        m->memoryInUse[allocType] == true;
        *addr = m->cxml_packet_mem.addr() + allocType * 0x1000;
        *size = 0x1000;
        return;
    case AllocationType_Free_Tree:
    case AllocationType_Free_IDTable:
    case AllocationType_Free_StringTable:
    case AllocationType_Free_IntArrayTable:
    case AllocationType_Free_FileTable:
        m->memoryInUse[allocType - 6] = false;
        return;

    }
}

// Constructor for blank cxml document
s32 _ZN4cxml8DocumentC1Ev(vm::ptr<cxml_document> cxmlDoc)
{
    cellSysutil.error("_ZN4cxml8DocumentC1Ev(cxmlDoc=*0x%x)", cxmlDoc);

    std::memset(cxmlDoc.get_ptr(), 0, sizeof(cxml_document));
    cxmlDoc->accessMode = 0;
    cxmlDoc->header.version = 0x110;
    std::memcpy(cxmlDoc->header.magic, "CXML", 4);

	return CELL_OK;
}

s32 _ZN4cxml8Document5ClearEv(vm::ptr<cxml_document> cxmlDoc)
{
    cellSysutil.error("_ZN4cxml8Document5ClearEv(cxmlDoc=*0x%x)", cxmlDoc);

    std::memset(&cxmlDoc->header, 0, sizeof(cxml_document::Header));
    cxmlDoc->header.version = 0x110;
    std::memcpy(cxmlDoc->header.magic, "CXML", 4);

    if (cxmlDoc->accessMode == 2)
    {
        cxmlDoc->accessMode = 0;
        return CELL_OK;
    }

    if (cxmlDoc->allocator == vm::null)
        return CELL_SYSUTIL_CXML_NO_ALLOCATOR;

    if (cxmlDoc->tree != vm::null)
        cellSysutil_B59872EF(AllocationType_Free_Tree, cxmlDoc->allocator_userdata, cxmlDoc->tree, 0, vm::null, vm::null);

    cxmlDoc->tree.set(0);
    cxmlDoc->tree_capacity = 0;

    if (cxmlDoc->idtable != vm::null)
        cellSysutil_B59872EF(AllocationType_Free_IDTable, cxmlDoc->allocator_userdata, cxmlDoc->idtable, 0, vm::null, vm::null);

    cxmlDoc->idtable.set(0);
    cxmlDoc->idtable_capacity = 0;

    if (cxmlDoc->stringtable != vm::null)
        cellSysutil_B59872EF(AllocationType_Free_StringTable, cxmlDoc->allocator_userdata, cxmlDoc->stringtable, 0, vm::null, vm::null);

    cxmlDoc->stringtable.set(0);
    cxmlDoc->stringtable_capacity = 0;

    if (cxmlDoc->intarraytable != vm::null)
        cellSysutil_B59872EF(AllocationType_Free_IntArrayTable, cxmlDoc->allocator_userdata, cxmlDoc->intarraytable, 0, vm::null, vm::null);

    cxmlDoc->intarraytable.set(0);
    cxmlDoc->intarraytable_capacity = 0;

    if (cxmlDoc->floatarraytable != vm::null)
        cellSysutil_B59872EF(AllocationType_Free_FloatArrayTable, cxmlDoc->allocator_userdata, cxmlDoc->floatarraytable, 0, vm::null, vm::null);

    cxmlDoc->floatarraytable.set(0);
    cxmlDoc->floatarraytable_capacity = 0;

    if (cxmlDoc->filetable != vm::null)
        cellSysutil_B59872EF(AllocationType_Free_FileTable, cxmlDoc->allocator_userdata, cxmlDoc->filetable, 0, vm::null, vm::null);

    cxmlDoc->filetable.set(0);
    cxmlDoc->filetable_capacity = 0;

    cxmlDoc->accessMode = 0;

    return CELL_OK;
}

// Deconstructor for cxml
s32 _ZN4cxml8DocumentD1Ev(vm::ptr<cxml_document> cxmlDoc)
{
    cellSysutil.error("_ZN4cxml8DocumentD1Ev(cxmlDoc=*0x%x)", cxmlDoc);
	return _ZN4cxml8Document5ClearEv(cxmlDoc);
}

// helper function
s32 CXmlAddGetStringOffset(vm::ptr<cxml_document> cxmlDoc, vm::cptr<char> name, u32 nameLen)
{
    int i = 0;
    while (i < cxmlDoc->header.stringtable_size)
    {
        if (std::strcmp(cxmlDoc->stringtable.get_ptr() + i, name.get_ptr()) == 0)
            return i;
        i += std::strlen(cxmlDoc->stringtable.get_ptr() + i) + 1;
    }
    // else add at the end
    if (cxmlDoc->stringtable_capacity < (cxmlDoc->header.stringtable_size + nameLen + 1))
    {
        const u32 reqSize = cxmlDoc->header.stringtable_size + nameLen + 1;
        vm::var<u32> size;
        vm::var<u32> addr;
        
        cellSysutil_B59872EF(AllocationType_Alloc_StringTable, cxmlDoc->allocator_userdata, cxmlDoc->stringtable, reqSize, addr, size);
        if (addr == vm::null)
            return CELL_SYSUTIL_CXML_ALLOCATION_ERROR;

        cxmlDoc->stringtable.set(*addr);
        cxmlDoc->stringtable_capacity = *size;
    }

    std::memcpy(cxmlDoc->stringtable.get_ptr() + cxmlDoc->header.stringtable_size, name.get_ptr(), nameLen + 1);

    const u32 stringOffset = cxmlDoc->header.stringtable_size;
    cxmlDoc->header.stringtable_size += nameLen + 1;

    return stringOffset;
}

s32 _ZN4cxml8Document13CreateElementEPKciPNS_7ElementE(vm::ptr<cxml_document> cxmlDoc, vm::cptr<char> name, u32 attrNum, vm::ptr<cxml_element> element)
{
    cellSysutil.error("_ZN4cxml8Document13CreateElementEPKciPNS_7ElementE(cxmlDoc=*0x%x, name=%s, attrNum=%d, element=*0x%x)", cxmlDoc, name, attrNum, element);

    if (cxmlDoc->accessMode != 0)
        return CELL_SYSUTIL_CXML_ACCESS_VIOLATION;

    if (cxmlDoc->allocator == vm::null)
        return CELL_SYSUTIL_CXML_NO_ALLOCATOR;

    const u32 neededSize = (attrNum << 4) + sizeof(cxml_childelement_bin);
    const u32 nameLen = std::strlen(name.get_ptr());

    if (cxmlDoc->header.tree_size + neededSize > cxmlDoc->tree_capacity)
    {
        vm::var<u32> size = 0;
        vm::var<u32> addr = 0;
        cellSysutil_B59872EF(AllocationType_Alloc_Tree, cxmlDoc->allocator_userdata, cxmlDoc->tree, cxmlDoc->header.tree_size + neededSize, addr, size);
        if (addr == vm::null)
            return CELL_SYSUTIL_CXML_ALLOCATION_ERROR;

        cxmlDoc->tree.set(*addr);
        cxmlDoc->tree_capacity = *size;
    }

    u32 treeOffset = cxmlDoc->header.tree_size;
    cxmlDoc->header.tree_size += neededSize;

    element->offset = treeOffset;
    element->doc.set(cxmlDoc.addr());

    const s32 stringOffset = CXmlAddGetStringOffset(cxmlDoc, name, nameLen);
    if (stringOffset < 0)
        return stringOffset;

    auto& lastElement = vm::_ref<cxml_childelement_bin>(cxmlDoc->tree.addr() + treeOffset);

    lastElement.name = stringOffset;
    lastElement.attr_num = attrNum;
    lastElement.parent = -1;
    lastElement.prev = -1;
    lastElement.next = -1;
    lastElement.first_child = -1;
    lastElement.last_child = -1;

    const u32 counter = attrNum < 0 ? 1 : attrNum + 1;

    for (int i = counter; i != 0; --i) {
        auto& newElement = vm::_ref<cxml_childelement_bin>(cxmlDoc->tree.addr() + treeOffset + sizeof(cxml_childelement_bin));
        newElement.name = -1;
        newElement.attr_num = 0;
        newElement.parent = 0;
        newElement.prev = 0;

        treeOffset += 0x10;
    }

	return CELL_OK;
}

s32 _ZN4cxml8Document14SetHeaderMagicEPKc(vm::ptr<cxml_document> cxmlDoc, vm::cptr<char[4]> magic)
{
    cellSysutil.error("_ZN4cxml8Document14SetHeaderMagicEPKc(cxmlDoc=*0x%x, magic=\"%s\")", cxmlDoc, *magic);
    std::memcpy(cxmlDoc->header.magic, *magic, 4);
	return CELL_OK;
}

s32 _ZN4cxml8Document16CreateFromBufferEPKvjb(vm::ptr<cxml_document> cxmlDoc, vm::ptr<void> buf, u32 size, u8 accessMode)
{
    cellSysutil.error("_ZN4cxml8Document16CreateFromBufferEPKvjb(cxmlDoc=*0x%x, buf=*0x%x, size=0x%x, accessMode=%d)", cxmlDoc, buf, size, accessMode);

    _ZN4cxml8Document5ClearEv(cxmlDoc);

    if (size < sizeof(cxml_document::Header))
        return CELL_SYSUTIL_CXML_INVALID_BUFFER_SIZE;

    // theres a weird formula for this, its probly just a sign change but too lazy to figure it out
    // bnut it looks like this function is always given 0 in most cases so its fine
    if (accessMode != 0)
        fmt::throw_exception("createFromBuffer, accessMode not 0");

    cxmlDoc->accessMode = accessMode;

    std::memcpy(&cxmlDoc->header, vm::base(buf.addr()), sizeof(cxml_document::Header));

    if (cxmlDoc->header.version != 0x110 && cxmlDoc->header.version != 0x100)
        return CELL_SYSUTIL_CXML_INVALID_VERSION;

    if ((cxmlDoc->header.intarraytable_size & 0x3) != 0)
        return CELL_SYSUTIL_CXML_INVALID_TABLE;

    if ((cxmlDoc->header.floatarraytable_size & 0x3) != 0)
        return CELL_SYSUTIL_CXML_INVALID_TABLE;

    if (accessMode != 0 && cxmlDoc->allocator == vm::null)
        return CELL_SYSUTIL_CXML_NO_ALLOCATOR;

    if (cxmlDoc->header.tree_size > 0) {
        if (cxmlDoc->header.tree_offset >= size)
            return CELL_SYSUTIL_CXML_INVALID_OFFSET;
        if (cxmlDoc->header.tree_offset + cxmlDoc->header.tree_size > size)
            return CELL_SYSUTIL_CXML_INVALID_BUFFER_SIZE;

        if (accessMode == 0)
            cxmlDoc->tree.set(buf.addr() + cxmlDoc->header.tree_offset);
        else {
            // todo: this, but with the allocator reloc's in their current state, i dont want to deal with this
            fmt::throw_exception("createFromBuffer: unxpected accessMode");
        }
    }

    if (cxmlDoc->header.idtable_size > 0) {
        if (cxmlDoc->header.idtable_offset >= size)
            return CELL_SYSUTIL_CXML_INVALID_OFFSET;
        if (cxmlDoc->header.idtable_offset + cxmlDoc->header.idtable_size > size)
            return CELL_SYSUTIL_CXML_INVALID_BUFFER_SIZE;

        if (accessMode == 0)
            cxmlDoc->idtable.set(buf.addr() + cxmlDoc->header.idtable_offset);
        else {
            // todo: this, but with the allocator reloc's in their current state, i dont want to deal with this
            fmt::throw_exception("createFromBuffer: unxpected accessMode");
        }
    }

    if (cxmlDoc->header.stringtable_size > 0) {
        if (cxmlDoc->header.stringtable_offset >= size)
            return CELL_SYSUTIL_CXML_INVALID_OFFSET;
        if (cxmlDoc->header.stringtable_offset + cxmlDoc->header.stringtable_size > size)
            return CELL_SYSUTIL_CXML_INVALID_BUFFER_SIZE;

        if (accessMode == 0)
            cxmlDoc->stringtable.set(buf.addr() + cxmlDoc->header.stringtable_offset);
        else {
            // todo: this, but with the allocator reloc's in their current state, i dont want to deal with this
            fmt::throw_exception("createFromBuffer: unxpected accessMode");
        }
    }

    // check str table ends on null char
    const auto& strTest = vm::_ref<u8>(cxmlDoc->stringtable.addr() + cxmlDoc->header.stringtable_size - 1);
    if (strTest != 0)
        return CELL_SYSUTIL_CXML_INVALID_TABLE;

    if (cxmlDoc->header.intarraytable_size > 0) {
        if (cxmlDoc->header.intarraytable_offset >= size)
            return CELL_SYSUTIL_CXML_INVALID_OFFSET;
        if (cxmlDoc->header.intarraytable_offset + cxmlDoc->header.intarraytable_size > size)
            return CELL_SYSUTIL_CXML_INVALID_BUFFER_SIZE;

        if (accessMode == 0)
            cxmlDoc->intarraytable.set(buf.addr() + cxmlDoc->header.intarraytable_offset);
        else {
            // todo: this, but with the allocator reloc's in their current state, i dont want to deal with this
            fmt::throw_exception("createFromBuffer: unxpected accessMode");
        }
    }

    if (cxmlDoc->header.floatarraytable_size > 0) {
        if (cxmlDoc->header.floatarraytable_offset >= size)
            return CELL_SYSUTIL_CXML_INVALID_OFFSET;
        if (cxmlDoc->header.floatarraytable_offset + cxmlDoc->header.floatarraytable_size > size)
            return CELL_SYSUTIL_CXML_INVALID_BUFFER_SIZE;

        if (accessMode == 0)
            cxmlDoc->floatarraytable.set(buf.addr() + cxmlDoc->header.floatarraytable_offset);
        else {
            // todo: this, but with the allocator reloc's in their current state, i dont want to deal with this
            fmt::throw_exception("createFromBuffer: unxpected accessMode");
        }
    }

    if (cxmlDoc->header.filetable_size > 0) {
        if (cxmlDoc->header.filetable_offset >= size)
            return CELL_SYSUTIL_CXML_INVALID_OFFSET;
        if (cxmlDoc->header.filetable_offset + cxmlDoc->header.filetable_size > size)
            return CELL_SYSUTIL_CXML_INVALID_BUFFER_SIZE;

        if (accessMode == 0)
            cxmlDoc->filetable.set(buf.addr() + cxmlDoc->header.filetable_offset);
        else {
            // todo: this, but with the allocator reloc's in their current state, i dont want to deal with this
            fmt::throw_exception("createFromBuffer: unxpected accessMode");
        }
    }

	return CELL_OK;
}

bool DoesTreeHaveElement(vm::ptr<cxml_document> doc, s32 lastChild) {
    if (lastChild < 0)
        return false;

    const u32 neededSize = lastChild + sizeof(cxml_childelement_bin);
    u32 test = doc->header.tree_size;
    if (doc->header.tree_size < neededSize)
        return false;

    s32 attrNum = static_cast<s32>(vm::_ref<cxml_childelement_bin>(doc->tree.addr() + lastChild).attr_num);
    if (attrNum < 0)
        return false;

    if (((u32)attrNum << 4) + neededSize > doc->header.tree_size)
        return false;
    return true;
}

u32 _ZN4cxml8Document18GetDocumentElementEv(vm::ptr<cxml_element> element, vm::ptr<cxml_document> doc)
{
    cellSysutil.error("_ZN4cxml8Document18GetDocumentElementEv(element=*0x%x, doc=*0x%x)", element, doc);

    if (!DoesTreeHaveElement(doc, 0)) {
        element->doc.set(0);
        element->offset = 0xFFFFFFFF;
    }
    else {
        element->doc.set(doc.addr());
        element->offset = 0;
    }
    
	return element.addr();
}

s32 _ZNK4cxml4File7GetAddrEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

// helper function
vm::cptr<char> GetStringAddrFromNameOffset(vm::ptr<cxml_document> doc, s32 nameOffset) {
    if (nameOffset < 0)
        return vm::null;

    if (doc->header.stringtable_size < nameOffset)
        return vm::null;

    return vm::addr_t(doc->stringtable.addr() + nameOffset);
}

// cxml::element::getAttribute
s32 _ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE(vm::ptr<cxml_element> cxmlElement, vm::cptr<char> name, vm::ptr<cxml_attribute> attribute)
{
    cellSysutil.error("_ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE(cxmlElement=*0x%x, name=%s, attribute=*0x%x)", cxmlElement, name, attribute);

    if (cxmlElement->doc == vm::null)
        return CELL_SYSUTIL_CXML_INVALID_DOC;

    const be_t<u32> attr_num = vm::_ref<cxml_childelement_bin>(cxmlElement->doc->tree.addr() + cxmlElement->offset).attr_num;

    u32 treeOffset = cxmlElement->doc->tree.addr() + cxmlElement->offset + sizeof(cxml_childelement_bin);
    u32 elementOffset = cxmlElement->offset;

    for (u32 i = 0; i < attr_num; ++i) {
        const be_t<s32> nameOffset = vm::_ref<cxml_childelement_bin>(treeOffset).name;
        const auto str = GetStringAddrFromNameOffset(cxmlElement->doc, nameOffset);

        if (str != vm::null) {
            if (std::strcmp(str.get_ptr(), name.get_ptr()) == 0) {
                attribute->doc.set(cxmlElement->doc.addr());
                attribute->element_offset = cxmlElement->offset;
                attribute->offset = (elementOffset + sizeof(cxml_childelement_bin));
                return CELL_OK;
            }
        }
        treeOffset += 0x10;
        elementOffset += 0x10;
    }

	return CELL_SYSUTIL_CXML_ELEMENT_CANT_FIND_ATTRIBUTE;
}

s32 _ZN4cxml7Element11AppendChildERS0_(vm::ptr<cxml_element> element, vm::ptr<cxml_element> newElement)
{
    cellSysutil.error("_ZN4cxml7Element11AppendChildERS0_(element=*0x%x, newElement=*0x%x)", element, newElement);
    if (element->doc == vm::null)
        return CELL_SYSUTIL_CXML_INVALID_DOC;
    if (element->doc->accessMode != 0)
        return CELL_SYSUTIL_CXML_ACCESS_VIOLATION;

    if (newElement->offset == 0)
        return CELL_SYSUTIL_CXML_ELEMENT_INVALID_OFFSET;

    auto &newBin= vm::_ref<cxml_childelement_bin>(element->doc->tree.addr() + newElement->offset);

    if (newBin.parent >= 0)
        return CELL_SYSUTIL_CXML_ELEMENT_INVALID_OFFSET;

    auto &bin = vm::_ref<cxml_childelement_bin>(element->doc->tree.addr() + element->offset);

    const u32 prevLastChild = bin.last_child;

    if (bin.first_child < 0)
        bin.first_child = element->offset;
    bin.last_child = newElement->offset;

    newBin.prev = prevLastChild;
    newBin.parent = element->offset;

    if (DoesTreeHaveElement(element->doc, prevLastChild)) {
        auto &prevElement = vm::_ref<cxml_childelement_bin>(element->doc->tree.addr() + prevLastChild);
        prevElement.next = newElement->offset;
    }

    return CELL_OK;
}

s32 _ZNK4cxml7Element14GetNextSiblingEv()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

// helper
s32 MoveFileAttribute(vm::ptr<cxml_attribute> attr, vm::ptr<cxml_attribute> newAttr) {
    if (attr->doc == vm::null)
        return CELL_SYSUTIL_CXML_INVALID_DOC;
    if (attr->doc->accessMode != 0)
        return CELL_SYSUTIL_CXML_ACCESS_VIOLATION;

    auto & bin = vm::_ref<cxml_childelement_bin>(attr->doc->tree.addr() + attr->offset);
    bin.attr_num = 6;
    bin.parent = newAttr->element_offset;
    bin.prev = newAttr->offset;

    return CELL_OK;
}

s32 GetElementAttrNum(vm::ptr<cxml_element> element) {
    if (element->doc == vm::null)
        return 0;

    return vm::_ref<cxml_childelement_bin>(element->doc->tree.addr() + element->offset).attr_num;
}

s32 GetAttrFromElemAndAttrNum(vm::ptr<cxml_element> element, s32 attr_num, vm::ptr<cxml_attribute> attr) {
    if (element->doc == vm::null)
        return CELL_SYSUTIL_CXML_INVALID_DOC;

    const u32 shiftAttr = (u32)attr_num << 4;
    const u32 elemoffset = element->offset + sizeof(cxml_childelement_bin) + shiftAttr;

    const u32 binAttrNum = vm::_ref<cxml_childelement_bin>(element->doc->tree.addr() + element->offset).attr_num;

    if (attr_num >= binAttrNum)
        return CELL_SYSUTIL_CXML_INVALID_DOC;

    attr->doc.set(element->doc.addr());
    attr->element_offset = element->offset;
    attr->offset = shiftAttr + elemoffset;

    return CELL_OK;
}

s32 GetAttrNumFromAttr(vm::ptr<cxml_attribute> attr) {
    if (attr->doc == vm::null)
        return 0;

    return vm::_ref<cxml_childelement_bin>(attr->doc->tree.addr() + attr->offset).attr_num;
}

// helper
s32 GetEmptyAttribute(vm::ptr<cxml_element> element, vm::ptr<cxml_attribute> attr) {
    u32 i = 0;
    const u32 attr_num = GetElementAttrNum(element);
    for(u32 i = 0; i < attr_num; ++i) {
        vm::var<cxml_attribute> tmpAttr;

        const s32 ret = GetAttrFromElemAndAttrNum(element, i, tmpAttr);
        if (ret != 0)
            return ret;

        const u32 attr_num = GetAttrNumFromAttr(tmpAttr);

        if (attr_num == 0) {
            attr->doc.set(tmpAttr->doc.addr());
            attr->element_offset = tmpAttr->element_offset;
            attr->offset = tmpAttr->offset;
            return CELL_OK;
        }
    }

    return CELL_SYSUTIL_CXML_ELEMENT_CANT_FIND_ATTRIBUTE;
}

s32 SetAttrName(vm::ptr<cxml_attribute> attr, vm::cptr<char> name) {
    if (attr->doc == vm::null)
        return CELL_SYSUTIL_CXML_INVALID_DOC;

    if (attr->doc->accessMode != 0)
        return CELL_SYSUTIL_CXML_ACCESS_VIOLATION;

    const u32 nameLen = std::strlen(name.get_ptr());

    const s32 strOffset = CXmlAddGetStringOffset(attr->doc, name, nameLen);
    if (strOffset < 0)
        return strOffset;

    auto &bin = vm::_ref<cxml_childelement_bin>(attr->offset + attr->doc->tree.addr());

    bin.name = strOffset;
    return CELL_OK;
}

// cxml::element::appendFileAttribute?
s32 cellSysutil_35F7ED00(vm::ptr<cxml_element> element, vm::cptr<char> name, vm::ptr<cxml_attribute> attribute)
{
    cellSysutil.error("cellSysutil_35F7ED00(element=*0x%x, name=%s, attribute=*0x%x)", element, name, attribute);

    vm::var<cxml_attribute> tempAttr;

    if (_ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE(element, name, tempAttr) != 0) {
        s32 ret = GetEmptyAttribute(element, tempAttr);
        if (ret != 0)
            return ret;

        ret = SetAttrName(tempAttr, name);
        if (ret < 0)
            return ret;
    }

    const s32 ret = MoveFileAttribute(tempAttr, attribute);
    return (((ret + -1) | ret) >> 0x1F) & ret;

    return CELL_OK;
}

s32 _ZNK4cxml9Attribute6GetIntEPi(vm::ptr<cxml_attribute> attr, vm::ptr<u32> out)
{
    cellSysutil.error("_ZNK4cxml9Attribute6GetIntEPi(attr=*0x%x, out=*0x%x)", attr, out);

    if (attr->doc == vm::null)
        CELL_SYSUTIL_CXML_INVALID_DOC;

    const auto& elem = vm::_ref<cxml_childelement_bin>(attr->doc->tree.addr() + attr->offset);

    if (elem.attr_num != 1)
        return CELL_SYSUTIL_CXML_ELEMENT_INVALID_ATTRIBUTE_NUM;

    *out = elem.parent;

	return CELL_OK;
}

s32 _ZNK4cxml9Attribute7GetFileEPNS_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil6SetIntERKN4cxml7ElementEPKci()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil6GetIntERKN4cxml7ElementEPKcPi(vm::ptr<cxml_element> element, vm::cptr<char> name, vm::ptr<u32> out)
{
    cellSysutil.error("_ZN8cxmlutil6GetIntERKN4cxml7ElementEPKcPi(element=*0x%x, name=%s, out=*0x%x)", element, name, out);
	
    vm::var<cxml_attribute> attr;
    attr->doc.set(0);
    attr->element_offset = 0xFFFFFFFF;
    attr->offset = 0xFFFFFFFF;

    s32 ret = _ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE(element, name, attr);
    if (ret != CELL_OK)
        return ret;

    ret = _ZNK4cxml9Attribute6GetIntEPi(attr, out);
    return (((ret + -1) | ret) >> 0x1F) & ret;
}

s32 _ZN8cxmlutil9GetStringERKN4cxml7ElementEPKcPS5_Pj()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil9SetStringERKN4cxml7ElementEPKcS5_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil16CheckElementNameERKN4cxml7ElementEPKc()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil16FindChildElementERKN4cxml7ElementEPKcS5_S5_()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

s32 _ZN8cxmlutil7GetFileERKN4cxml7ElementEPKcPNS0_4FileE()
{
	UNIMPLEMENTED_FUNC(cellSysutil);
	return CELL_OK;
}

inline u32 GetNeededAlignBytes(u32 size) {
    return ((-size) & 0xF);
}

u32 GetTotalAlignedSize(u32 size) {
    return size + GetNeededAlignBytes(size);
}

u32 GetDocAlignedSize(vm::ptr<cxml_document> cxmlDoc) {
    u32 neededSize = GetTotalAlignedSize(sizeof(cxml_document::Header));
    neededSize += GetTotalAlignedSize(cxmlDoc->header.tree_size);
    neededSize += GetTotalAlignedSize(cxmlDoc->header.idtable_size);
    neededSize += GetTotalAlignedSize(cxmlDoc->header.stringtable_size);
    neededSize += GetTotalAlignedSize(cxmlDoc->header.intarraytable_size);
    neededSize += GetTotalAlignedSize(cxmlDoc->header.floatarraytable_size);
    neededSize += GetTotalAlignedSize(cxmlDoc->header.filetable_size);

    return neededSize;
}

// Get struct / needed size for storage into packet?
s32 cellSysutil_20957CD4(vm::ptr<SysutilPacketInfo> packetInfo, u32 slot, u32 evnt, vm::ptr<cxml_document> cxmlDoc)
{
    cellSysutil.error("cellSysutil_20957CD4(packetInfo=*0x%x, slot=%d, evnt=0x%x, cxmlDoc=*0x%x)", packetInfo, slot, evnt, cxmlDoc);

    packetInfo->slot = slot;
    packetInfo->evnt = evnt;
    packetInfo->size = GetDocAlignedSize(cxmlDoc);

    return CELL_OK;
}

void SetHeaderAlignedSizes(vm::ptr<cxml_document> cxmlDoc) {
    cxmlDoc->header.version = 0x110;

    u32 neededSize = GetTotalAlignedSize(sizeof(cxml_document::Header));
    cxmlDoc->header.tree_offset = neededSize;

    neededSize += GetTotalAlignedSize(cxmlDoc->header.tree_size);
    cxmlDoc->header.idtable_offset = neededSize;

    neededSize += GetTotalAlignedSize(cxmlDoc->header.idtable_size);
    cxmlDoc->header.stringtable_offset = neededSize;

    neededSize += GetTotalAlignedSize(cxmlDoc->header.stringtable_size);
    cxmlDoc->header.intarraytable_offset = neededSize;

    neededSize += GetTotalAlignedSize(cxmlDoc->header.intarraytable_size);
    cxmlDoc->header.floatarraytable_offset = neededSize;

    neededSize += GetTotalAlignedSize(cxmlDoc->header.floatarraytable_size);
    cxmlDoc->header.filetable_offset = neededSize;
}

s32 cellSysutil_7FC8F72C(ppu_thread& ppu, u32 addr, u32 size, vm::ptr<SysutilPacketInfo> packetInfo) {
    cellSysutil.warning("cellSysutil_7FC8F72C(data=*0x%x, size=0x%x, packetInfo=*0x%x)", addr, size, packetInfo);
    if (packetInfo->size == 0)
        return 0;

    const s32 txor = packetInfo->size ^ size;
    const s32 txorShift = txor >> 0x1F;
    const s32 txor2 = ((txorShift ^ txor) - txorShift) + -1;
    // extrdi txor, txor, 1, 32....select high bits?
    // k im just ignoring it
    // todo: deal with above

    // lets assume that its 'is size less than whats left in the packet'
    u32 packetSize = packetInfo->size;

    const s32 ret = cellSysutilPacketWrite(ppu, packetInfo->slot, packetInfo->evnt, addr, size, size < packetInfo->size ? 0 : 1);
    if (ret != 0)
        return ret;

    packetInfo->size -= size;

    return size;
}

// sweet, this funcptr callback suffers the same reloc issue as the allocator function which ive mentioned above in the file, the good news is that internally, sysutil seems to just use 7FC8F72C, so lets just try that for now
using CXmlReAlloc = void(u32 addr, u32 size, vm::ptr<SysutilPacketInfo> packetInfo);
s32 cellSysutil_75AA7373(ppu_thread& ppu, vm::ptr<cxml_document> cxmlDoc, vm::ptr<CXmlReAlloc> funcptr, vm::ptr<SysutilPacketInfo> packetInfo)
{
    cellSysutil.error("cellSysutil_75AA7373(cxmlDoc=*0x%x, funcptr=*0x%x, packetInfo=*0x%x)", cxmlDoc, funcptr, packetInfo);

    if (cxmlDoc->accessMode == 1)
        return CELL_SYSUTIL_CXML_ACCESS_VIOLATION;

    SetHeaderAlignedSizes(cxmlDoc);

    // send addr of the header
    s32 ret = cellSysutil_7FC8F72C(ppu, cxmlDoc.addr() + 4, sizeof(cxml_document::Header), packetInfo);
    if (ret != sizeof(cxml_document::Header))
        return ret;

    u32 sizeNeeded = GetNeededAlignBytes(sizeof(cxml_document::Header));

    vm::var<cxml_document::Header> tmpHeader;
    ret = cellSysutil_7FC8F72C(ppu, tmpHeader.addr(), sizeNeeded, packetInfo);
    if (ret != sizeNeeded)
        return ret;

    ret = cellSysutil_7FC8F72C(ppu, cxmlDoc->tree.addr(), cxmlDoc->header.tree_size, packetInfo);
    if (ret != cxmlDoc->header.tree_size)
        return ret;

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.tree_size);
    ret = cellSysutil_7FC8F72C(ppu, tmpHeader.addr(), sizeNeeded, packetInfo);

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.tree_size);
    if (ret != sizeNeeded)
        return ret;
        
    ret = cellSysutil_7FC8F72C(ppu, cxmlDoc->idtable.addr(), cxmlDoc->header.idtable_size, packetInfo);
    if (ret != cxmlDoc->header.idtable_size)
        return ret;

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.idtable_size);
    ret = cellSysutil_7FC8F72C(ppu, tmpHeader.addr(), sizeNeeded, packetInfo);

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.idtable_size);
    if (ret != sizeNeeded)
        return ret;

    ret = cellSysutil_7FC8F72C(ppu, cxmlDoc->stringtable.addr(), cxmlDoc->header.stringtable_size, packetInfo);
    if (ret != cxmlDoc->header.stringtable_size)
        return ret;

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.stringtable_size);
    ret = cellSysutil_7FC8F72C(ppu, tmpHeader.addr(), sizeNeeded, packetInfo);

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.stringtable_size);
    if (ret != sizeNeeded)
        return ret;


    ret = cellSysutil_7FC8F72C(ppu, cxmlDoc->intarraytable.addr(), cxmlDoc->header.intarraytable_size, packetInfo);
    if (ret != cxmlDoc->header.intarraytable_size)
        return ret;

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.intarraytable_size);
    ret = cellSysutil_7FC8F72C(ppu, tmpHeader.addr(), sizeNeeded, packetInfo);

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.intarraytable_size);
    if (ret != sizeNeeded)
        return ret;


    ret = cellSysutil_7FC8F72C(ppu, cxmlDoc->floatarraytable.addr(), cxmlDoc->header.floatarraytable_size, packetInfo);
    if (ret != cxmlDoc->header.floatarraytable_size)
        return ret;

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.floatarraytable_size);
    ret = cellSysutil_7FC8F72C(ppu, tmpHeader.addr(), sizeNeeded, packetInfo);

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.floatarraytable_size);
    if (ret != sizeNeeded)
        return ret;

    ret = cellSysutil_7FC8F72C(ppu, cxmlDoc->filetable.addr(), cxmlDoc->header.filetable_size, packetInfo);
    if (ret != cxmlDoc->header.filetable_size)
        return ret;

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.filetable_size);
    ret = cellSysutil_7FC8F72C(ppu, tmpHeader.addr(), sizeNeeded, packetInfo);

    sizeNeeded = GetNeededAlignBytes(cxmlDoc->header.filetable_size);
    if (ret != sizeNeeded)
        return ret;
    return CELL_OK;
}

// append filetable attribute?, probably a mangled c++ name
s32 cellSysutil_D3CDD694(vm::ptr<cxml_document> cxmlDoc, vm::cptr<char> name, s32 strlen, vm::ptr<cxml_attribute> attribute)
{
    cellSysutil.error("cellSysutil_D3CDD694(cxmlDoc=*0x%x, name=%s, strlen=0x%x, attribute=*0x%x)", cxmlDoc, name, strlen, attribute);

    if (cxmlDoc->accessMode != 0)
        return CELL_SYSUTIL_CXML_ACCESS_VIOLATION;

    if (cxmlDoc->allocator == vm::null)
        return CELL_SYSUTIL_CXML_NO_ALLOCATOR;

    const u32 neededSize = GetTotalAlignedSize(strlen);

    if (cxmlDoc->header.filetable_size + neededSize > cxmlDoc->filetable_capacity) {
        vm::var<u32> size = 0;
        vm::var<u32> addr = 0;
        cellSysutil_B59872EF(AllocationType_Alloc_FileTable, cxmlDoc->allocator_userdata, cxmlDoc->filetable, cxmlDoc->header.filetable_size + neededSize, addr, size);
        if (addr == vm::null)
            return CELL_SYSUTIL_CXML_ALLOCATION_ERROR;

        cxmlDoc->filetable.set(*addr);
        cxmlDoc->filetable_capacity = *size;
    }

    const u32 ftSize = cxmlDoc->header.filetable_size;
    cxmlDoc->header.filetable_size += neededSize;

    std::memcpy(cxmlDoc->filetable.get_ptr() + ftSize, name.get_ptr(), strlen);

    std::memset(cxmlDoc->filetable.get_ptr() + ftSize + strlen, 0, neededSize - strlen);

    attribute->doc.set(cxmlDoc.addr());
    attribute->element_offset = strlen;
    attribute->offset = ftSize;

    return CELL_OK;
}

s32 cellSysutil_E1EC7B6A(vm::ptr<u32> unk)
{
	cellSysutil.todo("cellSysutil_E1EC7B6A(unk=*0x%x)", unk);
	*unk = 0;
	return CELL_OK;
}

extern void cellSysutil_SaveData_init();
extern void cellSysutil_GameData_init();
extern void cellSysutil_MsgDialog_init();
extern void cellSysutil_OskDialog_init();
extern void cellSysutil_Storage_init();
extern void cellSysutil_Sysconf_init();
extern void cellSysutil_SysutilAvc_init();
extern void cellSysutil_WebBrowser_init();
extern void cellSysutil_AudioOut_init();
extern void cellSysutil_VideoOut_init();

DECLARE(ppu_module_manager::cellSysutil)("cellSysutil", []()
{
	cellSysutil_SaveData_init(); // cellSaveData functions
	cellSysutil_GameData_init(); // cellGameData, cellHddGame functions
	cellSysutil_MsgDialog_init(); // cellMsgDialog functions
	cellSysutil_OskDialog_init(); // cellOskDialog functions
	cellSysutil_Storage_init(); // cellStorage functions
	cellSysutil_Sysconf_init(); // cellSysconf functions
	cellSysutil_SysutilAvc_init(); // cellSysutilAvc functions
	cellSysutil_WebBrowser_init(); // cellWebBrowser, cellWebComponent functions
	cellSysutil_AudioOut_init(); // cellAudioOut functions
	cellSysutil_VideoOut_init(); // cellVideoOut functions

	REG_FUNC(cellSysutil, cellSysutilGetSystemParamInt);
	REG_FUNC(cellSysutil, cellSysutilGetSystemParamString);

	REG_FUNC(cellSysutil, cellSysutilCheckCallback);
	REG_FUNC(cellSysutil, cellSysutilRegisterCallback);
	REG_FUNC(cellSysutil, cellSysutilUnregisterCallback);

	REG_FUNC(cellSysutil, cellSysutilGetBgmPlaybackStatus);
	REG_FUNC(cellSysutil, cellSysutilGetBgmPlaybackStatus2);
	REG_FUNC(cellSysutil, cellSysutilEnableBgmPlayback);
	REG_FUNC(cellSysutil, cellSysutilEnableBgmPlaybackEx);
	REG_FUNC(cellSysutil, cellSysutilDisableBgmPlayback);
	REG_FUNC(cellSysutil, cellSysutilDisableBgmPlaybackEx);
	REG_FUNC(cellSysutil, cellSysutilSetBgmPlaybackExtraParam);

	REG_FUNC(cellSysutil, cellSysCacheMount);
	REG_FUNC(cellSysutil, cellSysCacheClear);

	REG_FUNC(cellSysutil, cellSysutilRegisterCallbackDispatcher);
	REG_FUNC(cellSysutil, cellSysutilUnregisterCallbackDispatcher);
	REG_FUNC(cellSysutil, cellSysutilPacketRead);
	REG_FUNC(cellSysutil, cellSysutilPacketWrite);
	REG_FUNC(cellSysutil, cellSysutilPacketBegin);
	REG_FUNC(cellSysutil, cellSysutilPacketEnd);

	REG_FUNC(cellSysutil, cellSysutilGameDataAssignVmc);
	REG_FUNC(cellSysutil, cellSysutilGameDataExit);
	REG_FUNC(cellSysutil, cellSysutilGameExit_I);
	REG_FUNC(cellSysutil, cellSysutilGamePowerOff_I);
	REG_FUNC(cellSysutil, cellSysutilGameReboot_I);

	REG_FUNC(cellSysutil, cellSysutilSharedMemoryAlloc);
	REG_FUNC(cellSysutil, cellSysutilSharedMemoryFree);

	REG_FUNC(cellSysutil, _ZN4cxml7Element11AppendChildERS0_);

	REG_FUNC(cellSysutil, _ZN4cxml8DocumentC1Ev);
	REG_FUNC(cellSysutil, _ZN4cxml8DocumentD1Ev);
    REG_FUNC(cellSysutil, _ZN4cxml8Document5ClearEv);
	REG_FUNC(cellSysutil, _ZN4cxml8Document13CreateElementEPKciPNS_7ElementE);
	REG_FUNC(cellSysutil, _ZN4cxml8Document14SetHeaderMagicEPKc);
	REG_FUNC(cellSysutil, _ZN4cxml8Document16CreateFromBufferEPKvjb);
	REG_FUNC(cellSysutil, _ZN4cxml8Document18GetDocumentElementEv);

	REG_FUNC(cellSysutil, _ZNK4cxml4File7GetAddrEv);
	REG_FUNC(cellSysutil, _ZNK4cxml7Element12GetAttributeEPKcPNS_9AttributeE);
	REG_FUNC(cellSysutil, _ZNK4cxml7Element14GetNextSiblingEv);
	REG_FUNC(cellSysutil, _ZNK4cxml9Attribute6GetIntEPi);
	REG_FUNC(cellSysutil, _ZNK4cxml9Attribute7GetFileEPNS_4FileE);

	REG_FUNC(cellSysutil, _ZN8cxmlutil6SetIntERKN4cxml7ElementEPKci);
	REG_FUNC(cellSysutil, _ZN8cxmlutil6GetIntERKN4cxml7ElementEPKcPi);
	REG_FUNC(cellSysutil, _ZN8cxmlutil9GetStringERKN4cxml7ElementEPKcPS5_Pj);
	REG_FUNC(cellSysutil, _ZN8cxmlutil9SetStringERKN4cxml7ElementEPKcS5_);
	REG_FUNC(cellSysutil, _ZN8cxmlutil16CheckElementNameERKN4cxml7ElementEPKc);
	REG_FUNC(cellSysutil, _ZN8cxmlutil16FindChildElementERKN4cxml7ElementEPKcS5_S5_);
	REG_FUNC(cellSysutil, _ZN8cxmlutil7GetFileERKN4cxml7ElementEPKcPNS0_4FileE);

	REG_FNID(cellSysutil, 0xE1EC7B6A, cellSysutil_E1EC7B6A);
    REG_FNID(cellSysutil, 0xB47470E1, cellSysutil_B47470E1);
    REG_FNID(cellSysutil, 0x20957CD4, cellSysutil_20957CD4);
    REG_FNID(cellSysutil, 0x75AA7373, cellSysutil_75AA7373);
    REG_FNID(cellSysutil, 0x35F7ED00, cellSysutil_35F7ED00);
    REG_FNID(cellSysutil, 0xD3CDD694, cellSysutil_D3CDD694);
    REG_FNID(cellSysutil, 0x40719C8C, cellSysutil_40719C8C);
    REG_FNID(cellSysutil, 0xB59872EF, cellSysutil_B59872EF);

    REG_FNID(cellSysutil, 0x7FC8F72C, cellSysutil_7FC8F72C);
});
