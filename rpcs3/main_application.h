#pragma once

#include "stdafx.h"

#include "Emu/System.h"

#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/GL/GLGSRender.h"
#ifdef _MSC_VER
#include "Emu/RSX/D3D12/D3D12GSRender.h"
#endif
#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKGSRender.h"
#endif

#include <QWindow>

class main_application
{
public:
	virtual void Init() = 0;

	static bool InitializeEmulator(const std::string& user, bool force_init);

protected:
	virtual QThread* get_thread() = 0;

	EmuCallbacks CreateCallbacks();

	QWindow* m_game_window = nullptr; // (Currently) only needed so that pad handlers have a valid target for event filtering.
};
