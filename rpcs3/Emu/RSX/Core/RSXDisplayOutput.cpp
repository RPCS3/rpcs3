#include "stdafx.h"
#include "RSXDisplayOutput.h"

#include "Emu/emu_callbacks.h"
#include "Emu/System.h"
#include "Emu/RSX/GSFrameBase.h"

namespace rsx
{
	display_output::display_output()
	{
		if (auto gs_frame = g_emu_callbacks.get_gs_frame())
		{
			m_frame = gs_frame.release();
		}
	}

	display_output::~display_output()
	{
		if (m_frame && !m_continuous_mode)
		{
			m_frame->close();
		}
	}

	void display_output::on_exit()
	{
		if (m_frame && !m_continuous_mode)
		{
			m_frame->hide();
		}
	}

	void display_output::prepare_game_window(video_renderer type)
	{
		if (!m_frame)
		{
			return;
		}

		if (m_frame->renderer() == type)
		{
			// Same backend (e.g. only the adapter changed). Reuse the same frame
			m_frame->reset();
			return;
		}

		// The window is set up for a specific backend. Some platforms cannot share it between APIs, so close and reopen it
		GSFrameBase* frame = nullptr;
		Emu.BlockingCallFromMainThread([&]()
		{
			if (auto gs_frame = g_emu_callbacks.replace_gs_frame())
			{
				frame = gs_frame.release();
			}
		});

		m_frame = frame;
	}

	f64 display_output::refresh_rate() const
	{
		if (m_frame)
		{
			return m_frame->client_display_rate();
		}

		// Minimum
		return 20.;
	}
}
