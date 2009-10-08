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

void _allocateHeap_wxString()
{
	if( win32_string_heap == INVALID_HANDLE_VALUE )
	{
		//wxASSERT( win32_string_heap_refcount == 0 );
		win32_string_heap = HeapCreate( HEAP_NO_SERIALIZE, 0x200000, 0 );
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

void _freeHeap_wxString( void* ptr )
{
	HeapFree( win32_string_heap, 0, ptr );
}


void _allocateHeap_wxObject()
{
	if( win32_object_heap == INVALID_HANDLE_VALUE )
	{
		//wxASSERT( win32_object_heap_refcount == 0 );
		win32_object_heap = HeapCreate( HEAP_NO_SERIALIZE, 0x200000, 0 );
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

void _freeHeap_wxObject( void* ptr )
{
	HeapFree( win32_object_heap, 0, ptr );
}
