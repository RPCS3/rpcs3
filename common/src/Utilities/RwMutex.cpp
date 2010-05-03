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

#include "PrecompiledHeader.h"
#include "RwMutex.h"

// --------------------------------------------------------------------------------------
//  RwMutex
// --------------------------------------------------------------------------------------
Threading::RwMutex::RwMutex()
{
	pthread_rwlock_init( &m_rwlock, NULL );
}

Threading::RwMutex::~RwMutex() throw()
{
	pthread_rwlock_destroy( &m_rwlock );
}

void Threading::RwMutex::AcquireRead()
{
	pthread_rwlock_rdlock( &m_rwlock );
}

void Threading::RwMutex::AcquireWrite()
{
	pthread_rwlock_wrlock( &m_rwlock );
}

bool Threading::RwMutex::TryAcquireRead()
{
	return pthread_rwlock_tryrdlock( &m_rwlock ) != EBUSY;
}

bool Threading::RwMutex::TryAcquireWrite()
{
	return pthread_rwlock_trywrlock( &m_rwlock ) != EBUSY;
}

void Threading::RwMutex::Release()
{
	pthread_rwlock_unlock( &m_rwlock );
}

// --------------------------------------------------------------------------------------
//  
// --------------------------------------------------------------------------------------
Threading::BaseScopedReadWriteLock::~BaseScopedReadWriteLock() throw()
{
	if( m_IsLocked )
		m_lock.Release();
}

// Provides manual unlocking of a scoped lock prior to object destruction.
void Threading::BaseScopedReadWriteLock::Release()
{
	if( !m_IsLocked ) return;
	m_IsLocked = false;
	m_lock.Release();
}

// --------------------------------------------------------------------------------------
//  ScopedReadLock / ScopedWriteLock
// --------------------------------------------------------------------------------------
Threading::ScopedReadLock::ScopedReadLock( RwMutex& locker )
	: BaseScopedReadWriteLock( locker )
{
	m_IsLocked = true;
	m_lock.AcquireRead();
}

// provides manual locking of a scoped lock, to re-lock after a manual unlocking.
void Threading::ScopedReadLock::Acquire()
{
	if( m_IsLocked ) return;
	m_lock.AcquireRead();
	m_IsLocked = true;
}

Threading::ScopedWriteLock::ScopedWriteLock( RwMutex& locker )
	: BaseScopedReadWriteLock( locker )
{
	m_IsLocked = true;
	m_lock.AcquireWrite();
}

// provides manual locking of a scoped lock, to re-lock after a manual unlocking.
void Threading::ScopedWriteLock::Acquire()
{
	if( m_IsLocked ) return;
	m_lock.AcquireWrite();
	m_IsLocked = true;
}

// Special constructor used by ScopedTryLock
Threading::ScopedWriteLock::ScopedWriteLock( RwMutex& locker, bool isTryLock )
	: BaseScopedReadWriteLock( locker )
{
	//m_IsLocked = isTryLock ? m_lock.TryAcquireWrite() : false;
}
