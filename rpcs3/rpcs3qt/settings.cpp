#include "settings.h"

#include "Utilities/File.h"

settings::settings(QObject* parent) : QObject(parent),
	m_settings_dir(ComputeSettingsDir())
{
}

settings::~settings()
{
	sync();
}

void settings::sync()
{
	if (m_settings)
	{
		m_settings->sync();
	}
}

QString settings::GetSettingsDir() const
{
	return m_settings_dir.absolutePath();
}

QString settings::ComputeSettingsDir()
{
	return QString::fromStdString(fs::get_config_dir()) + "/GuiConfigs/";
}

void settings::RemoveValue(const QString& key, const QString& name, bool sync) const
{
	if (m_settings)
	{
		m_settings->beginGroup(key);
		m_settings->remove(name);
		m_settings->endGroup();

		if (sync)
		{
			m_settings->sync();
		}
	}
}

void settings::RemoveValue(const gui_save& entry, bool sync) const
{
	RemoveValue(entry.key, entry.name, sync);
}

QVariant settings::GetValue(const QString& key, const QString& name, const QVariant& def) const
{
	return m_settings ? m_settings->value(key + "/" + name, def) : def;
}

QVariant settings::GetValue(const gui_save& entry) const
{
	return GetValue(entry.key, entry.name, entry.def);
}

QVariant settings::List2Var(const q_pair_list& list)
{
	QByteArray ba;
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << list;
	return QVariant(ba);
}

q_pair_list settings::Var2List(const QVariant& var)
{
	q_pair_list list;
	QByteArray ba = var.toByteArray();
	QDataStream stream(&ba, QIODevice::ReadOnly);
	stream >> list;
	return list;
}

void settings::SetValue(const gui_save& entry, const QVariant& value, bool sync) const
{
	SetValue(entry.key, entry.name, value, sync);
}

void settings::SetValue(const QString& key, const QVariant& value, bool sync) const
{
	if (m_settings)
	{
		m_settings->setValue(key, value);

		if (sync)
		{
			m_settings->sync();
		}
	}
}

void settings::SetValue(const QString& key, const QString& name, const QVariant& value, bool sync) const
{
	if (m_settings)
	{
		m_settings->beginGroup(key);
		m_settings->setValue(name, value);
		m_settings->endGroup();

		if (sync)
		{
			m_settings->sync();
		}
	}
}
