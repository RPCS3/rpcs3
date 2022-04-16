#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QFileDialog>
#include <QPushButton>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QSplitter>

#include <charconv>
#include <optional>

#include "cheat_manager.h"
#include "Utilities/Config.h"
#include "Utilities/StrUtil.h"
#include "util/logs.hpp"
#include "Emu/System.h"
#include "util/yaml.hpp"
#include "rpcs3/rpcs3qt/qt_utils.h"
#include "Utilities/bin_patch.h" // get_patches_path()

LOG_CHANNEL(log_cheat, "Cheat");

namespace ncl_parser
{
	std::optional<u32> hexstr_to_u32(std::string_view str)
	{
		u32 result;
		if (std::from_chars(str.data(), str.data() + str.size(), result, 16).ec != std::errc())
		{
			log_cheat.warning("Couldn't parse %s into an u32!", str);
			return std::nullopt;
		}

		return result;
	}

	std::optional<cheat_inst> get_type(std::string_view str)
	{
		if (str.empty())
		{
			return std::nullopt;
		}

		switch (str[0])
		{
		case '0':
		{
			auto subtype = hexstr_to_u32(str);
			if (!subtype)
			{
				log_cheat.warning("Failed to parse 0 subtype in '%s'", str);
				return std::nullopt;
			}

			switch (*subtype)
			{
			case 0: return cheat_inst::write_bytes;
			case 1: return cheat_inst::or_bytes;
			case 2: return cheat_inst::and_bytes;
			case 3: return cheat_inst::xor_bytes;
			default: return std::nullopt;
			}
		}
		case '1': return cheat_inst::write_text;
		case '2': return cheat_inst::write_float;
		case '4': return cheat_inst::write_condensed;
		case '6': return cheat_inst::read_pointer;
		case 'A':
		{
			auto subtype = hexstr_to_u32(std::string_view(str.data() + 1, str.size() - 1));
			if (!subtype)
			{
				log_cheat.warning("Failed to parse A subtype in '%s'", str);
				return std::nullopt;
			}

			switch (*subtype)
			{
			case '1': return cheat_inst::copy;
			case '2': return cheat_inst::paste;
			default: return std::nullopt;
			}
		}
		case 'B': return cheat_inst::find_replace;
		case 'D': return cheat_inst::compare_cond;
		case 'E': return cheat_inst::and_compare_cond;
		case 'F': return cheat_inst::copy_bytes;
		default: return std::nullopt;
		}
	}

	std::optional<cheat_code> parse_codeline(std::string_view line, std::optional<cheat_code> current_inst = std::nullopt)
	{
		cheat_code code;

		auto validate_hexzstr = [](std::string_view str) -> bool
		{
			return std::all_of(str.begin(), str.end(), [](char c)
				{
					return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || c == 'Z';
				});
		};

		auto validate_float = [](std::string_view str) -> bool
		{
			// Fails on FreeBSD and MacOS which don't support FP from_chars
			// float result;
			// if (std::from_chars(str.data(), str.data() + str.size(), result).ec != std::errc())
			// {
			// 	return false;
			// }

			char* str_end = nullptr;
			std::strtof(str.data(), &str_end);
			return (str_end == (str.data() + str.size()));
		};

		std::vector<std::string> args = fmt::split(line, {" "});
		if (args.size() < 3)
		{
			return std::nullopt;
		}

		auto type = get_type(args[0]);
		if (!type)
		{
			return std::nullopt;
		}

		// Special processing for 2 lines instructions
		if (current_inst)
		{
			if (current_inst->type != *type)
			{
				log_cheat.error("Expected a second line and found a different instruction: '%s'!", line);
				return std::nullopt;
			}

			if (*type == cheat_inst::write_condensed || *type == cheat_inst::find_replace)
			{
				auto opt1 = validate_hexzstr(args[1]);
				auto opt2 = validate_hexzstr(args[2]);
				if (!opt1 || !opt2)
				{
					log_cheat.error("Failed to parse 2nd line of %s: '%s'!", *type, line);
					return std::nullopt;
				}
				current_inst->opt1 = args[1];
				current_inst->opt2 = args[2];
				return current_inst;
			}
			else
			{
				log_cheat.fatal("Logic error in 2nd line processing!");
				return std::nullopt;
			}
		}

		auto addr = validate_hexzstr(args[1]);
		if (!addr)
		{
			return std::nullopt;
		}

		code.type = *type;
		code.addr = args[1];

		switch (code.type)
		{
		case cheat_inst::write_bytes:
		case cheat_inst::or_bytes:
		case cheat_inst::and_bytes:
		case cheat_inst::xor_bytes:
		case cheat_inst::write_condensed:
		case cheat_inst::read_pointer:
		case cheat_inst::copy:
		case cheat_inst::paste:
		case cheat_inst::find_replace:
		{
			auto value = validate_hexzstr(args[2]);
			if (!value)
			{
				return std::nullopt;
			}
			break;
		}
		case cheat_inst::write_text: break;
		case cheat_inst::write_float:
		{
			auto value = validate_float(args[2]);
			if (!value)
			{
				return std::nullopt;
			}
			break;
		}
		case cheat_inst::compare_cond:
		case cheat_inst::and_compare_cond:
		case cheat_inst::copy_bytes:
		{
			auto num   = validate_hexzstr(std::string_view(args[0].data() + 1, args[0].size() - 1));
			auto value = validate_hexzstr(args[2]);
			if (!num || !value)
			{
				return std::nullopt;
			}
			code.opt1 = args[0].data() + 1;
			break;
		}
		default:
		{
			return std::nullopt;
		}
		}

		code.value = args[2];
		return code;
	}

