/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Threading.h"

namespace Threading
{
// --------------------------------------------------------------------------------------
//  RwMutex
// --------------------------------------------------------------------------------------
	class RwMutex
	{
		DeclareNoncopyableObject(RwMutex);

	protected:
		pthread_rwlock_t	m_rwlock;

	public:
		RwMutex();
		virtual ~RwMutex() throw();
		
		virtual void AcquireRead();
		virtual void AcquireWrite();
		virtual bool TryAcquireRead();
		virtual bool TryAcquireWrite();

		virtual void Release();
	};

// --------------------------------------------------------------------------------------
//  BaseScopedReadWriteLock
// --------------------------------------------------------------------------------------
	class BaseScopedReadWriteLock
	{
		DeclareNoncopyableObject(BaseScopedReadWriteLock);

	protected:
		RwMutex&	m_lock;
		bool		m_IsLocked;

	public:
		BaseScopedReadWriteLock( RwMutex& locker )
			: m_lock( locker )
		{
		}

		virtual ~BaseScopedReadWriteLock() throw();

		void Release();
		bool IsLocked() const { return m_IsLocked; }
	};

// --------------------------------------------------------------------------------------
//  ScopedReadLock / ScopedWriteLock
// --------------------------------------------------------------------------------------
	class ScopedReadLock : public BaseScopedReadWriteLock
	{
	public:
		ScopedReadLock( RwMutex& locker );
		virtual ~ScopedReadLock() throw() {}

		void Acquire();
	};

	class ScopedWriteLock : public BaseScopedReadWriteLock
	{
	public:
		ScopedWriteLock( RwMutex& locker );
		virtual ~ScopedWriteLock() throw() {}

		void Acquire();

	protected:
		ScopedWriteLock( RwMutex& locker, bool isTryLock );
	};
}
