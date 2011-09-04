#include <stdlib.h>

#include "../USB.h"

#include "CfgHelpers.h"

void CALLBACK USBsetSettingsDir( const char* dir )
{
	CfgSetSettingsDir(dir);
}

void SaveConfig() 
{
	CfgWriteBool(L"Interface", L"Logging", conf.Log);
}

void LoadConfig() 
{
	conf.Log = CfgReadBool(L"Interface", L"Logging", false);
}

