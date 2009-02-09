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
	
//
// Includes
//
#include "PrecompiledHeader.h"

#include "PsxCommon.h"
#include "Paths.h"
#include "Patch.h"
#include "VU.h"

#ifdef _WIN32
#include "windows/cheats/cheats.h"
#else
#include <stdio.h>
#include <ctype.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4996) //ignore the stricmp deprecated warning
#endif

IniPatch patch[ MAX_PATCH ];

u32 SkipCount=0;
u32 IterationCount=0;
u32 IterationIncrement=0;
u32 ValueIncrement=0;
u32 PrevCheatType=0;
u32 PrevCheataddr = 0;
u32 LastType = 0;

int g_ZeroGSOptions=0;
int patchnumber;

char strgametitle[256]= {0};

//
// Variables
//
PatchTextTable commands[] =
{
	{ "comment", 1, patchFunc_comment },
	{ "gametitle", 2, patchFunc_gametitle },
	{ "patch", 3, patchFunc_patch },
	{ "fastmemory", 4, patchFunc_fastmemory }, // enable for faster but bugger mem (mvc2 is faster)
	{ "roundmode", 5, patchFunc_roundmode }, // changes rounding mode for floating point
											// syntax: roundmode=X,Y
											// possible values for X,Y: NEAR, DOWN, UP, CHOP
											// X - EE rounding mode (default is NEAR)
											// Y - VU rounding mode (default is CHOP)
	{ "zerogs", 6, patchFunc_zerogs }, // zerogs=hex
	{ "path3hack", 7, patchFunc_path3hack },
	{ "vunanmode",8, patchFunc_vunanmode },
	{ "ffxhack",9, patchFunc_ffxhack},
	{ "xkickdelay",10, patchFunc_xkickdelay},
	{ "", 0, NULL }
};

PatchTextTable dataType[] =
{
	{ "byte", 1, NULL },
	{ "short", 2, NULL },
	{ "word", 3, NULL },
	{ "double", 4, NULL },
	{ "extended", 5, NULL },
	{ "", 0, NULL }
};

PatchTextTable cpuCore[] =
{
	{ "EE", 1, NULL },
	{ "IOP", 2, NULL },
	{ "", 0, NULL }
};

//
// Function Implementations
//

int PatchTableExecute( char * text1, char * text2, PatchTextTable * Table )
{
	int i = 0;

	while ( Table[ i ].text[ 0 ] )
	{
		if ( !strcmp( Table[ i ].text, text1 ) )
		{
			if ( Table[ i ].func )
			{
				Table[ i ].func( text1, text2 );
			}
			break;
		}
		i++;
	}

	return Table[ i ].code;
}

