#pragma once

// SPURS defines.
enum SPURSKernelInterfaces
{
	CELL_SPURS_MAX_SPU           = 8,
	CELL_SPURS_MAX_WORKLOAD      = 16,
	CELL_SPURS_MAX_WORKLOAD2     = 32,
	CELL_SPURS_MAX_PRIORITY      = 16,
	CELL_SPURS_NAME_MAX_LENGTH   = 15,
	CELL_SPURS_SIZE              = 4096,
	CELL_SPURS_SIZE2             = 8192,
	CELL_SPURS_ALIGN             = 128,
	CELL_SPURS_ATTRIBUTE_SIZE    = 512,
	CELL_SPURS_ATTRIBUTE_ALIGN   = 8,
	CELL_SPURS_INTERRUPT_VECTOR  = 0x0,
	CELL_SPURS_LOCK_LINE         = 0x80,
	CELL_SPURS_KERNEL_DMA_TAG_ID = 31,
};

enum RangeofEventQueuePortNumbers
{
	CELL_SPURS_STATIC_PORT_RANGE_BOTTOM  = 15,
	CELL_SPURS_DYNAMIC_PORT_RANGE_TOP    = 16,
	CELL_SPURS_DYNAMIC_PORT_RANGE_BOTTOM = 63,
};

enum SPURSTraceTypes
{
	CELL_SPURS_TRACE_TAG_LOAD  = 0x2a, 
	CELL_SPURS_TRACE_TAG_MAP   = 0x2b,
	CELL_SPURS_TRACE_TAG_START = 0x2c,
	CELL_SPURS_TRACE_TAG_STOP  = 0x2d,
	CELL_SPURS_TRACE_TAG_USER  = 0x2e,
	CELL_SPURS_TRACE_TAG_GUID  = 0x2f,
};

// SPURS task defines.
enum TaskConstants
{
	CELL_SPURS_MAX_TASK             = 128,
	CELL_SPURS_TASK_TOP             = 0x3000,
	CELL_SPURS_TASK_BOTTOM          = 0x40000,
	CELL_SPURS_MAX_TASK_NAME_LENGTH = 32,
};

// Internal class to shape a SPURS attribute.
class SPURSManagerAttribute
{
public:
	SPURSManagerAttribute(int nSpus, int spuPriority, int ppuPriority, bool exitIfNoWork)
	{
		this->nSpus = nSpus;
		this->spuThreadGroupPriority = spuPriority;
		this->ppuThreadPriority = ppuPriority;
		this->exitIfNoWork = exitIfNoWork;
		memset(this->namePrefix, 0, CELL_SPURS_NAME_MAX_LENGTH + 1);
		this->threadGroupType = 0;
	}

	int _setNamePrefix(const char *name, u32 size)
	{
		strncpy(this->namePrefix, name, size);
		this->namePrefix[0] = 0;
		return 0;
	}

	int _setSpuThreadGroupType(int type)
	{
		this->threadGroupType = type;
		return 0;
	}

protected:
	be_t<int> nSpus; 
	be_t<int> spuThreadGroupPriority; 
	be_t<int> ppuThreadPriority; 
	bool exitIfNoWork;
	char namePrefix[CELL_SPURS_NAME_MAX_LENGTH+1];
	be_t<int> threadGroupType; 
};

// Main SPURS manager class.
class SPURSManager
{
public:
	SPURSManager(SPURSManagerAttribute *attr);
	void Finalize();

protected:
	SPURSManagerAttribute *attr;
};