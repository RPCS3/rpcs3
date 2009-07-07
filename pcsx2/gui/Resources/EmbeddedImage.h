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

#pragma once

#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/gdicmn.h>

//////////////////////////////////////////////////////////////////////////////////////////
// Interface for loadable embedded images.  See EmbeddedImage description for details.
//
class IEmbeddedImage
{
public:
	virtual const wxImage& Get()=0;
	virtual wxImage Rescale( int width, int height )=0;
	virtual wxImage Resample( int width, int height )=0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// EmbeddedImage - Helper class for loading images which have been embedded into the binary
// manifest of the executable file.  Images are only loaded when Get() is called,
// which allows this type to be passed to functions as a fallback or default action without
// incurring an unnecessary memory footprint.
//
// Note: Get() only loads the image once.  All subsequent calls to Get will use the
// previously loaded image data.
//
template< typename ImageType >
class EmbeddedImage : public IEmbeddedImage
{
protected:
	wxImage m_Image;
	const wxSize m_ResampleTo;

protected:
	// ------------------------------------------------------------------------
	// Internal function used to ensure the image is loaded before returning the image
	// handle (called from all methods that return an image handle).
	//
	void _loadImage()
	{
		if( !m_Image.Ok() )
		{
			wxMemoryInputStream joe( ImageType::Data, ImageType::Length );
			m_Image.LoadFile( joe, ImageType::GetFormat() );

			if( m_ResampleTo.IsFullySpecified() && ( m_ResampleTo.GetWidth() != m_Image.GetWidth() || m_ResampleTo.GetHeight() != m_Image.GetHeight() ) )
				m_Image = m_Image.ResampleBox( m_ResampleTo.GetWidth(), m_ResampleTo.GetHeight() );
		}
	}

public:
	EmbeddedImage() :
		m_Image()
	,	m_ResampleTo( wxDefaultSize )
	{
	}

	// ------------------------------------------------------------------------
	// Constructor for creating an embedded image that gets resampled to the specified size when
	// loaded.
	//
	// Implementation Note: This function uses wxWidgets ResamplBox method to resize the image.
	// wxWidgets ResampleBicubic appears to be buggy and produces fairly poor results when down-
	// sampling images (basically resembles a pixel resize).  ResampleBox produces much cleaner
	// results.
	//
	EmbeddedImage( int newWidth, int newHeight ) :
		m_Image()
	,	m_ResampleTo( newWidth, newHeight )
	{
	}

	// ------------------------------------------------------------------------
	// Loads and retrieves the embedded image.  The embedded image is only loaded from its em-
	// bedded format once.  Any subsequent calls to Get(), Rescale(), or Resample() will use
	// the pre-loaded copy.  Translation: the png/jpeg decoding overhead happens only once,
	// and only happens when the image is actually fetched.  Simply creating an instance
	// of an EmbeddedImage object uses no excess memory nor cpu overhead. :)
	//
	const wxImage& Get()
	{
		_loadImage();
		return m_Image;
	}

	// ------------------------------------------------------------------------
	// Performs a pixel resize of the loaded image and returns a new image handle (EmbeddedImage
	// is left unmodified).  Useful for creating ugly versions of otherwise nice graphics.  Do
	// us all a favor and use Resample() instead.
	//
	// (this method included for sake of completeness only).
	//
	wxImage Rescale( int width, int height )
	{
		_loadImage();
		if( width != m_Image.GetWidth() || height != m_Image.GetHeight() )
			return m_Image.Rescale( width, height );
		else
			return m_Image;
	}

	// ------------------------------------------------------------------------
	// Implementation Note: This function uses wxWidgets ResampleBox method to resize the image.
	// wxWidgets ResampleBicubic appears to be buggy and produces fairly poor results when down-
	// sampling images (basically resembles a pixel resize).  ResampleBox produces much cleaner
	// results.
	//
	wxImage Resample( int width, int height )
	{
		_loadImage();
		if( width != m_Image.GetWidth() || height != m_Image.GetHeight() )
			return m_Image.ResampleBox( width, height );
		else
			return m_Image;
	}
};

