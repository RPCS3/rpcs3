#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "GSManager.h"
#include "GSRender.h"

GSLock::GSLock(GSRender& renderer, GSLockType type)
	: m_renderer(renderer)
	, m_type(type)
{
	switch (m_type)
	{
	case GS_LOCK_NOT_WAIT: m_renderer.m_cs_main.lock(); break;
	case GS_LOCK_WAIT_FLIP: m_renderer.m_sem_flip.wait(); break;
	}
}

GSLock::~GSLock()
{
	switch (m_type)
	{
	case GS_LOCK_NOT_WAIT: m_renderer.m_cs_main.unlock(); break;
	case GS_LOCK_WAIT_FLIP: m_renderer.m_sem_flip.try_post(); break;
	}
}

GSLockCurrent::GSLockCurrent(GSLockType type) : GSLock(Emu.GetGSManager().GetRender(), type)
{
}
