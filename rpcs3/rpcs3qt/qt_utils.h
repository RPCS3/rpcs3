#pragma once

#include "stdafx.h"
#include <QtCore>

namespace gui
{
	namespace utils
	{
		// Creates a frame geometry rectangle with given width height that's centered inside the origin,
		// while still considering screen boundaries.
		QRect create_centered_window_geometry(const QRect& origin, s32 width, s32 height);
	} // utils
} // gui
