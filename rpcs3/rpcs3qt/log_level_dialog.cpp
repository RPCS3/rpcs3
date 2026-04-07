#include "stdafx.h"
#include "log_level_dialog.h"
#include "emu_settings.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LOG_CHANNEL(cfg_log, "CFG");

log_level_dialog::log_level_dialog(QWidget* parent, std::shared_ptr<emu_settings> emu_settings)
	: QDialog(parent), m_emu_settings(emu_settings)
{
	setWindowTitle(tr("Configure minimum log levels"));
	setObjectName("log_level_dialog");
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumSize(QSize(700, 400));

	const std::set<std::string> channels = logs::get_channels();
	std::vector<std::pair<std::string, QString>> levels;

	const auto add_level = [&levels](logs::level level, const QString& localized)
	{
		levels.push_back(std::pair<std::string, QString>(fmt::format("%s", level), localized));
	};

	add_level(logs::level::always, tr("Always"));
	add_level(logs::level::fatal, tr("Fatal"));
	add_level(logs::level::error, tr("Error"));
	add_level(logs::level::todo, tr("Todo"));
	add_level(logs::level::success, tr("Success"));
	add_level(logs::level::warning, tr("Warning"));
	add_level(logs::level::notice, tr("Notice"));
	add_level(logs::level::trace, tr("Trace"));

	const std::map<std::string, std::string> current_settings = m_emu_settings->GetMapSetting(emu_settings_type::Log);
	for (const auto& [channel, level] : current_settings)
	{
		if (!channels.contains(channel))
		{
			cfg_log.warning("log_level_dialog: Ignoring unknown channel '%s' found in config file.", channel);
		}
	}

	m_table = new QTableWidget(static_cast<int>(channels.size()), 2, this);
	m_table->setHorizontalHeaderLabels({ tr("Channel"), tr("Level") });

	int i = 0;
	for (const std::string& channel : channels)
	{
		QComboBox* combo = new QComboBox();

		for (const auto& [level, localized] : levels)
		{
			combo->addItem(localized, QString::fromStdString(level));
		}

		connect(combo, &QComboBox::currentIndexChanged, combo, [this, combo, ch = channel](int index)
		{
			if (index < 0) return;

			const QVariant var = combo->itemData(index);
			if (!var.canConvert<QString>()) return;

			std::map<std::string, std::string> settings = m_emu_settings->GetMapSetting(emu_settings_type::Log);

			settings[ch] = var.toString().toStdString();

			m_emu_settings->SetMapSetting(emu_settings_type::Log, settings);
		});

		m_table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(channel)));
		m_table->setCellWidget(i, 1, combo);

		i++;
	}

	QLineEdit* filter_edit = new QLineEdit(this);
	filter_edit->setPlaceholderText(tr("Filter channels"));
	connect(filter_edit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		for (int i = 0; i < m_table->rowCount(); i++)
		{
			if (QTableWidgetItem* item = m_table->item(i, 0))
			{
				m_table->setRowHidden(i, !text.isEmpty() && !item->text().contains(text, Qt::CaseInsensitive));
			}
		}
	});

	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
	connect(button_box, &QDialogButtonBox::clicked, this, [this, button_box, old_settings = m_emu_settings->GetMapSetting(emu_settings_type::Log)](QAbstractButton* button)
	{
		if (button == button_box->button(QDialogButtonBox::Ok))
		{
			accept();
		}
		else if (button == button_box->button(QDialogButtonBox::Cancel))
		{
			m_emu_settings->SetMapSetting(emu_settings_type::Log, old_settings);
			reject();
		}
		else if (button == button_box->button(QDialogButtonBox::RestoreDefaults))
		{
			m_emu_settings->SetMapSetting(emu_settings_type::Log, m_emu_settings->GetMapSettingDefault(emu_settings_type::Log));
			reload_page();
		}
	});

	reload_page();

	m_table->resizeColumnsToContents();
	m_table->horizontalHeader()->stretchLastSection();

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(filter_edit);
	layout->addWidget(m_table);
	layout->addWidget(button_box);
	setLayout(layout);
}

log_level_dialog::~log_level_dialog()
{
}

void log_level_dialog::reload_page()
{
	const std::map<std::string, std::string> settings = m_emu_settings->GetMapSetting(emu_settings_type::Log);
	const QString def_str = QString::fromStdString(fmt::format("%s", logs::level::_default));

	for (int i = 0; i < m_table->rowCount(); i++)
	{
		QTableWidgetItem* item = m_table->item(i, 0);
		if (!item) continue;

		const std::string channel = item->text().toStdString();

		if (QComboBox* combo = static_cast<QComboBox*>(m_table->cellWidget(i, 1)))
		{
			combo->blockSignals(true);
			combo->setCurrentIndex(combo->findData(def_str));
			if (settings.contains(channel))
			{
				if (const int index = combo->findData(QString::fromStdString(settings.at(channel))); index >= 0)
				{
					combo->setCurrentIndex(index);
				}
			}
			combo->blockSignals(false);
		}
	}
}
