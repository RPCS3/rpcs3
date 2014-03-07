#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "libmixer.h"

void libmixer_init();
Module libmixer("libmixer", libmixer_init);

int cellAANAddData(u32 handle, u32 port, u32 offset, u32 addr, u32 samples)
{
	libmixer.Error("cellAANAddData(handle=0x%x, port=0x%x, offset=0x%x, addr=0x%x, samples=0x%x)",
		handle, port, offset, addr, samples);
	return CELL_OK; 
}

int cellAANConnect(u32 receive, u32 receivePortNo, u32 source, u32 sourcePortNo)
{
	libmixer.Error("cellAANConnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcrPortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);
	return 0;
}

int cellAANDisconnect(u32 receive, u32 receivePortNo, u32 source, u32 sourcePortNo)
{
	libmixer.Error("cellAANDisconnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcrPortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);
	return 0;
}
 
/*int cellSSPlayerCreate(CellAANHandle *handle, CellSSPlayerConfig *config)
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSSPlayerRemove(CellAANHandle handle)
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSSPlayerSetWave() //CellAANHandle handle, CellSSPlayerWaveParam *waveInfo, CellSSPlayerCommonParam *commonInfo  //mem_class_t waveInfo
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSSPlayerPlay() //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSSPlayerStop() //CellAANHandle handle, u32 mode
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSSPlayerSetParam() //CellAANHandle handle, CellSSPlayerRuntimeInfo *info
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}
 
s32 cellSSPlayerGetState() //CellAANHandle handle
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}*/

int cellSurMixerCreate(const mem_ptr_t<CellSurMixerConfig> config)
{
	libmixer.Error("cellSurMixerCreate(config_addr=0x%x)", config.GetAddr());
	return CELL_OK;
}

int cellSurMixerGetAANHandle(mem32_t handle)
{
	libmixer.Error("cellSurMixerGetAANHandle(handle_addr=0x%x)", handle.GetAddr());
	return CELL_OK;
}

int cellSurMixerChStripGetAANPortNo(mem32_t port, u32 type, u32 index)
{
	libmixer.Error("cellSurMixerChStripGetAANPortNo(port_addr=0x%x, type=0x%x, index=0x%x)", port.GetAddr(), type, index);
	return CELL_OK;
}

int cellSurMixerSetNotifyCallback(u32 func, u32 arg)
{
	libmixer.Error("cellSurMixerSetNotifyCallback(func_addr=0x%x, arg=0x%x)", func, arg);
	return CELL_OK;
}

int cellSurMixerRemoveNotifyCallback(u32 func)
{
	libmixer.Error("cellSurMixerSetNotifyCallback(func_addr=0x%x)", func);
	return CELL_OK;
}

int cellSurMixerStart()
{
	libmixer.Error("cellSurMixerStart()");
	return CELL_OK;
}

int cellSurMixerSetParameter(u32 param, float value)
{
	libmixer.Error("cellSurMixerSetParameter(param=0x%x, value=%f)", param, value);
	return CELL_OK;
}

int cellSurMixerFinalize()
{
	libmixer.Error("cellSurMixerFinalize()");
	return CELL_OK;
}

/*int cellSurMixerSurBusAddData() //u32 busNo, u32 offset, float *addr, u32 samples
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSurMixerChStripSetParameter() //u32 type, u32 index, CellSurMixerChStripParam *param
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSurMixerPause() //u32 switch
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSurMixerGetCurrentBlockTag() //u64 *tag
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

int cellSurMixerGetTimestamp() //u64 tag, u64 *stamp
{
	UNIMPLEMENTED_FUNC(libmixer);
	return 0;
}

void cellSurMixerBeep(); //void *arg

float cellSurMixerUtilGetLevelFromDB() //float dB
{
	UNIMPLEMENTED_FUNC(libmixer);
	return CELL_OK; //it's NOT real value
	//TODO;
}

float cellSurMixerUtilGetLevelFromDBIndex() //int index
{
	UNIMPLEMENTED_FUNC(libmixer);
	return CELL_OK; //it's NOT real value
	//TODO;
}

float cellSurMixerUtilNoteToRatio() //unsigned char refNote, unsigned char note
{
	UNIMPLEMENTED_FUNC(libmixer);
	return CELL_OK; //it's NOT real value
	//TODO
}*/

void libmixer_init()
{
	static const u64 cellAANAddData_table[] = {
		// TODO
		0xffffffff7c691b78,
		0xffffffff7c0802a6,
		0xfffffffff821ff91,
		0xfffffffff8010080,
		0xffffffff7c802378,
		0xffffffff7caa2b78,
		0xffffffff81690000,
		0xffffffff7c050378,
		0xffffffff7cc43378,
		0xffffffff7d465378,
		0xffffffff812b0030,
		0xffffffff80090000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff80490004,
		0xffffffff4e800421,
		0xffffffffe8410028,
		0xffffffffe8010080,
		0xffffffff7c6307b4,
		0xffffffff7c0803a6,
		0xffffffff38210070,
		0xffffffff4e800020,
		0
	};
	libmixer.AddFuncSub(cellAANAddData_table, "cellAANAddData", cellAANAddData);

	u64 cellAANConnect_table[39] = {
		0xfffffffff821ff71,
		0xffffffff7c0802a6,
		0xffffffff2f830000,
		0xfffffffff80100a0,
		0xffffffff3c008031,
		0xffffffff7c691b78,
		0xffffffff7c8a2378,
		0xffffffff60000003,
		0xffffff00409e0018, // bne
		0xffffffff7c0307b4,
		0xffffffffe80100a0,
		0xffffffff38210090,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xffffffff2f850000,
		0xffffffff78630020,
		0xffffffff38810070,
		0xffffff00419effe0, // beq
		0xffffffff81690000,
		0xffffffff38000001,
		0xffffffff91210074,
		0xffffffff90a10070,
		0xffffffff90c10078,
		0xffffffff9141007c,
		0xffffffff812b0018, // [24]
		0xffffffff90010080,
		0xffffffff80090000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff80490004,
		0xffffffff4e800421,
		0xffffffffe8410028,
		0xffffffff7c601b78,
		0xffffffff7c0307b4,
		0xffffffffe80100a0,
		0xffffffff38210090,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0, // [38]
	};
	libmixer.AddFuncSub(cellAANConnect_table, "cellAANConnect", cellAANConnect);
	cellAANConnect_table[24] = 0xffffffff812b001c;
	libmixer.AddFuncSub(cellAANConnect_table, "cellAANDisconnect", cellAANDisconnect);

	static const u64 cellAANAddData_table1[] = {
		// TODO
		0xffffffff7c691b78,
		0xffffffff7c0802a6,
		0xfffffffff821ff91,
		0xfffffffff8010080,
		0xffffffff7c802378,
		0xffffffff7caa2b78,
		0xffffffff81690000,
		0xffffffff7c050378,
		0xffffffff7cc43378,
		0xffffffff78630020, // clrldi r3,r3,32
		0xffffffff7d465378,
		0xffffffff812b0030,
		0xffffffff80090000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff80490004,
		0xffffffff4e800421,
		0xffffffffe8410028,
		0xffffffffe8010080,
		0xffffffff7c6307b4,
		0xffffffff7c0803a6,
		0xffffffff38210070,
		0xffffffff4e800020,
		0
	};
	libmixer.AddFuncSub(cellAANAddData_table1, "cellAANAddData(1)", cellAANAddData);

	static const u64 cellSurMixerCreate_table[] = {
		0xffffffff2f830000,
		0xffffffff7c0802a6,
		0xfffffffff821ff51,
		0xfffffffffbc100a0,
		0xfffffffffb210078,
		0xfffffffffb410080,
		0xfffffffffb610088,
		0xfffffffffb810090,
		0xfffffffffba10098,
		0xfffffffffbe100a8,
		0xfffffffff80100c0,
		0xffffffff7c7e1b78,
		0xf000000040000000, // bne
		0xffffffff3fe08031,
		0xffffffff63ff0003,
		0xffffffffe80100c0,
		0xffffffff7fe307b4,
		0xffffffffeb210078,
		0xffffffffeb410080,
		0xffffffff7c0803a6,
		0xffffffffeb610088,
		0xffffffffeb810090,
		0xffffffffeba10098,
		0xffffffffebc100a0,
		0xffffffffebe100a8,
		0xffffffff382100b0,
		0
	};
	libmixer.AddFuncSub(cellSurMixerCreate_table, "cellSurMixerCreate", cellSurMixerCreate);

	static const u64 cellSurMixerGetAANHandle_table[] = {
		// first instruction ignored
		0xffffffff3d607fce,
		0xffffffff616bfffe,
		0xffffffff812a0018,
		0xffffffff7d2afe70,
		0xffffffff91230000,
		0xffffffff7d404a78,
		0xffffffff7c005050,
		0xffffffff7c00fe70,
		0xffffffff7c035838,
		0xffffffff3c638031,
		0xffffffff38630002,
		0xffffffff7c6307b4,
		0xffffffff4e800020,
		0
	};
	libmixer.AddFuncSub(cellSurMixerGetAANHandle_table, "cellSurMixerGetAANHandle", cellSurMixerGetAANHandle);

	static const u64 cellSurMixerChStripGetAANPortNo_table[] = {
		// first instruction ignored
		0xffffffff7c661b78,
		0xffffffff3c608031,
		0xffffffff78c60020,
		0xffffffff78840020,
		0xffffffff60630002,
		0xffffffff80090018,
		0xffffffff78a50020,
		0xffffffff2f800000,
		0xffffffff4d9e0020,
		0xffffffff78030020,
		0xf000000040000000, // b
		0
	};
	libmixer.AddFuncSub(cellSurMixerChStripGetAANPortNo_table, "cellSurMixerChStripGetAANPortNo", cellSurMixerChStripGetAANPortNo);

	static const u64 cellSurMixerSetNotifyCallback_table[] = {
		// first instruction ignored
		0xffffffff7c0802a6,
		0xfffffffff821ff81,
		0xfffffffff8010090,
		0xffffffff7c6b1b78,
		0xffffffff3c608031,
		0xffffffff812a0018,
		0xffffffff7c882378,
		0xffffffff60630003,
		0xffffffff2f890000,
		0xffffffff2f0b0000,
		0xffffff00409e0020, // bne
		0xffffffff3c608031,
		0xffffffff60630002,
		0xffffffffe8010090,
		0xffffffff7c6307b4,
		0xffffffff38210080,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xffffff00419affec, // beq
		0xf0000000800a001c, // lwz
		0xffffffff79290020,
		0xffffffff38810070,
		0xffffffff2f800000,
		0xffffffff7d234b78,
		0
	};
	libmixer.AddFuncSub(cellSurMixerSetNotifyCallback_table, "cellSurMixerSetNotifyCallback", cellSurMixerSetNotifyCallback);

	static const u64 cellSurMixerRemoveNotifyCallback_table[] = {
		// first instruction ignored
		0xffffffff7c0802a6,
		0xfffffffff821ff81,
		0xfffffffff8010090,
		0xffffffff7c6a1b78,
		0xffffffff3d208031,
		0xffffffff806b0018,
		0xffffffff61290002,
		0xffffffff2f830000,
		0xf0000000409e0018, // bne
		0xffffffffe8010090,
		0xffffffff7d2307b4,
		0xffffffff38210080,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0
	};
	libmixer.AddFuncSub(cellSurMixerRemoveNotifyCallback_table, "cellSurMixerRemoveNotifyCallback", cellSurMixerRemoveNotifyCallback);

	static const u64 cellSurMixerStart_table[] = {
		0xfffffffff821ff71,
		0xffffffff7c0802a6,
		0xfffffffffbc10080,
		0xf000000083c20000, // lwz
		0xfffffffff80100a0,
		0xfffffffffba10078,
		0xfffffffffbe10088,
		0xffffffff801e0018,
		0xffffffff2f800000,
		0xf0000000409e002c, // bne
		0xffffffff3fe08031,
		0xffffffff63ff0002,
		0xffffffffe80100a0,
		0xffffffff7fe307b4,
		0xffffffffeba10078,
		0xffffffffebc10080,
		0xffffffff7c0803a6,
		0xffffffffebe10088,
		0xffffffff38210090,
		0xffffffff4e800020,
		0
	};
	libmixer.AddFuncSub(cellSurMixerStart_table, "cellSurMixerStart", cellSurMixerStart);

	static const u64 cellSurMixerSetParameter_table[] = {
		0xfffffffff821ff81,
		0xffffffff7c0802a6,
		0xfffffffffbc10070,
		0xfffffffffc000890,
		0xf000000083c28250, // lwz
		0xffffffff3d208031,
		0xfffffffff8010090,
		0xfffffffffbe10078,
		0xffffffff61290002,
		0xffffffff7c7f1b78,
		0xffffffff801e0018,
		0xffffffff2f800000,
		0xffff0000409e0020, // bne
		0xffffffffe8010090,
		0xffffffff7d2307b4,
		0xffffffffebc10070,
		0xffffffffebe10078,
		0xffffffff7c0803a6,
		0xffffffff38210080,
		0xffffffff4e800020,
		0xffffffff801e001c,
		0xffffffff2b03001f,
		0xffffffff2f800000,
		0xffff0000419cffd8, // blt
		0xffffffff2b83002b,
		0xffff000040990008, // ble
		0xffff0000409d0054, // ble
		0
	};
	libmixer.AddFuncSub(cellSurMixerSetParameter_table, "cellSurMixerSetParameter", cellSurMixerSetParameter);

	static const u64 cellSurMixerFinalize_table[] = {
		0xfffffffff821ff91,
		0xffffffff7c0802a6,
		0xfffffffff8010080,
		0xffffff004bfffde1, // bl
		0xffffffffe8010080,
		0xffffffff38600000,
		0xffffffff38210070,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xfffffffff821ff71,
		0xffffffff7c0802a6,
		0xfffffffffba10078,
		0xf000000083a28250, // lwz
		0xfffffffff80100a0,
		0xffffffff817d0018,
		0xffffffff7d635b78,
		0xffffffff812b0000,
		0xffffffff81490000,
		0xffffffff800a0000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff804a0004,
		0xffffffff4e800421,
		0
	};
	libmixer.AddFuncSub(cellSurMixerFinalize_table, "cellSurMixerFinalize", cellSurMixerFinalize);
}