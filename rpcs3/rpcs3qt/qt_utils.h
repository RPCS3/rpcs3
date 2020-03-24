#pragma once

#include "stdafx.h"
#include <QDir>
#include <QComboBox>
#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>

namespace gui
{
	namespace utils
	{
		template<typename T>
		static QSet<T> list_to_set(const QList<T>& list)
		{
			return QSet<T>(list.begin(), list.end());
		}

		// Creates a frame geometry rectangle with given width height that's centered inside the origin,
		// while still considering screen boundaries.
		QRect create_centered_window_geometry(const QRect& origin, s32 width, s32 height);

		// Returns a custom colored QPixmap based on another QPixmap.
		// use colorize_all to repaint every opaque pixel with the chosen color
		// use_special_masks is only used for pixmaps with multiple predefined colors
		QPixmap get_colorized_pixmap(const QPixmap& old_pixmap, const QColor& old_color, const QColor& new_color, bool use_special_masks = false, bool colorize_all = false);

		// Returns a custom colored QIcon based on another QIcon.
		// use colorize_all to repaint every opaque pixel with the chosen color
		// use_special_masks is only used for icons with multiple predefined colors
		QIcon get_colorized_icon(const QIcon& old_icon, const QColor& old_color, const QColor& new_color, bool use_special_masks = false, bool colorize_all = false);

		// Returns a list of all base names of files in dir whose complete file names contain one of the given name_filters
		QStringList get_dir_entries(const QDir& dir, const QStringList& name_filters);

		// Returns the color specified by its color_role for the QLabels with object_name
		QColor get_label_color(const QString& object_name, QPalette::ColorRole color_role = QPalette::WindowText);

		// Returns the font of the QLabels with object_name
		QFont get_label_font(const QString& object_name);

		// Returns the part of the image loaded from path that is inside the bounding box of its opaque areas
		QImage get_opaque_image_area(const QString& path);

		// Workaround: resize the dropdown combobox items
		void resize_combo_box_view(QComboBox* combo);

		// Recalculates a table's item count based on the available visible space and fills it with empty items
		void update_table_item_count(QTableWidget* table);

		// Opens an image in a new window with original size
		void show_windowed_image(const QImage& img, const QString& title = "");

		// Loads the app icon from path and embeds it centered into an empty square icon
		QIcon get_app_icon_from_path(const std::string& path, const std::string& title_id);

		// Open a path in the explorer and mark the file
		void open_dir(const std::string& spath);

		// Open a path in the explorer and mark the file
		void open_dir(const QString& path);
	} // utils
} // gui
