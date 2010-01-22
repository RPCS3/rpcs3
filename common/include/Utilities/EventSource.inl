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

using Threading::ScopedLock;

template< typename ListenerType >
typename EventSource<ListenerType>::ListenerIterator EventSource<ListenerType>::Add( ListenerType& listener )
{
	ScopedLock locker( m_listeners_lock );

	// Check for duplicates before adding the event.
	if( IsDebugBuild )
	{
		ListenerIterator iter = m_listeners.begin();
		while( iter != m_listeners.end() )
		{
			if( (*iter) == &listener ) return iter;
			++iter;
		}
	}
	return _AddFast_without_lock( listener );
}

template< typename ListenerType >
void EventSource<ListenerType>::Remove( ListenerType& listener )
{
	ScopedLock locker( m_listeners_lock );
	m_cache_valid = false;
	m_listeners.remove( &listener );
}

template< typename ListenerType >
void EventSource<ListenerType>::Remove( const ListenerIterator& listenerHandle )
{
	ScopedLock locker( m_listeners_lock );
	m_cache_valid = false;
	m_listeners.erase( listenerHandle );
}

template< typename ListenerType >
typename EventSource<ListenerType>::ListenerIterator EventSource<ListenerType>::_AddFast_without_lock( ListenerType& listener )
{
	m_cache_valid = false;
	m_listeners.push_front( &listener );
	return m_listeners.begin();
}


template< typename ListenerType >
__forceinline void EventSource<ListenerType>::_DispatchRaw( ListenerIterator iter, const ListenerIterator& iend, const EvtParams& evtparams )
{
	while( iter != iend )
	{
		try	{
			(*iter)->DispatchEvent( evtparams );
		}
		catch( Exception::RuntimeError& ex )
		{
			if( IsDevBuild ) {
				pxFailDev( L"Ignoring runtime error thrown from event listener (event listeners should not throw exceptions!): " + ex.FormatDiagnosticMessage() );
			}
			else {
				Console.Error( L"Ignoring runtime error thrown from event listener: " + ex.FormatDiagnosticMessage() );
			}
		}
		catch( Exception::BaseException& ex )
		{
			if( IsDevBuild )
			{
				ex.DiagMsg() = L"Non-runtime BaseException thrown from event listener .. " + ex.DiagMsg();
				throw;
			}
			Console.Error( L"Ignoring non-runtime BaseException thrown from event listener: " + ex.FormatDiagnosticMessage() );
		}
		++iter;
	}
}

template< typename ListenerType >
void EventSource<ListenerType>::Dispatch( const EvtParams& evtparams )
{
	if( !m_cache_valid )
	{
		m_cache_copy = m_listeners;
		m_cache_valid = true;
	}

	if( m_cache_copy.empty() ) return;
	_DispatchRaw( m_cache_copy.begin(), m_cache_copy.end(), evtparams );
}
