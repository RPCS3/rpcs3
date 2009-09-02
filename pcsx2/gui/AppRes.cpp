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

#include "PrecompiledHeader.h"
#include "MainFrame.h"

#include <wx/zipstrm.h>
#include <wx/wfstream.h>

#include "Resources/EmbeddedImage.h"
#include "Resources/BackgroundLogo.h"

#include "Resources/ConfigIcon_Cpu.h"
#include "Resources/ConfigIcon_Video.h"
#include "Resources/ConfigIcon_Speedhacks.h"
#include "Resources/ConfigIcon_Gamefixes.h"
#include "Resources/ConfigIcon_Paths.h"
#include "Resources/ConfigIcon_Plugins.h"

// ------------------------------------------------------------------------
const wxImage& LoadImageAny(
	wxImage& dest, bool useTheme, wxFileName& base, const wxChar* filename, IEmbeddedImage& onFail )
{
	if( useTheme )
	{
		base.SetName( filename );

		base.SetExt( L"png" );
		if( base.FileExists() )
		{
			if( dest.LoadFile( base.GetFullPath() ) ) return dest;
		}

		base.SetExt( L"jpg" );
		if( base.FileExists() )
		{
			if( dest.LoadFile( base.GetFullPath() ) ) return dest;
		}

		base.SetExt( L"bmp" );
		if( base.FileExists() )
		{
			if( dest.LoadFile( base.GetFullPath() ) ) return dest;
		}
	}

	return dest = onFail.Get();
}

// ------------------------------------------------------------------------
const wxBitmap& Pcsx2App::GetLogoBitmap()
{
	if( m_Bitmap_Logo != NULL )
		return *m_Bitmap_Logo;

	wxFileName mess;
	bool useTheme = (g_Conf->DeskTheme != L"default");

	if( useTheme )
	{
		wxDirName theme( PathDefs::GetThemes() + g_Conf->DeskTheme );
		wxFileName zipped( theme.GetFilename() );

		zipped.SetExt( L"zip" );
		if( zipped.FileExists() )
		{
			// loading theme from zipfile.
			//wxFileInputStream stream( zipped.ToString() )
			//wxZipInputStream zstream( stream );

			Console::Error( "Loading themes from zipfile is not supported yet.\nFalling back on default theme." );
		}

		// Overrides zipfile settings (fix when zipfile support added)
		mess = theme.ToString();
	}

	wxImage img;
	EmbeddedImage<png_BackgroundLogo> temp;	// because gcc can't allow non-const temporaries.
	LoadImageAny( img, useTheme, mess, L"BackgroundLogo", temp );
	m_Bitmap_Logo = new wxBitmap( img );

	return *m_Bitmap_Logo;
}

// ------------------------------------------------------------------------
wxImageList& Pcsx2App::GetImgList_Config()
{
	if( !m_ConfigImagesAreLoaded )
	{
		wxFileName mess;
		bool useTheme = (g_Conf->DeskTheme != L"default");

		if( useTheme )
		{
			wxDirName theme( PathDefs::GetThemes() + g_Conf->DeskTheme );
			mess = theme.ToString();
		}

		wxImage img;

		// GCC Specific: wxT() macro is required when using string token pasting.  For some reason L
		// generates syntax errors. >_<

		#undef  FancyLoadMacro
		#define FancyLoadMacro( name ) \
		{ \
			EmbeddedImage<png_ConfigIcon_##name> temp( g_Conf->Listbook_ImageSize, g_Conf->Listbook_ImageSize ); \
			m_ImageId.Config.name = m_ConfigImages.Add( LoadImageAny( \
				img, useTheme, mess, L"ConfigIcon_" wxT(#name), temp ) \
			); \
		}

		FancyLoadMacro( Paths );
		FancyLoadMacro( Plugins );
		FancyLoadMacro( Gamefixes );
		FancyLoadMacro( Speedhacks );
		FancyLoadMacro( Video );
		FancyLoadMacro( Cpu );
	}
	m_ConfigImagesAreLoaded = true;
	return m_ConfigImages;
}

// ------------------------------------------------------------------------
wxImageList& Pcsx2App::GetImgList_Toolbars()
{
	if( m_ToolbarImages == NULL )
	{
		const int imgSize = g_Conf->Toolbar_ImageSize ? 64 : 32;
		m_ToolbarImages = new wxImageList( imgSize, imgSize );
		wxFileName mess;
		bool useTheme = (g_Conf->DeskTheme != L"default");

		if( useTheme )
		{
			wxDirName theme( PathDefs::GetThemes() + g_Conf->DeskTheme );
			mess = theme.ToString();
		}

		wxImage img;
		#undef  FancyLoadMacro
		#define FancyLoadMacro( name ) \
		{ \
			EmbeddedImage<png_ToolbarIcon_##name> temp( imgSize, imgSize ); \
			m_ImageId.Toolbars.name = m_ConfigImages.Add( LoadImageAny( img, useTheme, mess, L"ToolbarIcon" wxT(#name), temp ) ); \
		}

	}
	return *m_ToolbarImages;
}

