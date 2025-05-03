#pragma once

#include "util/types.hpp"
#include "Utilities/Thread.h"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>

#include <string>
#include <map>

class QLineEdit;
class QCheckBox;
class QComboBox;
class QLabel;
class QThread;
class QHBoxLayout;
class QKeyEvent;

class cpu_thread;
class CPUDisAsm;

namespace utils
{
	class shm;
}

namespace rsx
{
	class thread;
}

enum search_mode : unsigned
{
	no_mode          = 1,
	clear_modes      = 2,
	as_string        = 4,
	as_hex           = 8,
	as_f64           = 16,
	as_f32           = 32,
	as_inst          = 64,
	as_regex_inst    = 128,
	as_fake_spu_inst = 256,
	as_regex_fake_spu_inst = 512,
	search_mode_last = 1024,
};

class memory_viewer_panel final : public QDialog
{
	Q_OBJECT

public:
	memory_viewer_panel(QWidget* parent, std::shared_ptr<CPUDisAsm> disasm, u32 addr = 0, std::function<cpu_thread*()> func = []() -> cpu_thread* { return {}; });
	~memory_viewer_panel();

	static void ShowAtPC(u32 pc, std::function<cpu_thread*()> func = nullptr);

	enum class color_format : int
	{
		RGB,
		ARGB,
		RGBA,
		ABGR,
		G8,
		G32MAX
	};
	Q_ENUM(color_format)

protected:
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	u32 m_addr = 0;
	u32 m_colcount = 0;
	u32 m_rowcount = 0;
	u32 m_min_height = 0;

	QLineEdit* m_addr_line = nullptr;

	QLabel* m_mem_addr = nullptr;
	QLabel* m_mem_hex = nullptr;
	QLabel* m_mem_ascii = nullptr;

	QFontMetrics* m_fontMetrics = nullptr;

	static constexpr int c_pad_memory_labels = 15;

	QLineEdit* m_search_line = nullptr;
	QCheckBox* m_chkbox_case_insensitive = nullptr;
	QComboBox* m_cbox_input_mode = nullptr;
	QHBoxLayout* m_hbox_mem_panel = nullptr;
	QThread* m_search_thread = nullptr;

	const std::function<cpu_thread*()> m_get_cpu;
	const thread_class m_type;
	const std::add_pointer_t<rsx::thread> m_rsx;
	const std::shared_ptr<utils::shm> m_spu_shm;
	const u32 m_addr_mask;

	std::shared_ptr<CPUDisAsm> m_disasm;

	const void* m_ptr = nullptr;
	usz m_size = 0;

	search_mode m_modes{};

	std::string getHeaderAtAddr(u32 addr) const;
	void scroll(s32 steps);
	void* to_ptr(u32 addr, u32 size = 1) const;
	void SetPC(const uint pc);

	void ShowMemory();

	void ShowImage(QWidget* parent, u32 addr, color_format format, u32 width, u32 height, bool flipv) const;
	u64 OnSearch(std::string wstr, u32 mode);

	void keyPressEvent(QKeyEvent* event) override;
};

// Lifetime management with IDM
struct memory_viewer_handle
{
	static constexpr u32 id_base = 1;
	static constexpr u32 id_step = 1;
	static constexpr u32 id_count = 2048;
	SAVESTATE_INIT_POS(33); // Of course not really used

	template <typename... Args> requires (std::is_constructible_v<memory_viewer_panel, Args&&...>)
	memory_viewer_handle(Args&&... args)
		: m_mvp(new memory_viewer_panel(std::forward<Args>(args)...))
	{
	}

	~memory_viewer_handle() { m_mvp->close(); m_mvp->deleteLater(); }

private:
	const std::add_pointer_t<memory_viewer_panel> m_mvp;
};

struct memory_viewer_fxo
{
	std::map<thread_class, memory_viewer_panel*> last_opened;

	memory_viewer_fxo() = default;
	memory_viewer_fxo(const memory_viewer_fxo&) = delete;
	memory_viewer_fxo& operator=(const memory_viewer_fxo&) = delete;
};
