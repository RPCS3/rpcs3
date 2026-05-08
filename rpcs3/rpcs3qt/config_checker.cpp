#include "stdafx.h"
#include "config_checker.h"
#include "midi_creator.h"
#include "microphone_creator.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QVBoxLayout>

LOG_CHANNEL(gui_log, "GUI");

config_checker::config_checker(QWidget* parent, const QString& content_or_serial, checker_mode mode, const std::string& db_config)
	: QDialog(parent)
	, m_checker_mode(mode)
	, m_content_or_serial(content_or_serial)
	, m_db_config(db_config)
{
	setObjectName("config_checker");
	setWindowTitle(tr("Config Checker"));
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout* layout = new QVBoxLayout();
	QComboBox* combo = nullptr;

	if (mode == checker_mode::gamelist)
	{
		m_serial = content_or_serial.toStdString();

		combo = new QComboBox(this);

		std::string custom_config_path;
		if (std::string config_path = rpcs3::utils::get_custom_config_path(m_serial); fs::is_file(config_path))
		{
			custom_config_path = std::move(config_path);
			combo->addItem(tr("Custom Configuration"), static_cast<int>(cfg_mode::custom));
		}

		combo->addItem(tr("Database + Global Configuration"), static_cast<int>(cfg_mode::database_config));
		combo->setCurrentIndex(combo->findData(static_cast<int>(custom_config_path.empty() ? cfg_mode::database_config : cfg_mode::custom)));

		connect(combo, &QComboBox::currentIndexChanged, this, [this, combo]()
		{
			check_config(static_cast<cfg_mode>(combo->currentData().toInt()));
		});

		layout->addWidget(combo);
	}

	m_label = new QLabel(this);
	layout->addWidget(m_label);

	m_text_box = new QTextEdit();
	m_text_box->setReadOnly(true);
	layout->addWidget(m_text_box);

	QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(box);

	setLayout(layout);
	resize(400, 600);

	check_config(combo ? static_cast<cfg_mode>(combo->currentData().toInt()) : cfg_mode::database_config);
}

void config_checker::check_config(cfg_mode mode)
{
	QString result;

	if (check_config(mode, m_content_or_serial, result))
	{
		if (m_checker_mode == checker_mode::gamelist)
		{
			if (result.isEmpty())
			{
				m_label->setText(tr("The configuration seems to match the default config."));
			}
			else
			{
				m_label->setText(tr("Config database settings are marked with an * in front of the name.\nSome settings seem to deviate from the default config:"));
			}
		}
		else
		{
			if (result.isEmpty())
			{
				m_label->setText(tr("Found config.\nIt seems to match the default config."));
			}
			else
			{
				m_label->setText(tr("Found config.\nSome settings seem to deviate from the default config:"));
			}
		}

		m_text_box->setVisible(!result.isEmpty());
		m_text_box->setHtml(result);
	}
	else
	{
		m_label->setText(result);
	}
}

