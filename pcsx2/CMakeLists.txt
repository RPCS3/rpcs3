# Check that people use the good file
if(NOT TOP_CMAKE_WAS_SOURCED)
    message(FATAL_ERROR "
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt")
endif(NOT TOP_CMAKE_WAS_SOURCED)


# set common flags
set(CommonFlags 
	-pthread
    -fvisibility=hidden
	-fno-guess-branch-probability
	-fno-dse
	-fno-tree-dse
	-fno-strict-aliasing
	-pipe
	-Wno-format
	-Wno-unused-parameter
	-Wno-unused-value
	-Wunused-variable)

# set warning flags
set(DebugFlags
	-W
	-g)

# set optimization flags
set(OptimizationFlags
	-falign-functions
	-falign-jumps
	-falign-labels
	-falign-loops
	-fcaller-saves
	-fcprop-registers
	-fcrossjumping
	-fcse-follow-jumps
	-fcse-skip-blocks
	-fdefer-pop
	-fdelete-null-pointer-checks
	-fgcse
	-fgcse-lm
	-fif-conversion
	-fif-conversion2
	-fmerge-constants
	-foptimize-sibling-calls
	-fpeephole2
	-fregmove
	-freorder-blocks
	-freorder-functions 
	-frerun-cse-after-loop
	-fsched-interblock
	-fsched-spec
	-fstrict-overflow
	-fthread-jumps
	-ftree-ccp
	-ftree-ch
	-ftree-copyrename
	-ftree-dce
	-ftree-dominator-opts
	-ftree-fre
	-ftree-lrs
	-ftree-pre
	-ftree-sra
	-ftree-ter
	-ftree-vrp
	-funit-at-a-time)

# Debug - Build
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	
	# executable name
	set(Output pcsx2-dbg)
	
	# add defines
	add_definitions(${CommonFlags} ${DebugFlags} -DPCSX2_DEVBUILD -DPCSX2_DEBUG -DWX_PRECOMP)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

# Devel - Build
if(CMAKE_BUILD_TYPE STREQUAL Devel)

	# executable name
	set(Output pcsx2-dev)
	
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags} -DPCSX2_DEVBUILD -DWX_PRECOMP -DNDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Devel)

# Release - Build
if(CMAKE_BUILD_TYPE STREQUAL Release)

	# executable name
	set(Output pcsx2)
	
	# add defines
	add_definitions(${CommonFlags} ${OptimizationFlags} -DWX_PRECOMP -DNDEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL Release)

