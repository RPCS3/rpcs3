#pragma once

#include "Emu/RSX/RSXThread.h"
#include <memory>

struct RSXDebuggerProgram
{
	u32 id;
	u32 vp_id;
	u32 fp_id;
	std::string vp_shader;
	std::string fp_shader;
	bool modified;

	RSXDebuggerProgram()
		: modified(false)
	{
	}
};

using RSXDebuggerPrograms = std::vector<RSXDebuggerProgram>;

using draw_context_t = std::shared_ptr<void>;

class GSFrameBase
{
public:
	GSFrameBase() = default;
	GSFrameBase(const GSFrameBase&) = delete;

	virtual void close() = 0;
	virtual bool shown() = 0;
	virtual void hide() = 0;
	virtual void show() = 0;

	draw_context_t new_context();

	virtual void set_current(draw_context_t ctx) = 0;
	virtual void flip(draw_context_t ctx, bool skip_frame=false) = 0;
	virtual int client_width() = 0;
	virtual int client_height() = 0;

	virtual void* handle() const = 0;

protected:
	virtual void delete_context(void* ctx) = 0;
	virtual void* make_context() = 0;
};

class GSRender : public rsx::thread
{
protected:
	GSFrameBase* m_frame;
	draw_context_t m_context;

public:
	GSRender();
	virtual ~GSRender();

	void on_init_rsx() override;
	void on_init_thread() override;

	void flip(int buffer) override;
};
