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
#include "Win32.h"

#include <commctrl.h>
#include <math.h>
#include "libintlmsc.h"

#include "System.h"
#include "McdsDlg.h"

#include <vector>
using namespace std;


u16 SJISTable[0xFFFF];


void IniSJISTable()
{
	memzero_obj(SJISTable);
	//Blow me sony for using this retarded sjis to store the savegame name
	SJISTable[0x20]	= 0x0020;
	SJISTable[0x21]	= 0x0021;
	SJISTable[0x22]	= 0x0022;
	SJISTable[0x23]	= 0x0023;
	SJISTable[0x24]	= 0x0024;
	SJISTable[0x25]	= 0x0025;
	SJISTable[0x26]	= 0x0026;
	SJISTable[0x27]	= 0x0027;
	SJISTable[0x28]	= 0x0028;
	SJISTable[0x29]	= 0x0029;
	SJISTable[0x2A]	= 0x002A;
	SJISTable[0x2B]	= 0x002B;
	SJISTable[0x2C]	= 0x002C;
	SJISTable[0x2D]	= 0x002D;
	SJISTable[0x2E]	= 0x002E;
	SJISTable[0x2F]	= 0x002F;
	SJISTable[0x30]	= 0x0030;
	SJISTable[0x31]	= 0x0031;
	SJISTable[0x32]	= 0x0032;
	SJISTable[0x33]	= 0x0033;
	SJISTable[0x34]	= 0x0034;
	SJISTable[0x35]	= 0x0035;
	SJISTable[0x36]	= 0x0036;
	SJISTable[0x37]	= 0x0037;
	SJISTable[0x38]	= 0x0038;
	SJISTable[0x39]	= 0x0039;
	SJISTable[0x3A]	= 0x003A;
	SJISTable[0x3B]	= 0x003B;
	SJISTable[0x3C]	= 0x003C;
	SJISTable[0x3D]	= 0x003D;
	SJISTable[0x3E]	= 0x003E;
	SJISTable[0x3F]	= 0x003F;
	SJISTable[0x40]	= 0x0040;
	SJISTable[0x41]	= 0x0041;
	SJISTable[0x42]	= 0x0042;
	SJISTable[0x43]	= 0x0043;
	SJISTable[0x44]	= 0x0044;
	SJISTable[0x45]	= 0x0045;
	SJISTable[0x46]	= 0x0046;
	SJISTable[0x47]	= 0x0047;
	SJISTable[0x48]	= 0x0048;
	SJISTable[0x49]	= 0x0049;
	SJISTable[0x4A]	= 0x004A;
	SJISTable[0x4B]	= 0x004B;
	SJISTable[0x4C]	= 0x004C;
	SJISTable[0x4D]	= 0x004D;
	SJISTable[0x4E]	= 0x004E;
	SJISTable[0x4F]	= 0x004F;
	SJISTable[0x50]	= 0x0050;
	SJISTable[0x51]	= 0x0051;
	SJISTable[0x52]	= 0x0052;
	SJISTable[0x53]	= 0x0053;
	SJISTable[0x54]	= 0x0054;
	SJISTable[0x55]	= 0x0055;
	SJISTable[0x56]	= 0x0056;
	SJISTable[0x57]	= 0x0057;
	SJISTable[0x58]	= 0x0058;
	SJISTable[0x59]	= 0x0059;
	SJISTable[0x5A]	= 0x005A;
	SJISTable[0x5B]	= 0x005B;
	SJISTable[0x5C]	= 0x00A5;
	SJISTable[0x5D]	= 0x005D;
	SJISTable[0x5E]	= 0x005E;
	SJISTable[0x5F]	= 0x005F;
	SJISTable[0x60]	= 0x0060;
	SJISTable[0x61]	= 0x0061;
	SJISTable[0x62]	= 0x0062;
	SJISTable[0x63]	= 0x0063;
	SJISTable[0x64]	= 0x0064;
	SJISTable[0x65]	= 0x0065;
	SJISTable[0x66]	= 0x0066;
	SJISTable[0x67]	= 0x0067;
	SJISTable[0x68]	= 0x0068;
	SJISTable[0x69]	= 0x0069;
	SJISTable[0x6A]	= 0x006A;
	SJISTable[0x6B]	= 0x006B;
	SJISTable[0x6C]	= 0x006C;
	SJISTable[0x6D]	= 0x006D;
	SJISTable[0x6E]	= 0x006E;
	SJISTable[0x6F]	= 0x006F;
	SJISTable[0x70]	= 0x0070;
	SJISTable[0x71]	= 0x0071;
	SJISTable[0x72]	= 0x0072;
	SJISTable[0x73]	= 0x0073;
	SJISTable[0x74]	= 0x0074;
	SJISTable[0x75]	= 0x0075;
	SJISTable[0x76]	= 0x0076;
	SJISTable[0x77]	= 0x0077;
	SJISTable[0x78]	= 0x0078;
	SJISTable[0x79]	= 0x0079;
	SJISTable[0x7A]	= 0x007A;
	SJISTable[0x7B]	= 0x007B;
	SJISTable[0x7C]	= 0x007C;
	SJISTable[0x7D]	= 0x007D;
	SJISTable[0x7E]	= 0x007E;
	
	SJISTable[0x8140]	= 0x0020;
	SJISTable[0x8149]	= 0x0021;
	SJISTable[0x22]	= 0x0022;
	SJISTable[0x8194]	= 0x0023;
	SJISTable[0x8190]	= 0x0024;
	SJISTable[0x8193]	= 0x0025;
	SJISTable[0x8195]	= 0x0026;
	SJISTable[0x27]	= 0x0027;
	SJISTable[0x8169]	= 0x0028;
	SJISTable[0x816A]	= 0x0029;
	SJISTable[0x8196]	= 0x002A;
	SJISTable[0x817B]	= 0x002B;
	SJISTable[0x8143]	= 0x002C;
	SJISTable[0x815D]	= 0x002D;
	SJISTable[0x8144]	= 0x002E;
	SJISTable[0x815E]	= 0x002F;
	SJISTable[0x824F]	= 0x0030;
	SJISTable[0x8250]	= 0x0031;
	SJISTable[0x8251]	= 0x0032;
	SJISTable[0x8252]	= 0x0033;
	SJISTable[0x8253]	= 0x0034;
	SJISTable[0x8254]	= 0x0035;
	SJISTable[0x8255]	= 0x0036;
	SJISTable[0x8256]	= 0x0037;
	SJISTable[0x8257]	= 0x0038;
	SJISTable[0x8258]	= 0x0039;
	SJISTable[0x8146]	= 0x003A;
	SJISTable[0x8147]	= 0x003B;
	SJISTable[0x8183]	= 0x003C;
	SJISTable[0x8181]	= 0x003D;
	SJISTable[0x8184]	= 0x003E;
	SJISTable[0x8148]	= 0x003F;
	SJISTable[0x8197]	= 0x0040;
	SJISTable[0x8260]	= 0x0041;
	SJISTable[0x8261]	= 0x0042;
	SJISTable[0x8262]	= 0x0043;
	SJISTable[0x8263]	= 0x0044;
	SJISTable[0x8264]	= 0x0045;
	SJISTable[0x8265]	= 0x0046;
	SJISTable[0x8266]	= 0x0047;
	SJISTable[0x8267]	= 0x0048;
	SJISTable[0x8268]	= 0x0049;
	SJISTable[0x8269]	= 0x004A;
	SJISTable[0x826A]	= 0x004B;
	SJISTable[0x826B]	= 0x004C;
	SJISTable[0x826C]	= 0x004D;
	SJISTable[0x826D]	= 0x004E;
	SJISTable[0x826E]	= 0x004F;
	SJISTable[0x826F]	= 0x0050;
	SJISTable[0x8270]	= 0x0051;
	SJISTable[0x8271]	= 0x0052;
	SJISTable[0x8272]	= 0x0053;
	SJISTable[0x8273]	= 0x0054;
	SJISTable[0x8274]	= 0x0055;
	SJISTable[0x8275]	= 0x0056;
	SJISTable[0x8276]	= 0x0057;
	SJISTable[0x8277]	= 0x0058;
	SJISTable[0x8278]	= 0x0059;
	SJISTable[0x8279]	= 0x005A;
	SJISTable[0x816D]	= 0x005B;
	SJISTable[0x818F]	= 0x00A5;
	SJISTable[0x816E]	= 0x005D;
	SJISTable[0x814F]	= 0x005E;
	SJISTable[0x8151]	= 0x005F;
	SJISTable[0x814D]	= 0x0060;
	SJISTable[0x8281]	= 0x0061;
	SJISTable[0x8282]	= 0x0062;
	SJISTable[0x8283]	= 0x0063;
	SJISTable[0x8284]	= 0x0064;
	SJISTable[0x8285]	= 0x0065;
	SJISTable[0x8286]	= 0x0066;
	SJISTable[0x8287]	= 0x0067;
	SJISTable[0x8288]	= 0x0068;
	SJISTable[0x8289]	= 0x0069;
	SJISTable[0x828A]	= 0x006A;
	SJISTable[0x828B]	= 0x006B;
	SJISTable[0x828C]	= 0x006C;
	SJISTable[0x828D]	= 0x006D;
	SJISTable[0x828E]	= 0x006E;
	SJISTable[0x828F]	= 0x006F;
	SJISTable[0x8290]	= 0x0070;
	SJISTable[0x8291]	= 0x0071;
	SJISTable[0x8292]	= 0x0072;
	SJISTable[0x8293]	= 0x0073;
	SJISTable[0x8294]	= 0x0074;
	SJISTable[0x8295]	= 0x0075;
	SJISTable[0x8296]	= 0x0076;
	SJISTable[0x8297]	= 0x0077;
	SJISTable[0x8298]	= 0x0078;
	SJISTable[0x8299]	= 0x0079;
	SJISTable[0x829A]	= 0x007A;
	SJISTable[0x816F]	= 0x007B;
	SJISTable[0x8162]	= 0x007C;
	SJISTable[0x8170]	= 0x007D;
	SJISTable[0x7E]	= 0x007E;
}


