#pragma once


//Callback Functions 
typedef int (*CellSurMixerNotifyCallbackFunction)(void *arg, u32 counter, u32 samples); //Currently unused.

//libmixer datatypes
typedef void * CellAANHandle;

struct CellSSPlayerConfig
{
	be_t<u32> channels;
	be_t<u32> outputMode;
}; 

struct CellSSPlayerWaveParam 
{ 
	void *addr;
	be_t<s32> format;
	be_t<u32> samples;
	be_t<u32> loopStartOffset;
	be_t<u32> startOffset;
};

struct CellSSPlayerCommonParam 
{ 
	be_t<u32> loopMode;
	be_t<u32> attackMode;
};

struct CellSurMixerPosition 
{ 
	be_t<float> x;
	be_t<float> y;
	be_t<float> z;
};

struct CellSSPlayerRuntimeInfo 
{ 
	be_t<float> level;
	be_t<float> speed;
	CellSurMixerPosition position;
};

struct CellSurMixerConfig 
{ 
	be_t<s32> priority;
	be_t<u32> chStrips1;
	be_t<u32> chStrips2;
	be_t<u32> chStrips6;
	be_t<u32> chStrips8;
};

struct CellSurMixerChStripParam 
{ 
	be_t<u32> param;
	void *attribute;
	be_t<s32> dBSwitch;
	be_t<float> floatVal;
	be_t<s32> intVal;
};

CellSSPlayerWaveParam current_SSPlayerWaveParam;