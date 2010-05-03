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

#include <list>
#include "Threading.h"

// --------------------------------------------------------------------------------------
//  EventSource< template EvtType >
// --------------------------------------------------------------------------------------

template< typename ListenerType >
class EventSource
{
public:
	typedef typename ListenerType::EvtParams		EvtParams;
	typedef typename std::list< ListenerType* >		ListenerList;
	typedef typename ListenerList::iterator			ListenerIterator;

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
	EventSource()
	{
		m_cache_valid = false;
	}

	virtual ~EventSource() throw() {}

	virtual ListenerIterator Add( ListenerType& listener );
	virtual void Remove( ListenerType& listener );
	virtual void Remove( const ListenerIterator& listenerHandle );

	void Add( ListenerType* listener )
	{
		if( listener == NULL ) return;
		Add( *listener );
	}

	void Remove( ListenerType* listener )
	{
		if( listener == NULL ) return;
		Remove( *listener );
	}

	void Dispatch( const EvtParams& params );

protected:
	virtual ListenerIterator _AddFast_without_lock( ListenerType& listener );
	virtual void _DispatchRaw( ListenerIterator iter, const ListenerIterator& iend, const EvtParams& params );
};

// --------------------------------------------------------------------------------------
//  IEventDispatcher
// --------------------------------------------------------------------------------------
// This class is used as a base interface for EventListeners.  It allows the listeners to do
// customized dispatching of several event types into "user friendly" function overrides.
//
template< typename EvtParams >
class IEventDispatcher
{
protected:
	IEventDispatcher() {}

public:
	virtual ~IEventDispatcher() throw() {}
	virtual void DispatchEvent( const EvtParams& params )=0;
};