HWND mcdDlg;
HTREEITEM root;
HWND treewindow;

HTREEITEM AddTreeItem(HWND treeview, HTREEITEM parent, const char *name, LPARAM lp)
{
	TVINSERTSTRUCT node={
		parent,
		0,
		{
			TVIF_TEXT,
			NULL,
			TVIS_EXPANDED,
			TVIS_EXPANDED,
			(LPSTR)name,
			0,
			0,
			0,
			0,
			lp,
			1
		}
	};
	return TreeView_InsertItem(treeview,&node);
}

void ReadDir(int cluster, int rec, HTREEITEM tree);


class Time
{
	public:
		u8 sec;
		u8 min;
		u8 hour;
		u8 day;
		u8 month;
		u16 year;
};


class Dir
{
	protected:
		

	public:
		static const u16 DF_READ      = 0x0001;
		static const u16 DF_WRITE     = 0x0002;
		static const u16 DF_EXECUTE   = 0x0004;
		static const u16 DF_PROTECTED = 0x0008;
		static const u16 DF_FILE      = 0x0010;
		static const u16 DF_DIRECTORY = 0x0020;
		static const u16 DF_HIDDEN    = 0x2000;
		static const u16 DF_EXISTS    = 0x8000;

		// Directory Attributes
		u16 Mode;
		s32 Lenght;
		Time Created;
		s32 Cluster;
		u32 DirEntry;
		Time Modified;
		u32 Attribute;
		s8 Name[256];  // just in case some names are longer

		// Parent dir and contents of dir
		Dir *Parent;
		vector<Dir> Sons;

		// Used to store the contents of a file
		u8 *File;

		Dir()
		{
			File = 0;
		}

		bool IsHidden()
		{
			return (Mode & DF_HIDDEN) ? 1 : 0;
		}

		bool IsFile()
		{
			return (Mode & DF_FILE) ? 1 : 0;
		}

		bool IsDirectory()
		{
			return (Mode & DF_DIRECTORY) ? 1 : 0;
		}

		bool Exists()
		{
			return (Mode & DF_EXISTS) ? 1 : 0;
		}

		void Load(s8 *d)
		{
			File = 0;
			// For the moment we dont need to load more data
			Mode = *(u16 *)d;
			Lenght = *(u32 *)(d + 0x04);
			Cluster = *(u32 *)(d + 0x10);
			strncpy(Name, d + 0x40, 255);
		}

		Dir* AddSon(Dir &d)
		{
			d.Parent = this;
			vector<Dir>::iterator i = Sons.insert(Sons.end(), d);
			return &i[0];
		}

		void Release()
		{
			for(unsigned int i=0;i<Sons.size();i++)
			{
				Sons[i].Release();
			}

			Sons.clear();

			if(File != 0)
				free(File);
		}

