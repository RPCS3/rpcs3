#pragma once
#include "Emu/RSX/RSXThread.h"

using draw_context_t = std::shared_ptr<void>;

class GSFrameBase
{
protected:
	std::wstring m_title_message;

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

	virtual void* handle() const = 0;
	void title_message(const std::wstring&);

protected:
	virtual void delete_context(void* ctx) = 0;
	virtual void* make_context() = 0;
};

enum class frame_type
{
	Null,
	OpenGL,
	DX12
};

class GSRender : public rsx::thread
{
protected:
	GSFrameBase* m_frame;
	draw_context_t m_context;

public:
	GSRender(frame_type type);
	virtual ~GSRender();

	void oninit() override;
	void oninit_thread() override;

	void close();
	void flip(int buffer) override;
};

enum GSLockType
{
	GS_LOCK_NOT_WAIT,
	GS_LOCK_WAIT_FLIP,
};

struct GSLock
{
private:
	GSRender& m_renderer;
	GSLockType m_type;

public:
	GSLock(GSRender& renderer, GSLockType type);

	~GSLock();
};

struct GSLockCurrent : GSLock
{
	GSLockCurrent(GSLockType type);
};