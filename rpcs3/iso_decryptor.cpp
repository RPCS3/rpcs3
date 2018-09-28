

#include "iso_decryptor.h"

#include <QDir>
#include <QUrl>
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QFileDevice>
#include <QDesktopServices>

#include "Crypto/aes.h"
#include "Utilities/Log.h"
#include "Utilities/File.h"

iso_decryptor::iso_decryptor(QString filePath)
    : m_iso_file(filePath)
{
}

int iso_decryptor::decrypt()
{

	if (!(m_iso_file.permissions() & QFileDevice::ReadUser))
	{
		return -1;
	}

	if (!m_iso_file.open(QIODevice::ReadOnly))
	{
		return 0;
	}

	m_filesize = m_iso_file.size();

	auto readInt  = [this](int length) { return m_iso_file.read(length).toHex().toInt(nullptr, 16); };
	auto readUint = [this](int length) { return m_iso_file.read(length).toHex().toUInt(nullptr, 16); };

	m_iso_file.seek(0);
	m_region_count = readInt(4);
	m_iso_file.skip(4); // Skip unused bytes

	for (int i = 0; i < m_region_count * 2; i++)
	{
		m_regions.push_back((qint64(readUint(4))) * 2048);
	}

	if (m_regions.back() > m_filesize)
	{
		return -2;
	}

	m_regions.push_back(m_filesize);

	m_iso_file.seek(SECTOR + 16); // game_id is located 16 bytes after the start of sector 2
	m_game_id = QString::fromUtf8(m_iso_file.read(16)).trimmed();

	m_iso_file.seek(3968); // data1 is located at a specific location in sector 2
	m_data1 = m_iso_file.read(16);

	aes_context enc;
	aes_setkey_enc(&enc, keyCast(ISO_SECRET), ISO_SECRET.length() * 8); // 128

	unsigned char disc_key[16];
	aes_crypt_cbc(&enc, AES_ENCRYPT, m_data1.length(), keyCast(ISO_IV), keyCast(m_data1), disc_key);
	m_disc_key = QByteArray::fromRawData((const char*)disc_key, 16);

	QString dir = QString::fromStdString(fs::get_config_dir()) + QString("dev_hdd0/disc/") + m_game_id;
	if (!QDir().mkpath(dir))
	{
		return -4;
	}

	QString filepath = dir + "/" + m_game_id + ".iso";
	QFile out_iso(filepath);

	if (!out_iso.open(QIODevice::WriteOnly))
	{
		return -3;
	}

	// Set AES context for decrypting data:
	aes_context dec;
	aes_setkey_dec(&dec, keyCast(m_disc_key), m_disc_key.length() * 8); // 128

	m_iso_file.seek(0);
	QByteArray buffer;
	buffer.resize(SECTOR);
	QByteArray iv;
	iv.resize(m_data1.length());
	unsigned char decrypted[SECTOR];

	int current_region = 1;
	bool encrypted     = false;
	while (!m_iso_file.atEnd())
	{
		if (encrypted)
		{
			int sector_num = m_iso_file.pos() / 2048;
			for (int i = 15; i >= 0; i--)
			{
				iv[i] = (unsigned char)(sector_num & 0xFF);
				sector_num >>= 8;
			}

			m_iso_file.read(buffer.data(), SECTOR);

			aes_crypt_cbc(&dec, AES_DECRYPT, SECTOR, keyCast(iv), keyCast(buffer), decrypted);

			if (out_iso.write((const char*)decrypted, SECTOR) < SECTOR)
			{
				return -3;
			}
		}
		else
		{
			m_iso_file.read(buffer.data(), SECTOR);
			if (out_iso.write(buffer) < SECTOR)
			{
				return -3;
			}
		}

		if (m_iso_file.pos() == m_regions[current_region])
		{
			encrypted = !encrypted;
			current_region++;
		}
	}

	QUrl url("file:" + dir);
	QDesktopServices::openUrl(url);

	return 1;
}
