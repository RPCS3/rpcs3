
#include "qt_utils.h"
#include <QApplication>
#include <QScreen>

namespace gui
{
	namespace utils
	{
		QRect create_centered_window_geometry(const QRect& origin, s32 width, s32 height)
		{
			// Get minimum virtual screen x & y for clamping the
			// window x & y later while taking the width and height
			// into account, so they don't go offscreen
			s32 min_screen_x = std::numeric_limits<s32>::max();
			s32 max_screen_x = std::numeric_limits<s32>::min();
			s32 min_screen_y = std::numeric_limits<s32>::max();
			s32 max_screen_y = std::numeric_limits<s32>::min();
			for (auto screen : QApplication::screens())
			{
				auto screen_geometry = screen->availableGeometry();
				min_screen_x = std::min(min_screen_x, screen_geometry.x());
				max_screen_x = std::max(max_screen_x, screen_geometry.x() + screen_geometry.width() - width);
				min_screen_y = std::min(min_screen_y, screen_geometry.y());
				max_screen_y = std::max(max_screen_y, screen_geometry.y() + screen_geometry.height() - height);
			}

			s32 frame_x_raw = origin.left() + ((origin.width() - width) / 2);
			s32 frame_y_raw = origin.top() + ((origin.height() - height) / 2);

			s32 frame_x = std::clamp(frame_x_raw, min_screen_x, max_screen_x);
			s32 frame_y = std::clamp(frame_y_raw, min_screen_y, max_screen_y);

			return QRect(frame_x, frame_y, width, height);
		}
	} // utils
} // gui