		u32 BuildICO(char *dest)
		{
			if(IsDirectory() || !Exists())
				return 0;
			
			u32 FileID = *(u32 *)File;
			u32 AnimationShapes = *(u32 *)(File + 4);
			u32 TextureType = *(u32 *)(File + 8);
			//u32 ConstantCheck = *(u32 *)(d->File + 12);
			u32 NumVertex = *(u32 *)(File + 16);

			// Check if it's an ico file
			if(FileID != 0x00010000)
			{
				MessageBox(mcdDlg, "It's not an ICO file.", "Error", 0);
				return 0;
			}

			// Is texture compressed?
			if(TextureType != 0x07)
			{
				//MessageBox(mcdDlg, "Compressed texture, not yet supported.", "Error", 0);
				//return;
			}

			// Calculate the offset to the animation segment
			u32 VertexSize = (AnimationShapes * 8) + 8 + 8;
			u32 AnimationSegmentOffset = (VertexSize * NumVertex) + 20;

			// Check if we really are at the animation header
			u32 AnimationID = *(u32 *)(File + AnimationSegmentOffset);
			if(AnimationID != 0x00000001)
			{
				MessageBox(mcdDlg, "Animation header ID is incorrect.", "Error", 0);
				return 0;
			}

			// Get the size of all the animation segment
			u32 FrameLenght = *(u32 *)(File + AnimationSegmentOffset + 4);
			u32 NumberOfFrames = *(u32 *)(File + AnimationSegmentOffset + 16);

			u32 Offset = AnimationSegmentOffset + 20;
			for(u32 i=0;i<NumberOfFrames;i++)
			{
				u32 KeyNum = *(u32 *)(File + Offset + 4);
				Offset += 16 + (KeyNum * 8) - 8; // The -8 is there because the doc with the ico format spec is either wrong or I'm stupid
			}


			int z, y=0;
			u16 ico[0x4000];
			//u8 cico[128*128*3];

			if(TextureType > 0x07)
			{
				u32 CompressedDataSize = *(u32 *)(File + Offset);
				//RLE compression
				
				Offset += 4;
				u32 OffsetEnd = Offset + CompressedDataSize;
				u32 WriteCount = 0;

				while(WriteCount < 0x8000 && Offset < OffsetEnd)
				{

					u16 Data = *(u16 *)(File + Offset);

					if(Data < 0xFF00) //Replication
					{
						Offset += 2;
						u16 Rep = *(u16 *)(File + Offset);
						
						for(int i=0;i<Data;i++)
						{
							ico[WriteCount] = Rep;
							WriteCount++;
						}
						Offset += 2;
					}
					else	// Lenght
					{
						u16 Lenght = 0xFFFF - Data;
						Offset += 2;
						Lenght++; // doh! This shit made me debug for hours
						for(int i=0;i<Lenght;i++)
						{
							ico[WriteCount] = *(u16 *)(File + Offset);
							WriteCount++;
							Offset += 2;
						}
					}

				}

			}
			else
			{
				// Get the texture from the file
				memcpy(ico, &File[/*d->Lenght-0x8000*/Offset], 0x8000);
			}

			// Convert from TIM to rgb
			for(z=0;z<0x4000;z++)
			{
				dest[y] = 8 * (ico[z] & 0x1F);
				dest[y+1] = 8 * ((ico[z] >>5) & 0x1F);
				dest[y+2] = 8 * (ico[z] >>10);
				y += 3;
			}

			return 1;
		}

};


class SaveGame
{
	public:

		Dir *D;
		char Name1[256];
		char Name2[256];
		
		u8 Icon[128*128*3];
		

		SaveGame(Dir *_Dir)
		{
			char IconName[256];

			D = _Dir;

			// Find icon.sys
			for(unsigned int i=0;i<D->Sons.size();i++)
			{
				if(!strcmp("icon.sys", D->Sons[i].Name))
				{
					char temp[256];

					// Get Name in SJIS format
					u16 *p = (u16 *)(D->Sons[i].File + 192);

					while(*p != 0x0000)
					{
						// Switch bytes
						u8 *pp = (u8 *)p;
						u8 tb = *pp;
						*pp = pp[1];
						pp[1] = tb;

						// Translate char
						*p = SJISTable[*p];
						
						p++;
					}
					
					// Convert unicode string
					wcstombs(temp, (wchar_t *)(D->Sons[i].File + 192), 255);

					// Get the second line separator
					u16 SecondLine = *(u16 *)(D->Sons[i].File + 6);
					SecondLine /= 2;

					// Copy the 2 parts of the string to different strings
					strncpy(Name1, temp, SecondLine);
					Name1[SecondLine] = 0;
					
					strcpy(Name2, &temp[SecondLine]);
					if(strlen(Name2) + SecondLine > 68)
						Name2[0] = 0;

					// Get the icon file name
					strcpy(IconName, (char *)D->Sons[i].File + 260);

				}
			}

			// Find the icon now
			for(unsigned int i=0;i<D->Sons.size();i++)
			{
				if(!strcmp(IconName, D->Sons[i].Name))
				{
					D->Sons[i].BuildICO((char*)Icon);
				}

			}
		}

		void AddIcoToImageList(HIMAGELIST il)
		{

			HDC hDC = GetDC(NULL);
			HDC memDC = CreateCompatibleDC (hDC);
			HBITMAP memBM = CreateCompatibleBitmap ( hDC, 128, 128 );
			SelectObject ( memDC, memBM );

			int px, py, pc=0;
			for(px=0;px<128;px++){
				for(py=0;py<128;py++){
					SetPixel(memDC, py, px, RGB(Icon[pc], Icon[pc+1], Icon[pc+2]));
					pc+=3;
				}
			}

			HDC memDC2 = CreateCompatibleDC (hDC);
			HBITMAP memBM2 = CreateCompatibleBitmap ( hDC, 64, 64 );
			SelectObject ( memDC2, memBM2 );
			SetStretchBltMode(memDC2, HALFTONE);
			StretchBlt(memDC2, 0, 0, 64, 64, memDC, 0, 0, 128, 128, SRCCOPY);

			DeleteDC(memDC2);
			DeleteDC(memDC);
			ImageList_Add(il, memBM2, NULL);
			DeleteObject(memBM);
			DeleteObject(memBM2);
			ReleaseDC(NULL, hDC);


		}

		void DrawICO(HWND hWnd)
		{
	//		int px, py, pc=0;
			HDC dc = GetDC(hWnd);
	/*		
			for(px=0;px<128;px++){
				for(py=0;py<128;py++){
					SetPixel(dc, py, px, RGB(Icon[pc], Icon[pc+1], Icon[pc+2]));
					pc+=3;
					
				}
			}
*/
			/*
			HDC hDC = GetDC(NULL);
			HDC memDC = CreateCompatibleDC ( hDC );
			HBITMAP memBM = CreateCompatibleBitmap ( hDC, 256, 128 );
			SelectObject ( memDC, memBM );
			for(px=0;px<128;px++){
				for(py=0;py<128;py++){
					SetPixel(memDC, py, px, RGB(Icon[pc], Icon[pc+1], Icon[pc+2]));
					pc+=3;
					
				}
			}
			RECT rect;
			rect.left=132;
			rect.right=255;
			rect.top=4;
			rect.bottom=255;
			DrawText(memDC, Name1, strlen(Name1), &rect, 0);
			rect.top=20;
			DrawText(memDC, Name2, strlen(Name2), &rect, 0);


			SetStretchBltMode(dc, HALFTONE);
			StretchBlt(dc, 0, 0, 64, 64, memDC, 0, 0, 128, 128, SRCCOPY);
			//BitBlt(dc, 0, 0, 256, 128, memDC, 0, 0, SRCCOPY);
			*/
		}

};


class MemoryCard
{
	public:
		FILE *fp;
		int FAT[256*256];
		int indirect_table[256];
		char FileName[1024];
		Dir Root;
		vector<SaveGame> SaveGameList;
		HIMAGELIST ImageList;
		
		MemoryCard()
		{
			fp = 0;
			ImageList = 0;
		}

