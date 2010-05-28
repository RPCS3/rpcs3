// HeapAllocator.cpp
//
// Copyright 2009-2010 by the PCSX Dev Team.
// (under whatever license is appropriate for this sort of hack, I guess).
//
// Provides a specialized heap for wxStrings, which helps keep the main heap from being
// excessively fragmented/cluttered by billions of micro-sized string allocations.  This
// should also improve multithreaded performace since strings can be allocated in parallel
// with other "common" allocation tasks on other threads.
//

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/thread.h"
#endif

#include <ctype.h>

#ifndef __WXWINCE__
    #include <errno.h>
#endif

#include <string.h>
#include <stdlib.h>

static HANDLE win32_string_heap = INVALID_HANDLE_VALUE;
static int win32_string_heap_refcount;

static HANDLE win32_object_heap = INVALID_HANDLE_VALUE;
static int win32_object_heap_refcount;

void _createHeap_wxString()
{
	if( win32_string_heap == INVALID_HANDLE_VALUE )
	{
		//wxASSERT( win32_string_heap_refcount == 0 );
		win32_string_heap = HeapCreate( 0, 0x200000, 0 );
	}
}

void _destroyHeap_wxString()
{
	if( win32_string_heap == INVALID_HANDLE_VALUE ) return;
}

void* _allocHeap_wxString( size_t size )
{
	return (void*)HeapAlloc( win32_string_heap, 0, size );
}

void* _reallocHeap_wxString( void* original, size_t size )
{
	if( !original )
		return _allocHeap_wxString( size );
	else
		return (void*)HeapReAlloc( win32_string_heap, 0, original, size );
}

void _freeHeap_wxString( void* ptr )
{
	HeapFree( win32_string_heap, 0, ptr );
}


char* _mswHeap_Strdup( const char* src )
{
	if( !src ) return NULL;
	const size_t len = strlen(src);
	char* retval = (char*)_allocHeap_wxString( len+1 );
	if( !retval ) return NULL;
	memcpy( retval, src, len+1 );
	return retval;
}

wchar_t* _mswHeap_Strdup( const wchar_t* src )
{
	if( !src ) return NULL;
	const size_t len = (wcslen(src) + 1) * sizeof(wchar_t);
	wchar_t* retval = (wchar_t*)_allocHeap_wxString( len );
	if( !retval ) return NULL;
	memcpy( retval, src, len );
	return retval;
}

// --------------------------------------------------------------------------------------
//  
// --------------------------------------------------------------------------------------

void _createHeap_wxObject()
{
	if( win32_object_heap == INVALID_HANDLE_VALUE )
	{
		//wxASSERT( win32_object_heap_refcount == 0 );
		win32_object_heap = HeapCreate( 0, 0x200000, 0 );
	}
}

void _destroyHeap_wxObject()
{
	if( win32_object_heap == INVALID_HANDLE_VALUE ) return;
}

void* _allocHeap_wxObject( size_t size )
{
	return (void*)HeapAlloc( win32_object_heap, 0, size );
}

void* _reallocHeap_wxObject( void* original, size_t size )
{
	if( !original )
		return _allocHeap_wxObject( size );
	else
		return (void*)HeapReAlloc( win32_object_heap, 0, original, size );
}

void _freeHeap_wxObject( void* ptr )
{
	HeapFree( win32_object_heap, 0, ptr );
}
