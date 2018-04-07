#include "breakpoint_settings.h"

#include "Utilities/File.h"

breakpoint_settings::breakpoint_settings(QObject* parent) : QObject(parent), 
	m_bp_settings(QString::fromStdString(fs::get_config_dir()) + "/GuiConfigs/" + tr("Breakpoints") + ".ini", QSettings::Format::IniFormat, this)
{
}

breakpoint_settings::~breakpoint_settings()
{
	m_bp_settings.sync();
}

void breakpoint_settings::SetBreakpoint(const QString& gameid, u32 addr, const breakpoint_data& bp_data)
{
	m_bp_settings.beginGroup(gameid);
	m_bp_settings.setValue(QString::number(addr, 16), QString("%0|%1").arg(QString::number(bp_data.flags), bp_data.name)); // Split flag and name with delimiter
	m_bp_settings.endGroup();
}

void breakpoint_settings::RemoveBreakpoint(const QString& gameid, u32 addr)
{
	m_bp_settings.beginGroup(gameid);
	m_bp_settings.remove(QString::number(addr, 16));
	m_bp_settings.endGroup();
}

QMap<QString, QMap<u32, breakpoint_data>> breakpoint_settings::ReadBreakpoints()
{
	QMap<QString, QMap<u32, breakpoint_data>> ret;

	const QStringList& gameids = m_bp_settings.childGroups();
	for (const QString& id: gameids)
	{
		m_bp_settings.beginGroup(id);
		const QStringList& addrs = m_bp_settings.childKeys();
		for (const QString& addr : addrs)
		{
			u32 addr_ = addr.toUInt(nullptr, 16);
			QString& flags_name = m_bp_settings.value(addr).toString();
			u32 flags = flags_name.section("|", 0, 0).toInt(); // Ignore any | in the name itself.
			QString name = flags_name.section("|", 1);
			ret[id][addr_] = { flags, name };
		}
		m_bp_settings.endGroup();
	}
	return ret;
}
