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

#include <wx/event.h>

template< typename EvtType >
struct EventListener
{
	typedef void FuncType( void* object, const EvtType& evt );

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

template< typename EvtType >
class EventSource
{
public:
	typedef typename EventListener<EvtType>			ListenerType;
	typedef typename std::list< ListenerType >		ListenerList;
	typedef typename ListenerList::const_iterator	Handle;
	
protected:
	ListenerList m_listeners;

public:
	virtual ~EventSource() throw() {}

	virtual void Remove( const ListenerType& listener )
	{
		m_listeners.remove( listener );
	}

	virtual void Remove( const Handle& listenerHandle )
	{
		m_listeners.erase( listenerHandle );
	}

	virtual Handle AddFast( const ListenerType& listener )
	{
		m_listeners.push_front( listener );
		return m_listeners.begin();
	}

	// Checks for duplicates before adding the event.
	virtual inline void Add( const ListenerType& listener );
	virtual inline void RemoveObject( const void* object );

	Handle Add( void* objhandle, typename ListenerType::FuncType* fnptr )
	{
		return Add( Handle( objhandle, fnptr ) );
	}

	void Remove( void* objhandle, typename ListenerType::FuncType* fnptr )
	{
		Remove( Handle( objhandle, fnptr ) );
	}

	void Dispatch( const wxCommandEvent& evt ) const
	{
		Handle iter = m_listeners.begin();
		while( iter != m_listeners.end() )
		{
			try
			{
				iter->OnEvent( iter->object, evt );
			}
			catch( Exception::RuntimeError& ex )
			{
				Console::Error( L"Ignoring runtime error thrown from event listener: " + ex.FormatDiagnosticMessage() );
			}
			catch( BaseException& ex )
			{
			}
		}
	}
};

// --------------------------------------------------------------------------------------
//  EventListenerBinding
// --------------------------------------------------------------------------------------
// Encapsulated event listener binding, provides the "benefits" of object unwinding.
//
template< typename EvtType = wxCommandEvent >
class EventListenerBinding
{
public:
	typedef typename EventSource<EvtType>::ListenerType	ListenerHandle;
	typedef typename EventSource<EvtType>::Handle		ConstIterator;

protected:
	EventSource<EvtType>&	m_source;
	const ListenerHandle			m_listener;
	ConstIterator					m_iter;
	bool							m_attached;

public:
	EventListenerBinding( EventSource<EvtType>& source, const ListenerHandle& listener, bool autoAttach=true ) :
		m_source( source )
	,	m_listener( listener )
	,	m_attached( false )
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


// Checks for duplicates before adding the event.
template< typename EvtType >
void EventSource<EvtType>::Add( const ListenerType& listener )
{
	if( !pxAssertDev( listener.OnEvent != NULL, "NULL listener callback function." ) ) return;

	Handle iter = m_listeners.begin();
	while( iter != m_listeners.end() )
	{
		if( *iter == listener ) return;
		++iter;
	}
	AddFast( listener );
}

// removes all listeners which reference the given object.  Use for assuring object deletion.
template< typename EvtType >
void EventSource<EvtType>::RemoveObject( const void* object )
{
	class PredicatesAreTheThingsOfNightmares
	{
	protected:
		const void* const m_object_match;

	public:
		PredicatesAreTheThingsOfNightmares( const void* objmatch ) : m_object_match( objmatch ) { }

		bool operator()( const ListenerType& src )
		{
			return src.object == m_object_match;
		}
	};
	m_listeners.remove_if( PredicatesAreTheThingsOfNightmares( object ) );
}

