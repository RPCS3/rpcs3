#pragma once

#include "stdafx.h"
#include <QtCore>

namespace gui
{
	namespace utils
	{
		// Estimate height of a title bar on Windows. Subject to change depending on user settings.
		const s32 ESTIMATED_TITLE_BAR_HEIGHT = 32;

		// Creates a frame geometry rectangle with given width height that's centered inside the origin,
		// while still considering screen boundaries.
		QRect create_centered_window_geometry(const QRect& origin, s32 width, s32 height);
	} // utils
} // gui
