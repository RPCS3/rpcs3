#include "qt_utils.h"
#include <QApplication>
#include <QBitmap>
#include <QDesktopServices>
#include <QFontMetrics>
#include <QPainter>
#include <QProcess>
#include <QScreen>
#include <QUrl>

#include "Emu/system_utils.hpp"
#include "Utilities/File.h"

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

namespace gui
{
	namespace utils
	{
		QRect create_centered_window_geometry(const QScreen* screen, const QRect& base, s32 target_width, s32 target_height)
		{
			ensure(screen);

			// Get minimum virtual screen x & y for clamping the
			// window x & y later while taking the width and height
			// into account, so they don't go offscreen
			const QRect screen_geometry = screen->availableGeometry();
			const s32 min_screen_x = screen_geometry.x();
			const s32 max_screen_x = screen_geometry.x() + screen_geometry.width() - target_width;
			const s32 min_screen_y = screen_geometry.y();
			const s32 max_screen_y = screen_geometry.y() + screen_geometry.height() - target_height;

			const s32 frame_x_raw = base.left() + ((base.width() - target_width) / 2);
			const s32 frame_y_raw = base.top() + ((base.height() - target_height) / 2);

			const s32 frame_x = std::clamp(frame_x_raw, min_screen_x, max_screen_x);
			const s32 frame_y = std::clamp(frame_y_raw, min_screen_y, max_screen_y);

			return QRect(frame_x, frame_y, target_width, target_height);
		}

		QPixmap get_colorized_pixmap(const QPixmap& old_pixmap, const QColor& old_color, const QColor& new_color, bool use_special_masks, bool colorize_all)
		{
			QPixmap pixmap = old_pixmap;

			if (colorize_all)
			{
				const QBitmap mask = pixmap.createMaskFromColor(Qt::transparent, Qt::MaskInColor);
				pixmap.fill(new_color);
				pixmap.setMask(mask);
				return pixmap;
			}

			const QBitmap mask = pixmap.createMaskFromColor(old_color, Qt::MaskOutColor);
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

				const QColor white_color(Qt::white);
				QPixmap white_pixmap = old_pixmap;
				const QBitmap white_mask = white_pixmap.createMaskFromColor(white_color, Qt::MaskOutColor);
				white_pixmap.fill(white_color);
				white_pixmap.setMask(white_mask);

				QPainter painter(&pixmap);
				painter.setRenderHint(QPainter::SmoothPixmapTransform);
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
		}

		QFont get_label_font(const QString& object_name)
		{
			QLabel dummy_font;
			dummy_font.setObjectName(object_name);
			dummy_font.ensurePolished();
			return dummy_font.font();
		}

		int get_label_width(const QString& text, const QFont* font)
		{
			QLabel l(text);
			if (font) l.setFont(*font);
			return l.sizeHint().width();
		}

