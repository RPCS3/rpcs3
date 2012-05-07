/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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
 
#ifndef ZZOGLSHOOTS_H_INCLUDED
#define ZZOGLSHOOTS_H_INCLUDED

void SaveSnapshot(const char* filename);
bool SaveRenderTarget(const char* filename, int width, int height, int jpeg);
bool SaveTexture(const char* filename, u32 textarget, u32 tex, int width, int height, int ext_format = 0);
bool SaveJPEG(const char* filename, int width, int height, const void* pdata, int quality);
bool SaveTGA(const char* filename, int width, int height, void* pdata);
bool SaveBMP(const char* filename, int width, int height, void* pdata);
void Stop_Avi();
void Delete_Avi_Capture();

void StartCapture();
void StopCapture();
void CaptureFrame();

enum {
	EXT_TGA = 0,
	EXT_BMP = 1,
	EXT_JPG = 2
};

#endif // ZZOGLSHOOTS_H_INCLUDED
