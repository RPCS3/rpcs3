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
	class TlsVariable
	{
		DeclareNoncopyableObject(TlsVariable);

	protected:
		pthread_key_t	m_thread_key;
		T				m_initval;

	public:
		TlsVariable();
		TlsVariable( T initval );

		virtual ~TlsVariable() throw();
		T* GetPtr() const;
		T& GetRef() const { return *GetPtr(); }

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

		operator T&() const { return GetRef(); }

	protected:
		void CreateKey();
	};
};

template< typename T >
Threading::TlsVariable<T>::TlsVariable()
{
	CreateKey();
}

template< typename T >
Threading::TlsVariable<T>::TlsVariable( T initval )
{
	CreateKey();
	m_initval = initval;
}

template< typename T >
Threading::TlsVariable<T>::~TlsVariable() throw()
{
	if( m_thread_key != NULL )
		pthread_key_delete( m_thread_key );
}

template< typename T >
T* Threading::TlsVariable<T>::GetPtr() const
{
	T* result = (T*)pthread_getspecific( m_thread_key );
	if( result == NULL )
	{
		pthread_setspecific( m_thread_key, result = (T*)_aligned_malloc( sizeof(T), 16 ) );
		if( result == NULL )
			throw Exception::OutOfMemory( "Out of memory allocating thread local storage variable." );
		*result = m_initval;
	}
	return result;
}

template< typename T >
void Threading::TlsVariable<T>::CreateKey()
{
	if( 0 != pthread_key_create(&m_thread_key, _aligned_free) )
	{
		pxFailRel( "Thread Local Storage Error: key creation failed." );
	}
}