		int BuildFAT()
		{
			int IFC;
			 // indirect table containing the clusters with FAT data
			int i, o;

			// First read IFC from the superblock (8MB cards only have 1)
			fseek(fp, 0x50, SEEK_SET);
			fread(&IFC, 4, 1, fp);

			if(IFC != 8)
			{
				MessageBox(mcdDlg, "IFC is not 8. Memory Card is probably corrupt.", "Error", 0);
				return 0;
			}

			DbgCon::WriteLn("IFC: %d", params IFC);

			// Read the cluster with the indirect data to the FAT.
			fseek(fp, 0x420 * IFC, SEEK_SET);
			fread(indirect_table, 4, 128, fp);
			fseek(fp, (0x420 * IFC) + 0x210, SEEK_SET);
			fread(&indirect_table[128], 4, 128, fp);

			// Build the FAT table from the indirect_table
			o = 0;
			i = 0;

			while(indirect_table[i] != 0xFFFFFFFF)
			{

				fseek(fp, 0x420 * indirect_table[i], SEEK_SET);
				fread(&FAT[o], 4, 128, fp);
				o+=128;
				
				fseek(fp, (0x420 * indirect_table[i]) + 0x210, SEEK_SET);
				fread(&FAT[o], 4, 128, fp);
				o+=128;

				i++;
			}
			return 1;
		}

		void ReadFile(Dir *d)
		{

			int cluster = d->Cluster;
			int size = d->Lenght;
			int numclusters = (int)ceil((double)size / 1024);
			int numpages = (int)ceil((double)size / 512);
			int i=0, j=0, c=0, read=0;
			d->File = (u8 *) malloc(size);


			for(i=0;i<numclusters;i++)
			{
				read = size - c;
				if(read > 512)
					read = 512;

				fseek(fp, 0xA920 + ((cluster) * 0x420), SEEK_SET);
				fread(&d->File[c], 1, read, fp);
				c+=read;
				j++;
				if(j == numpages)
					break;

				read = size - c;
				if(read > 512)
					read = 512;

				fseek(fp, 0xA920 + (((cluster) * 0x420)+528), SEEK_SET);
				fread(&d->File[c], 1, read, fp);
				c+=read;
				j++;
				if(j == numpages)
					break;

				cluster = FAT[cluster];
				cluster = cluster ^ 0x80000000;
				if(cluster == 0x7FFFFFFF || cluster == 0xFFFFFFFF)
					break;
			}

		}

		void Read(u32 cluster, Dir *d)
		{
			int i;
			s8 file1[512], file2[512];
			Dir D1, D2;

			for(i = 0; i < 0xffff; i++)
			{
				
				// Read first page containing the first dir
				fseek(fp, 0xA920 + ((cluster) * 0x420), SEEK_SET);
				fread(file1, 1, 512, fp);
				// Initialize the temporal dir
				D1.Load(file1);
				if(D1.Exists() && D1.Lenght >= 0 && D1.Cluster >= 0)
				{
					// If it exists add the dir to current dir
					Dir *td = d->AddSon(D1);

					// If it's a directory and not . or .. recursive call
					if(D1.IsDirectory() && D1.Name[0]!= '.' )
					{
						Read(td->Cluster, td);
					}

					// If it's a file load the file
					if(D1.IsFile())
					{
						ReadFile(td);
					}
				}
				
				
				// Read second page with second dir or empty
				fseek(fp, 0xA920 + (((cluster) * 0x420) + 528), SEEK_SET);
				fread(file2, 1, 512, fp);
				// Initialize the temporal dir and add it to current dir
				D2.Load(file2);
				if(D2.Exists()  && D2.Lenght >= 0 && D2.Cluster >= 0)
				{
					Dir *td = d->AddSon(D2);

					if(D2.IsDirectory() && D2.Name[0]!= '.')
					{
						Read(td->Cluster, td);
					}

					// If it's a file load the file
					if(D2.IsFile() && D2.Name[0]!= '.')
					{
						ReadFile(td);
					}
				}
				
				// Get next cluster from the FAT table
				cluster = FAT[cluster];
				cluster = cluster ^ 0x80000000;
				if(cluster == 0x7FFFFFFF || cluster == 0xFFFFFFFF)
					break;
			}

		}

		void Load(char *filename)
		{
			strcpy(FileName, filename);

			// Clear previous data
			if(fp != 0)
			{
				fclose(fp);
			}
			memzero_obj(FAT);
			Root.Release();

			SaveGameList.clear();
			ImageList_Destroy(ImageList);

			// Open memory card file
			fp = fopen(FileName, "rb");

			// Check if we opened the file
			if(fp == NULL)
			{
				MessageBox(mcdDlg, "Unable to open memory card file.", "Error", 0);
				return;
			}

			// Build the FAT table for the card
			if(BuildFAT() == 1)
			{
				// Read the root dir
				Read(0, &Root);
			}

			fclose(fp);

			ReadSaveGames();
		}
		
		void AddDirToTreeView(HWND hWnd, HTREEITEM tree, Dir *d)
		{
			for(unsigned int i=0;i<d->Sons.size();i++)
			{
				HTREEITEM temptree = AddTreeItem(hWnd, tree, d->Sons[i].Name, 0);
				if(d->Sons[i].IsDirectory())
				{
					TreeView_SetItemState(hWnd, temptree, TVIS_EXPANDED|TVIS_BOLD, 0x00F0);
					AddDirToTreeView(hWnd, temptree, &d->Sons[i]);
				}
			}
		}

		void AddToListView(HWND hWnd)
		{
			ListView_SetIconSpacing(hWnd, 128, 128);
			ListView_DeleteAllItems(hWnd);
			ListView_SetImageList(hWnd, ImageList, LVSIL_NORMAL);

			LVITEM item;
			char temp[2560];
			item.mask = LVIF_IMAGE | LVIF_TEXT;
			item.iItem = 0;
			item.iSubItem = 0;
			item.state = 0;
			item.stateMask = 0;
			item.pszText = temp;
			item.cchTextMax = 255;
			item.iImage = 1;
			item.iIndent = 0;
			item.lParam = 0;

			for(unsigned int i=0;i<SaveGameList.size();i++)
			{
				strcpy(item.pszText, SaveGameList[i].Name1);
				if(SaveGameList[i].Name2[0] != 0)
				{
					strcat(item.pszText, "\n");
					strcat(item.pszText, SaveGameList[i].Name2);
				}

				item.iImage = i;
				ListView_InsertItem(hWnd, &item);
			}
		}

		void AddCardToTreeView(HWND hWnd)
		{
			TreeView_DeleteAllItems(hWnd);
			AddDirToTreeView(hWnd, 0, &Root);

		}

		// In theory all the games use a folder and then put files inside it, so for the moment this will be enough
		Dir *FindFile(char *dir, char *file)
		{
			//Find the dir first
			unsigned int i;
			for(i=0;i<Root.Sons.size();i++)
			{
				if(!strcmp(dir, Root.Sons[i].Name))
					break;
			}

			if(i == Root.Sons.size())
			{
				MessageBox(mcdDlg, "Unable to find the directory.", "Error", 0);
				return NULL;
			}

			// Find the file now
			Dir *d = &Root.Sons[i];
			for(i=0;i<d->Sons.size();i++)
			{
				if(!strcmp(file, d->Sons[i].Name))
					break;
			}

			if(i == d->Sons.size())
			{
				MessageBox(mcdDlg, "Unable to find the file.", "Error", 0);
				return NULL;
			}

			return &d->Sons[i];
		}

