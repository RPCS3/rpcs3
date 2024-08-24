#pragma once

#include <QSettings>
#include <QDir>
#include <QVariant>
#include <QSize>

#include <memory>

#include "gui_save.h"

typedef QPair<QString, QString> q_string_pair;
typedef QPair<QString, QSize> q_size_pair;
typedef QList<q_string_pair> q_pair_list;
typedef QList<q_size_pair> q_size_list;

namespace gui
{
	const QString savedata = "SaveData";
	const QString users    = "Users";
}

// Parent Class for GUI settings
class settings : public QObject
{
	Q_OBJECT

public:
	explicit settings(QObject* parent = nullptr);
	~settings();

	void sync();

	QString GetSettingsDir() const;

	QVariant GetValue(const QString& key, const QString& name, const QVariant& def) const;
	QVariant GetValue(const gui_save& entry) const;
	static QVariant List2Var(const q_pair_list& list);
	static q_pair_list Var2List(const QVariant& var);

public Q_SLOTS:
	/** Remove entry */
	void RemoveValue(const QString& key, const QString& name, bool sync = true) const;
	void RemoveValue(const gui_save& entry, bool sync = true) const;

	/** Write value to entry */
	void SetValue(const gui_save& entry, const QVariant& value, bool sync = true) const;
	void SetValue(const QString& key, const QVariant& value, bool sync = true) const;
	void SetValue(const QString& key, const QString& name, const QVariant& value, bool sync = true) const;

protected:
	static QString ComputeSettingsDir();

	std::unique_ptr<QSettings> m_settings;
	QDir m_settings_dir;
};