	std::optional<std::pair<std::string, std::map<std::string, std::string>>> parse_variable(std::string_view str)
	{
		if (str.empty() || str[0] != '[')
		{
			return std::nullopt;
		}

		std::string var_name;
		std::map<std::string, std::string> options;
		std::string cur_name, cur_value;

		auto it = str.begin() + 1;
		for (; it != str.end() && *it == 'Z'; it++)
		{
			var_name += *it;
		}

		if (it == str.end() || var_name.empty() || *it != ']')
		{
			return std::nullopt;
		}

		it++;

		bool value = true;

		for (; it != str.end(); it++)
		{
			if (*it == ';' || *it == '[')
			{
				options.insert_or_assign(std::move(cur_name), std::move(cur_value));
				cur_name  = {};
				cur_value = {};
				value     = true;
				continue;
			}

			if (*it == '=')
			{
				value = false;
				continue;
			}

			auto& to_add = value ? cur_value : cur_name;
			to_add += *it;
		}

		return {{var_name, options}};
	}

	std::optional<std::map<std::string, cheat_entry>> parse_ncl(const std::string& file_to_open)
	{
		std::map<std::string, cheat_entry> entries;

		auto file = fs::file(file_to_open);
		if (!file || !file.size())
		{
			return std::nullopt;
		}

		std::string buf(file.size(), ' ');
		file.read(buf.data(), buf.size());

		const std::vector<std::string> lines = fmt::split(buf, {"\r\n", "\n"});

		std::string title;
		cheat_entry entry;
		u32 cur_line = 0;
		std::optional<cheat_code> two_liner_code{};

		for (const auto& line : lines)
		{
			if (line == "#")
			{
				if (!entry.codes.empty())
				{
					entries.insert_or_assign(std::move(title), std::move(entry));
				}
				title          = {};
				entry          = {};
				cur_line       = 0;
				two_liner_code = std::nullopt;
				continue;
			}

			switch (cur_line)
			{
			case 0:
			{
				title = line;
				break;
			}
			case 1:
			{
				switch (line[0])
				{
				case '0':
				{
					entry.type = cheat_type::normal;
					break;
				}
				case '1':
				case 'T':
				{
					entry.type = cheat_type::constant;
					break;
				}
				default:
				{
					log_cheat.error("Unsupported cheat type: %s", line);
					return std::nullopt;
				}
				}
				break;
			}
			default:
			{
				auto code = parse_codeline(line, two_liner_code);
				if (!code)
				{
					if (cur_line == 2)
					{
						entry.author = line;
						break;
					}

					auto variable = parse_variable(line);
					if (!variable)
					{
						entry.comments.push_back(line);
						break;
					}

					entry.variables.insert_or_assign(variable->first, variable->second);
					break;
				}

				auto type = code->type;
				if (type == cheat_inst::write_condensed || type == cheat_inst::find_replace)
				{
					if (two_liner_code)
					{
						entry.codes.push_back(*code);
						two_liner_code = std::nullopt;
						break;
					}

					two_liner_code = code;
					break;
				}

				entry.codes.push_back(*code);
				break;
			}
			}

			cur_line++;
		}

		return entries;
	}
} // namespace ncl_parser

