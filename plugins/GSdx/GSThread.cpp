/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSThread.h"

GSThread::GSThread()
{
    #ifdef _WINDOWS

	m_ThreadId = 0;
	m_hThread = NULL;

    #else

    #endif
}

GSThread::~GSThread()
{
	CloseThread();
}

#ifdef _WINDOWS

DWORD WINAPI GSThread::StaticThreadProc(void* lpParam)
{
	((GSThread*)lpParam)->ThreadProc();

	return 0;
}

#else

void* GSThread::StaticThreadProc(void* param)
{
	((GSThread*)param)->ThreadProc();

	pthread_exit(NULL);

	return NULL;
}

#endif

void GSThread::CreateThread()
{
    #ifdef _WINDOWS

	m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (void*)this, 0, &m_ThreadId);

	#else

    pthread_attr_init(&m_thread_attr);
    pthread_create(&m_thread, &m_thread_attr, StaticThreadProc, (void*)this);

	#endif
}

void GSThread::CloseThread()
{
    #ifdef _WINDOWS

	if(m_hThread != NULL)
	{
		if(WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0)
		{
			TerminateThread(m_hThread, 1);
		}

		CloseHandle(m_hThread);

		m_hThread = NULL;
		m_ThreadId = 0;
	}

    #else

    void* ret = NULL;

    pthread_join(m_thread, &ret);
    pthread_attr_destroy(&m_thread_attr);

    #endif
}