void _applypatch(int place, IniPatch *p) {
	u8 u8Val=0;
	u16 u16Val=0;
	u32 u32Val=0;
	u32 i;

	if (p->placetopatch != place) return;
	if (p->enabled == 0) return;
	
	switch (p->cpu) {
		case CPU_EE:
			switch (p->type) {
				case BYTE_T: 
					memWrite8(p->addr, (u8)p->data);
					break;
				case SHORT_T:
					memWrite16(p->addr, (u16)p->data);
					break;
				case WORD_T:
					memWrite32(p->addr, (u32)p->data);
					break;
				case DOUBLE_T:
					memWrite64(p->addr, &p->data);
					break;
				case EXTENDED_T:
						if (SkipCount > 0){
							SkipCount--;
						}
					else switch (PrevCheatType) {
						case 0x3040:                                                          // vvvvvvvv  00000000  Inc
							memRead32(PrevCheataddr,&u32Val);
							memWrite32(PrevCheataddr, u32Val+(p->addr));
							PrevCheatType = 0;
							break;
						case 0x3050:                                                          // vvvvvvvv  00000000  Dec
							memRead32(PrevCheataddr,&u32Val);
							memWrite32(PrevCheataddr, u32Val-(p->addr));
							PrevCheatType = 0;
							break;
						case 0x4000:							  // vvvvvvvv  iiiiiiii  
							for(i=0;i<IterationCount;i++)
								memWrite32((u32)(PrevCheataddr+(i*IterationIncrement)),(u32)(p->addr+((u32)p->data*i)));
							PrevCheatType = 0;
							break;
						case 0x5000:							  // dddddddd  iiiiiiii  
							for(i=0;i<IterationCount;i++){
								memRead8(PrevCheataddr+i,&u8Val);
								memWrite8(((u32)p->data)+i,u8Val);
							}
							PrevCheatType = 0;
							break;
						case 0x6000:							  // 000Xnnnn  iiiiiiii  
							// Get Number of pointers 
							if (IterationCount == 0)
								IterationCount = (u32)p->addr&0x0000FFFF;
		
							// Read first pointer
							LastType = ((u32)p->addr&0x000F0000)/0x10000;
							memRead32(PrevCheataddr,&u32Val);	
							PrevCheataddr = u32Val+(u32)p->data;
							IterationCount--;

							// Check if needed to read another pointer
							if (IterationCount == 0){
								PrevCheatType = 0;
								if (LastType==0x0)
									memWrite8(PrevCheataddr,IterationIncrement&0xFF);
								if (LastType==0x1)
									memWrite16(PrevCheataddr,IterationIncrement&0xFFFF);
								if (LastType==0x2)
									memWrite32(PrevCheataddr,IterationIncrement);
							}
							else
								PrevCheatType = 0x6001;
						case 0x6001:							// 000Xnnnn  iiiiiiii  
							// Read first pointer
							memRead32(PrevCheataddr,&u32Val);	
							PrevCheataddr =u32Val+(u32)p->addr;
							IterationCount--;

							// Check if needed to read another pointer
							if (IterationCount == 0){
								PrevCheatType = 0;
								if (LastType==0x0)
									memWrite8(PrevCheataddr,IterationIncrement&0xFF);
								if (LastType==0x1)
									memWrite16(PrevCheataddr,IterationIncrement&0xFFFF);
								if (LastType==0x2)
									memWrite32(PrevCheataddr,IterationIncrement);
							}
							else
								{
								memRead32(PrevCheataddr,&u32Val);	
								PrevCheataddr =u32Val+(u32)p->data;
								IterationCount--;
								if (IterationCount == 0){
									PrevCheatType = 0;
									if (LastType==0x0)
										memWrite8(PrevCheataddr,IterationIncrement&0xFF);
									if (LastType==0x1)
										memWrite16(PrevCheataddr,IterationIncrement&0xFFFF);
									if (LastType==0x2)
										memWrite32(PrevCheataddr,IterationIncrement);
								}
							}
						default:
							if ((p->addr&0xF0000000) == 0x00000000){ // 0aaaaaaa 0000000vv
								memWrite8(p->addr&0x0FFFFFFF, (u8)p->data&0x000000FF);
								PrevCheatType = 0;
							}
							else if ((p->addr&0xF0000000) == 0x10000000){ // 0aaaaaaa 0000vvvv
								memWrite16(p->addr&0x0FFFFFFF, (u16)p->data&0x0000FFFF);
								PrevCheatType = 0;
							}
							else if ((p->addr&0xF0000000) == 0x20000000){ // 0aaaaaaa vvvvvvvv
								memWrite32(p->addr&0x0FFFFFFF, (u32)p->data);
								PrevCheatType = 0;
							}
							else if ((p->addr&0xFFFF0000) == 0x30000000){ // 300000vv 0aaaaaaa  Inc
								memRead8((u32)p->data,&u8Val);
								memWrite8((u32)p->data, u8Val+(p->addr&0x000000FF));
								PrevCheatType = 0;
							}
							else if ((p->addr&0xFFFF0000) == 0x30100000){ // 301000vv 0aaaaaaa  Dec
								memRead8((u32)p->data,&u8Val);
								memWrite8((u32)p->data, u8Val-(p->addr&0x000000FF));
								PrevCheatType = 0;
							}
							else if ((p->addr&0xFFFF0000) == 0x30200000){  // 3020vvvv 0aaaaaaa Inc
								memRead16((u32)p->data,&u16Val);
								memWrite16((u32)p->data, u16Val+(p->addr&0x0000FFFF));
								PrevCheatType = 0;
							}
							else if ((p->addr&0xFFFF0000) == 0x30300000){  // 3030vvvv 0aaaaaaa Dec
								memRead16((u32)p->data,&u16Val);
								memWrite16((u32)p->data, u16Val-(p->addr&0x0000FFFF));
								PrevCheatType = 0;
							}
							else if ((p->addr&0xFFFF0000) == 0x30400000){  // 30400000 0aaaaaaa Inc	+ Another line
								PrevCheatType= 0x3040;
								PrevCheataddr= (u32)p->data;
							}
							else if ((p->addr&0xFFFF0000) == 0x30500000){	// 30500000 0aaaaaaa Inc	+ Another line
								PrevCheatType= 0x3050;
								PrevCheataddr= (u32)p->data;
							}
							else if ((p->addr&0xF0000000) == 0x40000000){	// 4aaaaaaa nnnnssss + Another line
								IterationCount=((u32)p->data&0xFFFF0000)/0x10000;
								IterationIncrement=((u32)p->data&0x0000FFFF)*4;
								PrevCheataddr=(u32)p->addr&0x0FFFFFFF;
								PrevCheatType= 0x4000;
							}
							else if  ((p->addr&0xF0000000) == 0x50000000){  // 5sssssss nnnnnnnn + Another line
								PrevCheataddr = (u32)p->addr&0x0FFFFFFF;
								IterationCount=((u32)p->data);
								PrevCheatType= 0x5000;
							}
							else if  ((p->addr&0xF0000000) == 0x60000000){ // 6aaaaaaa 000000vv + Another line/s
								PrevCheataddr = (u32)p->addr&0x0FFFFFFF;
								IterationIncrement=((u32)p->data);
								IterationCount=0;
								PrevCheatType= 0x6000;
							}
							else if ((p->addr&0xF0000000) == 0x70000000) {
								if  ((p->data&0x00F00000) == 0x00000000){	 // 7aaaaaaa 000000vv 
									memRead8((u32)p->addr&0x0FFFFFFF,&u8Val);
									memWrite8((u32)p->addr&0x0FFFFFFF,(u8)(u8Val|(p->data&0x000000FF)));
								}else if  ((p->data&0x00F00000) == 0x00100000){ // 7aaaaaaa 0010vvvv
									memRead16((u32)p->addr&0x0FFFFFFF,&u16Val);
									memWrite16((u32)p->addr&0x0FFFFFFF,(u16)(u16Val|(p->data&0x0000FFFF)));
								}else if  ((p->data&0x00F00000) == 0x00200000){ // 7aaaaaaa 002000vv
									memRead8((u32)p->addr&0x0FFFFFFF,&u8Val);
									memWrite8((u32)p->addr&0x0FFFFFFF,(u8)(u8Val&(p->data&0x000000FF)));
								}else if  ((p->data&0x00F00000) == 0x00300000){ // 7aaaaaaa 0030vvvv
									memRead16((u32)p->addr&0x0FFFFFFF,&u16Val);
									memWrite16((u32)p->addr&0x0FFFFFFF,(u16)(u16Val&(p->data&0x0000FFFF)));
								}else if  ((p->data&0x00F00000) == 0x00400000){ // 7aaaaaaa 004000vv
									memRead8((u32)p->addr&0x0FFFFFFF,&u8Val);
									memWrite8((u32)p->addr&0x0FFFFFFF,(u8)(u8Val^(p->data&0x000000FF)));
								}else if  ((p->data&0x00F00000) == 0x00500000){ // 7aaaaaaa 0050vvvv
									memRead16((u32)p->addr&0x0FFFFFFF,&u16Val);
									memWrite16((u32)p->addr&0x0FFFFFFF,(u16)(u16Val^(p->data&0x0000FFFF)));
								}
							}
							else if (p->addr < 0xE0000000) {
								if ((((u32)p->data & 0xFFFF0000)==0x00000000) ||
								     (((u32)p->data & 0xFFFF0000)==0x00100000) ||
								     (((u32)p->data & 0xFFFF0000)==0x00200000) ||
								     (((u32)p->data & 0xFFFF0000)==0x00300000)) {
									memRead16((u32)p->addr&0x0FFFFFFF,&u16Val);
									if (u16Val != (0x0000FFFF&(u32)p->data))
										SkipCount = 1;
									PrevCheatType= 0;
								}
							}
							else if (p->addr < 0xF0000000) {
								if ((((u32)p->data&0xF0000000)==0x00000000) ||
								     (((u32)p->data&0xF0000000)==0x10000000) ||
								     (((u32)p->data&0xF0000000)==0x20000000) ||
								     (((u32)p->data&0xF0000000)==0x30000000)) {
										memRead16((u32)p->data&0x0FFFFFFF,&u16Val);
										if (u16Val != (0x0000FFFF&(u32)p->addr))
											SkipCount = ((u32)p->addr&0xFFF0000)/0x10000;
										PrevCheatType= 0;
									}
				}
			}
		}
		break;
	case CPU_IOP: { 
		switch (p->type) {
			case BYTE_T:
				psxMemWrite8(p->addr, (u8)p->data);
				break;
			case SHORT_T:
				psxMemWrite16(p->addr, (u16)p->data);
				break;
			case WORD_T:
				psxMemWrite32(p->addr, (u32)p->data);
				break;
		}
	}
	}
}