bool config_checker::check_config(cfg_mode mode, QString content_or_serial, QString& result)
{
	std::unique_ptr<cfg_root> config = std::make_unique<cfg_root>();
	std::unique_ptr<cfg_root> config_db_only;

	if (m_checker_mode == checker_mode::log)
	{
		const QString start_token = "SYS: Used configuration:\n";
		const QString end_token = "\n·";

		qsizetype start = content_or_serial.indexOf(start_token);
		qsizetype end = -1;

		if (start >= 0)
		{
			start += start_token.size();
			end = content_or_serial.indexOf(end_token, start);
		}

		if (end < 0)
		{
			result = tr("Cannot find any config!");
			return false;
		}

		content_or_serial = content_or_serial.mid(start, end - start);
	}

	if (m_checker_mode == checker_mode::gamelist)
	{
		config->from_default();

		// Load global config
		const std::string cfg_path = fs::get_config_dir(true) + "config.yml";
		if (const fs::file cfg_file{cfg_path})
		{
			gui_log.notice("config_checker: Applying global config: %s", cfg_path);

			if (!config->from_string(cfg_file.to_string()))
			{
				gui_log.error("config_checker: Failed to apply global config: %s", cfg_path);
				result = tr("Failed to apply global config!");
				return false;
			}
		}

		// Load custom config
		const std::string custom_config_path = rpcs3::utils::get_custom_config_path(m_serial);
		if (mode == cfg_mode::custom && !custom_config_path.empty())
		{
			if (const fs::file cfg_file{custom_config_path})
			{
				gui_log.notice("config_checker: Applying custom config: %s", custom_config_path);

				if (!config->from_string(cfg_file.to_string()))
				{
					gui_log.error("config_checker: Failed to apply custom config: %s", custom_config_path);
					result = tr("Failed to apply custom config!");
					return false;
				}
			}
		}

		if (mode == cfg_mode::database_config && !m_db_config.empty())
		{
			gui_log.notice("config_checker: Applying database config: %s", custom_config_path);

			if (!config->from_string(m_db_config))
			{
				gui_log.error("config_checker: Failed to apply database config:\n%s", m_db_config);
				result = tr("Failed to apply database config!");
				return false;
			}

			config_db_only = std::make_unique<cfg_root>();
			config_db_only->from_default();
			if (!config_db_only->from_string(m_db_config))
			{
				gui_log.error("config_checker: Failed to apply database config:\n%s", m_db_config);
				result = tr("Failed to apply database config!");
				return false;
			}
		}
	}
	else if (!config->from_string(content_or_serial.toStdString()))
	{
		gui_log.error("config_checker: Failed to parse config:\n%s", content_or_serial);
		result = tr("Cannot find any config!");
		return false;
	}

	std::function<void(const cfg::_base*, const cfg::_base*, std::string&, int)> print_diff_recursive;
	print_diff_recursive = [this, &print_diff_recursive, &config](const cfg::_base* base, const cfg::_base* base_db_only, std::string& diff, int indentation) -> void
	{
		if (!base)
		{
			return;
		}

		// Ignore some irrelevant settings in gamelist mode
		if (m_checker_mode == checker_mode::gamelist && base->get_type() != cfg::type::node)
		{
			const std::string key = base->get_name();

			if (key == config->sys.console_psid.get_name() ||
				key == config->sys.system_name.get_name() ||
				key == config->video.vk.adapter.get_name())
			{
				return;
			}
		}

		const auto indent = [](std::string& str, int indentation)
		{
			for (int i = 0; i < indentation * 2; i++)
			{
				str += "&nbsp;";
			}
		};

		const auto base_name_db = [base](bool is_db_config)
		{
			if (is_db_config)
			{
				return "*" + base->get_name();
			}

			return base->get_name();
		};

		switch (base->get_type())
		{
		case cfg::type::node:
		{
			if (const auto& node = static_cast<const cfg::node*>(base))
			{
				std::string diff_tmp;

				for (const auto& n : node->get_nodes())
				{
					const cfg::_base* n_db_only = nullptr;
					if (const auto& node_db_only = static_cast<const cfg::node*>(base_db_only))
					{
						for (const auto& n_db : node_db_only->get_nodes())
						{
							if (n_db->get_name() == n->get_name())
							{
								n_db_only = n_db;
								break;
							}
						}
					}

					print_diff_recursive(n, n_db_only, diff_tmp, indentation + 1);
				}

				if (!diff_tmp.empty())
				{
					indent(diff, indentation);

					if (!base->get_name().empty())
					{
						fmt::append(diff, "<b>%s:</b><br>", base->get_name());
					}

					fmt::append(diff, "%s", diff_tmp);
				}
			}
			break;
		}
		case cfg::type::_bool:
		case cfg::type::_enum:
		case cfg::type::_int:
		case cfg::type::uint:
		case cfg::type::uint128:
		case cfg::type::string:
		{
			const std::string val = base->to_string();
			const std::string def = base->def_to_string();

			if (val == def)
				break;

			indent(diff, indentation);

			if (m_checker_mode == checker_mode::gamelist)
			{
				if (base->get_name() == config->io.midi_devices.get_name())
				{
					fmt::append(diff, "%s:<br>", base->get_name());

					midi_creator mc {};

					mc.parse_devices(def);
					const std::array<midi_device, max_midi_devices> def_devices = mc.get_selection_list();

					mc.parse_devices(val);
					const std::array<midi_device, max_midi_devices> devices = mc.get_selection_list();

					for (usz i = 0; i < devices.size(); i++)
					{
						const midi_device& def_device = def_devices[i];
						const midi_device& device = devices[i];

						if (device.name == def_device.name)
							continue;

						indent(diff, indentation + 1);
						fmt::append(diff, "Device %d: <span style=\"color:red;\">%s: %s</span><br>", i + 1, device.type, device.name);
					}
					break;
				}
				else if (base->get_name() == config->audio.microphone_devices.get_name())
				{
					fmt::append(diff, "%s:<br>", base->get_name());

					microphone_creator mc {};

					mc.parse_devices(def);
					const std::array<std::string, 4> def_devices = mc.get_selection_list();

					mc.parse_devices(val);
					const std::array<std::string, 4> devices = mc.get_selection_list();

					for (usz i = 0; i < devices.size(); i++)
					{
						const std::string& def_device = def_devices[i];
						const std::string& device = devices[i];

						if (device == def_device)
							continue;

						indent(diff, indentation + 1);
						fmt::append(diff, "Device %d: <span style=\"color:red;\">%s</span><br>", i + 1, device);
					}
					break;
				}
			}

			const bool is_db_config = base_db_only && base_db_only->to_string() != def;

			if (def.empty())
			{
				fmt::append(diff, "%s: <span style=\"color:red;\">%s</span><br>", base_name_db(is_db_config), val);
			}
			else
			{
				fmt::append(diff, "%s: <span style=\"color:red;\">%s</span> <span style=\"color:gray;\">default:</span> <span style=\"color:green;\">%s</span><br>", base_name_db(is_db_config), val, def);
			}
			break;
		}
		case cfg::type::set:
		{
			if (const auto& node = static_cast<const cfg::set_entry*>(base))
			{
				const std::vector<std::string> set_entries = node->to_list();

				if (!set_entries.empty())
				{
					indent(diff, indentation);
					fmt::append(diff, "<b>%s:</b><br>", base->get_name());

					for (const std::string& entry : set_entries)
					{
						indent(diff, indentation + 1);
						fmt::append(diff, "- <span style=\"color:red;\">%s</span><br>", entry);
					}
				}
			}
			break;
		}
		case cfg::type::log:
		{
			if (const auto& node = static_cast<const cfg::log_entry*>(base))
			{
				const auto& log_entries = node->get_map();

				if (!log_entries.empty())
				{
					indent(diff, indentation);
					fmt::append(diff, "<b>%s:</b><br>", base->get_name());

					for (const auto& entry : log_entries)
					{
						indent(diff, indentation + 1);
						fmt::append(diff, "<span style=\"color:red;\">%s: %s</span><br>", entry.first, entry.second);
					}
				}
			}
			break;
		}
		case cfg::type::map:
		case cfg::type::node_map:
		case cfg::type::device:
		{
			// Ignored
			break;
		}
		}
	};

	std::string diff;
	print_diff_recursive(config.get(), config_db_only.get(), diff, 0);
	result = QString::fromStdString(diff);

	return true;
}
