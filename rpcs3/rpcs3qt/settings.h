#pragma once

#include <QSettings>
#include <QDir>
#include <QVariant>
#include <QSize>

struct gui_save
{
	QString key;
	QString name;
	QVariant def;

	gui_save()
	{
		key = "";
		name = "";
		def = QVariant();
	}

	gui_save(const QString& k, const QString& n, const QVariant& d)
	{
		key = k;
		name = n;
		def = d;
	}
};

typedef QPair<QString, QString> q_string_pair;
typedef QPair<QString, QSize> q_size_pair;
typedef QList<q_string_pair> q_pair_list;
typedef QList<q_size_pair> q_size_list;

// Parent Class for GUI settings
class settings : public QObject
{
	Q_OBJECT

public:
	explicit settings(QObject* parent = nullptr);
	~settings();

	QString GetSettingsDir();

	QVariant GetValue(const gui_save& entry);
	QVariant GetValue(const QString& key, const QString& name, const QString& def);
	QVariant List2Var(const q_pair_list& list);
	q_pair_list Var2List(const QVariant& var);

public Q_SLOTS:
	/** Remove entry */
	void RemoveValue(const QString& key, const QString& name);

	/** Write value to entry */
	void SetValue(const gui_save& entry, const QVariant& value);
	void SetValue(const QString& key, const QString& name, const QVariant& value);

protected:
	QString ComputeSettingsDir();

	QSettings* m_settings = nullptr;
	QDir m_settings_dir;
	QString m_current_name;
};
