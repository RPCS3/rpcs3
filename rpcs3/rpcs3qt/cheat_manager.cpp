#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>

#include <yaml-cpp/yaml.h>

#include "cheat_manager.h"

#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "Emu/CPU/CPUThread.h"

#include "Emu/IdManager.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/PPUFunction.h"

#include "Utilities/StrUtil.h"

LOG_CHANNEL(log_cheat, "Cheat");

cheat_manager_dialog* cheat_manager_dialog::inst = nullptr;

template <>
void fmt_class_string<cheat_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cheat_type value)
	{
		switch (value)
		{
		case cheat_type::unsigned_8_cheat: return "Unsigned 8 bits";
		case cheat_type::unsigned_16_cheat: return "Unsigned 16 bits";
		case cheat_type::unsigned_32_cheat: return "Unsigned 32 bits";
		case cheat_type::unsigned_64_cheat: return "Unsigned 64 bits";
		case cheat_type::signed_8_cheat: return "Signed 8 bits";
		case cheat_type::signed_16_cheat: return "Signed 16 bits";
		case cheat_type::signed_32_cheat: return "Signed 32 bits";
		case cheat_type::signed_64_cheat: return "Signed 64 bits";
		case cheat_type::max: break;
		}

		return unknown;
	});
}

namespace YAML
{
	template <>
	struct convert<cheat_info>
	{
		static bool decode(const Node& node, cheat_info& rhs)
		{
			if (node.size() != 3)
			{
				return false;
			}

			rhs.description = node[0].as<std::string>();
			u64 type64      = 0;
			if (!cfg::try_to_enum_value(&type64, &fmt_class_string<cheat_type>::format, node[1].Scalar()))
				return false;
			if (type64 >= cheat_type_max)
				return false;
			rhs.type       = cheat_type{::narrow<u8>(type64)};
			rhs.red_script = node[2].as<std::string>();
			return true;
		}
	};
} // namespace YAML

YAML::Emitter& operator<<(YAML::Emitter& out, const cheat_info& rhs)
{
	std::string type_formatted;
	fmt::append(type_formatted, "%s", rhs.type);

	out << YAML::BeginSeq << rhs.description << type_formatted << rhs.red_script << YAML::EndSeq;

	return out;
}

bool cheat_info::from_str(const std::string& cheat_line)
{
	auto cheat_vec = fmt::split(cheat_line, {"@@@"}, false);

	s64 val64 = 0;
	if (cheat_vec.size() != 5 || !cfg::try_to_int64(&val64, cheat_vec[2], 0, cheat_type_max - 1))
	{
		log_cheat.fatal("Failed to parse cheat line");
		return false;
	}

	game        = cheat_vec[0];
	description = cheat_vec[1];
	type        = cheat_type{::narrow<u8>(val64)};
	offset      = std::stoul(cheat_vec[3]);
	red_script  = cheat_vec[4];

	return true;
}

std::string cheat_info::to_str() const
{
	std::string cheat_str = game + "@@@" + description + "@@@" + std::to_string(static_cast<u8>(type)) + "@@@" + std::to_string(offset) + "@@@" + red_script + "@@@";
	return cheat_str;
}

cheat_engine::cheat_engine()
{
	try
	{
		YAML::Node yml_cheats = YAML::Load(fs::file{fs::get_config_dir() + cheats_filename, fs::read + fs::create}.to_string());

		for (const auto& yml_cheat : yml_cheats)
		{
			const std::string& game_name = yml_cheat.first.Scalar();
			for (const auto& yml_offset : yml_cheat.second)
			{
				const u32 offset          = yml_offset.first.as<u32>();
				cheat_info cheat          = yml_offset.second.as<cheat_info>();
				cheat.game                = game_name;
				cheat.offset              = offset;
				cheats[game_name][offset] = std::move(cheat);
			}
		}
	}
	catch (YAML::Exception& e)
	{
		log_cheat.error("Error parsing %s\n%s", cheats_filename, e.what());
	}
}

void cheat_engine::save() const
{
	fs::file cheat_file(fs::get_config_dir() + cheats_filename, fs::rewrite);
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
		for (const auto& offset : cheats.at(game.first))
		{
			cheats_str += offset.second.to_str();
			cheats_str += "^^^";
		}
	}

	return cheats_str;
}

