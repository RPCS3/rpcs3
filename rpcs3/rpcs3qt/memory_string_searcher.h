#pragma once

#include "util/types.hpp"

#include <QDialog>

class QLineEdit;
class QCheckBox;
class QComboBox;

class CPUDisAsm;

class memory_string_searcher : public QDialog
{
	Q_OBJECT

	QLineEdit* m_addr_line = nullptr;
	QCheckBox* m_chkbox_case_insensitive = nullptr;
	QComboBox* m_cbox_input_mode = nullptr;

	std::shared_ptr<CPUDisAsm> m_disasm;

	const void* m_ptr;
	usz m_size;

public:
	explicit memory_string_searcher(QWidget* parent, std::shared_ptr<CPUDisAsm> disasm, std::string_view title = {});

private Q_SLOTS:
	void OnSearch();
};

// Lifetime management with IDM
struct memory_searcher_handle
{
	static constexpr u32 id_base = 1;
	static constexpr u32 id_step = 1;
	static constexpr u32 id_count = 2048;

	template <typename... Args> requires (std::is_constructible_v<memory_string_searcher, Args&&...>)
	memory_searcher_handle(Args&&... args)
		: m_mss(new memory_string_searcher(std::forward<Args>(args)...))
	{
	}

	~memory_searcher_handle() { m_mss->close(); m_mss->deleteLater(); }

private:
	const std::add_pointer_t<memory_string_searcher> m_mss;
};
