#include "stdafx.h"
#include "config_checker.h"
#include "Emu/system_config.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>

LOG_CHANNEL(gui_log, "GUI");

config_checker::config_checker(QWidget* parent, const QString& content, bool is_log) : QDialog(parent)
{
	setObjectName("config_checker");
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout* layout = new QVBoxLayout();
	QLabel* label = new QLabel(this);
	layout->addWidget(label);

	QString result;

	if (check_config(content, result, is_log))
	{
		setWindowTitle(tr("Interesting!"));

		if (result.isEmpty())
		{
			label->setText(tr("Found config.\nIt seems to match the default config."));
		}
		else
		{
			label->setText(tr("Found config.\nSome settings seem to deviate from the default config:"));

			QTextEdit* text_box = new QTextEdit();
			text_box->setReadOnly(true);
			text_box->setHtml(result);
			layout->addWidget(text_box);

			resize(400, 600);
		}
	}
	else
	{
		setWindowTitle(tr("Ooops!"));
		label->setText(result);
	}

	QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(box);

	setLayout(layout);
}

bool config_checker::check_config(QString content, QString& result, bool is_log)
{
	cfg_root config{};

	if (is_log)
	{
		const QString start_token = "SYS: Used configuration:\n";
		const QString end_token = "\n·";

		qsizetype start = content.indexOf(start_token);
		qsizetype end = -1;

		if (start >= 0)
		{
			start += start_token.size();
			end = content.indexOf(end_token, start);
		}

		if (end < 0)
		{
			result = tr("Cannot find any config!");
			return false;
		}

		content = content.mid(start, end - start);
	}

	if (!config.from_string(content.toStdString()))
	{
		gui_log.error("log_viewer: Failed to parse config:\n%s", content);
		result = tr("Cannot find any config!");
		return false;
	}

	std::function<void(const cfg::_base*, std::string&, int)> print_diff_recursive;
	print_diff_recursive = [&print_diff_recursive](const cfg::_base* base, std::string& diff, int indentation) -> void
	{
		if (!base)
		{
			return;
		}

		const auto indent = [](std::string& str, int indentation)
		{
			for (int i = 0; i < indentation * 2; i++)
			{
				str += "&nbsp;";
			}
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
					print_diff_recursive(n, diff_tmp, indentation + 1);
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
		case cfg::type::string:
		{
			const std::string val = base->to_string();
			const std::string def = base->def_to_string();

			if (val != def)
			{
				indent(diff, indentation);

				if (def.empty())
				{
					fmt::append(diff, "%s: <span style=\"color:red;\">%s</span><br>", base->get_name(), val);
				}
				else
				{
					fmt::append(diff, "%s: <span style=\"color:red;\">%s</span> <span style=\"color:gray;\">default:</span> <span style=\"color:green;\">%s</span><br>", base->get_name(), val, def);
				}
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
	print_diff_recursive(&config, diff, 0);
	result = QString::fromStdString(diff);

	return true;
}