bool cheat_engine::exist(const std::string& name, const u32 offset) const
{
	if (cheats.count(name) && cheats.at(name).count(offset))
		return true;

	return false;
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
		ASSERT(false);
	};

	operand cur_op = operand_equal;
	u32 index      = 0;

	while (index < red_script.size())
	{
		if (red_script[index] >= '0' && red_script[index] <= '9')
		{
			std::string num_string;
			for (; index < red_script.size(); index++)
			{
				if (red_script[index] < '0' || red_script[index] > '9')
					break;

				num_string += red_script[index];
			}

			u32 num_value = std::stoul(num_string);
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
				u32 res_value = get_value<u32>(res_addr, success);

				if (!success)
					return false;

				do_operation(cur_op, final_offset, res_value);
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

	be_t<T> value_swapped = value;

	if (Emu.IsStopped())
		return {};

	cpu_thread::suspend_all cpu_lock(nullptr);

	if (to_filter.size())
	{
		for (const auto& off : to_filter)
		{
			if (vm::check_addr(off, sizeof(T)))
			{
				if (*vm::get_super_ptr<T>(off) == value_swapped)
					results.push_back(off);
			}
		}
	}
	else
	{
		// Looks through mapped memory
		for (u32 page_ind = (0x10000 / 4096); page_ind < (0xF0000000 / 4096); page_ind++)
		{
			if (vm::check_addr(page_ind * 4096))
			{
				// Assumes the values are aligned
				for (u32 index = 0; index < 4096; index += sizeof(T))
				{
					if (*vm::get_super_ptr<T>((page_ind * 4096) + index) == value_swapped)
						results.push_back((page_ind * 4096) + index);
				}
			}
		}
	}

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

	cpu_thread::suspend_all cpu_lock(nullptr);

	if (!vm::check_addr(offset, sizeof(T)))
	{
		success = false;
		return 0;
	}

	success = true;

	T ret_value = *vm::get_super_ptr<T>(offset);

	return ret_value;
}

template <typename T>
bool cheat_engine::set_value(const u32 offset, const T value)
{
	if (Emu.IsStopped())
		return false;

	cpu_thread::suspend_all cpu_lock(nullptr);

	if (!vm::check_addr(offset, sizeof(T)))
	{
		return false;
	}

	*vm::get_super_ptr<T>(offset) = value;

	const bool exec_code_at_start = vm::check_addr(offset, 1, vm::page_executable);
	const bool exec_code_at_end = [&]()
	{
		if constexpr (sizeof(T) == 1)
		{
			return exec_code_at_start;
		}
		else
		{
			return vm::check_addr(offset + sizeof(T) - 1, 1, vm::page_executable);
		}
	}();

	if (exec_code_at_end || exec_code_at_start)
	{
		extern void ppu_register_function_at(u32, u32, ppu_function_t);

		u32 addr = offset, size = sizeof(T);

		if (exec_code_at_end && exec_code_at_start)
		{
			size = align<u32>(addr + size, 4) - (addr & -4);
			addr &= -4;
		}
		else if (exec_code_at_end)
		{
			size -= align<u32>(size - 4096 + (addr & 4095), 4);
			addr = align<u32>(addr, 4096);
		}
		else if (exec_code_at_start)
		{
			size = align<u32>(4096 - (addr & 4095), 4);
			addr &= -4;
		}

		// Reinitialize executable code
		ppu_register_function_at(addr, size, nullptr);
	}

	return true;
}

bool cheat_engine::is_addr_safe(const u32 offset)
{
	if (Emu.IsStopped())
		return false;

	const auto ppum = g_fxo->get<ppu_module>();
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
			segs.push_back({seg.addr, seg.size});
		}
	}

	if (!segs.size())
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
	u32 result;

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
		if (ptrs.size() && cur_depth < max_depth)
		{
			for (const auto& ptr : ptrs)
			{
				result = reverse_lookup(ptr, max_offset, max_depth, cur_depth + 1);
				if (result)
					return result;
			}
		}
	}

	return 0;
}

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
	btn_filter_results                    = new QPushButton(tr("Filter Results"));
	btn_filter_results->setEnabled(false);
	edt_cheat_search_value = new QLineEdit();
	cbx_cheat_search_type  = new QComboBox();

	for (u64 i = 0; i < cheat_type_max; i++)
	{
		std::string type_formatted;
		fmt::append(type_formatted, "%s", static_cast<cheat_type>(i));
		cbx_cheat_search_type->insertItem(i, QString::fromStdString(type_formatted));
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
	connect(tbl_cheats, &QTableWidget::itemClicked, [=](QTableWidgetItem* item)
	{
		if (!item)
			return;

		const int row = item->row();

		if (row == -1)
			return;

		cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, 0)->text().toStdString(), tbl_cheats->item(row, 3)->data(Qt::UserRole).toUInt());
		if (cheat)
		{
			QString cur_value;
			bool success;

			u32 final_offset;
			if (cheat->red_script.size())
			{
				final_offset = 0;
				if (!cheat_engine::resolve_script(final_offset, cheat->offset, cheat->red_script))
				{
					btn_apply->setEnabled(false);
					edt_value_final->setText(tr("Failed to resolve redirection script"));
				}
			}
			else
			{
				final_offset = cheat->offset;
			}

			u64 result_value;
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
			default: log_cheat.fatal("Unsupported cheat type"); return;
			}

			if (success)
			{
				if (cheat->type >= cheat_type::signed_8_cheat && cheat->type <= cheat_type::signed_64_cheat)
					cur_value = tr("%1").arg(static_cast<s64>(result_value));
				else
					cur_value = tr("%1").arg(result_value);

				btn_apply->setEnabled(true);
			}
			else
			{
				cur_value = tr("Failed to get the value from memory");
				btn_apply->setEnabled(false);
			}

			edt_value_final->setText(cur_value);
		}
		else
		{
			log_cheat.fatal("Failed to retrieve cheat selected from internal cheat_engine");
		}
	});

	connect(tbl_cheats, &QTableWidget::cellChanged, [=](int row, int column)
	{
		if (column != 1 && column != 4)
		{
			log_cheat.fatal("A column other than description and script was edited");
			return;
		}

		cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, 0)->text().toStdString(), tbl_cheats->item(row, 3)->data(Qt::UserRole).toUInt());
		if (!cheat)
		{
			log_cheat.fatal("Failed to retrieve cheat edited from internal cheat_engine");
			return;
		}

		switch (column)
		{
		case 1: cheat->description = tbl_cheats->item(row, 1)->text().toStdString(); break;
		case 4: cheat->red_script = tbl_cheats->item(row, 4)->text().toStdString(); break;
		default: break;
		}

		g_cheat.save();
	});

	connect(tbl_cheats, &QTableWidget::customContextMenuRequested, [=](const QPoint& loc)
	{
		QPoint globalPos       = tbl_cheats->mapToGlobal(loc);
		QMenu* menu            = new QMenu();
		QAction* delete_cheats = new QAction(tr("Delete"), menu);
		QAction* import_cheats = new QAction(tr("Import Cheats"));
		QAction* export_cheats = new QAction(tr("Export Cheats"));
		QAction* reverse_cheat = new QAction(tr("Reverse-Lookup Cheat"));

		connect(delete_cheats, &QAction::triggered, [=]()
		{
			const auto selected = tbl_cheats->selectedItems();

			std::set<int> rows;

			for (const auto& sel : selected)
			{
				const int row = sel->row();

				if (rows.count(row))
					continue;

				g_cheat.erase(tbl_cheats->item(row, 0)->text().toStdString(), tbl_cheats->item(row, 3)->data(Qt::UserRole).toUInt());
				rows.insert(row);
			}

			update_cheat_list();
		});

		connect(import_cheats, &QAction::triggered, [=]()
		{
			QClipboard* clipboard = QGuiApplication::clipboard();
			g_cheat.import_cheats_from_str(clipboard->text().toStdString());
			update_cheat_list();
		});

		connect(export_cheats, &QAction::triggered, [=]()
		{
			const auto selected = tbl_cheats->selectedItems();

			std::set<int> rows;
			std::string export_string;

			for (const auto& sel : selected)
			{
				const int row = sel->row();

				if (rows.count(row))
					continue;

				cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, 0)->text().toStdString(), tbl_cheats->item(row, 3)->data(Qt::UserRole).toUInt());
				if (cheat)
					export_string += cheat->to_str() + "^^^";

				rows.insert(row);
			}

			QClipboard* clipboard = QGuiApplication::clipboard();
			clipboard->setText(QString::fromStdString(export_string));
		});

		connect(reverse_cheat, &QAction::triggered, [=]()
		{
			QTableWidgetItem* item = tbl_cheats->item(tbl_cheats->currentRow(), 3);
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

	connect(btn_apply, &QPushButton::clicked, [=](bool /*checked*/)
	{
		const int row     = tbl_cheats->currentRow();
		cheat_info* cheat = g_cheat.get(tbl_cheats->item(row, 0)->text().toStdString(), tbl_cheats->item(row, 3)->data(Qt::UserRole).toUInt());

		if (!cheat)
		{
			log_cheat.fatal("Failed to retrieve cheat selected from internal cheat_engine");
			return;
		}

		std::pair<bool, bool> results;

		u32 final_offset;
		if (cheat->red_script.size())
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
	connect(btn_new_search, &QPushButton::clicked, [=](bool /*checked*/)
	{
		offsets_found.clear();
		do_the_search();
	});

	connect(btn_filter_results, &QPushButton::clicked, [=](bool /*checked*/) { do_the_search(); });

	connect(lst_search, &QListWidget::customContextMenuRequested, [=](const QPoint& loc)
	{
		QPoint globalPos      = lst_search->mapToGlobal(loc);
		QListWidgetItem* item = lst_search->item(lst_search->currentRow());
		if (!item)
			return;

		QMenu* menu = new QMenu();

		QAction* add_to_cheat_list = new QAction(tr("Add to cheat list"), menu);

		const u32 offset       = offsets_found[lst_search->currentRow()];
		const cheat_type type  = static_cast<cheat_type>(cbx_cheat_search_type->currentIndex());
		const std::string name = Emu.GetTitle();

		connect(add_to_cheat_list, &QAction::triggered, [=]()
		{
			if (g_cheat.exist(name, offset))
			{
				if (QMessageBox::question(this, tr("Cheat already exist"), tr("Do you want to overwrite the existing cheat?"), QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok)
					return;
			}

			std::string comment;
			if (!cheat_engine::is_addr_safe(offset))
				comment = "Unsafe";

			g_cheat.add(name, comment, type, offset, "");
			update_cheat_list();
		});

		menu->addAction(add_to_cheat_list);
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
T cheat_manager_dialog::convert_from_QString(QString& str, bool& success)
{
	T result;

	if constexpr (std::is_same<T, u8>::value)
	{
		u16 result_16 = str.toUShort(&success);

		if (result_16 > 0xFF)
			success = false;

		result = static_cast<T>(result_16);
	}

	if constexpr (std::is_same<T, u16>::value)
		result = str.toUShort(&success);

	if constexpr (std::is_same<T, u32>::value)
		result = str.toUInt(&success);

	if constexpr (std::is_same<T, u64>::value)
		result = str.toULongLong(&success);

	if constexpr (std::is_same<T, s8>::value)
	{
		s16 result_16 = str.toShort(&success);
		if (result_16 < -128 || result_16 > 127)
			success = false;

		result = static_cast<T>(result_16);
	}

	if constexpr (std::is_same<T, s16>::value)
		result = str.toShort(&success);

	if constexpr (std::is_same<T, s32>::value)
		result = str.toInt(&success);

	if constexpr (std::is_same<T, s64>::value)
		result = str.toLongLong(&success);

	return result;
}

template <typename T>
bool cheat_manager_dialog::convert_and_search()
{
	T value;
	bool res_conv;
	QString to_search = edt_cheat_search_value->text();

	value = convert_from_QString<T>(to_search, res_conv);

	if (!res_conv)
		return false;

	offsets_found = cheat_engine::search(value, offsets_found);
	return true;
}

template <typename T>
std::pair<bool, bool> cheat_manager_dialog::convert_and_set(u32 offset)
{
	T value;
	bool res_conv;
	QString to_set = edt_value_final->text();

	value = convert_from_QString<T>(to_set, res_conv);

	if (!res_conv)
		return {false, false};

	return {true, cheat_engine::set_value(offset, value)};
}

void cheat_manager_dialog::do_the_search()
{
	bool res_conv          = false;

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
	default: log_cheat.fatal("Unsupported cheat type"); break;
	}

	if (!res_conv)
	{
		QMessageBox::warning(this, tr("Error converting value"), tr("Couldn't convert the search value you typed to the integer type you selected"), QMessageBox::Ok);
		return;
	}

	lst_search->clear();
	for (u32 row = 0; row < offsets_found.size(); row++)
	{
		lst_search->insertItem(row, tr("0x%1").arg(offsets_found[row], 1, 16).toUpper());
	}
	btn_filter_results->setEnabled(offsets_found.size());
}

void cheat_manager_dialog::update_cheat_list()
{
	size_t num_rows = 0;
	for (const auto& name : g_cheat.cheats)
		num_rows += g_cheat.cheats[name.first].size();

	tbl_cheats->setRowCount(::narrow<int>(num_rows));

	u32 row = 0;
	{
		const QSignalBlocker blocker(tbl_cheats);
		for (const auto& game : g_cheat.cheats)
		{
			for (const auto& offset : g_cheat.cheats[game.first])
			{
				QTableWidgetItem* item_game = new QTableWidgetItem(QString::fromStdString(offset.second.game));
				item_game->setFlags(item_game->flags() & ~Qt::ItemIsEditable);
				tbl_cheats->setItem(row, 0, item_game);

				tbl_cheats->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(offset.second.description)));

				std::string type_formatted;
				fmt::append(type_formatted, "%s", offset.second.type);
				QTableWidgetItem* item_type = new QTableWidgetItem(QString::fromStdString(type_formatted));
				item_type->setFlags(item_type->flags() & ~Qt::ItemIsEditable);
				tbl_cheats->setItem(row, 2, item_type);

				QTableWidgetItem* item_offset = new QTableWidgetItem(tr("0x%1").arg(offset.second.offset, 1, 16).toUpper());
				item_offset->setData(Qt::UserRole, QVariant(offset.second.offset));
				item_offset->setFlags(item_offset->flags() & ~Qt::ItemIsEditable);
				tbl_cheats->setItem(row, 3, item_offset);

				tbl_cheats->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(offset.second.red_script)));

				row++;
			}
		}
	}

	g_cheat.save();
}
