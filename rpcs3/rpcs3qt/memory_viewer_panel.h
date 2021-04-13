#pragma once

#include "util/types.hpp"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>

#include <string>

class cpu_thread;

namespace utils
{
	class shm;
}

namespace rsx
{
	class thread;
}

class memory_viewer_panel : public QDialog
{
	Q_OBJECT

public:
	memory_viewer_panel(QWidget* parent, u32 addr = 0, std::function<cpu_thread*()> func = []() -> cpu_thread* { return {}; });
	~memory_viewer_panel();

	enum class color_format : int
	{
		RGB,
		ARGB,
		RGBA,
		ABGR
	};
	Q_ENUM(color_format)

protected:
	void wheelEvent(QWheelEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	u32 m_addr;
	u32 m_colcount;
	u32 m_rowcount;

	QLineEdit* m_addr_line;

	QLabel* m_mem_addr;
	QLabel* m_mem_hex;
	QLabel* m_mem_ascii;

	QFontMetrics* m_fontMetrics;

	enum class thread_type
	{
		none,
		ppu,
		spu,
		rsx,
	};

	const std::function<cpu_thread*()> m_get_cpu;
	const thread_type m_type;
	const std::add_pointer_t<rsx::thread> m_rsx;
	const std::shared_ptr<utils::shm> m_spu_shm;
	const u32 m_addr_mask;

	std::string getHeaderAtAddr(u32 addr) const;
	void scroll(s32 steps);
	void* to_ptr(u32 addr, u32 size = 1) const;
	void SetPC(const uint pc);

	virtual void ShowMemory();

	void ShowImage(QWidget* parent, u32 addr, color_format format, u32 width, u32 height, bool flipv) const;
};

// Lifetime management with IDM
struct memory_viewer_handle
{
	static constexpr u32 id_base = 1;
	static constexpr u32 id_step = 1;
	static constexpr u32 id_count = 2048;

	template <typename... Args>
	memory_viewer_handle(Args&&... args)
		: m_mvp(new memory_viewer_panel(std::forward<Args>(args)...))
	{
	}

	~memory_viewer_handle() { m_mvp->close(); m_mvp->deleteLater(); }

private:
	const std::add_pointer_t<memory_viewer_panel> m_mvp;
};