		void PaintICOFile(HWND hWnd, char *dir, char *name)
		{
			// First find the file
			Dir *d = FindFile(dir, name);
			if(d == NULL)
				return;



			u32 FileID = *(u32 *)d->File;
			u32 AnimationShapes = *(u32 *)(d->File + 4);
			u32 TextureType = *(u32 *)(d->File + 8);
			//u32 ConstantCheck = *(u32 *)(d->File + 12);
			u32 NumVertex = *(u32 *)(d->File + 16);

			// Check if it's an ico file
			if(FileID != 0x00010000)
			{
				MessageBox(mcdDlg, "It's not an ICO file.", "Error", 0);
				return;
			}

			// Is texture compressed?
			if(TextureType != 0x07)
			{
				//MessageBox(mcdDlg, "Compressed texture, not yet supported.", "Error", 0);
				//return;
			}

			// Calculate the offset to the animation segment
			u32 VertexSize = (AnimationShapes * 8) + 8 + 8;
			u32 AnimationSegmentOffset = (VertexSize * NumVertex) + 20;

			// Check if we really are at the animation header
			u32 AnimationID = *(u32 *)(d->File + AnimationSegmentOffset);
			if(AnimationID != 0x00000001)
			{
				MessageBox(mcdDlg, "Animation header ID is incorrect.", "Error", 0);
				return;
			}

			// Get the size of all the animation segment
			u32 FrameLenght = *(u32 *)(d->File + AnimationSegmentOffset + 4);
			u32 NumberOfFrames = *(u32 *)(d->File + AnimationSegmentOffset + 16);

			u32 Offset = AnimationSegmentOffset + 20;
			for(unsigned int i=0;i<NumberOfFrames;i++)
			{
				u32 KeyNum = *(u32 *)(d->File + Offset + 4);
				Offset += 16 + (KeyNum * 8) - 8; // The -8 is there because the doc with the ico format spec is either wrong or I'm stupid
			}

			int z, y=0;
			u16 ico[0x4000];
			u8 cico[128*128*3];

			if(TextureType > 0x07)
			{
				u32 CompressedDataSize = *(u32 *)(d->File + Offset);
				//RLE compression
				
				Offset += 4;
				u32 OffsetEnd = Offset + CompressedDataSize;
				u32 WriteCount = 0;

				while(WriteCount < 0x8000 && Offset < OffsetEnd)
				{

					u16 Data = *(u16 *)(d->File + Offset);

					if(Data < 0xFF00) //Replication
					{
						Offset += 2;
						u16 Rep = *(u16 *)(d->File + Offset);
						
						for(int i=0;i<Data;i++)
						{
							ico[WriteCount] = Rep;
							WriteCount++;
						}
						Offset += 2;
					}
					else	// Lenght
					{
						u16 Lenght = 0xFFFF - Data;
						Offset += 2;
						Lenght++; // doh! This shit made me debug for hours
						for(int i=0;i<Lenght;i++)
						{
							ico[WriteCount] = *(u16 *)(d->File + Offset);
							WriteCount++;
							Offset += 2;
						}
					}

				}

			}
			else
			{
				// Get the texture from the file
				memcpy(ico, &d->File[/*d->Lenght-0x8000*/Offset], 0x8000);
			}

			// Convert from TIM to rgb
			for(z=0;z<0x4000;z++)
			{
				cico[y] = 8 * (ico[z] & 0x1F);
				cico[y+1] = 8 * ((ico[z] >>5) & 0x1F);
				cico[y+2] = 8 * (ico[z] >>10);
				y += 3;
			}

			// Draw
			int px, py, pc=0;
			HDC dc = GetDC(hWnd);
			for(px=0;px<128;px++){
				for(py=0;py<128;py++){
					SetPixel(dc, py, px, RGB(cico[pc], cico[pc+1], cico[pc+2]));
					pc+=3;
					
				}
			}
		}


		void ReadSaveGames()
		{
			// Initialize the conversion table for SJIS strings
			IniSJISTable();

			// Create the image list
			ImageList = ImageList_Create(64, 64, ILC_COLOR32, 10, 256);

			for(unsigned int i=0;i<Root.Sons.size();i++)
			{
				if(Root.Sons[i].Name[0] != '.')
				{
					SaveGame SG(&Root.Sons[i]);
					SaveGameList.push_back(SG);
					SG.AddIcoToImageList(ImageList);
				}
			}
		}

		int FindEmptyCluster()
		{
			// Read fat until free is found 0x7FFFFFFF
			for(int i=0;i<256*256;i++)
			{
				if(FAT[i] == 0x7FFFFFFF)
					return i;
			}


			// if we dont find it, memcard is full

			return -1;

		}


