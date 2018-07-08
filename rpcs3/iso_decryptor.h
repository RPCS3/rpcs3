#pragma once

#include <map>
#include <vector>

#include <QFile>
#include <QString>
#include <QByteArray>

const int SECTOR            = 2048;
const QByteArray ISO_SECRET = QByteArray::fromHex("380bcf0b53455b3c7817ab4fa3ba90ed");
const QByteArray ISO_IV     = QByteArray::fromHex("69474772af6fdab342743aefaa186287");

auto keyCast = [](QByteArray array) { return reinterpret_cast<unsigned char*>(array.data()); };

class iso_decryptor
{
private:
	QByteArray m_data1;
	QByteArray m_disc_key;
	qint64 m_filesize;
	int m_region_count;
	std::vector<qint64> m_regions;
	QString m_game_id;
	QFile m_iso_file;

public:
	explicit iso_decryptor(QString filePath);

	int decrypt();
};