# you must have both svn client executable and a source that contains svn metadata
if(Subversion_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.svn")
    # execute subversion
    Subversion_WC_INFO(${CMAKE_SOURCE_DIR} tmpvar)

    # write svnrev.h
    file(WRITE ${CMAKE_BINARY_DIR}/pcsx2/svnrev.h "#define SVN_REV ${tmpvar_WC_REVISION} \n#define SVN_MODS 0")
else(Subversion_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.svn")
    # write svnrev.h
    file(WRITE ${CMAKE_BINARY_DIR}/pcsx2/svnrev.h "#define SVN_REV 0 \n#define SVN_MODS 0")
endif(Subversion_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.svn")

# variable with sources of this executable
set(pcsx2Sources
	Cache.cpp
	COP0.cpp
	COP2.cpp
	Counters.cpp
	GameDatabase.cpp
	Dump.cpp
	Elfheader.cpp
	FiFo.cpp
	FPU.cpp
	Gif.cpp
	GS.cpp
	GSState.cpp
	Hw.cpp
	HwRead.cpp
	HwWrite.cpp
	Interpreter.cpp
	IopBios.cpp
	IopCounters.cpp
	IopDma.cpp
	IopHw.cpp
	IopIrq.cpp
	IopMem.cpp
	IopSio2.cpp
#	Mdec.cpp
	Memory.cpp
	MMI.cpp
	MTGS.cpp
	Patch.cpp
	Patch_Memory.cpp
	Pcsx2Config.cpp
	PluginManager.cpp
	PrecompiledHeader.cpp
	R3000A.cpp
	R3000AInterpreter.cpp
	R3000AOpcodeTables.cpp
	R5900.cpp
	R5900OpcodeImpl.cpp
	R5900OpcodeTables.cpp
	SaveState.cpp
	ShiftJisToUnicode.cpp
	Sif.cpp
	Sif0.cpp
	Sif1.cpp
	Sio.cpp
	SourceLog.cpp
	SPR.cpp
	Stats.cpp
	System.cpp
	Vif0_Dma.cpp
	Vif1_Dma.cpp
	Vif1_MFIFO.cpp
	Vif.cpp
	Vif_Codes.cpp
	Vif_Transfer.cpp
	Vif_Unpack.cpp
	vtlb.cpp
	VU0.cpp
	VUmicro.cpp
	VU0micro.cpp
	VU0microInterp.cpp
	VU1micro.cpp
	VU1microInterp.cpp
	VUflags.cpp
	VUmicroMem.cpp
	VUops.cpp)

# variable with sources of this executable
set(pcsx2Headers
	Cache.h
	cheatscpp.h
	Common.h
	Config.h
	COP0.h
	Counters.h
	Dmac.h
	Dump.h
	GameDatabase.h
	Elfheader.h
	Gif.h
	GS.h
	Hardware.h
	Hw.h
	IopBios.h
	IopCommon.h
	IopCounters.h
	IopDma.h
	IopHw.h
	IopMem.h
	IopSio2.h
#	Mdec.h
	Memory.h
	MemoryTypes.h
	NakedAsm.h
	Patch.h
	Patch_Obsolete.h
	PathDefs.h
	Plugins.h
	PrecompiledHeader.h
	R3000A.h
	R5900Exceptions.h
	R5900.h
	R5900OpcodeTables.h
	SamplProf.h
	SaveState.h
	Sifcmd.h
	Sif.h
	Sio.h
	sio_internal.h
	SPR.h
	Stats.h
	SysForwardDefs.h
	System.h
	Vif_Dma.h
	Vif.h
	Vif_Unpack.h
	Vif_Unpack.inl
	vtlb.h
	VUflags.h
	VUmicro.h
	VUops.h)

# CDVD sources
set(pcsx2CDVDSources
	CDVD/CdRom.cpp
	CDVD/CDVDaccess.cpp
	CDVD/CDVD.cpp
	CDVD/CDVDisoReader.cpp
	CDVD/IsoFileFormats.cpp
	CDVD/IsoFileTools.cpp
	CDVD/IsoFS/IsoFile.cpp
	CDVD/IsoFS/IsoFSCDVD.cpp
	CDVD/IsoFS/IsoFS.cpp)

# CDVD headers
set(pcsx2CDVDHeaders
	CDVD/CdRom.h
	CDVD/CDVDaccess.h
	CDVD/CDVD.h
	CDVD/CDVD_internal.h
	CDVD/CDVDisoReader.h
	CDVD/IsoFileFormats.h
	CDVD/IsoFileTools.h
	CDVD/IsoFS/IsoDirectory.h
	CDVD/IsoFS/IsoFileDescriptor.h
	CDVD/IsoFS/IsoFile.h
	CDVD/IsoFS/IsoFSCDVD.h
	CDVD/IsoFS/IsoFS.h
	CDVD/IsoFS/SectorSource.h)

# DebugTools sources
set(pcsx2DebugToolsSources
	DebugTools/DisR3000A.cpp
	DebugTools/DisR5900asm.cpp
	DebugTools/DisR5900.cpp
	DebugTools/DisVU0Micro.cpp
	DebugTools/DisVU1Micro.cpp)

# DebugTools headers
set(pcsx2DebugToolsHeaders
	DebugTools/Debug.h
	DebugTools/DisASm.h
	DebugTools/DisVUmicro.h
	DebugTools/DisVUops.h)

# gui sources
set(pcsx2GuiSources
	gui/AdvancedDialog.cpp
	gui/AppAssert.cpp
	gui/AppConfig.cpp
	gui/AppCorePlugins.cpp
	gui/AppCoreThread.cpp
	gui/AppEventSources.cpp
	gui/AppGameDatabase.cpp
	gui/AppInit.cpp
	gui/AppMain.cpp
	gui/AppRes.cpp
	gui/ConsoleLogger.cpp
	gui/CpuUsageProvider.cpp
	gui/CpuUsageProviderLnx.cpp
	gui/Dialogs/AboutBoxDialog.cpp
	gui/Dialogs/AppConfigDialog.cpp
	gui/Dialogs/AssertionDialog.cpp
	gui/Panels/BaseApplicableConfigPanel.cpp
	gui/Panels/MemoryCardListView.cpp
	gui/Dialogs/BaseConfigurationDialog.cpp
	gui/Dialogs/BiosSelectorDialog.cpp
	gui/Dialogs/ConfirmationDialogs.cpp
	gui/Dialogs/CreateMemoryCardDialog.cpp
	gui/Dialogs/FirstTimeWizard.cpp
	gui/Dialogs/GameDatabaseDialog.cpp
	gui/Dialogs/ImportSettingsDialog.cpp
	gui/Dialogs/LogOptionsDialog.cpp
	gui/Dialogs/McdConfigDialog.cpp
	gui/Dialogs/PickUserModeDialog.cpp
	gui/Dialogs/StuckThreadDialog.cpp
	gui/Dialogs/SysConfigDialog.cpp
	gui/ExecutorThread.cpp
	gui/FrameForGS.cpp
	gui/GlobalCommands.cpp
	gui/i18n.cpp
	gui/IsoDropTarget.cpp
	gui/MainFrame.cpp
	gui/MainMenuClicks.cpp
	gui/MemoryCardFile.cpp
	gui/Panels/BaseApplicableConfigPanel.cpp
	gui/Panels/MemoryCardListPanel.cpp
	gui/MessageBoxes.cpp
	gui/MSWstuff.cpp
	gui/Panels/AudioPanel.cpp
	gui/Panels/BiosSelectorPanel.cpp
	gui/Panels/CpuPanel.cpp
	gui/Panels/DirPickerPanel.cpp
	gui/Panels/GameDatabasePanel.cpp
	gui/Panels/GameFixesPanel.cpp
	gui/Panels/GSWindowPanel.cpp
	gui/Panels/LogOptionsPanels.cpp
	gui/Panels/MiscPanelStuff.cpp
	gui/Panels/PathsPanel.cpp
	gui/Panels/PluginSelectorPanel.cpp
	gui/Panels/SpeedhacksPanel.cpp
	gui/Panels/VideoPanel.cpp
	gui/pxLogTextCtrl.cpp
	gui/RecentIsoList.cpp
	gui/Saveslots.cpp
	gui/SysState.cpp
	gui/UpdateUI.cpp
	)

# gui headers
set(pcsx2GuiHeaders
	gui/AdvancedDialog.h
	gui/App.h
	gui/ApplyState.h
	gui/AppAccelerators.h
	gui/AppCommon.h
	gui/AppConfig.h
	gui/AppCorePlugins.h
	gui/AppEventListeners.h
	gui/AppForwardDefs.h
	gui/AppGameDatabase.h
	gui/ConsoleLogger.h
	gui/CpuUsageProvider.h
	gui/Dialogs/BaseConfigurationDialog.inl
	gui/Dialogs/ConfigurationDialog.h
	gui/Dialogs/LogOptionsDialog.h
	gui/Dialogs/ModalPopups.h
	gui/i18n.h
	gui/IsoDropTarget.h
	gui/MainFrame.h
	gui/MemoryCardFile.h
	gui/MSWstuff.h
	gui/Panels/ConfigurationPanels.h
	gui/Panels/LogOptionsPanels.h
	gui/Panels/MemoryCardPanels.h
	gui/RecentIsoList.h
	)

# IPU sources
set(pcsx2IPUSources
	IPU/IPU.cpp
	IPU/IPU_Fifo.cpp
	IPU/mpeg2lib/Idct.cpp
	IPU/mpeg2lib/Mpeg.cpp
	IPU/yuv2rgb.cpp)

# IPU headers
set(pcsx2IPUHeaders
	IPU/IPU.h
	IPU/IPU_Fifo.h
	IPU/yuv2rgb.h)

# Linux sources
set(pcsx2LinuxSources
	Linux/LnxHostSys.cpp
	Linux/LnxKeyCodes.cpp)

# Linux headers
set(pcsx2LinuxHeaders
	)

# ps2 sources
set(pcsx2ps2Sources
	ps2/BiosTools.cpp
	ps2/GIFpath.cpp
	ps2/LegacyDmac.cpp
	ps2/Iop/IopHwRead.cpp
	ps2/Iop/IopHwWrite.cpp)

# ps2 headers
set(pcsx2ps2Headers
	ps2/BiosTools.h
	ps2/eeHwTraceLog.inl
	ps2/HwInternal.inl
	ps2/Iop/IopHw_Internal.h)

# RDebug sources
set(pcsx2RDebugSources
	RDebug/deci2.cpp
	RDebug/deci2_dbgp.cpp
	RDebug/deci2_dcmp.cpp
	RDebug/deci2_drfp.cpp
	RDebug/deci2_iloadp.cpp
	RDebug/deci2_netmp.cpp
	RDebug/deci2_ttyp.cpp)

# RDebug headers
set(pcsx2RDebugHeaders
	RDebug/deci2_dbgp.h
	RDebug/deci2_dcmp.h
	RDebug/deci2_drfp.h
	RDebug/deci2.h
	RDebug/deci2_iloadp.h
	RDebug/deci2_netmp.h
	RDebug/deci2_ttyp.h)

# System sources
set(pcsx2SystemSources
	System/SysCoreThread.cpp
	System/SysThreadBase.cpp)

# System headers
set(pcsx2SystemHeaders
	System/PageFaultSource.h
	System/SysThreads.h)

# Utilities sources
set(pcsx2UtilitiesSources
	Utilities/FileUtils.cpp)

# Utilities headers
set(pcsx2UtilitiesHeaders
	Utilities/AsciiFile.h)

# Zip tools utilies sources
set(pcsx2ZipToolsSources
    ZipTools/thread_gzip.cpp
    ZipTools/thread_lzma.cpp)

# Zip tools utilies headers
set(pcsx2ZipToolsHeaders
    ZipTools/ThreadedZipTools.h)


# Windows sources
set(pcsx2WindowsSources
	windows/cheats/browser.cpp
	windows/cheats/cheats.cpp
	windows/DwmSetup.cpp
	windows/ini.cpp
	windows/PatchBrowser.cpp
	windows/SampleProf.cp
	windows/WinCompressNTFS.cpp
	windows/WinConsolePipe.cpp
	windows/WinSysExec.cpp)

# Windows headers
set(pcsx2WindowsHeaders
	windows/cheats/cheats.h
	windows/resource.h
	windows/Win32.h)

# x86 sources
set(pcsx2x86Sources
	x86/BaseblockEx.cpp
	x86/iCOP0.cpp
	x86/iCore.cpp
	x86/iFPU.cpp
	x86/iFPUd.cpp
	x86/iMisc.cpp
	x86/iMMI.cpp
	x86/iPsxMem.cpp
	x86/iR3000A.cpp
	x86/iR3000Atables.cpp
	x86/iR5900Misc.cpp
	x86/ir5900tables.cpp
	x86/iVU1micro.cpp
	x86/ix86-32/iCore-32.cpp
	x86/ix86-32/iR5900-32.cpp
	x86/ix86-32/iR5900Arit.cpp
	x86/ix86-32/iR5900AritImm.cpp
	x86/ix86-32/iR5900Branch.cpp
	x86/ix86-32/iR5900Jump.cpp
	x86/ix86-32/iR5900LoadStore.cpp
	x86/ix86-32/iR5900Move.cpp
	x86/ix86-32/iR5900MultDiv.cpp
	x86/ix86-32/iR5900Shift.cpp
	x86/ix86-32/iR5900Templates.cpp
	x86/ix86-32/recVTLB.cpp
	x86/newVif_Dynarec.cpp
	x86/newVif_Unpack.cpp
	x86/newVif_UnpackSSE.cpp
	x86/sVU_Lower.cpp
	x86/sVU_Micro.cpp
	x86/sVU_Upper.cpp
	x86/sVU_zerorec.cpp
	)

# x86 headers
set(pcsx2x86Headers
	x86/BaseblockEx.h
	x86/iCOP0.h
	x86/iCore.h
	x86/iFPU.h
	x86/iMMI.h
	x86/iR3000A.h
	x86/iR5900Arit.h
	x86/iR5900AritImm.h
	x86/iR5900Branch.h
	x86/iR5900.h
	x86/iR5900Jump.h
	x86/iR5900LoadStore.h
	x86/iR5900Move.h
	x86/iR5900MultDiv.h
	x86/iR5900Shift.h
	x86/microVU.h
	x86/microVU_IR.h
	x86/microVU_Misc.h
	x86/microVU_Alloc.inl
	x86/microVU_Analyze.inl
	x86/microVU_Branch.inl
	x86/microVU_Clamp.inl
	x86/microVU_Compile.inl
	x86/microVU.cpp
	x86/microVU_Execute.inl
	x86/microVU_Flags.inl
	x86/microVU_Log.inl
	x86/microVU_Lower.inl
	x86/microVU_Macro.inl
	x86/microVU_Misc.inl
	x86/microVU_Tables.inl
	x86/microVU_Upper.inl
	x86/newVif.h
	x86/newVif_BlockBuffer.h
	x86/newVif_HashBucket.h
	x86/newVif_UnpackSSE.h
	x86/sVU_Compare.h
	x86/sVU_Debug.h
	x86/sVU_Micro.h
	x86/sVU_zerorec.h
	)

# colect .asm files
set(pcsx2AsmFiles
	IPU/yuv2rgb.asm
	x86/ix86-32/aVif_proc-32.asm)

# collect .S files
set(pcsx2SSources
	x86/aVUzerorec.S)

# change language of .S-files to c++
set_source_files_properties(${pcsx2SSources} PROPERTIES LANGUAGE CXX)

# common Sources
set(Common
	${pcsx2Sources}
	${pcsx2Headers}
	${pcsx2CDVDSources}
	${pcsx2CDVDHeaders}
	${pcsx2DebugToolsSources}
	${pcsx2DebugToolsSources}
	${pcsx2GuiSources}
	${pcsx2GuiHeaders}
	${pcsx2IPUSources}
	${pcsx2IPUHeaders}
	${pcsx2ps2Sources}
	${pcsx2ps2Headers}
	${pcsx2SystemSources}
	${pcsx2SystemHeaders}
	${pcsx2UtilitiesSources}
	${pcsx2UtilitiesHeaders}
	${pcsx2x86Sources}
	${pcsx2x86Headers}
	${pcsx2ZipToolsSources}
	${pcsx2ZipToolsHeaders}
	${pcsx2SSources})

# platform sources
# Linux
if(Linux)
	set(Platform
		${pcsx2LinuxSources}
		${pcsx2LinuxHeaders})
endif(Linux)		

# Windows
if(Windows)
	set(Platform
		${pcsx2WindowsSources}
		${pcsx2WindowsHeaders})
endif(Windows)

# MacOSX
if(MacOSX)
	set(PlatformSources
		)
endif(MacOSX)	

# additonal include directories
include_directories(.
					gui
					x86
					${CMAKE_BINARY_DIR}/pcsx2)

# set runtime output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# add executable
add_executable(${Output}
			   ${Common}
			   ${Platform})

# add dependencies
add_dependencies(${Output} Resources)

# link target with project internal libraries
target_link_libraries(${Output} Utilities x86emitter)
					
# link target with wx
target_link_libraries(${Output} ${wxWidgets_LIBRARIES})

if(Linux)
	# link target with gtk2
	target_link_libraries(${Output} ${GTK2_LIBRARIES})
endif(Linux)

# link target with zlib
target_link_libraries(${Output} ${ZLIB_LIBRARIES})

# User flags options
if(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
    target_link_libraries(${Output} "${USER_CMAKE_LD_FLAGS}")
endif(NOT USER_CMAKE_LD_FLAGS STREQUAL "")