//this is for apply patches directly to memory
void applypatch(int place) {
	int i;

	if (place == 0) {
		Console::WriteLn(" patchnumber: %d", params patchnumber);
	}

	for ( i = 0; i < patchnumber; i++ ) {
		_applypatch(place, &patch[i]);
	}
}

void patchFunc_comment( char * text1, char * text2 )
{
	Console::WriteLn( "comment: %s", params text2 );
}


void patchFunc_gametitle( char * text1, char * text2 )
{
	Console::WriteLn( "gametitle: %s", params text2 );
	sprintf(strgametitle,"%s",text2);
	Console::SetTitle(strgametitle);
}

void patchFunc_patch( char * cmd, char * param )
{
	char * pText;

	if ( patchnumber >= MAX_PATCH )
	{
		Console::Error( "Patch ERROR: Maximum number of patches reached: %s=%s", params cmd, param );
		return;
	}

	pText = strtok( param, "," );
	pText = param;

	patch[ patchnumber ].placetopatch = strtol( pText, (char **)NULL, 0 );

	pText = strtok( NULL, "," );
	inifile_trim( pText );
	patch[ patchnumber ].cpu = (patch_cpu_type)PatchTableExecute( pText, NULL, cpuCore );
	if ( patch[ patchnumber ].cpu == 0 ) 
	{
		Console::Error( "Unrecognized patch '%s'", params pText );
		return;
	}

	pText = strtok( NULL, "," );
	inifile_trim( pText );
	sscanf( pText, "%X", &patch[ patchnumber ].addr );

	pText = strtok( NULL, "," );
	inifile_trim( pText );
	patch[ patchnumber ].type = (patch_data_type)PatchTableExecute( pText, NULL, dataType );
	if ( patch[ patchnumber ].type == 0 ) 
	{
		Console::Error( "Unrecognized patch '%s'", params pText );
		return;
	}
	
	pText = strtok( NULL, "," );
	inifile_trim( pText );
	sscanf( pText, "%I64X", &patch[ patchnumber ].data );

	patch[ patchnumber ].enabled = 1;

	patchnumber++;
}