cheat_variables_dialog::cheat_variables_dialog(const QString& title, const cheat_entry& entry, QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Variables for %0").arg(title));
	setObjectName("cheat_variables_dialog");

	QVBoxLayout* layout_main = new QVBoxLayout();

	QGroupBox* gb_var          = new QGroupBox(tr("Variables:"));
	QVBoxLayout* layout_gb_var = new QVBoxLayout();

	for (const auto& [var_name, var_options] : entry.variables)
	{
		QHBoxLayout* layout_line_and_cb = new QHBoxLayout();
		QLabel* label_var_name          = new QLabel(QString::fromStdString(var_name));
		QComboBox* cb_var_opts          = new QComboBox();

		for (const auto& [var_opt_name, var_opt_value] : var_options)
		{
			cb_var_opts->addItem(QString::fromStdString(var_opt_name), QString::fromStdString(var_opt_value));
		}

		m_combos.push_back({var_name, cb_var_opts});

		layout_line_and_cb->addWidget(label_var_name);
		layout_line_and_cb->addWidget(cb_var_opts);

		layout_gb_var->addLayout(layout_line_and_cb);
	}

	gb_var->setLayout(layout_gb_var);

	layout_main->addWidget(gb_var);

	QDialogButtonBox* btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(btn_box, &QDialogButtonBox::accepted, this, [this]()
		{
			m_var_choices.clear();
			for (const auto& [var_name, combo] : m_combos)
			{
				m_var_choices.insert_or_assign(var_name, combo->currentData().toString().toStdString());
			}

			QDialog::accept();
		});
	connect(btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	layout_main->addWidget(btn_box);

	setLayout(layout_main);
}

std::unordered_map<std::string, std::string> cheat_variables_dialog::get_choices()
{
	return std::move(m_var_choices);
}

cheat_manager_dialog::cheat_manager_dialog(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
	: QDialog(parent), m_gui_settings(std::move(gui_settings))
{
	setWindowTitle(tr("Cheat Manager"));
	setObjectName("cheat_manager");
	setMinimumSize(QSize(800, 400));

	QVBoxLayout* layout_main = new QVBoxLayout();

	QGroupBox* grp_filter = new QGroupBox();
	grp_filter->setAlignment(Qt::AlignLeft);

	QHBoxLayout* layout_grp_filter = new QHBoxLayout();
	QLabel* lbl_filter             = new QLabel(tr("Filter:"));
	m_edt_filter                   = new QLineEdit();
	m_edt_filter->setClearButtonEnabled(true);
	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::VLine);
	line->setFrameShadow(QFrame::Sunken);
	m_chk_search_cur = new QCheckBox(tr("Search Current Game Only"));
	m_chk_search_cur->setCheckState(m_gui_settings->GetValue(gui::ch_active_game).toBool() ? Qt::Checked : Qt::Unchecked);
	layout_grp_filter->addWidget(lbl_filter);
	layout_grp_filter->addWidget(m_edt_filter);
	layout_grp_filter->addWidget(line);
	layout_grp_filter->addWidget(m_chk_search_cur);
	layout_grp_filter->addStretch();

	grp_filter->setLayout(layout_grp_filter);
	layout_main->addWidget(grp_filter);

	QSplitter* splitter_tree_and_desc = new QSplitter(Qt::Horizontal);

	m_tree = new QTreeWidget();
	m_tree->setColumnCount(1);
	m_tree->setHeaderHidden(true);

	QGroupBox* grp_views          = new QGroupBox(tr("Information:"));
	QVBoxLayout* layout_grp_views = new QVBoxLayout();

	QLabel* lbl_author = new QLabel(tr("Author:"));
	m_edt_author       = new QLineEdit();
	m_edt_author->setReadOnly(true);
	QLabel* lbl_comments = new QLabel(tr("Comments:"));
	m_pte_comments       = new QPlainTextEdit();
	m_pte_comments->setReadOnly(true);
	QPushButton* btn_import     = new QPushButton(tr("Import"));
	QPushButton* btn_import_dir = new QPushButton(tr("Import Dir"));
	layout_grp_views->addWidget(lbl_author);
	layout_grp_views->addWidget(m_edt_author);
	layout_grp_views->addWidget(lbl_comments);
	layout_grp_views->addWidget(m_pte_comments);
	layout_grp_views->addWidget(btn_import);
	layout_grp_views->addWidget(btn_import_dir);

	grp_views->setLayout(layout_grp_views);

	splitter_tree_and_desc->addWidget(m_tree);
	splitter_tree_and_desc->addWidget(grp_views);
	splitter_tree_and_desc->setStretchFactor(0, 2);

	layout_main->addWidget(splitter_tree_and_desc);

	setLayout(layout_main);

	connect(m_edt_filter, &QLineEdit::textChanged, this, [this](const QString& term)
		{
			if (!m_chk_search_cur->isChecked())
			{
				filter_cheats("", term);
			}
		});

	connect(m_chk_search_cur, &QCheckBox::stateChanged, this, [this](int state)
		{
			m_gui_settings->SetValue(gui::ch_active_game, state == Qt::Checked);

			if (state == Qt::Checked)
			{
				QString game_id    = QString::fromStdString(Emu.GetTitleID());
				QString game_title = QString::fromStdString(Emu.GetTitle());
				filter_cheats(game_id, game_title);
				return;
			}

			filter_cheats("", m_edt_filter->text());
		});

	connect(btn_import, &QPushButton::clicked, this, [this]([[maybe_unused]] bool checked)
		{
			const QString last_import_dir_path = m_gui_settings->GetValue(gui::ch_import_path).toString();
			const QStringList paths            = QFileDialog::getOpenFileNames(this, tr("Select Artemis cheat files to import"),
						   last_import_dir_path, tr("Artemis files (*.nlc *.NCL);;All files (*.*)"));

			if (paths.isEmpty())
			{
				return;
			}

			m_gui_settings->SetValue(gui::ch_import_path, QFileInfo(paths[0]).path());

			for (const auto& path : paths)
			{
				const QFileInfo file_info(path);
				auto res = ncl_parser::parse_ncl(path.toStdString());
				if (!res)
				{
					continue;
				}

				add_cheats(file_info.completeBaseName().toStdString(), std::move(*res));
			}

			save_cheats();
			refresh_tree();
		});

	connect(btn_import_dir, &QPushButton::clicked, this, [this]([[maybe_unused]] bool checked)
		{
			const QString last_import_dir_path = m_gui_settings->GetValue(gui::ch_import_path).toString();
			const QString dir                  = QFileDialog::getExistingDirectory(this, tr("Select the directory to import Artemis cheats from"), last_import_dir_path);

			if (dir.isEmpty())
			{
				return;
			}

			m_gui_settings->SetValue(gui::ch_import_path, dir);

			auto qt_dir = QDir(dir);
			if (!qt_dir.exists())
			{
				return;
			}

			QStringList ncl_files = qt_dir.entryList(QStringList({"*.NCL", "*.ncl"}), QDir::Files);

			for (const auto& ncl_file : ncl_files)
			{
				const QString full_path = qt_dir.filePath(ncl_file);
				const QFileInfo file_info(full_path);
				auto res = ncl_parser::parse_ncl(full_path.toStdString());
				if (!res)
				{
					continue;
				}

				add_cheats(file_info.completeBaseName().toStdString(), std::move(*res));
			}

			save_cheats();
			refresh_tree();
		});

	connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item, [[maybe_unused]] int column)
		{
			if (!item)
			{
				return;
			}

			const auto* parent = item->parent();
			if (!parent)
			{
				return;
			}

			const std::string game_name  = parent->text(0).toStdString();
			const std::string cheat_name = item->text(0).toStdString();

			if (!m_cheats.contains(game_name) || !m_cheats.at(game_name).contains(cheat_name))
			{
				return;
			}

			const auto& cheat = m_cheats.at(game_name).at(cheat_name);
			std::unordered_map<std::string, std::string> var_choices;
			if (!cheat.variables.empty())
			{
				cheat_variables_dialog var_dlg(QString::fromStdString(cheat_name), cheat, this);
				if (var_dlg.exec() == QDialog::Rejected)
				{
					return;
				}
				var_choices = var_dlg.get_choices();
			}

			if (!g_cheat_engine.activate_cheat(game_name, cheat_name, cheat, var_choices))
			{
				g_cheat_engine.deactivate_cheat(game_name, cheat_name);
				item->setBackground(0, QBrush());
				return;
			}

			item->setBackground(0, QBrush(gui::utils::get_label_color("background_queued_cheat", QPalette::AlternateBase)));
		});

	connect(m_tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, [[maybe_unused]] int column)
		{
			const auto* parent = item->parent();
			if (!parent)
			{
				return;
			}

			const std::string game_name  = parent->text(0).toStdString();
			const std::string cheat_name = item->text(0).toStdString();

			if (!m_cheats.contains(game_name) || !m_cheats.at(game_name).contains(cheat_name))
			{
				return;
			}

			const auto& cheat = m_cheats.at(game_name).at(cheat_name);
			m_edt_author->setText(QString::fromStdString(cheat.author));

			std::string str_comments;
			for (const auto& comment : cheat.comments)
			{
				str_comments += comment;
				str_comments += "\n";
			}

			m_pte_comments->setPlainText(QString::fromStdString(str_comments));
		});

	load_cheats();
	refresh_tree();
}

