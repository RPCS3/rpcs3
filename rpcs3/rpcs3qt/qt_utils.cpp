
#include "qt_utils.h"
#include <QApplication>
#include <QBitmap>
#include <QFontMetrics>
#include <QPainter>
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

		QPixmap get_colorized_pixmap(const QPixmap& old_pixmap, const QColor& old_color, const QColor& new_color, bool use_special_masks, bool colorize_all)
		{
			QPixmap pixmap = old_pixmap;

			if (colorize_all)
			{
				QBitmap mask = pixmap.createMaskFromColor(Qt::transparent, Qt::MaskInColor);
				pixmap.fill(new_color);
				pixmap.setMask(mask);
				return pixmap;
			}

			QBitmap mask = pixmap.createMaskFromColor(old_color, Qt::MaskOutColor);
			pixmap.fill(new_color);
			pixmap.setMask(mask);

			// special masks for disc icon and others
			if (use_special_masks)
			{
				// Example usage for an icon with multiple shades of the same color

				//auto saturatedColor = [](const QColor& col, float sat /* must be < 1 */)
				//{
				//	int r = col.red() + sat * (255 - col.red());
				//	int g = col.green() + sat * (255 - col.green());
				//	int b = col.blue() + sat * (255 - col.blue());
				//	return QColor(r, g, b, col.alpha());
				//};

				//QColor test_color(0, 173, 246, 255);
				//QPixmap test_pixmap = old_pixmap;
				//QBitmap test_mask = test_pixmap.createMaskFromColor(test_color, Qt::MaskOutColor);
				//test_pixmap.fill(saturatedColor(new_color, 0.6f));
				//test_pixmap.setMask(test_mask);

				QColor white_color(Qt::white);
				QPixmap white_pixmap = old_pixmap;
				QBitmap white_mask = white_pixmap.createMaskFromColor(white_color, Qt::MaskOutColor);
				white_pixmap.fill(white_color);
				white_pixmap.setMask(white_mask);

				QPainter painter(&pixmap);
				painter.drawPixmap(QPoint(0, 0), white_pixmap);
				//painter.drawPixmap(QPoint(0, 0), test_pixmap);
				painter.end();
			}
			return pixmap;
		}

		QIcon get_colorized_icon(const QIcon& old_icon, const QColor& old_color, const QColor& new_color, bool use_special_masks, bool colorize_all)
		{
			return QIcon(get_colorized_pixmap(old_icon.pixmap(old_icon.availableSizes().at(0)), old_color, new_color, use_special_masks, colorize_all));
		}

		QStringList get_dir_entries(const QDir& dir, const QStringList& name_filters)
		{
			QFileInfoList entries = dir.entryInfoList(name_filters, QDir::Files);
			QStringList res;
			for (const QFileInfo &entry : entries)
			{
				res.append(entry.baseName());
			}
			return res;
		}

		QColor get_label_color(const QString& object_name, QPalette::ColorRole color_role)
		{
			QLabel dummy_color;
			dummy_color.setObjectName(object_name);
			dummy_color.ensurePolished();
			return dummy_color.palette().color(color_role);
		};

		QFont get_label_font(const QString& object_name)
		{
			QLabel dummy_font;
			dummy_font.setObjectName(object_name);
			dummy_font.ensurePolished();
			return dummy_font.font();
		};

		QImage get_opaque_image_area(const QString& path)
		{
			QImage image = QImage(path);

			int w_min = 0;
			int w_max = image.width();
			int h_min = 0;
			int h_max = image.height();

			for (int y = 0; y < image.height(); ++y)
			{
				QRgb *row = (QRgb*)image.scanLine(y);
				bool row_filled = false;

				for (int x = 0; x < image.width(); ++x)
				{
					if (qAlpha(row[x]))
					{
						row_filled = true;
						w_min = std::max(w_min, x);

						if (w_max > x)
						{
							w_max = x;
							x = w_min;
						}
					}
				}

				if (row_filled)
				{
					h_max = std::min(h_max, y);
					h_min = y;
				}
			}

			return image.copy(QRect(QPoint(w_max, h_max), QPoint(w_min, h_min)));
		}

		// taken from https://stackoverflow.com/a/30818424/8353754
		// because size policies won't work as expected (see similar bugs in Qt bugtracker)
		void resize_combo_box_view(QComboBox* combo)
		{
			int max_width = 0;
			QFontMetrics font_metrics(combo->font());

			for (int i = 0; i < combo->count(); ++i)
			{
				max_width = std::max(max_width, font_metrics.width(combo->itemText(i)));
			}

			if (combo->view()->minimumWidth() < max_width)
			{
				// add scrollbar width and margin
				max_width += combo->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
				max_width += combo->view()->autoScrollMargin();
				combo->view()->setMinimumWidth(max_width);
			}
		};

		void update_table_item_count(QTableWidget* table)
		{
			if (!table)
				return;

			int item_count = table->rowCount();
			bool is_empty = item_count < 1;
			if (is_empty)
				table->insertRow(0);

			int item_height = table->rowHeight(0);
			if (is_empty)
			{
				table->clearContents();
				table->setRowCount(0);
			}

			int available_height = table->rect().height() - table->horizontalHeader()->height() - table->frameWidth() * 2;
			if (available_height < item_height || item_height < 1)
				return;

			int new_item_count = available_height / item_height;
			if (new_item_count == item_count)
				return;

			item_count = new_item_count;
			table->clearContents();
			table->setRowCount(0);

			for (u32 i = 0; i < item_count; ++i)
				table->insertRow(i);

			if (table->horizontalScrollBar())
				table->removeRow(--item_count);
		}

		void show_windowed_image(const QImage& img, const QString& title)
		{
			if (img.isNull())
				return;

			QLabel* canvas = new QLabel();
			canvas->setWindowTitle(title);
			canvas->setObjectName("windowed_image");
			canvas->setPixmap(QPixmap::fromImage(img));
			canvas->setFixedSize(img.size());
			canvas->ensurePolished();
			canvas->show();
		}
	} // utils
} // gui