		void SaveRootDir(Dir *Di)
		{
			// Open memcard
			int i;
			fclose(fp);
			fp = fopen(FileName, "rb+");
			if(fp == NULL)
			{
				MessageBox(mcdDlg, "Unable to open memcard.", "Error", 0);
				return;
			}

			//walk the root dir fat chain
			int cluster = 0, oldcluster = 0;
			//s8 file1[256], file2[256];
			int Lenght = Root.Sons[0].Lenght;

			for(i = 0; i < 0xffff; i++)
			{	
				// Get next cluster from the FAT table
				oldcluster = cluster;
				cluster = FAT[cluster];
				cluster = cluster ^ 0x80000000;
				if(cluster == 0x7FFFFFFF || cluster == 0xFFFFFFFF)
					break;
			}

			// oldcluster has in theory the last cluster
			// if lenght is even, there is empty space in cluster
			// if lenght is odd we'll need a new cluster.
			
			int dircluster = FindEmptyCluster();
			char dir[512];
			memzero_obj(dir);
			*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_DIRECTORY | Dir::DF_READ; // mode flag
			*(u32 *)&dir[4] = Di->Sons.size(); // number of files inside the dir
			*(u8 *)&dir[8] = 0; // creation time seconds
			*(u8 *)&dir[9] = 0; // creation time minuts
			*(u8 *)&dir[10] = 0; // creation time hours
			*(u8 *)&dir[11] = 1; // creation time day
			*(u8 *)&dir[12] = 1; // creation time month
			*(u16 *)&dir[13] = 2008; // creation time year
			*(u32 *)&dir[16] = dircluster; // cluster with the contents of the dir
			*(u32 *)&dir[20] = 0; // dir entry for '.'
			*(u8 *)&dir[24] = 0; // modification time seconds
			*(u8 *)&dir[25] = 0; // modification time minuts
			*(u8 *)&dir[26] = 0; // modification time hours
			*(u8 *)&dir[27] = 1; // modification time day
			*(u8 *)&dir[28] = 1; // modification time month
			*(u16 *)&dir[29] = 2008; // modification time year
			*(u32 *)&dir[32] = 0; // user attribute unused
			strcpy(&dir[64], Di->Name); // DIRECTORY NAME


			if(Lenght % 2)
			{
				// Find empty cluster...
				int c = FindEmptyCluster();

				// THIS MAY BE WRONG...TEST
				FAT[oldcluster] = c;
				FAT[c] = 0xFFFFFFFF;

				// Add dir to first page of new cluster
				fseek(fp, 0xA920 + (((c) * 0x420) + 0), SEEK_SET);
				fwrite(dir, 512, 1, fp);
			}
			else
			{
				// Add dir to second page of cluster
				fseek(fp, 0xA920 + (((oldcluster) * 0x420) + 528), SEEK_SET);
				fwrite(dir, 512, 1, fp);
			}

			// CALCULATE CRCS OF CLUSTER


			// INCREASE ROOT DIR SIZE BY 1
			fseek(fp, 0xA920 + (((0) * 0x420) + 4), SEEK_SET);
			int nl = Lenght + 1;
			fwrite(&nl, 4, 1, fp);


			int numfiles = 5;

			// ADD FILE ENTRIES TO DIR ., ..
			memzero_obj(dir);
			*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_DIRECTORY | Dir::DF_READ; // mode flag
			*(u32 *)&dir[4] = numfiles; // number of files inside the dir
			*(u8 *)&dir[8] = 0; // creation time seconds
			*(u8 *)&dir[9] = 0; // creation time minuts
			*(u8 *)&dir[10] = 0; // creation time hours
			*(u8 *)&dir[11] = 1; // creation time day
			*(u8 *)&dir[12] = 1; // creation time month
			*(u16 *)&dir[13] = 2008; // creation time year
			*(u32 *)&dir[16] = 0; // cluster with the contents of the dir
			*(u32 *)&dir[20] = 0; // dir entry for '.'
			*(u8 *)&dir[24] = 0; // modification time seconds
			*(u8 *)&dir[25] = 0; // modification time minuts
			*(u8 *)&dir[26] = 0; // modification time hours
			*(u8 *)&dir[27] = 1; // modification time day
			*(u8 *)&dir[28] = 1; // modification time month
			*(u16 *)&dir[29] = 2008; // modification time year
			*(u32 *)&dir[32] = 0; // user attribute unused
			strcpy(&dir[64], "."); // DIRECTORY NAME
			fseek(fp, 0xA920 + (((dircluster) * 0x420) + 0), SEEK_SET);
			fwrite(dir, 512, 1, fp);

			memzero_obj(dir);
			*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_DIRECTORY | Dir::DF_READ; // mode flag
			*(u32 *)&dir[4] = 2; // number of files inside the dir
			*(u8 *)&dir[8] = 0; // creation time seconds
			*(u8 *)&dir[9] = 0; // creation time minuts
			*(u8 *)&dir[10] = 0; // creation time hours
			*(u8 *)&dir[11] = 1; // creation time day
			*(u8 *)&dir[12] = 1; // creation time month
			*(u16 *)&dir[13] = 2008; // creation time year
			*(u32 *)&dir[16] = 0; // cluster with the contents of the dir
			*(u32 *)&dir[20] = 0; // dir entry for '.'
			*(u8 *)&dir[24] = 0; // modification time seconds
			*(u8 *)&dir[25] = 0; // modification time minuts
			*(u8 *)&dir[26] = 0; // modification time hours
			*(u8 *)&dir[27] = 1; // modification time day
			*(u8 *)&dir[28] = 1; // modification time month
			*(u16 *)&dir[29] = 2008; // modification time year
			*(u32 *)&dir[32] = 0; // user attribute unused
			strcpy(&dir[64], ".."); // DIRECTORY NAME
			fseek(fp, 0xA920 + (((dircluster) * 0x420) + 528), SEEK_SET);
			fwrite(dir, 512, 1, fp);

			FAT[dircluster] = 0xFFFFFFFF;


			// ADD REST OF FILES
			
			for(int i=2;i<numfiles;i+=2)
			{
				// Find a new cluster to store files and updata FAT table
				int newcluster = FindEmptyCluster();
				FAT[dircluster] = newcluster;
				FAT[newcluster] = 0xFFFFFFFF;
				dircluster = newcluster;

				// Add first file
				memzero_obj(dir);
				*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_FILE | Dir::DF_READ; // mode flag
				*(u32 *)&dir[4] = 0; // SIZE OF FILE
				*(u8 *)&dir[8] = 0; // creation time seconds
				*(u8 *)&dir[9] = 0; // creation time minuts
				*(u8 *)&dir[10] = 0; // creation time hours
				*(u8 *)&dir[11] = 1; // creation time day
				*(u8 *)&dir[12] = 1; // creation time month
				*(u16 *)&dir[13] = 2008; // creation time year
				*(u32 *)&dir[16] = 0; // cluster with the contents of the file
				*(u32 *)&dir[20] = 0; // dir entry for '.'
				*(u8 *)&dir[24] = 0; // modification time seconds
				*(u8 *)&dir[25] = 0; // modification time minuts
				*(u8 *)&dir[26] = 0; // modification time hours
				*(u8 *)&dir[27] = 1; // modification time day
				*(u8 *)&dir[28] = 1; // modification time month
				*(u16 *)&dir[29] = 2008; // modification time year
				*(u32 *)&dir[32] = 0; // user attribute unused
				strcpy(&dir[64], "Kaka1"); // DIRECTORY NAME
				fseek(fp, 0xA920 + (((dircluster) * 0x420) + 0), SEEK_SET);
				fwrite(dir, 512, 1, fp);
				// Add first file contents


				// Add second file

				// Add second file contents


			}







			// SAVE FAT HERE
			for(int i=0;i<8;i++)
			{
				fseek(fp, 0x420 * indirect_table[i], SEEK_SET);
				fwrite(&FAT[i*256], 512, 1, fp);

				fseek(fp, 16, SEEK_CUR);
				fwrite(&FAT[(i*256)+128], 512, 1, fp);

			}


			// Close the file
			fclose(fp);


		}


