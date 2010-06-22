/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "MainFrame.h"
#include "AppGameDatabase.h"

#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/imaglist.h>

#include "Resources/EmbeddedImage.h"
#include "Resources/BackgroundLogo.h"

#include "Resources/ConfigIcon_Cpu.h"
#include "Resources/ConfigIcon_Video.h"
#include "Resources/ConfigIcon_Speedhacks.h"
#include "Resources/ConfigIcon_Gamefixes.h"
#include "Resources/ConfigIcon_Paths.h"
#include "Resources/ConfigIcon_Plugins.h"
#include "Resources/ConfigIcon_MemoryCard.h"

#include "Resources/AppIcon16.h"
#include "Resources/AppIcon32.h"
#include "Resources/AppIcon64.h"

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

RecentIsoList::RecentIsoList()
{
	Menu = new wxMenu();
	Menu->Append( MenuId_IsoBrowse, _("Browse..."), _("Browse for an Iso that is not in your recent history.") );
	Manager = new RecentIsoManager( Menu );
}

pxAppResources::pxAppResources()
{
}

pxAppResources::~pxAppResources() throw() {}

wxMenu& Pcsx2App::GetRecentIsoMenu()
{
	pxAssert( !!m_RecentIsoList->Menu );
	return *m_RecentIsoList->Menu;
}

RecentIsoManager& Pcsx2App::GetRecentIsoManager()
{
	pxAssert( !!m_RecentIsoList->Manager );
	return *m_RecentIsoList->Manager;
}

pxAppResources& Pcsx2App::GetResourceCache()
{
	ScopedLock lock( m_mtx_Resources );
	if( !m_Resources )
		m_Resources = new pxAppResources();

	return *m_Resources;
}

const wxIconBundle& Pcsx2App::GetIconBundle()
{
	ScopedPtr<wxIconBundle>& bundle( GetResourceCache().IconBundle );
	if( !bundle )
	{
		bundle = new wxIconBundle();
		bundle->AddIcon( EmbeddedImage<res_AppIcon32>().GetIcon() );
		bundle->AddIcon( EmbeddedImage<res_AppIcon64>().GetIcon() );
		bundle->AddIcon( EmbeddedImage<res_AppIcon16>().GetIcon() );
	}

	return *bundle;
}

const wxBitmap& Pcsx2App::GetLogoBitmap()
{
	ScopedPtr<wxBitmap>& logo( GetResourceCache().Bitmap_Logo );
	if( logo ) return *logo;

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

			Console.Error( "Loading themes from zipfile is not supported yet.\nFalling back on default theme." );
		}

		// Overrides zipfile settings (fix when zipfile support added)
		mess = theme.ToString();
	}

	wxImage img;
	EmbeddedImage<res_BackgroundLogo> temp;	// because gcc can't allow non-const temporaries.
	LoadImageAny( img, useTheme, mess, L"BackgroundLogo", temp );
	logo = new wxBitmap( img );

	return *logo;
}

wxImageList& Pcsx2App::GetImgList_Config()
{
	ScopedPtr<wxImageList>& images( GetResourceCache().ConfigImages );
	if( !images )
	{
		images = new wxImageList(32, 32);
		wxFileName mess;
		bool useTheme = (g_Conf->DeskTheme != L"default");

		if( useTheme )
		{
			wxDirName theme( PathDefs::GetThemes() + g_Conf->DeskTheme );
			mess = theme.ToString();
		}

		wxImage img;

		// GCC Specific: wxT() macro is required when using string token pasting.  For some
		// reason L generates syntax errors. >_<

		#undef  FancyLoadMacro
		#define FancyLoadMacro( name ) \
		{ \
			EmbeddedImage<res_ConfigIcon_##name> temp( g_Conf->Listbook_ImageSize, g_Conf->Listbook_ImageSize ); \
			m_Resources->ImageId.Config.name = images->Add( LoadImageAny( \
				img, useTheme, mess, L"ConfigIcon_" wxT(#name), temp ) \
			); \
		}

		FancyLoadMacro( Paths );
		FancyLoadMacro( Plugins );
		FancyLoadMacro( Gamefixes );
		FancyLoadMacro( Speedhacks );
		FancyLoadMacro( MemoryCard );
		FancyLoadMacro( Video );
		FancyLoadMacro( Cpu );
	}
	return *images;
}

wxImageList& Pcsx2App::GetImgList_Toolbars()
{
	ScopedPtr<wxImageList>& images( GetResourceCache().ToolbarImages );

	if( !images )
	{
		const int imgSize = g_Conf->Toolbar_ImageSize ? 64 : 32;
		images = new wxImageList( imgSize, imgSize );
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
			EmbeddedImage<res_ToolbarIcon_##name> temp( imgSize, imgSize ); \
			m_Resources.ImageId.Toolbars.name = images->Add( LoadImageAny( img, useTheme, mess, L"ToolbarIcon" wxT(#name), temp ) ); \
		}

	}
	return *images;
}

const AppImageIds& Pcsx2App::GetImgId() const
{
	return m_Resources->ImageId;
}
