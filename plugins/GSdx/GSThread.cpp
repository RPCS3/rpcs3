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

#include "StdAfx.h"
#include "GSThread.h"

GSThread::GSThread()
	: m_ThreadId(0)
	, m_hThread(NULL)
{
}

GSThread::~GSThread()
{
	CloseThread();
}

DWORD WINAPI GSThread::StaticThreadProc(LPVOID lpParam)
{
	((GSThread*)lpParam)->ThreadProc();

	return 0;
}

void GSThread::CreateThread()
{
	m_hThread = ::CreateThread(NULL, 0, StaticThreadProc, (LPVOID)this, 0, &m_ThreadId);
}

void GSThread::CloseThread()
{
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
}

