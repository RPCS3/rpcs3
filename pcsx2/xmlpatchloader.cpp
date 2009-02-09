/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
#include "PrecompiledHeader.h"

using namespace std;

#include "tinyxml/tinyxml.h"

#include "Patch.h"
#include "System.h"

#ifdef _MSC_VER
#pragma warning(disable:4996) //ignore the stricmp deprecated warning
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#if !defined(_WIN32)
#ifndef strnicmp
#define strnicmp strncasecmp
#endif

#ifndef stricmp
#define stricmp strcasecmp
#endif
#else
#define strnicmp _strnicmp
#define stricmp _stricmp
#endif

#include "../cheatscpp.h"

int LoadGroup(TiXmlNode *group, int parent);

Group::Group(int nParent,bool nEnabled, string &nTitle):
		parentIndex(nParent),enabled(nEnabled),title(nTitle)
{
}

Patch::Patch(int patch, int grp, bool en, string &ttl):
	title(ttl),
	group(grp),
	enabled(en),
	patchIndex(patch)
{
}

Patch Patch::operator =(const Patch&p)
{
	title.assign(p.title);
	group=p.group;
	enabled=p.enabled;
	patchIndex=p.patchIndex;
	return *this;
}


vector<Group> groups;
vector<Patch> patches;

int LoadPatch( const string& crc)
{
	char pfile[256];
	sprintf(pfile,"patches\\%hs.xml",&crc);

	patchnumber=0;

	TiXmlDocument doc( pfile );
	bool loadOkay = doc.LoadFile();
	if ( !loadOkay )
	{
		//SysPrintf("XML Patch Loader: Could not load file '%s'. Error='%s'.\n", pfile, doc.ErrorDesc() );
		return -1;
	} else SysPrintf("XML Patch Loader: '%s' Found\n", pfile);

	TiXmlNode *root = doc.FirstChild("GAME");
	if(!root)
	{
		SysPrintf("XML Patch Loader: Root node is not GAME, invalid patch file.\n");
		return -1;
	}

	TiXmlElement *rootelement = root->ToElement();

	const char *title=rootelement->Attribute("title");
	if(title)
		SysPrintf("XML Patch Loader: Game Title: %s\n",title);

	int result=LoadGroup(root,-1);
	if(result) {
		patchnumber=0;
		return result;
	}

	Console::SetTitle(
		((title==NULL) || (strlen(title)==0)) ? "<No Title>" : title );

	return 0;
}