cheat_manager_dialog::~cheat_manager_dialog()
{
	m_inst = nullptr;
}

cheat_manager_dialog* cheat_manager_dialog::get_dlg(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
{
	if (m_inst == nullptr)
	{
		m_inst = new cheat_manager_dialog(gui_settings, parent);
	}

	return m_inst;
}

void cheat_manager_dialog::refresh_tree()
{
	m_tree->clear();

	QList<QTreeWidgetItem*> items;

	for (const auto& [game_name, cheats] : m_cheats)
	{
		auto top_entry = new QTreeWidgetItem(QStringList(QString::fromStdString(game_name)));
		items.append(top_entry);
		for (const auto& [cheat_name, entry] : cheats)
		{
			auto sub_entry = new QTreeWidgetItem();
			sub_entry->setText(0, QString::fromStdString(cheat_name));
			if (entry.type == cheat_type::constant)
			{
				sub_entry->setCheckState(0, Qt::CheckState::Unchecked);
			}
			top_entry->addChild(sub_entry);
		}
	}

	std::sort(items.begin(), items.end(), [](QTreeWidgetItem* a, QTreeWidgetItem* b) -> bool
		{
			return a->text(0).compare(b->text(0), Qt::CaseInsensitive) < 0;
		});

	m_tree->insertTopLevelItems(0, items);

	auto find_node = [this](const std::string& game_name, const std::string& cheat_name) -> QTreeWidgetItem*
	{
		auto found = m_tree->findItems(QString::fromStdString(game_name), Qt::MatchExactly);

		if (found.empty())
		{
			log_cheat.error("Failed to find game '%s' in the tree", game_name);
			return nullptr;
		}

		assert(found.size() == 1);

		auto* child = gui::utils::find_child(found[0], QString::fromStdString(cheat_name));
		if (!child)
		{
			log_cheat.error("Failed to find the cheat '%s' for game '%s'", cheat_name, game_name);
		}
		return child;
	};

	const auto& constant_cheats = g_cheat_engine.get_active_constant_cheats();
	for (const auto& cheat : constant_cheats)
	{
		const auto name = cheat.get_name();
		auto* node      = find_node(name.first, name.second);

		if (!node)
		{
			continue;
		}

		node->setCheckState(0, Qt::CheckState::Checked);
	}

	const auto& queued_cheats = g_cheat_engine.get_queued_cheats();
	for (const auto& cheat : queued_cheats)
	{
		const auto name = cheat.get_name();
		auto* node      = find_node(name.first, name.second);

		if (!node)
		{
			continue;
		}

		node->setBackground(0, QBrush(gui::utils::get_label_color("background_queued_cheat", QPalette::AlternateBase)));
	}
}

void cheat_manager_dialog::filter_cheats(const QString& game_id, const QString& term)
{
	const QStringList words = term.split(" ", Qt::SkipEmptyParts);

	const auto* root = m_tree->invisibleRootItem();

	for (int i = 0; i < root->childCount(); i++)
	{
		auto* child = root->child(i);

		if (!game_id.isEmpty() && child->text(0).contains(game_id, Qt::CaseInsensitive))
		{
			child->setHidden(false);
			continue;
		}

		if (std::all_of(words.begin(), words.end(), [child](const QString& word)
				{
					return child->text(0).contains(word, Qt::CaseInsensitive);
				}))
		{
			child->setHidden(false);
			continue;
		}

		child->setHidden(true);
	}
}

fs::file cheat_manager_dialog::open_cheats(bool load)
{
	const std::string cheats_path = patch_engine::get_patches_path();

	if (!fs::create_path(cheats_path))
	{
		log_cheat.fatal("Failed to create path: %s (%s)", cheats_path, fs::g_tls_error);
		return fs::file{};
	}

	const std::string path = cheats_path + std::string(m_cheats_filename);

	bs_t<fs::open_mode> mode = load ? fs::read + fs::create : fs::read + fs::rewrite;

	return fs::file(path, mode);
}

void cheat_manager_dialog::load_cheats()
{
	auto cheat_file = open_cheats(true);

	if (!cheat_file)
	{
		log_cheat.error("[load_cheats] Failed to open cheat file");
		return;
	}

	const auto [root, error] = yaml_load(cheat_file.to_string());
	if (!error.empty() || !root)
	{
		log_cheat.error("Error parsing %s: %s", m_cheats_filename, error);
		return;
	}

	for (const auto game_key : root)
	{
		const std::string game_name = game_key.first.Scalar();

		if (game_name.empty())
		{
			continue;
		}

		if (!game_key.second.IsMap())
		{
			log_cheat.error("Expected Map at game level(location: %s)", get_yaml_node_location(game_key.second));
			continue;
		}

		std::map<std::string, cheat_entry> cheats_to_add;

		for (const auto cheat_key : game_key.second)
		{
			const std::string cheat_name = cheat_key.first.Scalar();

			if (cheat_name.empty())
			{
				continue;
			}

			cheat_entry cheat_data{};

			if (!cheat_key.second.IsMap())
			{
				log_cheat.error("Expected Map at cheat level(location: %s)", get_yaml_node_location(cheat_key.second));
				continue;
			}

			if (const auto author = cheat_key.second[m_author_key]; author && author.IsScalar())
			{
				cheat_data.author = author.Scalar();
			}

			if (const auto comments = cheat_key.second[m_comments_key]; comments && comments.IsSequence())
			{
				for (const auto comment : comments)
				{
					if (!comment.IsScalar())
					{
						log_cheat.error("Non scalar comment in the comments(location : %s)", get_yaml_node_location(comment));
						continue;
					}

					cheat_data.comments.push_back(comment.Scalar());
				}
			}

			if (const auto type = cheat_key.second[m_type_key]; type && type.IsScalar())
			{
				u64 type64;
				if (!cfg::try_to_enum_value(&type64, &fmt_class_string<cheat_type>::format, type.Scalar()))
				{
					log_cheat.error("Invalid type value %s for cheat %s(location: %s)", type.Scalar(), cheat_name, get_yaml_node_location(type));
					continue;
				}
				cheat_data.type = static_cast<cheat_type>(::narrow<u8>(type64));
			}

			if (const auto codes = cheat_key.second[m_codes_key]; codes && codes.IsSequence())
			{
				for (const auto code : codes)
				{
					if (!code.IsSequence())
					{
						log_cheat.error("Found a non-sequence code(location: %s)", get_yaml_node_location(code));
						continue;
					}

					if (!code[0] || !code[1] || !code[2] || !code[3] || !code[4] ||
						!code[0].IsScalar() || !code[1].IsScalar() || !code[2].IsScalar() || !code[3].IsScalar() || !code[4].IsScalar())
					{
						log_cheat.error("Invalid node in the code sequence(location: %s)", get_yaml_node_location(code));
						continue;
					}

					cheat_code ccode;

					u64 type64;
					if (!cfg::try_to_enum_value(&type64, &fmt_class_string<cheat_inst>::format, code[0].Scalar()))
					{
						log_cheat.error("Invalid type value %s for cheat instruction(location: %s)", code[0].Scalar(), get_yaml_node_location(code));
						continue;
					}
					ccode.type  = static_cast<cheat_inst>(::narrow<u8>(type64));
					ccode.addr  = code[1].Scalar();
					ccode.value = code[2].Scalar();
					ccode.opt1  = code[3].Scalar();
					ccode.opt2  = code[4].Scalar();

					cheat_data.codes.push_back(std::move(ccode));
				}
			}
			else
			{
				log_cheat.error("Found cheat %s without codes", cheat_name);
				continue;
			}

			if (const auto vars = cheat_key.second[m_variables_key]; vars && vars.IsMap())
			{
				for (const auto var : vars)
				{
					const std::string var_name = var.first.Scalar();
					std::map<std::string, std::string> var_values;
					if (!var.second.IsMap())
					{
						log_cheat.error("Found a non-map variable(location: %s)", get_yaml_node_location(var.second));
						continue;
					}

					for (const auto var_opt : var.second)
					{
						if (!var_opt.second.IsScalar())
						{
							log_cheat.error("Found a non-scalar in var options(location: %s)", get_yaml_node_location(var_opt.second));
							continue;
						}
						var_values.insert_or_assign(var_opt.first.Scalar(), var_opt.second.Scalar());
					}

					cheat_data.variables.insert_or_assign(var_name, var_values);
				}
			}

			cheats_to_add.insert_or_assign(std::move(cheat_name), std::move(cheat_data));
		}

		add_cheats(game_name, cheats_to_add);
	}
}

void cheat_manager_dialog::save_cheats()
{
	auto cheat_file = open_cheats(false);
	if (!cheat_file)
	{
		log_cheat.fatal("[save_cheats] Failed to open cheat file");
		return;
	}

	YAML::Emitter out;

	out << YAML::BeginMap;

	auto output_cheat = [&out](const cheat_entry& cheat)
	{
		out << YAML::BeginMap;
		out << YAML::Key << m_author_key << YAML::Value << cheat.author;
		if (!cheat.comments.empty())
		{
			out << YAML::Key << m_comments_key << YAML::BeginSeq;
			for (const auto& comment : cheat.comments)
			{
				out << comment;
			}
			out << YAML::EndSeq;
		}
		out << YAML::Key << m_type_key << YAML::Value << fmt::format("%s", cheat.type);
		out << YAML::Key << m_codes_key << YAML::BeginSeq;
		for (const auto& code : cheat.codes)
		{
			out << YAML::Flow << YAML::BeginSeq << fmt::format("%s", code.type) << code.addr << code.value << code.opt1 << code.opt2 << YAML::EndSeq;
		}
		out << YAML::EndSeq;
		if (!cheat.variables.empty())
		{
			out << YAML::Key << m_variables_key << YAML::BeginMap;
			for (const auto& [var_name, var_options] : cheat.variables)
			{
				out << YAML::Key << var_name << YAML::BeginMap;
				for (const auto& [opt_name, opt_value] : var_options)
				{
					out << YAML::Key << opt_name << YAML::Value << opt_value;
				}
				out << YAML::EndMap;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;
	};

	for (const auto& [game_name, cheats] : m_cheats)
	{
		out << YAML::Key << game_name;
		out << YAML::BeginMap;

		for (const auto& [cheat_name, cheat] : cheats)
		{
			out << YAML::Key << cheat_name;
			output_cheat(cheat);
		}

		out << YAML::EndMap;
	}

	out << YAML::EndMap;

	ensure(out.good());

	cheat_file.write(out.c_str(), out.size());
}

void cheat_manager_dialog::add_cheats(std::string name, std::map<std::string, cheat_entry> to_add)
{
	m_cheats.insert_or_assign(std::move(name), std::move(to_add));
}
