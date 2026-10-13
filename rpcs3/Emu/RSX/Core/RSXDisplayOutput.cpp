#include "stdafx.h"
#include "RSXDisplayOutput.h"

#include "Emu/emu_callbacks.h"
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
