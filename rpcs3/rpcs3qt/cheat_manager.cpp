#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>

#include "cheat_manager.h"
#include "memory_viewer_panel.h"

#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "Emu/CPU/CPUThread.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/PPUInterpreter.h"
#include "Emu/Cell/lv2/sys_sync.h"

#include "util/yaml.hpp"
#include "util/asm.hpp"
#include "util/logs.hpp"
#include "util/to_endian.hpp"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "Utilities/bin_patch.h" // get_patches_path()

LOG_CHANNEL(log_cheat, "Cheat");

cheat_manager_dialog* cheat_manager_dialog::inst = nullptr;

YAML::Emitter& operator<<(YAML::Emitter& out, const cheat_info& rhs)
{
	std::string type_formatted;
	fmt::append(type_formatted, "%s", rhs.type);

	out << YAML::BeginSeq << rhs.description << type_formatted << rhs.red_script << YAML::EndSeq;

	return out;
}

cheat_engine::cheat_engine()
{
	const std::string patches_path = patch_engine::get_patches_path();

	if (!fs::create_path(patches_path))
	{
		log_cheat.fatal("Failed to create path: %s (%s)", patches_path, fs::g_tls_error);
		return;
	}

	const std::string path = patches_path + m_cheats_filename;

	if (fs::file cheat_file{path, fs::read + fs::create})
	{
		auto [yml_cheats, error] = yaml_load(cheat_file.to_string());

		if (!error.empty())
		{
			log_cheat.error("Error parsing %s: %s", path, error);
			return;
		}

		for (const auto& yml_cheat : yml_cheats)
		{
			const std::string& game_name = yml_cheat.first.Scalar();

			for (const auto& yml_offset : yml_cheat.second)
			{
				const u32 offset = get_yaml_node_value<u32>(yml_offset.first, error);
				if (!error.empty())
				{
					log_cheat.error("Error parsing %s: node key %s is not a u32 offset", path, yml_offset.first.Scalar());
					return;
				}

				cheat_info cheat = get_yaml_node_value<cheat_info>(yml_offset.second, error);
				if (!error.empty())
				{
					log_cheat.error("Error parsing %s: node %s is not a cheat_info node", path, yml_offset.first.Scalar());
					return;
				}

				cheat.game                = game_name;
				cheat.offset              = offset;
				cheats[game_name][offset] = std::move(cheat);
			}
		}
	}
	else
	{
		log_cheat.error("Error loading %s", path);
	}
}

void cheat_engine::save() const
{
	const std::string patches_path = patch_engine::get_patches_path();

	if (!fs::create_path(patches_path))
	{
		log_cheat.fatal("Failed to create path: %s (%s)", patches_path, fs::g_tls_error);
		return;
	}

	const std::string path = patches_path + m_cheats_filename;

	fs::file cheat_file(path, fs::rewrite);
	if (!cheat_file)
		return;

	YAML::Emitter out;

	out << YAML::BeginMap;
	for (const auto& game_entry : cheats)
	{
		out << game_entry.first;
		out << YAML::BeginMap;
		for (const auto& offset_entry : game_entry.second)
		{
			out << YAML::Hex << offset_entry.first;
			out << offset_entry.second;
		}
		out << YAML::EndMap;
	}
	out << YAML::EndMap;

	cheat_file.write(out.c_str(), out.size());
}

void cheat_engine::import_cheats_from_str(const std::string& str_cheats)
{
	auto cheats_vec = fmt::split(str_cheats, {"^^^"});

	for (auto& cheat_line : cheats_vec)
	{
		cheat_info new_cheat;
		if (new_cheat.from_str(cheat_line))
			cheats[new_cheat.game][new_cheat.offset] = new_cheat;
	}
}

std::string cheat_engine::export_cheats_to_str() const
{
	std::string cheats_str;

	for (const auto& game : cheats)
	{
		for (const auto& offset : ::at32(cheats, game.first))
		{
			cheats_str += offset.second.to_str();
			cheats_str += "^^^";
		}
	}

	return cheats_str;
}

bool cheat_engine::exist(const std::string& game, const u32 offset) const
{
	return cheats.contains(game) && ::at32(cheats, game).contains(offset);
}

