///////////////////////////////////////////////////////////////////////////////
// Name:        generic/selstore.cpp
// Purpose:     wxSelectionStore implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.06.03 (extracted from src/generic/listctrl.cpp)
// RCS-ID:      $Id: selstore.cpp 27853 2004-06-17 16:22:36Z ABX $
// Copyright:   (c) 2000-2003 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/selstore.h"

// ============================================================================
// wxSelectionStore
// ============================================================================

// ----------------------------------------------------------------------------
// tests
// ----------------------------------------------------------------------------

bool wxSelectionStore::IsSelected(size_t item) const
{
    bool isSel = m_itemsSel.Index(item) != wxNOT_FOUND;

    // if the default state is to be selected, being in m_itemsSel means that
    // the item is not selected, so we have to inverse the logic
    return m_defaultState ? !isSel : isSel;
}

// ----------------------------------------------------------------------------
// Select*()
// ----------------------------------------------------------------------------

bool wxSelectionStore::SelectItem(size_t item, bool select)
{
    // search for the item ourselves as like this we get the index where to
    // insert it later if needed, so we do only one search in the array instead
    // of two (adding item to a sorted array requires a search)
    size_t index = m_itemsSel.IndexForInsert(item);
    bool isSel = index < m_itemsSel.GetCount() && m_itemsSel[index] == item;

    if ( select != m_defaultState )
    {
        if ( !isSel )
        {
            m_itemsSel.AddAt(item, index);

            return true;
        }
    }
    else // reset to default state
    {
        if ( isSel )
        {
            m_itemsSel.RemoveAt(index);
            return true;
        }
    }

    return false;
}

bool wxSelectionStore::SelectRange(size_t itemFrom, size_t itemTo,
                                   bool select,
                                   wxArrayInt *itemsChanged)
{
    // 100 is hardcoded but it shouldn't matter much: the important thing is
    // that we don't refresh everything when really few (e.g. 1 or 2) items
    // change state
    static const size_t MANY_ITEMS = 100;

    wxASSERT_MSG( itemFrom <= itemTo, _T("should be in order") );

    // are we going to have more [un]selected items than the other ones?
    if ( itemTo - itemFrom > m_count/2 )
    {
        if ( select != m_defaultState )
        {
            // the default state now becomes the same as 'select'
            m_defaultState = select;

            // so all the old selections (which had state select) shouldn't be
            // selected any more, but all the other ones should
            wxSelectedIndices selOld = m_itemsSel;
            m_itemsSel.Empty();

            // TODO: it should be possible to optimize the searches a bit
            //       knowing the possible range

            size_t item;
            for ( item = 0; item < itemFrom; item++ )
            {
                if ( selOld.Index(item) == wxNOT_FOUND )
                    m_itemsSel.Add(item);
            }

            for ( item = itemTo + 1; item < m_count; item++ )
            {
                if ( selOld.Index(item) == wxNOT_FOUND )
                    m_itemsSel.Add(item);
            }

            // many items (> half) changed state
            itemsChanged = NULL;
        }
        else // select == m_defaultState
        {
            // get the inclusive range of items between itemFrom and itemTo
            size_t count = m_itemsSel.GetCount(),
                   start = m_itemsSel.IndexForInsert(itemFrom),
                   end = m_itemsSel.IndexForInsert(itemTo);

            if ( start == count || m_itemsSel[start] < itemFrom )
            {
                start++;
            }

            if ( end == count || m_itemsSel[end] > itemTo )
            {
                end--;
            }

            if ( start <= end )
            {
                // delete all of them (from end to avoid changing indices)
                for ( int i = end; i >= (int)start; i-- )
                {
                    if ( itemsChanged )
                    {
                        if ( itemsChanged->GetCount() > MANY_ITEMS )
                        {
                            // stop counting (see comment below)
                            itemsChanged = NULL;
                        }
                        else
                        {
                            itemsChanged->Add(m_itemsSel[i]);
                        }
                    }

                    m_itemsSel.RemoveAt(i);
                }
            }
        }
    }
    else // "few" items change state
    {
        if ( itemsChanged )
        {
            itemsChanged->Empty();
        }

        // just add the items to the selection
        for ( size_t item = itemFrom; item <= itemTo; item++ )
        {
            if ( SelectItem(item, select) && itemsChanged )
            {
                itemsChanged->Add(item);

                if ( itemsChanged->GetCount() > MANY_ITEMS )
                {
                    // stop counting them, we'll just eat gobs of memory
                    // for nothing at all - faster to refresh everything in
                    // this case
                    itemsChanged = NULL;
                }
            }
        }
    }

    // we set it to NULL if there are many items changing state
    return itemsChanged != NULL;
}

// ----------------------------------------------------------------------------
// callbacks
// ----------------------------------------------------------------------------

void wxSelectionStore::OnItemDelete(size_t item)
{
    size_t count = m_itemsSel.GetCount(),
           i = m_itemsSel.IndexForInsert(item);

    if ( i < count && m_itemsSel[i] == item )
    {
        // this item itself was in m_itemsSel, remove it from there
        m_itemsSel.RemoveAt(i);

        count--;
    }

    // and adjust the index of all which follow it
    while ( i < count )
    {
        // all following elements must be greater than the one we deleted
        wxASSERT_MSG( m_itemsSel[i] > item, _T("logic error") );

        m_itemsSel[i++]--;
    }
}

