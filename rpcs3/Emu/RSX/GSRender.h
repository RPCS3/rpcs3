#pragma once
#include "Emu/RSX/RSXThread.h"
#include <memory>

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
	virtual void flip(draw_context_t ctx) = 0;
	virtual size2i client_size() = 0;

protected:
	virtual void delete_context(void* ctx) = 0;
	virtual void* make_context() = 0;
};

struct GSRender : public rsx::thread
{
	virtual ~GSRender() = default;
	virtual void close()=0;
};