void cheat_engine::add(const std::string& game, const std::string& description, const cheat_type type, const u32 offset, const std::string& red_script)
{
	cheats[game][offset] = cheat_info{game, description, type, offset, red_script};
}

cheat_info* cheat_engine::get(const std::string& game, const u32 offset)
{
	if (!exist(game, offset))
		return nullptr;

	return &cheats[game][offset];
}

bool cheat_engine::erase(const std::string& game, const u32 offset)
{
	if (!exist(game, offset))
		return false;

	cheats[game].erase(offset);
	return true;
}

bool cheat_engine::resolve_script(u32& final_offset, const u32 offset, const std::string& red_script)
{
	enum operand
	{
		operand_equal,
		operand_add,
		operand_sub
	};

	auto do_operation = [](const operand op, u32& param1, const u32 param2) -> u32
	{
		switch (op)
		{
		case operand_equal: return param1 = param2;
		case operand_add: return param1 += param2;
		case operand_sub: return param1 -= param2;
		}

		return ensure(0);
	};

	operand cur_op = operand_equal;
	u32 index      = 0;

	while (index < red_script.size())
	{
		if (std::isdigit(static_cast<u8>(red_script[index])))
		{
			std::string num_string;
			for (; index < red_script.size(); index++)
			{
				if (!std::isdigit(static_cast<u8>(red_script[index])))
					break;

				num_string += red_script[index];
			}

			const u32 num_value = std::stoul(num_string);
			do_operation(cur_op, final_offset, num_value);
		}
		else
		{
			switch (red_script[index])
			{
			case '$':
			{
				do_operation(cur_op, final_offset, offset);
				index++;
				break;
			}
			case '[':
			{
				// find corresponding ]
				s32 found_close = 1;
				std::string sub_script;
				for (index++; index < red_script.size(); index++)
				{
					if (found_close == 0)
						break;

					if (red_script[index] == ']')
						found_close--;
					else if (red_script[index] == '[')
						found_close++;

					if (found_close != 0)
						sub_script += red_script[index];
				}

				if (found_close)
					return false;

				// Resolves content of []
				u32 res_addr = 0;
				if (!resolve_script(res_addr, offset, sub_script))
					return false;

				// Tries to get value at resolved address
				bool success;
				const u32 res_value = get_value<u32>(res_addr, success);

				if (!success)
					return false;

				do_operation(cur_op, final_offset, res_value);
				break;
			}
			case '+':
				cur_op = operand_add;
				index++;
				break;
			case '-':
				cur_op = operand_sub;
				index++;
				break;
			case ' ': index++; break;
			default: log_cheat.fatal("invalid character in redirection script"); return false;
			}
		}
	}

	return true;
}

template <typename T>
std::vector<u32> cheat_engine::search(const T value, const std::vector<u32>& to_filter)
{
	std::vector<u32> results;

	to_be_t<T> value_swapped = value;

	if (Emu.IsStopped())
		return {};

	cpu_thread::suspend_all(nullptr, {}, [&]
	{
		if (!to_filter.empty())
		{
			for (const auto& off : to_filter)
			{
				if (vm::check_addr<sizeof(T)>(off))
				{
					if (*vm::get_super_ptr<T>(off) == value_swapped)
						results.push_back(off);
				}
			}
		}
		else
		{
			// Looks through mapped memory
			for (u32 page_start = 0x10000; page_start < 0xF0000000; page_start += 4096)
			{
				if (vm::check_addr(page_start))
				{
					// Assumes the values are aligned
					for (u32 index = 0; index < 4096; index += sizeof(T))
					{
						if (*vm::get_super_ptr<T>(page_start + index) == value_swapped)
							results.push_back(page_start + index);
					}
				}
			}
		}
	});

	return results;
}

template <typename T>
T cheat_engine::get_value(const u32 offset, bool& success)
{
	if (Emu.IsStopped())
	{
		success = false;
		return 0;
	}

	return cpu_thread::suspend_all(nullptr, {}, [&]() -> T
	{
		if (!vm::check_addr<sizeof(T)>(offset))
		{
			success = false;
			return 0;
		}

		success = true;
		return *vm::get_super_ptr<T>(offset);
	});
}

