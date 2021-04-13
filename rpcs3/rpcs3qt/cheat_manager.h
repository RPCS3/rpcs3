#pragma once

#include "util/types.hpp"

#include <QDialog>
#include <QTableWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

#include <string>
#include <vector>
#include <map>

#include "Utilities/cheat_info.h"

class cheat_engine
{
public:
	cheat_engine();

	bool exist(const std::string& game, const u32 offset) const;
	void add(const std::string& game, const std::string& description, const cheat_type type, const u32 offset, const std::string& red_script);
	cheat_info* get(const std::string& game, const u32 offset);
	bool erase(const std::string& game, const u32 offset);

	void import_cheats_from_str(const std::string& str_cheats);
	std::string export_cheats_to_str() const;
	void save() const;

	// Static functions to find/get/set values in ps3 memory
	static bool resolve_script(u32& final_offset, const u32 offset, const std::string& red_script);

	template <typename T>
	static std::vector<u32> search(const T value, const std::vector<u32>& to_filter);

	template <typename T>
	static T get_value(const u32 offset, bool& success);
	template <typename T>
	static bool set_value(const u32 offset, const T value);

	static bool is_addr_safe(const u32 offset);
	static u32 reverse_lookup(const u32 addr, const u32 max_offset, const u32 max_depth, const u32 cur_depth = 0);

	std::map<std::string, std::map<u32, cheat_info>> cheats;

private:
	const std::string m_cheats_filename = "cheats.yml";
};

class cheat_manager_dialog : public QDialog
{
	Q_OBJECT
public:
	cheat_manager_dialog(QWidget* parent = nullptr);
	~cheat_manager_dialog();
	static cheat_manager_dialog* get_dlg(QWidget* parent = nullptr);

	cheat_manager_dialog(cheat_manager_dialog const&) = delete;
	void operator=(cheat_manager_dialog const&) = delete;

protected:
	void update_cheat_list();
	void do_the_search();

	template <typename T>
	T convert_from_QString(const QString& str, bool& success);

	template <typename T>
	bool convert_and_search();
	template <typename T>
	std::pair<bool, bool> convert_and_set(u32 offset);

protected:
	QTableWidget* tbl_cheats = nullptr;
	QListWidget* lst_search = nullptr;

	QLineEdit* edt_value_final = nullptr;
	QPushButton* btn_apply = nullptr;

	QLineEdit* edt_cheat_search_value = nullptr;
	QComboBox* cbx_cheat_search_type = nullptr;

	QPushButton* btn_filter_results = nullptr;

	u32 current_offset{};
	std::vector<u32> offsets_found;

	cheat_engine g_cheat;

private:
	static cheat_manager_dialog* inst;

	QString get_localized_cheat_type(cheat_type type);
};
