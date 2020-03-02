#pragma once

#include <QDialog>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>

class skylander_dialog : public QDialog
{
public:
	skylander_dialog(QWidget* parent);
	~skylander_dialog();
	static skylander_dialog* get_dlg(QWidget* parent);

	skylander_dialog(skylander_dialog const&) = delete;
	void operator=(skylander_dialog const&) = delete;

protected:
	// Update the edits from skylander loaded in memory
	void update_edits();
	// Parse edits and apply them to skylander in memory
	void process_edits();

	// Creates a new skylander
	void new_skylander();
	// Loads a skylander
	void load_skylander();

	u16 skylander_crc16(u16 init_value, const u8* buffer, u32 size);
	// Get hash used for encryption of block
	void get_hash(u8 block, std::array<u8, 16>& res_hash);
	// encrypt a block to memory
	void set_block(const u8 block, const std::array<u8, 16>& to_encrypt);
	// decrypt a block in memory
	void get_block(const u8 block, std::array<u8, 16>& decrypted);

	// get the active Area(0x08 or 0x24)
	u8 get_active_block();

	void set_checksums();

protected:
	QLineEdit* edit_curfile    = nullptr;
	QComboBox* combo_skylist   = nullptr;
	QLineEdit* edit_skyid      = nullptr;
	QLineEdit* edit_skyxp      = nullptr;
	QLineEdit* edit_skymoney   = nullptr;
	QPushButton* button_update = nullptr;

private:
	static skylander_dialog* inst;
};