template <typename T>
bool cheat_engine::set_value(const u32 offset, const T value)
{
	if (Emu.IsStopped())
		return false;

	if (!vm::check_addr<sizeof(T)>(offset))
	{
		return false;
	}

	return cpu_thread::suspend_all(nullptr, {}, [&]
	{
		if (!vm::check_addr<sizeof(T)>(offset))
		{
			return false;
		}

		*vm::get_super_ptr<T>(offset) = value;

		const bool exec_code_at_start = vm::check_addr(offset, vm::page_executable);
		const bool exec_code_at_end = [&]()
		{
			if constexpr (sizeof(T) == 1)
			{
				return exec_code_at_start;
			}
			else
			{
				return vm::check_addr(offset + sizeof(T) - 1, vm::page_executable);
			}
		}();

		if (exec_code_at_end || exec_code_at_start)
		{
			extern void ppu_register_function_at(u32, u32, ppu_intrp_func_t);

			u32 addr = offset, size = sizeof(T);

			if (exec_code_at_end && exec_code_at_start)
			{
				size = utils::align<u32>(addr + size, 4) - (addr & -4);
				addr &= -4;
			}
			else if (exec_code_at_end)
			{
				size -= utils::align<u32>(size - 4096 + (addr & 4095), 4);
				addr = utils::align<u32>(addr, 4096);
			}
			else if (exec_code_at_start)
			{
				size = utils::align<u32>(4096 - (addr & 4095), 4);
				addr &= -4;
			}

			// Reinitialize executable code
			ppu_register_function_at(addr, size, nullptr);
		}

		return true;
	});
}

bool cheat_engine::is_addr_safe(const u32 offset)
{
	if (Emu.IsStopped())
		return false;

	const auto ppum = g_fxo->try_get<main_ppu_module<lv2_obj>>();

	if (!ppum)
	{
		log_cheat.fatal("Failed to get ppu_module");
		return false;
	}

	std::vector<std::pair<u32, u32>> segs;

	for (const auto& seg : ppum->segs)
	{
		if ((seg.flags & 3))
		{
			segs.emplace_back(seg.addr, seg.size);
		}
	}

	if (segs.empty())
	{
		log_cheat.fatal("Couldn't find a +rw-x section");
		return false;
	}

	for (const auto& seg : segs)
	{
		if (offset >= seg.first && offset < (seg.first + seg.second))
			return true;
	}

	return false;
}

u32 cheat_engine::reverse_lookup(const u32 addr, const u32 max_offset, const u32 max_depth, const u32 cur_depth)
{
	for (u32 index = 0; index <= max_offset; index += 4)
	{
		std::vector<u32> ptrs = search(addr - index, {});

		log_cheat.fatal("Found %d pointer(s) for addr 0x%x [offset: %d cur_depth:%d]", ptrs.size(), addr, index, cur_depth);

		for (const auto& ptr : ptrs)
		{
			if (is_addr_safe(ptr))
				return ptr;
		}

		// If depth has not been reached dig deeper
		if (!ptrs.empty() && cur_depth < max_depth)
		{
			for (const auto& ptr : ptrs)
			{
				const u32 result = reverse_lookup(ptr, max_offset, max_depth, cur_depth + 1);
				if (result)
					return result;
			}
		}
	}

	return 0;
}

enum cheat_table_columns : int
{
	title = 0,
	description,
	type,
	offset,
	script
};