		void SaveRootDir(char *name)
		{
			int i;
			// Open memcard
			fclose(fp);
			fp = fopen(FileName, "rb+");
			if(fp == NULL)
			{
				MessageBox(mcdDlg, "Unable to open memcard.", "Error", 0);
				return;
			}

			//walk the root dir fat chain
			int cluster = 0, oldcluster = 0;
			//s8 file1[256], file2[256];
			int Lenght = Root.Sons[0].Lenght;

			for(i = 0; i < 0xffff; i++)
			{	
				// Get next cluster from the FAT table
				oldcluster = cluster;
				cluster = FAT[cluster];
				cluster = cluster ^ 0x80000000;
				if(cluster == 0x7FFFFFFF || cluster == 0xFFFFFFFF)
					break;
			}

			// oldcluster has in theory the last cluster
			// if lenght is even, there is empty space in cluster
			// if lenght is odd we'll need a new cluster.
			
			int dircluster = FindEmptyCluster();
			char dir[512];
			memzero_obj(dir);
			*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_DIRECTORY | Dir::DF_READ; // mode flag
			*(u32 *)&dir[4] = 2; // number of files inside the dir
			*(u8 *)&dir[8] = 0; // creation time seconds
			*(u8 *)&dir[9] = 0; // creation time minuts
			*(u8 *)&dir[10] = 0; // creation time hours
			*(u8 *)&dir[11] = 1; // creation time day
			*(u8 *)&dir[12] = 1; // creation time month
			*(u16 *)&dir[13] = 2008; // creation time year
			*(u32 *)&dir[16] = dircluster; // cluster with the contents of the dir
			*(u32 *)&dir[20] = 0; // dir entry for '.'
			*(u8 *)&dir[24] = 0; // modification time seconds
			*(u8 *)&dir[25] = 0; // modification time minuts
			*(u8 *)&dir[26] = 0; // modification time hours
			*(u8 *)&dir[27] = 1; // modification time day
			*(u8 *)&dir[28] = 1; // modification time month
			*(u16 *)&dir[29] = 2008; // modification time year
			*(u32 *)&dir[32] = 0; // user attribute unused
			strcpy(&dir[64], "DirName"); // DIRECTORY NAME


			if(Lenght % 2)
			{
				// Find empty cluster...
				int c = FindEmptyCluster();

				// THIS MAY BE WRONG...TEST
				FAT[oldcluster] = c;
				FAT[c] = 0xFFFFFFFF;

				// Add dir to first page of new cluster
				fseek(fp, 0xA920 + (((c) * 0x420) + 0), SEEK_SET);
				fwrite(dir, 512, 1, fp);
			}
			else
			{
				// Add dir to second page of cluster
				fseek(fp, 0xA920 + (((oldcluster) * 0x420) + 528), SEEK_SET);
				fwrite(dir, 512, 1, fp);
			}

			// CALCULATE CRCS OF CLUSTER


			// INCREASE ROOT DIR SIZE BY 1
			fseek(fp, 0xA920 + (((0) * 0x420) + 4), SEEK_SET);
			int nl = Lenght + 1;
			fwrite(&nl, 4, 1, fp);


			int numfiles = 5;

			// ADD FILE ENTRIES TO DIR ., ..
			memzero_obj(dir);
			*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_DIRECTORY | Dir::DF_READ; // mode flag
			*(u32 *)&dir[4] = numfiles; // number of files inside the dir
			*(u8 *)&dir[8] = 0; // creation time seconds
			*(u8 *)&dir[9] = 0; // creation time minuts
			*(u8 *)&dir[10] = 0; // creation time hours
			*(u8 *)&dir[11] = 1; // creation time day
			*(u8 *)&dir[12] = 1; // creation time month
			*(u16 *)&dir[13] = 2008; // creation time year
			*(u32 *)&dir[16] = 0; // cluster with the contents of the dir
			*(u32 *)&dir[20] = 0; // dir entry for '.'
			*(u8 *)&dir[24] = 0; // modification time seconds
			*(u8 *)&dir[25] = 0; // modification time minuts
			*(u8 *)&dir[26] = 0; // modification time hours
			*(u8 *)&dir[27] = 1; // modification time day
			*(u8 *)&dir[28] = 1; // modification time month
			*(u16 *)&dir[29] = 2008; // modification time year
			*(u32 *)&dir[32] = 0; // user attribute unused
			strcpy(&dir[64], "."); // DIRECTORY NAME
			fseek(fp, 0xA920 + (((dircluster) * 0x420) + 0), SEEK_SET);
			fwrite(dir, 512, 1, fp);

			memzero_obj(dir);
			*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_DIRECTORY | Dir::DF_READ; // mode flag
			*(u32 *)&dir[4] = 2; // number of files inside the dir
			*(u8 *)&dir[8] = 0; // creation time seconds
			*(u8 *)&dir[9] = 0; // creation time minuts
			*(u8 *)&dir[10] = 0; // creation time hours
			*(u8 *)&dir[11] = 1; // creation time day
			*(u8 *)&dir[12] = 1; // creation time month
			*(u16 *)&dir[13] = 2008; // creation time year
			*(u32 *)&dir[16] = 0; // cluster with the contents of the dir
			*(u32 *)&dir[20] = 0; // dir entry for '.'
			*(u8 *)&dir[24] = 0; // modification time seconds
			*(u8 *)&dir[25] = 0; // modification time minuts
			*(u8 *)&dir[26] = 0; // modification time hours
			*(u8 *)&dir[27] = 1; // modification time day
			*(u8 *)&dir[28] = 1; // modification time month
			*(u16 *)&dir[29] = 2008; // modification time year
			*(u32 *)&dir[32] = 0; // user attribute unused
			strcpy(&dir[64], ".."); // DIRECTORY NAME
			fseek(fp, 0xA920 + (((dircluster) * 0x420) + 528), SEEK_SET);
			fwrite(dir, 512, 1, fp);

			FAT[dircluster] = 0xFFFFFFFF;


			// ADD REST OF FILES
			
			for(int i=2;i<numfiles;i+=2)
			{
				// Find a new cluster to store files and updata FAT table
				int newcluster = FindEmptyCluster();
				FAT[dircluster] = newcluster;
				FAT[newcluster] = 0xFFFFFFFF;
				dircluster = newcluster;

				// Add first file
				memzero_obj(dir);
				*(u16 *)&dir[0] = Dir::DF_EXISTS | Dir::DF_FILE | Dir::DF_READ; // mode flag
				*(u32 *)&dir[4] = 2; // number of files inside the dir
				*(u8 *)&dir[8] = 0; // creation time seconds
				*(u8 *)&dir[9] = 0; // creation time minuts
				*(u8 *)&dir[10] = 0; // creation time hours
				*(u8 *)&dir[11] = 1; // creation time day
				*(u8 *)&dir[12] = 1; // creation time month
				*(u16 *)&dir[13] = 2008; // creation time year
				*(u32 *)&dir[16] = 0; // cluster with the contents of the file
				*(u32 *)&dir[20] = 0; // dir entry for '.'
				*(u8 *)&dir[24] = 0; // modification time seconds
				*(u8 *)&dir[25] = 0; // modification time minuts
				*(u8 *)&dir[26] = 0; // modification time hours
				*(u8 *)&dir[27] = 1; // modification time day
				*(u8 *)&dir[28] = 1; // modification time month
				*(u16 *)&dir[29] = 2008; // modification time year
				*(u32 *)&dir[32] = 0; // user attribute unused
				strcpy(&dir[64], "Kaka1"); // DIRECTORY NAME
				fseek(fp, 0xA920 + (((dircluster) * 0x420) + 0), SEEK_SET);
				fwrite(dir, 512, 1, fp);
				// Add first file contents


				// Add second file

				// Add second file contents


			}







			// SAVE FAT HERE
			for(int i=0;i<8;i++)
			{
				fseek(fp, 0x420 * indirect_table[i], SEEK_SET);
				fwrite(&FAT[i*256], 512, 1, fp);

				fseek(fp, 16, SEEK_CUR);
				fwrite(&FAT[(i*256)+128], 512, 1, fp);

			}


			// Close the file
			fclose(fp);

		}

};

MemoryCard MC1, MC2;




BOOL GetTreeViewSelection(HWND hWnd, char *d, char *n)
{

	HTREEITEM sel = TreeView_GetSelection(hWnd);
	TVITEMEX file, dir;
	file.hItem = sel;
	file.mask = TVIF_HANDLE|TVIF_TEXT;
	file.pszText = n;
	file.cchTextMax = 255;
	BOOL r = TreeView_GetItem(GetDlgItem(mcdDlg, IDC_TREE1), &file);
	if(r == FALSE)
	{
		return r;
	}

	HTREEITEM parentsel = TreeView_GetParent(hWnd, sel);
	dir.hItem = parentsel;
	dir.mask = TVIF_HANDLE|TVIF_TEXT;
	dir.pszText = d;
	dir.cchTextMax = 255;
	r = TreeView_GetItem(GetDlgItem(mcdDlg, IDC_TREE1), &dir);
	return r;
}