		QImage get_opaque_image_area(const QString& path)
		{
			QImage image = QImage(path);

			int w_min = 0;
			int w_max = image.width();
			int h_min = 0;
			int h_max = image.height();

			for (int y = 0; y < image.height(); ++y)
			{
				QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
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
			const QFontMetrics font_metrics(combo->font());

			for (int i = 0; i < combo->count(); ++i)
			{
				max_width = std::max(max_width, font_metrics.horizontalAdvance(combo->itemText(i)));
			}

			if (combo->view()->minimumWidth() < max_width)
			{
				// add scrollbar width and margin
				max_width += combo->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
				max_width += combo->view()->autoScrollMargin();
				combo->view()->setMinimumWidth(max_width);
			}
		}

		void update_table_item_count(QTableWidget* table)
		{
			if (!table)
				return;

			int item_count = table->rowCount();
			const bool is_empty = item_count < 1;
			if (is_empty)
				table->insertRow(0);

			const int item_height = table->rowHeight(0);
			if (is_empty)
			{
				table->clearContents();
				table->setRowCount(0);
			}

			const int available_height = table->rect().height() - table->horizontalHeader()->height() - table->frameWidth() * 2;
			if (available_height < item_height || item_height < 1)
				return;

			const int new_item_count = available_height / item_height;
			if (new_item_count == item_count)
				return;

			item_count = new_item_count;
			table->clearContents();
			table->setRowCount(0);

			for (int i = 0; i < item_count; ++i)
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

		// Loads the app icon from path and embeds it centered into an empty square icon
		QIcon get_app_icon_from_path(const std::string& path, const std::string& title_id)
		{
			// Try to find custom icon first
			std::string icon_path = fs::get_config_dir() + "/Icons/game_icons/" + title_id + "/ICON0.PNG";
			bool found_file       = fs::is_file(icon_path);

			if (!found_file)
			{
				// Get Icon for the gs_frame from path. this handles presumably all possible use cases
				const QString qpath = qstr(path);
				const std::string path_list[] = { path, sstr(qpath.section("/", 0, -2, QString::SectionIncludeTrailingSep)),
					                              sstr(qpath.section("/", 0, -3, QString::SectionIncludeTrailingSep)) };

				for (const std::string& pth : path_list)
				{
					if (!fs::is_dir(pth))
					{
						continue;
					}

					const std::string sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(pth, title_id);
					icon_path = sfo_dir + "/ICON0.PNG";
					found_file = fs::is_file(icon_path);

					if (found_file)
					{
						break;
					}
				}
			}

			if (found_file)
			{
				// load the image from path. It will most likely be a rectangle
				const QImage source = QImage(qstr(icon_path));
				const int edge_max = std::max(source.width(), source.height());

				// create a new transparent image with square size and same format as source (maybe handle other formats than RGB32 as well?)
				const QImage::Format format = source.format() == QImage::Format_RGB32 ? QImage::Format_ARGB32 : source.format();
				QImage dest(edge_max, edge_max, format);
				dest.fill(Qt::transparent);

				// get the location to draw the source image centered within the dest image.
				const QPoint dest_pos = source.width() > source.height() ? QPoint(0, (source.width() - source.height()) / 2) : QPoint((source.height() - source.width()) / 2, 0);

				// Paint the source into/over the dest
				QPainter painter(&dest);
				painter.setRenderHint(QPainter::SmoothPixmapTransform);
				painter.drawImage(dest_pos, source);
				painter.end();

				return QIcon(QPixmap::fromImage(dest));
			}

			// if nothing was found reset the icon to default
			return QApplication::windowIcon();
		}

		void open_dir(const std::string& spath)
		{
			fs::create_dir(spath);
			const QString path = qstr(spath);

			if (fs::is_file(spath))
			{
				// open directory and select file
				// https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
#ifdef _WIN32
				QProcess::startDetached("explorer.exe", { "/select,", QDir::toNativeSeparators(path) });
#elif defined(__APPLE__)
				QProcess::execute("/usr/bin/osascript", { "-e", "tell application \"Finder\" to reveal POSIX file \"" + path + "\"" });
				QProcess::execute("/usr/bin/osascript", { "-e", "tell application \"Finder\" to activate" });
#else
				// open parent directory
				QDesktopServices::openUrl(QUrl::fromLocalFile(qstr(fs::get_parent_dir(spath))));
#endif
				return;
			}

			QDesktopServices::openUrl(QUrl::fromLocalFile(path));
		}

		void open_dir(const QString& path)
		{
			open_dir(sstr(path));
		}

		QTreeWidgetItem* find_child(QTreeWidgetItem* parent, const QString& text)
		{
			if (parent)
			{
				for (int i = 0; i < parent->childCount(); i++)
				{
					if (parent->child(i)->text(0) == text)
					{
						return parent->child(i);
					}
				}
			}
			return nullptr;
		}

		QList<QTreeWidgetItem*> find_children_by_data(QTreeWidgetItem* parent, const QList<QPair<int /*role*/, QVariant /*data*/>>& criteria, bool recursive)
		{
			QList<QTreeWidgetItem*> list;

			if (parent)
			{
				for (int i = 0; i < parent->childCount(); i++)
				{
					if (auto item = parent->child(i))
					{
						bool match = true;

						for (const auto& [role, data] : criteria)
						{
							if (item->data(0, role) != data)
							{
								match = false;
								break;
							}
						}

						if (match)
						{
							list << item;
						}

						if (recursive)
						{
							list << find_children_by_data(item, criteria, recursive);
						}
					}
				}
			}

			return list;
		}

		QTreeWidgetItem* add_child(QTreeWidgetItem *parent, const QString& text, int column)
		{
			if (parent)
			{
				QTreeWidgetItem *tree_item = new QTreeWidgetItem();
				tree_item->setText(column, text);
				parent->addChild(tree_item);
				return tree_item;
			}
			return nullptr;
		};

		void remove_children(QTreeWidgetItem* parent)
		{
			if (parent)
			{
				for (int i = parent->childCount() - 1; i >= 0; i--)
				{
					parent->removeChild(parent->child(i));
				}
			}
		}

		void remove_children(QTreeWidgetItem* parent, const QList<QPair<int /*role*/, QVariant /*data*/>>& criteria, bool recursive)
		{
			if (parent)
			{
				for (int i = parent->childCount() - 1; i >= 0; i--)
				{
					if (const auto item = parent->child(i))
					{
						bool match = true;

						for (const auto& [role, data] : criteria)
						{
							if (item->data(0, role) != data)
							{
								match = false;
								break;
							}
						}

						if (!match)
						{
							parent->removeChild(item);
						}
						else if (recursive)
						{
							remove_children(item, criteria, recursive);
						}
					}
				}
			}
		}

		void sort_tree_item(QTreeWidgetItem* item, Qt::SortOrder sort_order, bool recursive)
		{
			if (item)
			{
				item->sortChildren(0, sort_order);

				if (recursive)
				{
					for (int i = 0; i < item->childCount(); i++)
					{
						sort_tree_item(item->child(i), sort_order, recursive);
					}
				}
			}
		}

		void sort_tree(QTreeWidget* tree, Qt::SortOrder sort_order, bool recursive)
		{
			if (tree)
			{
				tree->sortByColumn(0, sort_order);

				if (recursive)
				{
					for (int i = 0; i < tree->topLevelItemCount(); i++)
					{
						sort_tree_item(tree->topLevelItem(i), sort_order, recursive);
					}
				}
			}
		}
	} // utils
} // gui