cheat_manager_dialog::cheat_manager_dialog(QWidget* parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Cheat Manager"));
	setObjectName("cheat_manager");
	setMinimumSize(QSize(800, 400));

	QVBoxLayout* main_layout = new QVBoxLayout();

	tbl_cheats = new QTableWidget(this);
	tbl_cheats->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	tbl_cheats->setSelectionBehavior(QAbstractItemView::SelectRows);
	tbl_cheats->setContextMenuPolicy(Qt::CustomContextMenu);
	tbl_cheats->setColumnCount(5);
	tbl_cheats->setHorizontalHeaderLabels(QStringList() << tr("Game") << tr("Description") << tr("Type") << tr("Offset") << tr("Script"));
	main_layout->addWidget(tbl_cheats);

	QHBoxLayout* btn_layout = new QHBoxLayout();
	QLabel* lbl_value_final = new QLabel(tr("Current Value:"));
	edt_value_final         = new QLineEdit();
	btn_apply               = new QPushButton(tr("Apply"), this);
	btn_apply->setEnabled(false);
	btn_layout->addWidget(lbl_value_final);
	btn_layout->addWidget(edt_value_final);
	btn_layout->addWidget(btn_apply);
	main_layout->addLayout(btn_layout);

	QGroupBox* grp_add_cheat              = new QGroupBox(tr("Cheat Search"));
	QVBoxLayout* grp_add_cheat_layout     = new QVBoxLayout();
	QHBoxLayout* grp_add_cheat_sub_layout = new QHBoxLayout();
	QPushButton* btn_new_search           = new QPushButton(tr("New Search"));
	btn_new_search->setEnabled(false);
	btn_filter_results                    = new QPushButton(tr("Filter Results"));
	btn_filter_results->setEnabled(false);
	edt_cheat_search_value = new QLineEdit();
	cbx_cheat_search_type  = new QComboBox();

	for (u64 i = 0; i < cheat_type_max; i++)
	{
		const QString item_text = get_localized_cheat_type(static_cast<cheat_type>(i));
		cbx_cheat_search_type->addItem(item_text);
	}
	cbx_cheat_search_type->setCurrentIndex(static_cast<u8>(cheat_type::signed_32_cheat));
	grp_add_cheat_sub_layout->addWidget(btn_new_search);
	grp_add_cheat_sub_layout->addWidget(btn_filter_results);
	grp_add_cheat_sub_layout->addWidget(edt_cheat_search_value);
	grp_add_cheat_sub_layout->addWidget(cbx_cheat_search_type);
	grp_add_cheat_layout->addLayout(grp_add_cheat_sub_layout);
	lst_search = new QListWidget(this);
	lst_search->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	lst_search->setSelectionBehavior(QAbstractItemView::SelectRows);
	lst_search->setContextMenuPolicy(Qt::CustomContextMenu);
	grp_add_cheat_layout->addWidget(lst_search);
	grp_add_cheat->setLayout(grp_add_cheat_layout);
	main_layout->addWidget(grp_add_cheat);

	setLayout(main_layout);

	// Edit/Manage UI
	connect(tbl_cheats, &QTableWidget::itemClicked, [this](QTableWidgetItem* item)
	{
		if (!item)
			return;

		const int row = item->row();

		if (row == -1)
			return;

		cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, cheat_table_columns::title)->text().toStdString(), tbl_cheats->item(row, cheat_table_columns::offset)->data(Qt::UserRole).toUInt());
		if (!cheat)
		{
			log_cheat.fatal("Failed to retrieve cheat selected from internal cheat_engine");
			return;
		}

		u32 final_offset;
		if (!cheat->red_script.empty())
		{
			final_offset = 0;
			if (!cheat_engine::resolve_script(final_offset, cheat->offset, cheat->red_script))
			{
				btn_apply->setEnabled(false);
				edt_value_final->setText(tr("Failed to resolve redirection script"));
				return;
			}
		}
		else
		{
			final_offset = cheat->offset;
		}

		if (Emu.IsStopped())
		{
			btn_apply->setEnabled(false);
			edt_value_final->setText(tr("This Application is not running"));
			return;
		}

		bool success = false;
		u64 result_value {};
		f64 result_value_f {};

		switch (cheat->type)
		{
		case cheat_type::unsigned_8_cheat: result_value = cheat_engine::get_value<u8>(final_offset, success); break;
		case cheat_type::unsigned_16_cheat: result_value = cheat_engine::get_value<u16>(final_offset, success); break;
		case cheat_type::unsigned_32_cheat: result_value = cheat_engine::get_value<u32>(final_offset, success); break;
		case cheat_type::unsigned_64_cheat: result_value = cheat_engine::get_value<u64>(final_offset, success); break;
		case cheat_type::signed_8_cheat: result_value = cheat_engine::get_value<s8>(final_offset, success); break;
		case cheat_type::signed_16_cheat: result_value = cheat_engine::get_value<s16>(final_offset, success); break;
		case cheat_type::signed_32_cheat: result_value = cheat_engine::get_value<s32>(final_offset, success); break;
		case cheat_type::signed_64_cheat: result_value = cheat_engine::get_value<s64>(final_offset, success); break;
		case cheat_type::float_32_cheat: result_value_f = cheat_engine::get_value<f32>(final_offset, success); break;
		default: log_cheat.fatal("Unsupported cheat type"); return;
		}

		if (success)
		{
			if (cheat->type >= cheat_type::signed_8_cheat && cheat->type <= cheat_type::signed_64_cheat)
				edt_value_final->setText(tr("%1").arg(static_cast<s64>(result_value)));
			else if (cheat->type == cheat_type::float_32_cheat)
				edt_value_final->setText(tr("%1").arg(result_value_f));
			else
				edt_value_final->setText(tr("%1").arg(result_value));
		}
		else
		{
			edt_value_final->setText(tr("Failed to get the value from memory"));
		}

		btn_apply->setEnabled(success);
	});

	connect(tbl_cheats, &QTableWidget::cellChanged, [this](int row, int column)
	{
		QTableWidgetItem* item = tbl_cheats->item(row, column);
		if (!item)
		{
			return;
		}

		if (column != cheat_table_columns::description && column != cheat_table_columns::script)
		{
			log_cheat.fatal("A column other than description and script was edited");
			return;
		}

		cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, cheat_table_columns::title)->text().toStdString(), tbl_cheats->item(row, cheat_table_columns::offset)->data(Qt::UserRole).toUInt());
		if (!cheat)
		{
			log_cheat.fatal("Failed to retrieve cheat edited from internal cheat_engine");
			return;
		}

		switch (column)
		{
		case cheat_table_columns::description: cheat->description = item->text().toStdString(); break;
		case cheat_table_columns::script: cheat->red_script = item->text().toStdString(); break;
		default: break;
		}

		g_cheat.save();
	});

	connect(tbl_cheats, &QTableWidget::customContextMenuRequested, [this](const QPoint& loc)
	{
		const QPoint globalPos = tbl_cheats->mapToGlobal(loc);
		QMenu* menu            = new QMenu();
		QAction* delete_cheats = new QAction(tr("Delete"), menu);
		QAction* import_cheats = new QAction(tr("Import Cheats"));
		QAction* export_cheats = new QAction(tr("Export Cheats"));
		QAction* reverse_cheat = new QAction(tr("Reverse-Lookup Cheat"));

		connect(delete_cheats, &QAction::triggered, [this]()
		{
			const auto selected = tbl_cheats->selectedItems();

			std::set<int> rows;

			for (const auto& sel : selected)
			{
				const int row = sel->row();

				if (rows.count(row))
					continue;

				g_cheat.erase(tbl_cheats->item(row, cheat_table_columns::title)->text().toStdString(), tbl_cheats->item(row, cheat_table_columns::offset)->data(Qt::UserRole).toUInt());
				rows.insert(row);
			}

			update_cheat_list();
		});

		connect(import_cheats, &QAction::triggered, [this]()
		{
			QClipboard* clipboard = QGuiApplication::clipboard();
			g_cheat.import_cheats_from_str(clipboard->text().toStdString());
			update_cheat_list();
		});

		connect(export_cheats, &QAction::triggered, [this]()
		{
			const auto selected = tbl_cheats->selectedItems();

			std::set<int> rows;
			std::string export_string;

			for (const auto& sel : selected)
			{
				const int row = sel->row();

				if (rows.count(row))
					continue;

				cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, cheat_table_columns::title)->text().toStdString(), tbl_cheats->item(row, cheat_table_columns::offset)->data(Qt::UserRole).toUInt());
				if (cheat)
					export_string += cheat->to_str() + "^^^";

				rows.insert(row);
			}

			QClipboard* clipboard = QGuiApplication::clipboard();
			clipboard->setText(QString::fromStdString(export_string));
		});

		connect(reverse_cheat, &QAction::triggered, [this]()
		{
			QTableWidgetItem* item = tbl_cheats->item(tbl_cheats->currentRow(), cheat_table_columns::offset);
			if (item)
			{
				const u32 offset = item->data(Qt::UserRole).toUInt();
				const u32 result = cheat_engine::reverse_lookup(offset, 32, 12);

				log_cheat.fatal("Result is 0x%x", result);
			}
		});

		menu->addAction(delete_cheats);
		menu->addSeparator();
		// menu->addAction(reverse_cheat);
		// menu->addSeparator();
		menu->addAction(import_cheats);
		menu->addAction(export_cheats);
		menu->exec(globalPos);
	});

	connect(btn_apply, &QPushButton::clicked, [this](bool /*checked*/)
	{
		const int row     = tbl_cheats->currentRow();
		cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, cheat_table_columns::title)->text().toStdString(), tbl_cheats->item(row, cheat_table_columns::offset)->data(Qt::UserRole).toUInt());

		if (!cheat)
		{
			log_cheat.fatal("Failed to retrieve cheat selected from internal cheat_engine");
			return;
		}

		std::pair<bool, bool> results;

		u32 final_offset;
		if (!cheat->red_script.empty())
		{
			final_offset = 0;
			if (!g_cheat.resolve_script(final_offset, cheat->offset, cheat->red_script))
			{
				btn_apply->setEnabled(false);
				edt_value_final->setText(tr("Failed to resolve redirection script"));
			}
		}
		else
		{
			final_offset = cheat->offset;
		}

		// TODO: better way to do this?
		switch (static_cast<cheat_type>(cbx_cheat_search_type->currentIndex()))
		{
		case cheat_type::unsigned_8_cheat: results = convert_and_set<u8>(final_offset); break;
		case cheat_type::unsigned_16_cheat: results = convert_and_set<u16>(final_offset); break;
		case cheat_type::unsigned_32_cheat: results = convert_and_set<u32>(final_offset); break;
		case cheat_type::unsigned_64_cheat: results = convert_and_set<u64>(final_offset); break;
		case cheat_type::signed_8_cheat: results = convert_and_set<s8>(final_offset); break;
		case cheat_type::signed_16_cheat: results = convert_and_set<s16>(final_offset); break;
		case cheat_type::signed_32_cheat: results = convert_and_set<s32>(final_offset); break;
		case cheat_type::signed_64_cheat: results = convert_and_set<s64>(final_offset); break;
		case cheat_type::float_32_cheat: results = convert_and_set<f32>(final_offset); break;
		default: log_cheat.fatal("Unsupported cheat type"); return;
		}

		if (!results.first)
		{
			QMessageBox::warning(this, tr("Error converting value"), tr("Couldn't convert the value you typed to the integer type of that cheat"), QMessageBox::Ok);
			return;
		}

		if (!results.second)
		{
			QMessageBox::warning(this, tr("Error applying value"), tr("Couldn't patch memory"), QMessageBox::Ok);
			return;
		}
	});

	// Search UI
	connect(btn_new_search, &QPushButton::clicked, [this](bool /*checked*/)
	{
		offsets_found.clear();
		do_the_search();
	});

	connect(edt_cheat_search_value, &QLineEdit::textChanged, this, [btn_new_search, this](const QString& text)
	{
		if (btn_new_search)
		{
			btn_new_search->setEnabled(!text.isEmpty());
		}
		if (btn_filter_results)
		{
			btn_filter_results->setEnabled(!text.isEmpty() && !offsets_found.empty());
		}
	});

	connect(btn_filter_results, &QPushButton::clicked, [this](bool /*checked*/) { do_the_search(); });

	connect(lst_search, &QListWidget::customContextMenuRequested, [this](const QPoint& loc)
	{
		const QPoint globalPos = lst_search->mapToGlobal(loc);
		const int current_row  = lst_search->currentRow();
		QListWidgetItem* item  = lst_search->item(current_row);

		// Skip if the item was a placeholder
		if (!item || item->data(Qt::UserRole).toBool())
			return;

		QMenu* menu = new QMenu();

		QAction* add_to_cheat_list = new QAction(tr("Add to cheat list"), menu);
		QAction* show_in_mem_viewer = new QAction(tr("Show in Memory Viewer"), menu);

		const u32 offset       = offsets_found[current_row];
		const cheat_type type  = static_cast<cheat_type>(cbx_cheat_search_type->currentIndex());

		connect(add_to_cheat_list, &QAction::triggered, [name = Emu.GetTitle(), offset, type, this]()
		{
			if (g_cheat.exist(name, offset))
			{
				if (QMessageBox::question(this, tr("Cheat already exists"), tr("Do you want to overwrite the existing cheat?"), QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
					return;
			}

			std::string comment;
			if (!cheat_engine::is_addr_safe(offset))
				comment = "Unsafe";

			g_cheat.add(name, comment, type, offset, "");
			update_cheat_list();
		});

		connect(show_in_mem_viewer, &QAction::triggered, this, [offset]()
		{
			memory_viewer_panel::ShowAtPC(offset);
		});

		menu->addAction(add_to_cheat_list);
		menu->addAction(show_in_mem_viewer);
		menu->exec(globalPos);
	});

	update_cheat_list();
}

cheat_manager_dialog::~cheat_manager_dialog()
{
	inst = nullptr;
}

cheat_manager_dialog* cheat_manager_dialog::get_dlg(QWidget* parent)
{
	if (inst == nullptr)
		inst = new cheat_manager_dialog(parent);

	return inst;
}

template <typename T>
T cheat_manager_dialog::convert_from_QString(const QString& str, bool& success)
{
	if constexpr (std::is_same_v<T, u8>)
	{
		const u16 result_16 = str.toUShort(&success);

		if (result_16 > 0xFF)
			success = false;

		return static_cast<T>(result_16);
	}

	if constexpr (std::is_same_v<T, u16>)
		return str.toUShort(&success);

	if constexpr (std::is_same_v<T, u32>)
		return str.toUInt(&success);

	if constexpr (std::is_same_v<T, u64>)
		return str.toULongLong(&success);

	if constexpr (std::is_same_v<T, s8>)
	{
		const s16 result_16 = str.toShort(&success);
		if (result_16 < -128 || result_16 > 127)
			success = false;

		return static_cast<T>(result_16);
	}

	if constexpr (std::is_same_v<T, s16>)
		return str.toShort(&success);

	if constexpr (std::is_same_v<T, s32>)
		return str.toInt(&success);

	if constexpr (std::is_same_v<T, s64>)
		return str.toLongLong(&success);

	if constexpr (std::is_same_v<T, f32>)
		return str.toFloat(&success);

	return {};
}

template <typename T>
bool cheat_manager_dialog::convert_and_search()
{
	bool res_conv = false;
	const QString to_search = edt_cheat_search_value->text();

	const T value = convert_from_QString<T>(to_search, res_conv);

	if (!res_conv)
		return false;

	offsets_found = cheat_engine::search(value, offsets_found);
	return true;
}

template <typename T>
std::pair<bool, bool> cheat_manager_dialog::convert_and_set(u32 offset)
{
	bool res_conv = false;
	const QString to_set = edt_value_final->text();

	const T value = convert_from_QString<T>(to_set, res_conv);

	if (!res_conv)
		return {false, false};

	return {true, cheat_engine::set_value(offset, value)};
}

void cheat_manager_dialog::do_the_search()
{
	bool res_conv = false;

	// TODO: better way to do this?
	switch (static_cast<cheat_type>(cbx_cheat_search_type->currentIndex()))
	{
	case cheat_type::unsigned_8_cheat: res_conv = convert_and_search<u8>(); break;
	case cheat_type::unsigned_16_cheat: res_conv = convert_and_search<u16>(); break;
	case cheat_type::unsigned_32_cheat: res_conv = convert_and_search<u32>(); break;
	case cheat_type::unsigned_64_cheat: res_conv = convert_and_search<u64>(); break;
	case cheat_type::signed_8_cheat: res_conv = convert_and_search<s8>(); break;
	case cheat_type::signed_16_cheat: res_conv = convert_and_search<s16>(); break;
	case cheat_type::signed_32_cheat: res_conv = convert_and_search<s32>(); break;
	case cheat_type::signed_64_cheat: res_conv = convert_and_search<s64>(); break;
	case cheat_type::float_32_cheat: res_conv = convert_and_search<f32>(); break;
	default: log_cheat.fatal("Unsupported cheat type"); break;
	}

	if (!res_conv)
	{
		QMessageBox::warning(this, tr("Error converting value"), tr("Couldn't convert the search value you typed to the integer type you selected"), QMessageBox::Ok);
		return;
	}

	lst_search->clear();

	const usz size = offsets_found.size();

	if (size == 0)
	{
		QListWidgetItem* item = new QListWidgetItem(tr("Nothing found"));
		item->setData(Qt::UserRole, true);
		lst_search->insertItem(0, item);
	}
	else if (size > 10000)
	{
		// Only show entries below a fixed amount. Too many entries can take forever to render and fill up memory quickly.
		QListWidgetItem* item = new QListWidgetItem(tr("Too many entries to display (%0)").arg(size));
		item->setData(Qt::UserRole, true);
		lst_search->insertItem(0, item);
	}
	else
	{
		for (u32 row = 0; row < size; row++)
		{
			lst_search->insertItem(row, QString("0x%0").arg(offsets_found[row], 1, 16).toUpper());
		}
	}

	btn_filter_results->setEnabled(!offsets_found.empty() && edt_cheat_search_value && !edt_cheat_search_value->text().isEmpty());
}

void cheat_manager_dialog::update_cheat_list()
{
	usz num_rows = 0;
	for (const auto& name : g_cheat.cheats)
		num_rows += name.second.size();

	tbl_cheats->setRowCount(::narrow<int>(num_rows));

	u32 row = 0;
	{
		const QSignalBlocker blocker(tbl_cheats);
		for (const auto& game : g_cheat.cheats)
		{
			for (const auto& offset : game.second)
			{
				QTableWidgetItem* item_game = new QTableWidgetItem(QString::fromStdString(offset.second.game));
				item_game->setFlags(item_game->flags() & ~Qt::ItemIsEditable);
				tbl_cheats->setItem(row, cheat_table_columns::title, item_game);

				tbl_cheats->setItem(row, cheat_table_columns::description, new QTableWidgetItem(QString::fromStdString(offset.second.description)));

				std::string type_formatted;
				fmt::append(type_formatted, "%s", offset.second.type);
				QTableWidgetItem* item_type = new QTableWidgetItem(QString::fromStdString(type_formatted));
				item_type->setFlags(item_type->flags() & ~Qt::ItemIsEditable);
				tbl_cheats->setItem(row, cheat_table_columns::type, item_type);

				QTableWidgetItem* item_offset = new QTableWidgetItem(QString("0x%0").arg(offset.second.offset, 1, 16).toUpper());
				item_offset->setData(Qt::UserRole, QVariant(offset.second.offset));
				item_offset->setFlags(item_offset->flags() & ~Qt::ItemIsEditable);
				tbl_cheats->setItem(row, cheat_table_columns::offset, item_offset);

				tbl_cheats->setItem(row, cheat_table_columns::script, new QTableWidgetItem(QString::fromStdString(offset.second.red_script)));

				row++;
			}
		}
	}

	g_cheat.save();
}

QString cheat_manager_dialog::get_localized_cheat_type(cheat_type type)
{
	switch (type)
	{
	case cheat_type::unsigned_8_cheat: return tr("Unsigned 8 bits");
	case cheat_type::unsigned_16_cheat: return tr("Unsigned 16 bits");
	case cheat_type::unsigned_32_cheat: return tr("Unsigned 32 bits");
	case cheat_type::unsigned_64_cheat: return tr("Unsigned 64 bits");
	case cheat_type::signed_8_cheat: return tr("Signed 8 bits");
	case cheat_type::signed_16_cheat: return tr("Signed 16 bits");
	case cheat_type::signed_32_cheat: return tr("Signed 32 bits");
	case cheat_type::signed_64_cheat: return tr("Signed 64 bits");
	case cheat_type::float_32_cheat: return tr("Float 32 bits");
	case cheat_type::max: break;
	}
	std::string type_formatted;
	fmt::append(type_formatted, "%s", type);
	return QString::fromStdString(type_formatted);
}