void Open_Mcd_Proc(HWND hW, int mcd) {
	OPENFILENAME ofn;
	char szFileName[256];
	char szFileTitle[256];
	char szFilter[1024];
	char *str;

	memzero_obj(szFileName);
	memzero_obj(szFileTitle);
	memzero_obj(szFilter);


	strcpy(szFilter, _("Ps2 Memory Card (*.ps2)"));
	str = szFilter + strlen(szFilter) + 1; 
	strcpy(str, "*.ps2");

	str+= strlen(str) + 1;
	strcpy(str, _("All Files"));
	str+= strlen(str) + 1;
	strcpy(str, "*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hW;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= 256;
    ofn.lpstrInitialDir		= MEMCARDS_DIR;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= 256;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "MC2";
    ofn.Flags				=
		OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn)) {
		Edit_SetText(GetDlgItem(hW,mcd == 1 ? IDC_MCD1 : IDC_MCD2), szFileName);
	}
}


void SaveFileDialog(HWND hW, int MC, char *dir, char *name) {
	OPENFILENAME ofn;
	char szFileName[256];
	char szFileTitle[256];
	char szFilter[1024];
//	char *str;  (unused for now)

	memzero_obj(szFileName);
	memzero_obj(szFileTitle);
	memzero_obj(szFilter);

	strcpy(szFilter, "All Files (*.*)");
	strcpy(szFileName, name);

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hW;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= 256;
    ofn.lpstrInitialDir		= MEMCARDS_DIR;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= 256;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "MC2";
    ofn.Flags				=
		OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT |
		OFN_EXTENSIONDIFFERENT | OFN_EXPLORER;

	if (GetSaveFileName ((LPOPENFILENAME)&ofn))
	{
		Dir *d;

		if(MC == 1)
			d = MC1.FindFile(dir, name);
		else
			d = MC2.FindFile(dir, name);

		if(d != NULL)
		{
			FILE *fp = fopen(ofn.lpstrFile, "wb");
			fwrite(d->File, d->Lenght, 1, fp);
			fclose(fp);
		}
	}
}



FILE *fp;

BOOL CALLBACK ConfigureMcdsDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			mcdDlg = hW;

			SetWindowText(hW, _("Memcard Manager"));

			Button_SetText(GetDlgItem(hW, IDOK),        _("OK"));
			Button_SetText(GetDlgItem(hW, IDCANCEL),    _("Cancel"));
			Button_SetText(GetDlgItem(hW, IDC_MCDSEL1), _("Select Mcd"));
			Button_SetText(GetDlgItem(hW, IDC_MCDSEL2), _("Select Mcd"));

			Static_SetText(GetDlgItem(hW, IDC_FRAMEMCD1), _("Memory Card 1"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMEMCD2), _("Memory Card 2"));

			if (!strlen(Config.Mcd1)) strcpy(Config.Mcd1, MEMCARDS_DIR "\\" DEFAULT_MEMCARD1);
			if (!strlen(Config.Mcd2)) strcpy(Config.Mcd2, MEMCARDS_DIR "\\" DEFAULT_MEMCARD2);
			Edit_SetText(GetDlgItem(hW,IDC_MCD1), Config.Mcd1);
			Edit_SetText(GetDlgItem(hW,IDC_MCD2), Config.Mcd2);

			//MC1.Load(Config.Mcd1);		
			//MC2.Load(Config.Mcd2);

			//MC1.AddCardToTreeView(GetDlgItem(hW, IDC_TREE1));
			//MC2.AddCardToTreeView(GetDlgItem(hW, IDC_TREE2));

			//MC1.AddToListView(GetDlgItem(hW,IDC_LIST1));
			//MC2.AddToListView(GetDlgItem(hW,IDC_LIST2));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {

				case IDC_MCDSEL1: 
					Open_Mcd_Proc(hW, 1); 
					return TRUE;

				case IDC_MCDSEL2: 
					Open_Mcd_Proc(hW, 2); 
					return TRUE;

       			case IDCANCEL:
				{
					EndDialog(hW,FALSE);
					return TRUE;
				}

       			case IDOK:
					Edit_GetText(GetDlgItem(hW,IDC_MCD1), Config.Mcd1, 256);
					Edit_GetText(GetDlgItem(hW,IDC_MCD2), Config.Mcd2, 256);
//					SaveConfig();
					
					EndDialog(hW,TRUE);
					return TRUE;

				case IDC_RELOAD1:
				{
					char cardname[1024];
					Edit_GetText(GetDlgItem(hW, IDC_MCD1), cardname, 1024);
					MC1.Load(cardname);
					MC1.AddToListView(GetDlgItem(hW,IDC_LIST1));
					//MC1.AddCardToTreeView(GetDlgItem(hW,IDC_TREE1));
					break;
				}

				case IDC_RELOAD2:
				{
					char cardname[1024];
					Edit_GetText(GetDlgItem(hW, IDC_MCD2), cardname, 1024);
					MC2.Load(cardname);
					MC2.AddToListView(GetDlgItem(hW,IDC_LIST2));
					//MC2.AddCardToTreeView(GetDlgItem(hW,IDC_TREE2));
					break;
				}

				case IDC_LOADICO1:
				{
					char cardname[1024];
					MC1.SaveRootDir("test");
					Edit_GetText(GetDlgItem(hW, IDC_MCD1), cardname, 1024);
					MC1.Load(cardname);
					MC1.AddToListView(GetDlgItem(hW,IDC_LIST1));
					/*
					char dir[256], name[256];
					if(GetTreeViewSelection(GetDlgItem(mcdDlg, IDC_TREE1), dir, name))
						MC1.PaintICOFile(GetDlgItem(mcdDlg, IDC_ICON1), dir, name);
					else
						MessageBox(mcdDlg, "Please select a file.", "Error", 0);
					*/
					break;
				}

				case IDC_LOADICO2:
				{
					/*
					char dir[256], name[256];
					if(GetTreeViewSelection(GetDlgItem(mcdDlg, IDC_TREE2), dir, name))
						MC1.PaintICOFile(GetDlgItem(mcdDlg, IDC_ICON2), dir, name);
					else
						MessageBox(mcdDlg, "Please select a file.", "Error", 0);
					*/
					break;
				}

				case IDC_SAVE1:
				{
					/*
					char dir[256], name[256];
					if(GetTreeViewSelection(GetDlgItem(mcdDlg, IDC_TREE1), dir, name))
						SaveFileDialog(mcdDlg, 1, dir, name);
					else
						MessageBox(mcdDlg, "Please select a file.", "Error", 0);
					*/
					break;
				}

				case IDC_SAVE2:
				{
					/*
					char dir[256], name[256];
					if(GetTreeViewSelection(GetDlgItem(mcdDlg, IDC_TREE2), dir, name))
						SaveFileDialog(mcdDlg, 2, dir, name);
					else
						MessageBox(mcdDlg, "Please select a file.", "Error", 0);
					*/
					break;
				}
			}
		case WM_DESTROY:
			return TRUE;
	}
	return FALSE;
}




