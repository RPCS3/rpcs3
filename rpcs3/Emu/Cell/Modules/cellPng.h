#pragma once

#include "Emu/Memory/vm_ptr.h"

enum CellPngTxtType : s32
{
	CELL_PNG_TEXT = 0,
	CELL_PNG_ZTXT = 1,
	CELL_PNG_ITXT = 2,
};

enum CellPngUnknownLocation : s32
{
	CELL_PNG_BEFORE_PLTE = 1,
	CELL_PNG_BEFORE_IDAT = 2,
	CELL_PNG_AFTER_IDAT  = 8,
};

struct CellPngPLTEentry
{
	u8 red;
	u8 green;
	u8 blue;
};

struct CellPngPaletteEntries
{
	be_t<u16> red;
	be_t<u16> green;
	be_t<u16> blue;
	be_t<u16> alpha;
	be_t<u16> frequency;
};

struct CellPngSPLTentry
{
	vm::bptr<char> paletteName;
	u8 sampleDepth;
	vm::bptr<CellPngPaletteEntries> paletteEntries;
	be_t<u32> paletteEntriesNumber;
};

struct CellPngTextInfo
{
	be_t<s32> txtType; // CellPngTxtType
	vm::bptr<char> keyword;
	vm::bptr<char> text;
	be_t<u32> textLength;
	vm::bptr<char> languageTag;
	vm::bptr<char> translatedKeyword;
};

struct CellPngPLTE
{
	be_t<u32> paletteEntriesNumber;
	vm::bptr<CellPngPLTEentry> paletteEntries;
};

struct CellPngGAMA
{
	be_t<double> gamma;
};

struct CellPngSRGB
{
	be_t<u32> renderingIntent;
};

struct CellPngICCP
{
	vm::bptr<char> profileName;
	vm::bptr<char> profile;
	be_t<u32> profileLength;
};

struct CellPngSBIT
{
	be_t<u32> red;
	be_t<u32> green;
	be_t<u32> blue;
	be_t<u32> gray;
	be_t<u32> alpha;
};

struct CellPngTRNS
{
	vm::bptr<char> alphaForPaletteIndex;
	be_t<u32> alphaForPaletteIndexNumber;
	be_t<u16> red;
	be_t<u16> green;
	be_t<u16> blue;
	be_t<u16> gray;
};

struct CellPngHIST
{
	vm::bptr<u16> histgramEntries;
	be_t<u32> histgramEntriesNumber;
};

struct CellPngTIME
{
	be_t<u16> year;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
	u8 second;
};

struct CellPngBKGD
{
	u8 paletteIndex;
	be_t<u32> red;
	be_t<u32> green;
	be_t<u32> blue;
	be_t<u32> gray;
};

struct CellPngSPLT
{
	vm::bptr<CellPngSPLTentry> sPLTentries;
	be_t<u32> sPLTentriesNumber;
};

struct CellPngOFFS
{
	be_t<s32> xPosition;
	be_t<s32> yPosition;
	be_t<u32> unitSpecifier;
};

struct CellPngPHYS
{
	be_t<u32> xAxis;
	be_t<u32> yAxis;
	be_t<u32> unitSpecifier;
};

struct CellPngSCAL
{
	be_t<u32> unitSpecifier;
	be_t<double> pixelWidth;
	be_t<double> pixelHeight;
};

struct CellPngCHRM
{
	be_t<double> whitePointX;
	be_t<double> whitePointY;
	be_t<double> redX;
	be_t<double> redY;
	be_t<double> greenX;
	be_t<double> greenY;
	be_t<double> blueX;
	be_t<double> blueY;
};

struct CellPngPCAL
{
	vm::bptr<char> calibrationName;
	be_t<s32> x0;
	be_t<s32> x1;
	be_t<u32> equationType;
	be_t<u32> numberOfParameters;
	vm::bptr<char> unitName;
	vm::bpptr<char> parameter;
};

struct CellPngUnknownChunk
{
	char chunkType[5];
	vm::bptr<char> chunkData;
	be_t<u32> length;
	be_t<s32> location; // CellPngUnknownLocation
};
