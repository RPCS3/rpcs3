/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#include <list>
#include "Threading.h"

class wxCommandEvent;

// __evt_fastcall : Work-around for a GCC 4.3 compilation bug.  The templated FuncType
// throws type mismatches if we have a __fastcall qualifier. >_< --air

#if defined( __GNUC__ ) && (__GNUC__ < 4 ) || ((__GNUC__ == 4) && ( __GNUC_MINOR__ <= 3 ))
#	define __evt_fastcall
#else
#	define __evt_fastcall __fastcall
#endif

// --------------------------------------------------------------------------------------
//  EventListener< typename EvtType >
// --------------------------------------------------------------------------------------

template< typename EvtType >
struct EventListener
{
	typedef void __evt_fastcall FuncType( void* object, EvtType& evt );

	void*		object;
	FuncType*	OnEvent;

	EventListener( FuncType* fnptr )
	{
		object	= NULL;
		OnEvent	= fnptr;
	}

	EventListener( void* objhandle, FuncType* fnptr )
	{
		object	= objhandle;
		OnEvent	= fnptr;
	}

	bool operator ==( const EventListener& right ) const
	{
		return (object == right.object) && (OnEvent == right.OnEvent);
	}

	bool operator !=( const EventListener& right ) const
	{
		return this->operator ==(right);
	}
};

// --------------------------------------------------------------------------------------
//  EventSource< template EvtType >
// --------------------------------------------------------------------------------------

template< typename EvtType >
class EventSource
{
public:
	typedef EventListener< EvtType >				ListenerType;
	typedef typename std::list< ListenerType >		ListenerList;
	typedef typename ListenerList::iterator			Handle;

protected:
	typedef typename ListenerList::const_iterator	ConstIterator;

	ListenerList	m_listeners;

	// This is a cached copy of the listener list used to handle standard dispatching, which
	// allows for self-modification of the EventSource's listener list by the listeners.
	// Translation: The dispatcher uses this copy instead, to avoid iterator invalidation.
	ListenerList	m_cache_copy;
	bool			m_cache_valid;
	
	Threading::Mutex m_listeners_lock;

public:
	EventSource() : m_cache_valid( false )
	{
	}

	virtual ~EventSource() throw() {}

	virtual void Remove( const ListenerType& listener );
	virtual void Remove( const Handle& listenerHandle );

	Handle AddFast( const ListenerType& listener );
	void Add( void* objhandle, typename ListenerType::FuncType* fnptr );
	void Remove( void* objhandle, typename ListenerType::FuncType* fnptr );

	// Checks for duplicates before adding the event.
	virtual void Add( const ListenerType& listener );
	virtual void RemoveObject( const void* object );
	void Dispatch( EvtType& evt );

protected:
	virtual Handle _AddFast_without_lock( const ListenerType& listener );
	inline void _DispatchRaw( ConstIterator iter, const ConstIterator& iend, EvtType& evt );
};

// --------------------------------------------------------------------------------------
//  EventListenerBinding< typename EvtType >
// --------------------------------------------------------------------------------------
// Encapsulated event listener binding, provides the "benefits" of object unwinding.
//
template< typename EvtType >
class EventListenerBinding
{
public:
	typedef typename EventSource<EvtType>::ListenerType	ListenerHandle;
	typedef typename EventSource<EvtType>::Handle		ConstIterator;

protected:
	EventSource<EvtType>&		m_source;
	const ListenerHandle		m_listener;
	ConstIterator				m_iter;
	bool						m_attached;

public:
	EventListenerBinding( EventSource<EvtType>& source, const ListenerHandle& listener, bool autoAttach=true )
		: m_source( source )
		, m_listener( listener )
		, m_attached( false )
	{
		// If you want to assert on null pointers, you'll need to do the check yourself.  There's
		// too many cases where silently ignoring null pointers is the desired behavior.
		//if( !pxAssertDev( listener.OnEvent != NULL, "NULL listener callback function." ) ) return;
		if( autoAttach ) Attach();
	}

	virtual ~EventListenerBinding() throw()
	{
		Detach();
	}

	void Detach()
	{
		if( !m_attached ) return;
		m_source.Remove( m_iter );
		m_attached = false;
	}

	void Attach()
	{
		if( m_attached || (m_listener.OnEvent == NULL) ) return;
		m_iter = m_source.AddFast( m_listener );
		m_attached = true;
	}
};

typedef EventSource<wxCommandEvent>				CmdEvt_Source;
typedef EventListener<wxCommandEvent>			CmdEvt_Listener;
typedef EventListenerBinding<wxCommandEvent>	CmdEvt_ListenerBinding;

#define EventSource_ImplementType( tname )	template class EventSource<tname>