int LoadGroup(TiXmlNode *group,int gParent)
{
	TiXmlElement *groupelement = group->ToElement();

	const char *gtitle=groupelement->Attribute("title");
	if(gtitle)
		SysPrintf("XML Patch Loader: Group Title: %s\n",gtitle);

	const char *enable=groupelement->Attribute("enabled");
	bool gEnabled=true;
	if(enable)
	{
		if(strcmp(enable,"false")==0)
		{
			SysPrintf("XML Patch Loader: Group is disabled.\n");
			gEnabled=false;
		}
	}

	TiXmlNode *comment = group->FirstChild("COMMENT");
	if(comment)
	{
		TiXmlElement *cmelement = comment->ToElement();
		const char *comment = cmelement->GetText();
		if(comment)
			SysPrintf("XML Patch Loader: Group Comment:\n%s\n---\n",comment);
	}

	string t;
	
	if(gtitle)
		t.assign(gtitle);
	else
		t.clear();
	
	Group gp=Group(gParent,gEnabled,t);
	groups.push_back(gp);

	int gIndex=groups.size()-1;

	// only valid for recompilers
	TiXmlNode *fastmemory=group->FirstChild("FASTMEMORY");
	if(fastmemory!=NULL)
		SetFastMemory(1);

    TiXmlNode *zerogs=group->FirstChild("ZEROGS");
	if(zerogs!=NULL)
	{
        TiXmlElement *rm=zerogs->ToElement();
        const char* pid = rm->FirstAttribute()->Value();
        if( pid != NULL ) sscanf(pid, "%x", &g_ZeroGSOptions);
        else SysPrintf("zerogs attribute wrong");
    }

	TiXmlNode *roundmode=group->FirstChild("ROUNDMODE");
	if(roundmode!=NULL)
	{
		int eetype=0x0000;
		int vutype=0x6000;

		TiXmlElement *rm=roundmode->ToElement();
		if(rm!=NULL)
		{
			const char *eetext=rm->Attribute("ee");
			const char *vutext=rm->Attribute("vu");

			if(eetext != NULL) {
				eetype = 0xffff;
				if( stricmp(eetext, "near") == 0 ) {
					eetype = 0x0000;
				}
				else if( stricmp(eetext, "down") == 0 ) {
					eetype = 0x2000;
				}
				else if( stricmp(eetext, "up") == 0 ) {
					eetype = 0x4000;
				}
				else if( stricmp(eetext, "chop") == 0 ) {
					eetype = 0x6000;
				}
			}

			if(vutext != NULL) {
				vutype = 0xffff;
				if( stricmp(vutext, "near") == 0 ) {
					vutype = 0x0000;
				}
				else if( stricmp(vutext, "down") == 0 ) {
					vutype = 0x2000;
				}
				else if( stricmp(vutext, "up") == 0 ) {
					vutype = 0x4000;
				}
				else if( stricmp(vutext, "chop") == 0 ) {
					vutype = 0x6000;
				}
			}
		}
		if(( eetype == 0xffff )||( vutype == 0xffff )) {
			printf("XML Patch Loader: WARNING: Invalid value in ROUNDMODE.\n");
		}
		else {
			SetRoundMode(eetype,vutype);
		}
	}

	TiXmlNode *cpatch = group->FirstChild("PATCH");
	while(cpatch)
	{
		TiXmlElement *celement = cpatch->ToElement();
		if(!celement)
		{
			SysPrintf("XML Patch Loader: ERROR: Couldn't convert node to element.\n" );
			return -1;
		}


		const char *ptitle=celement->Attribute("title");
		const char *penable=celement->Attribute("enabled");
		const char *applymode=celement->Attribute("applymode");
		const char *place=celement->Attribute("place");
		const char *address=celement->Attribute("address");
		const char *size=celement->Attribute("size");
		const char *value=celement->Attribute("value");

		if(ptitle) {
			SysPrintf("XML Patch Loader: Patch title: %s\n", ptitle);
		}

		bool penabled=gEnabled;
		if(penable)
		{
			if(strcmp(penable,"false")==0)
			{
				SysPrintf("XML Patch Loader: Patch is disabled.\n");
				penabled=false;
			}
		}

		if(!applymode) applymode="frame";
		if(!place) place="EE";
		if(!address) {
			SysPrintf("XML Patch Loader: ERROR: Patch doesn't contain an address.\n");
			return -1;
		}
		if(!value) {
			SysPrintf("XML Patch Loader: ERROR: Patch doesn't contain a value.\n");
			return -1;
		}
		if(!size) {
			SysPrintf("XML Patch Loader: WARNING: Patch doesn't contain the size. Trying to deduce from the value size.\n");
			switch(strlen(value))
			{
				case 8:
				case 7:
				case 6:
				case 5:
					size="32";
					break;
				case 4:
				case 3:
					size="16";
					break;
				case 2:
				case 1:
					size="8";
					break;
				case 0:
					size="0";
					break;
				default:
					size="64";
					break;
			}
		}

		if(strcmp(applymode,"startup")==0)
		{
			patch[patchnumber].placetopatch=0;
		} else
		if(strcmp(applymode,"vsync")==0)
		{
			patch[patchnumber].placetopatch=1;
		} else
		{
			SysPrintf("XML Patch Loader: ERROR: Invalid applymode attribute.\n");
			patchnumber=0;
			return -1;
		}
		
		if(strcmp(place,"EE")==0)
		{
			patch[patchnumber].cpu= CPU_EE;
		} else
		if(strcmp(place,"IOP")==0)
		{
			patch[patchnumber].cpu= CPU_IOP;
		} else
		{
			SysPrintf("XML Patch Loader: ERROR: Invalid place attribute.\n");
			patchnumber=0;
			return -1;
		}

		if(strcmp(size,"64")==0)
		{
			patch[patchnumber].type = DOUBLE_T;
		} else
		if(strcmp(size,"32")==0)
		{
			patch[patchnumber].type = WORD_T;
		} else
		if(strcmp(size,"16")==0)
		{
			patch[patchnumber].type = SHORT_T;
		} else
		if(strcmp(size,"8")==0)
		{
			patch[patchnumber].type = BYTE_T;
		} else
		{
			SysPrintf("XML Patch Loader: ERROR: Invalid size attribute.\n");
			patchnumber=0;
			return -1;
		}

		sscanf( address, "%X", &patch[ patchnumber ].addr );
		sscanf( value, "%I64X", &patch[ patchnumber ].data );

		patch[patchnumber].enabled=penabled?1:0;

		string pt;

		if(ptitle)
			pt.assign(ptitle);
		else
			pt.clear();
		
		Patch p=Patch(patchnumber,gIndex,penabled,pt);
		patches.push_back(p);

		patchnumber++;

		cpatch = cpatch->NextSibling("PATCH");
	}

	cpatch = group->FirstChild("GROUP");
	while(cpatch) {
		int result=LoadGroup(cpatch,gIndex);
		if(result) return result;
		cpatch = cpatch->NextSibling("GROUP");
	}

	return 0;
}
