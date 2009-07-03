/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __CONFIGDLG_H__
#define __CONFIGDLG_H__

#include "Linux.h"
#include "R3000A.h"
#include "IopMem.h"

typedef enum
{
	NO_PLUGIN_TYPE = 0,
	GS,
	PAD1,
	PAD2,
	SPU2,
	CDVD,
	DEV9,
	USB,
	FW,
	BIOS
} plugin_types;

typedef enum
{
	PLUGIN_NULL = 0,
	PLUGIN_CONFIG,
	PLUGIN_TEST,
	PLUGIN_ABOUT
} plugin_callback;

typedef struct
{
	GtkWidget *Combo;
	GList *PluginNameList;
	char plist[255][255];
	int plugins;
} PluginConf;

PluginConf GSConfS;
PluginConf PAD1ConfS;
PluginConf PAD2ConfS;
PluginConf SPU2ConfS;
PluginConf CDVDConfS;
PluginConf DEV9ConfS;
PluginConf USBConfS;
PluginConf FWConfS;
PluginConf BiosConfS;

__forceinline PluginConf *ConfS(int type)
{
	switch (type)
	{
		case GS: return &GSConfS; 
		case PAD1: return &PAD1ConfS; 
		case PAD2: return &PAD2ConfS; 
		case SPU2: return &SPU2ConfS; 
		case CDVD: return &CDVDConfS; 
		case DEV9: return &DEV9ConfS; 
		case USB: return &USBConfS; 
		case FW: return &FWConfS; 
		case BIOS: return &BiosConfS; 
	}
}

__forceinline const char *PluginName(int type)
{
	switch (type)
	{
		case GS: return Config.GS; 
		case PAD1: return Config.PAD1; 
		case PAD2: return Config.PAD2; 
		case SPU2: return Config.SPU2; 
		case CDVD: return Config.CDVD; 
		case DEV9: return Config.DEV9; 
		case USB: return Config.USB; 
		case FW: return Config.FW; 
		case BIOS: return Config.Bios; 
	}
	return NULL;
}

__forceinline const char *PluginCallbackName(int type, int call)
{
	switch (type)
	{
		case GS:
			switch (call)
			{
				case PLUGIN_CONFIG: return "GSconfigure"; 
				case PLUGIN_TEST: return "GStest"; 
				case PLUGIN_ABOUT: return "GSabout"; 
				default: return NULL; 
			}
			break;
		case PAD1:
		case PAD2:
			switch (call)
			{
				case PLUGIN_CONFIG: return "PADconfigure"; 
				case PLUGIN_TEST: return "PADtest"; 
				case PLUGIN_ABOUT: return  "PADabout"; 
				default: return NULL; 
			}
			break;
		case SPU2:
			switch (call)
			{
				case PLUGIN_CONFIG: return "SPU2configure"; 
				case PLUGIN_TEST: return "SPU2test"; 
				case PLUGIN_ABOUT: return "SPU2about"; 
				default: return NULL; 
			}
			break;
		case CDVD:
			switch (call)
			{
				case PLUGIN_CONFIG: return "CDVDconfigure"; 
				case PLUGIN_TEST: return "CDVDtest"; 
				case PLUGIN_ABOUT: return "CDVDabout"; 
				default: return NULL; 
			}
			break;
		case DEV9:
			switch (call)
			{
				case PLUGIN_CONFIG: return "DEV9configure"; 
				case PLUGIN_TEST: return "DEV9test"; 
				case PLUGIN_ABOUT: return "DEV9about"; 
				default: return NULL; 
			}
			break;
		case USB:
			switch (call)
			{
				case PLUGIN_CONFIG: return "USBconfigure";
				case PLUGIN_TEST: return "USBtest";
				case PLUGIN_ABOUT: return "USBabout";
				default: return NULL;
			}
			break;
		case FW:
			switch (call)
			{
				case PLUGIN_CONFIG: return "FWconfigure";
				case PLUGIN_TEST: return "FWtest";
				case PLUGIN_ABOUT: return "FWabout";
				default: return NULL;
			}
			break;
		default:
			return NULL;
			break;
	}
	return NULL;
}

__forceinline plugin_callback strToPluginCall(char *s)
{
	char *sub = NULL;
	
	sub = strstr(s, "about");
	if (sub != NULL) return PLUGIN_ABOUT;
	
	sub = strstr(s, "test");
	if (sub != NULL) return PLUGIN_TEST;
	
	sub = strstr(s, "configure");
	if (sub != NULL) return PLUGIN_CONFIG;
	
	return PLUGIN_NULL;
}

__forceinline plugin_types strToPluginType(char *s)
{
	char *sub = NULL;
	
	sub = strstr(s, "GS");
	if (sub != NULL) return GS;
	
	sub = strstr(s, "SPU2");
	if (sub != NULL) return SPU2;
	
	sub = strstr(s, "PAD1");
	if (sub != NULL) return PAD1;
	
	sub = strstr(s, "PAD2");
	if (sub != NULL) return PAD2;
	
	sub = strstr(s, "CDVD");
	if (sub != NULL) return CDVD;
	
	sub = strstr(s, "DEV9");
	if (sub != NULL) return DEV9;
	
	sub = strstr(s, "USB");
	if (sub != NULL) return USB;
	
	sub = strstr(s, "FW");
	if (sub != NULL) return FW;
	
	sub = strstr(s, "Bios");
	if (sub != NULL) return BIOS;
	return NO_PLUGIN_TYPE;
}

__forceinline const char *PluginTypeToStr(int type)
{
	switch (type)
	{
		case GS: return "GS";
		case SPU2: return "SPU2";
		case PAD1: return "PAD1";
		case PAD2: return "PAD2";
		case CDVD: return "CDVD";
		case DEV9: return "DEV9";
		case USB: return "USB";
		case FW: return "FW";
		case BIOS: return "Bios";
		default: return "";
	}
	return "";
}
GtkWidget *ConfDlg;

_PS2EgetLibType		PS2EgetLibType = NULL;
_PS2EgetLibVersion2	PS2EgetLibVersion2 = NULL;
_PS2EgetLibName		PS2EgetLibName = NULL;

// Helper Functions
void FindPlugins();
void OnConf_Gs(GtkMenuItem *menuitem, gpointer user_data);
void OnConf_Pads(GtkMenuItem *menuitem, gpointer user_data);
void OnConf_Cpu(GtkMenuItem *menuitem, gpointer user_data);
void OnConf_Conf(GtkMenuItem *menuitem, gpointer user_data);
void SetActiveComboItem(GtkComboBox *widget, char plist[255][255], GList *list, char *conf);
void SetComboToGList(GtkComboBox *widget, GList *list);

#endif // __CONFIGDLG_H__