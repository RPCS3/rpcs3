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

template< typename EvtType >
class PredicatesAreTheThingsOfNightmares
{
	typedef EventListener< EvtType >	ListenerType;

protected:
	const void* const m_object_match;

public:
	PredicatesAreTheThingsOfNightmares( const void* objmatch ) : m_object_match( objmatch ) { }

	bool operator()( const ListenerType& src ) const
	{
		return src.object == m_object_match;
	}
};

// removes all listeners which reference the given object.  Use for assuring object deletion.
template< typename EvtType >
void EventSource<EvtType>::RemoveObject( const void* object )
{
	m_cache_valid = false;
	m_listeners.remove_if( PredicatesAreTheThingsOfNightmares<EvtType>( object ) );
}

template< typename EvtType >
__forceinline void EventSource<EvtType>::_DispatchRaw( ConstIterator iter, const ConstIterator& iend, EvtType& evt )
{
	while( iter != iend )
	{
		try
		{
			iter->OnEvent( iter->object, evt );
		}
		catch( Exception::RuntimeError& ex )
		{
			Console.Error( L"Ignoring runtime error thrown from event listener: " + ex.FormatDiagnosticMessage() );
		}
		catch( Exception::BaseException& ex )
		{
			if( IsDevBuild ) throw;
			Console.Error( L"Ignoring non-runtime BaseException thrown from event listener: " + ex.FormatDiagnosticMessage() );
		}
		++iter;
	}
}

template< typename EvtType >
void EventSource<EvtType>::Dispatch( EvtType& evt )
{
	if( !m_cache_valid )
	{
		m_cache_copy = m_listeners;
		m_cache_valid = true;
	}

	_DispatchRaw( m_cache_copy.begin(), m_cache_copy.end(), evt );
}
