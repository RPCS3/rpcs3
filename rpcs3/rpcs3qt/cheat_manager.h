#pragma once

#include "util/types.hpp"

#include <QDialog>
#include <QTreeWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>

#include <string>
#include <vector>
#include <map>

#include "Utilities/cheat_info.h"
#include "gui_settings.h"
#include "Utilities/File.h"

class cheat_variables_dialog : public QDialog
{
	Q_OBJECT
public:
	cheat_variables_dialog(const QString& title, const cheat_entry& entry, QWidget* parent = nullptr);

	std::unordered_map<std::string, std::string> get_choices();

private:
	std::vector<std::pair<std::string, QComboBox*>> m_combos;
	std::unordered_map<std::string, std::string> m_var_choices;
};

class cheat_manager_dialog : public QDialog
{
	Q_OBJECT
public:
	cheat_manager_dialog(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = nullptr);
	~cheat_manager_dialog();
	static cheat_manager_dialog* get_dlg(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = nullptr);

	cheat_manager_dialog(cheat_manager_dialog const&) = delete;
	void operator=(cheat_manager_dialog const&) = delete;

private:
	void refresh_tree();
	void filter_cheats(const QString& game_id, const QString& term);

	fs::file open_cheats(bool load);
	void load_cheats();
	void save_cheats();

	void add_cheats(std::string name, std::map<std::string, cheat_entry> to_add);

private:
	static constexpr std::string_view m_cheats_filename = "cheatsv2.yml";

	inline static const std::string m_author_key    = "Author";
	inline static const std::string m_comments_key  = "Comments";
	inline static const std::string m_type_key      = "Type";
	inline static const std::string m_codes_key     = "Codes";
	inline static const std::string m_variables_key = "Variables";

	std::map<std::string, std::map<std::string, cheat_entry>> m_cheats;

	inline static cheat_manager_dialog* m_inst = nullptr;
	std::shared_ptr<gui_settings> m_gui_settings;

	QTreeWidget* m_tree         = nullptr;
	QLineEdit* m_edt_filter     = nullptr;
	QCheckBox* m_chk_search_cur = nullptr;

	QLineEdit* m_edt_author        = nullptr;
	QPlainTextEdit* m_pte_comments = nullptr;
};