//this routine is for executing the commands of the ini file
void inifile_command( char * cmd )
{
	int code;
	char command[ 256 ];
	char parameter[ 256 ];

	// extract param part (after '=')
	char * pEqual = strchr( cmd, '=' );

	if ( ! pEqual )
	{
		// fastmemory doesn't have =
		pEqual = cmd+strlen(cmd);
	}

	memzero_obj( command );
	memzero_obj( parameter );
		
	strncpy( command, cmd, pEqual - cmd );
	strncpy( parameter, pEqual + 1, sizeof( parameter ) );

	inifile_trim( command );
	inifile_trim( parameter );

	code = PatchTableExecute( command, parameter, commands );
}

void inifile_trim( char * buffer )
{
	char * pInit = buffer;
	char * pEnd = NULL;

	while ( ( *pInit == ' ' ) || ( *pInit == '\t' ) ) //skip space
	{
		pInit++;
	}
	if ( ( pInit[ 0 ] == '/' ) && ( pInit[ 1 ] == '/' ) ) //remove comment
	{
		buffer[ 0 ] = '\0';
		return;
	}
	pEnd = pInit + strlen( pInit ) - 1;
	if ( pEnd <= pInit )
	{
		buffer[ 0 ] = '\0';
		return;
	}
	while ( ( *pEnd == '\r' ) || ( *pEnd == '\n' ) ||
			  ( *pEnd == ' ' ) || ( *pEnd == '\t' ) )
	{
		pEnd--;
	}
	if ( pEnd <= pInit )
	{
		buffer[ 0 ] = '\0';
		return;
	}
	memmove( buffer, pInit, pEnd - pInit + 1 );
	buffer[ pEnd - pInit + 1 ] = '\0';
}

