# Plugins

# make cdvdGigaherz
#if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/cdvdGigaherz" AND cdvdGigaherz)
#	add_subdirectory(cdvdGigaherz)
#endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/cdvdGigaherz" AND cdvdGigaherz)

# make CDVDiso
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDiso" AND CDVDiso)
	add_subdirectory(CDVDiso/src)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDiso" AND CDVDiso)

# make CDVDisoEFP
# if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDisoEFP" AND CDVDisoEFP)
# 	add_subdirectory(CDVDisoEFP)
# endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDisoEFP" AND CDVDisoEFP)

# make CDVDlinuz
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDlinuz" AND CDVDlinuz)
	add_subdirectory(CDVDlinuz/Src)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDlinuz" AND CDVDlinuz)

# make CDVDnull
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDnull" AND CDVDnull)
	add_subdirectory(CDVDnull)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDnull" AND CDVDnull)

# make CDVDolio
# if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDolio" AND CDVDolio)
# 	add_subdirectory(CDVDolio)
# endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDolio" AND CDVDolio)

# make CDVDpeops
#if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDpeops" AND CDVDpeops)
#	add_subdirectory(CDVDpeops)
#endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/CDVDpeops" AND CDVDpeops)

# make dev9null
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/dev9null" AND dev9null)
	add_subdirectory(dev9null)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/dev9null" AND dev9null)

# make FWnull
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/FWnull" AND FWnull)
	add_subdirectory(FWnull)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/FWnull" AND FWnull)

# make GSdx
#if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/GSdx" AND GSdx)
#	add_subdirectory(GSdx)
#endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/GSdx" AND GSdx)

# make GSnull
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/GSnull" AND GSnull)
	add_subdirectory(GSnull)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/GSnull" AND GSnull)

# make LilyPad
#if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/LilyPad" AND LilyPad)
#	add_subdirectory(LilyPad)
#endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/LilyPad" AND LilyPad)

# make onepad
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/onepad" AND onepad)
	add_subdirectory(onepad)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/onepad" AND onepad)

# make PadNull
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/PadNull" AND PadNull)
	add_subdirectory(PadNull)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/PadNull" AND PadNull)

# make PeopsSPU2
# if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/PeopsSPU2" AND PeopsSPU2)
# 	add_subdirectory(PeopsSPU2)
# endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/PeopsSPU2" AND PeopsSPU2)

# make SPU2null
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/SPU2null" AND SPU2null)
	add_subdirectory(SPU2null)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/SPU2null" AND SPU2null)

# make spu2-x
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/spu2-x" AND spu2-x)
	add_subdirectory(spu2-x/src)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/spu2-x" AND spu2-x)

# make SSSPSXPAD
#if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/SSSPSXPAD" AND SSSPSXPAD)
#	add_subdirectory(SSSPSXPAD)
#endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/SSSPSXPAD" AND SSSPSXPAD)

# make USBnull
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/USBnull" AND USBnull)
	add_subdirectory(USBnull)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/USBnull" AND USBnull)

# make xpad
#if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/xpad" AND xpad)
#	add_subdirectory(xpad)
#endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/xpad" AND xpad)

# make zerogs
#if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zerogs" AND zerogs)
#	add_subdirectory(zerogs)
#endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zerogs" AND zerogs)

# make zzogl-pg
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zzogl-pg" AND zzogl)
	add_subdirectory(zzogl-pg/opengl)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zzogl-pg" AND zzogl)

# make zeropad
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zeropad" AND zeropad)
	add_subdirectory(zeropad)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zeropad" AND zeropad)

# make zerospu2
if(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zerospu2" AND zerospu2)
	add_subdirectory(zerospu2)
endif(EXISTS "${PROJECT_SOURCE_DIR}/plugins/zerospu2" AND zerospu2)

