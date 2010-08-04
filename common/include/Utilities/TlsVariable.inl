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

#if PCSX2_THREAD_LOCAL
#	define DeclareTls(x) __threadlocal x
#else
#	define DeclareTls(x) Threading::TlsVariable<x>
#endif

namespace Threading
{
// --------------------------------------------------------------------------------------
//  TlsVariable - Thread local storage
// --------------------------------------------------------------------------------------
// Wrapper class for pthread_getspecific, which is pthreads language for "thread local
// storage."  This class enables code to act as a drop-in replacement for compiler-native
// thread local storage (typically specified via __threadlocal).  Mac OS/X (Darwin) does
// not have TLS, which is the main reason for this class existing.
//
// Performance considerations: While certainly convenient, performance of this class can
// be sub-optimal when the operator overloads are used, since each one will most likely
// result in repeated calls to pthread_getspecific.  (if the function inlines then it
// should actually optimize well enough, but I doubt it does).
//
template< typename T >
class BaseTlsVariable
{
	DeclareNoncopyableObject(BaseTlsVariable);

protected:
	pthread_key_t	m_thread_key;
	bool			m_IsDisposed;

public:
	BaseTlsVariable();

	virtual ~BaseTlsVariable() throw()
	{
		Dispose();
	}

	T* GetPtr() const;
	T& GetRef() const { return *GetPtr(); }

	operator T&() const		{ return GetRef(); }
	T* operator->() const	{ return GetPtr(); }

	void Dispose()
	{
		if (!m_IsDisposed)
		{
			m_IsDisposed = true;
			KillKey();
		}
	}

protected:
	void CreateKey();
	void KillKey();

	virtual void CreateInstance( T* result ) const
	{
		new (result) T();
	}

	static void _aligned_delete_and_free( void* ptr )
	{
		if (!ptr) return;
		((T*)ptr)->~T();
		_aligned_free(ptr);
	}
};


template< typename T >
class TlsVariable : public BaseTlsVariable<T>
{
	DeclareNoncopyableObject(TlsVariable);

protected:
	T				m_initval;

public:
	TlsVariable() {}
	TlsVariable( const T& initval )
		: m_initval(initval) { }

	virtual ~TlsVariable() throw()
	{
		// disable the parent cleanup.  This leaks memory blocks, but its necessary because
		// TLS is expected to be persistent until the very end of execution on the main thread.
		// Killing the pthread_key at all will lead to the console logger death, etc.
		m_IsDisposed = true;
	}

	// This is needed the C++ standard likes making life suck for programmers.
	using BaseTlsVariable<T>::GetRef;

	TlsVariable& operator=( const T& src )
	{
		GetRef() = src;
		return *this;
	}

	bool operator==( const T& src ) const	{ return GetRef() == src; }
	bool operator!=( const T& src ) const	{ return GetRef() != src; }
	bool operator>( const T& src ) const	{ return GetRef() > src; }
	bool operator<( const T& src ) const	{ return GetRef() < src; }
	bool operator>=( const T& src ) const	{ return GetRef() >= src; }
	bool operator<=( const T& src ) const	{ return GetRef() <= src; }

	T operator+( const T& src ) const		{ return GetRef() + src; }
	T operator-( const T& src ) const		{ return GetRef() - src; }

	void operator+=( const T& src )			{ GetRef() += src; }
	void operator-=( const T& src )			{ GetRef() -= src; }
	
protected:
	virtual void CreateInstance( T* result ) const
	{
		new (result) T(m_initval);
	}
};
};

template< typename T >
Threading::BaseTlsVariable<T>::BaseTlsVariable()
{
	m_IsDisposed = false;
	CreateKey();
}

template< typename T >
void Threading::BaseTlsVariable<T>::KillKey()
{
	if (!m_thread_key) return;

	// Delete the handle for the current thread (which should always be the main/UI thread!)
	// This is needed because pthreads does *not* clean up the dangling objects when you delete
	// the key.  The TLS for the process main thread will only be deleted when the process
	// ends; which is too damn late (it shows up int he leaked memory blocks).

	BaseTlsVariable<T>::_aligned_delete_and_free( pthread_getspecific(m_thread_key) );

	pthread_key_delete( m_thread_key );
	m_thread_key = NULL;
}

template< typename T >
T* Threading::BaseTlsVariable<T>::GetPtr() const
{
	T* result = (T*)pthread_getspecific( m_thread_key );
	if( result == NULL )
	{
		pthread_setspecific( m_thread_key, result = (T*)_aligned_malloc(sizeof(T), 16) );
		CreateInstance(result);
		if( result == NULL )
			throw Exception::OutOfMemory( L"thread local storage variable instance" );
	}
	return result;
}

template< typename T >
void Threading::BaseTlsVariable<T>::CreateKey()
{
	if( 0 != pthread_key_create(&m_thread_key, BaseTlsVariable<T>::_aligned_delete_and_free) )
	{
		pxFailRel( "Thread Local Storage Error: key creation failed.  This will most likely lead to a rapid application crash." );
	}
}