void inisection_process( FILE * f1 )
{
	char buffer[ 1024 ];
	while( fgets( buffer, sizeof( buffer ), f1 ) )
	{
		inifile_trim( buffer );
		if ( buffer[ 0 ] )
		{
			inifile_command( buffer );
		}
	}
}

//this routine is for reading the ini file

void inifile_read( const char * name )
{
	FILE * f1;
	char buffer[ 1024 ];

	patchnumber = 0;
#ifdef _WIN32
	sprintf( buffer, PATCHES_DIR "\\%s.pnach", name );
#else
	sprintf( buffer, PATCHES_DIR "/%s.pnach", name );
#endif

	f1 = fopen( buffer, "rt" );

#ifndef _WIN32
	if( !f1 ) {
		 // try all upper case because linux is case sensitive
		 char* pstart = buffer+8;
		 char* pend = buffer+strlen(buffer);
		 while(pstart != pend ) {
			  // stop at the first . since we only want to update the hex
			  if( *pstart == '.' )
					break;
			  *pstart++ = toupper(*pstart);
		 }

		 f1 = fopen(buffer, "rt");
	}
#endif

	if( !f1 )
	{
		Console::WriteLn( _( "No patch found.Resuming execution without a patch (this is NOT an error)." ));
		return;
	}

	inisection_process( f1 );

	fclose( f1 );
}

void resetpatch( void )
{
	patchnumber = 0;
}

int AddPatch(int Mode, int Place, int Address, int Size, u64 data)
{

	if ( patchnumber >= MAX_PATCH )
	{
		Console::Error( "Patch ERROR: Maximum number of patches reached.");
		return -1;
	}

	patch[patchnumber].placetopatch = Mode;
	
	patch[patchnumber].cpu = (patch_cpu_type)Place;
	patch[patchnumber].addr=Address;
	patch[patchnumber].type=(patch_data_type)Size;
	patch[patchnumber].data = data;
	return patchnumber++;
}
	
void patchFunc_ffxhack( char * cmd, char * param )
{
	 g_FFXHack = 1;
}

void patchFunc_xkickdelay( char * cmd, char * param )
{
	g_VUGameFixes |= VUFIX_XGKICKDELAY2;
}

void patchFunc_fastmemory( char * cmd, char * param )
{
	// only valid for recompilers
	SetFastMemory(1);
}


void patchFunc_vunanmode( char * cmd, char * param )
{
	// only valid for recompilers
	SetVUNanMode(param != NULL ? atoi(param) : 1);
}

void patchFunc_path3hack( char * cmd, char * param )
{
	path3hack = 1;
}

void patchFunc_roundmode( char * cmd, char * param )
{
	int index;
	char * pText;

	u32 eetype = (Config.sseMXCSR & 0x6000);
	u32 vutype = (Config.sseVUMXCSR & 0x6000);
	
	index = 0;
	pText = strtok( param, ", " );
	while(pText != NULL) {
		u32 type = 0xffff;
		
		if( stricmp(pText, "near") == 0 ) {
			type = 0x0000;
		}
		else if( stricmp(pText, "down") == 0 ) {
			type = 0x2000;
		}
		else if( stricmp(pText, "up") == 0 ) {
			type = 0x4000;
		}
		else if( stricmp(pText, "chop") == 0 ) {
			type = 0x6000;
		}

		if( type == 0xffff ) {
			printf("bad argument (%s) to round mode! skipping...\n", pText);
			break;
		}

		if( index == 0 ) 
			eetype=type;
		else			 
			vutype=type;

		if( index == 1 )
			break;

		index++;
		pText = strtok(NULL, ", ");
	}

	SetRoundMode(eetype,vutype);
}

void patchFunc_zerogs(char* cmd, char* param)
{
	 sscanf(param, "%x", &g_ZeroGSOptions);
}

void SetRoundMode(u32 ee, u32 vu)
{
	// don't set a state for interpreter only
	//Msgbox::Alert("SetRoundMode: Config.sseMXCSR = %x; Config.sseVUMXCSR = %x \n", Config.sseMXCSR, Config.sseVUMXCSR);

	SetCPUState( (Config.sseMXCSR & 0x9fff) | ee, (Config.sseVUMXCSR & 0x9fff) | vu);
}
