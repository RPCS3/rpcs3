/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/hash.cpp
// Purpose:     wxHashTable implementation
// Author:      Julian Smart
// Modified by: VZ at 25.02.00: type safe hashes with WX_DECLARE_HASH()
// Created:     01/02/97
// RCS-ID:      $Id: hash.cpp 49529 2007-10-30 00:32:18Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/hash.h"
#endif

#if wxUSE_OLD_HASH_TABLE

#include <string.h>
#include <stdarg.h>

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxHashTable, wxObject)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxHashTablleBase for working with "void *" data
// ----------------------------------------------------------------------------

wxHashTableBase::wxHashTableBase()
{
    m_deleteContents = false;
    m_hashTable = (wxListBase **)NULL;
    m_hashSize = 0;
    m_count = 0;
    m_keyType = wxKEY_NONE;
}

void wxHashTableBase::Create(wxKeyType keyType, size_t size)
{
    Destroy();

    m_hashSize = size;
    m_keyType = keyType;
    m_hashTable = new wxListBase *[size];
    for ( size_t n = 0; n < m_hashSize; n++ )
    {
        m_hashTable[n] = (wxListBase *) NULL;
    }
}

void wxHashTableBase::Destroy()
{
    if ( m_hashTable )
    {
        for ( size_t n = 0; n < m_hashSize; n++ )
        {
            delete m_hashTable[n];
        }

        delete [] m_hashTable;

        m_hashTable = (wxListBase **)NULL;

        m_count = 0;
    }
}

void wxHashTableBase::DeleteContents(bool flag)
{
    m_deleteContents = flag;
    for ( size_t n = 0; n < m_hashSize; n++ )
    {
        if ( m_hashTable[n] )
        {
            m_hashTable[n]->DeleteContents(flag);
        }
    }
}

wxNodeBase *wxHashTableBase::GetNode(long key, long value) const
{
    size_t slot = (size_t)abs((int)(key % (long)m_hashSize));

    wxNodeBase *node;
    if ( m_hashTable[slot] )
    {
        node = m_hashTable[slot]->Find(wxListKey(value));
    }
    else
    {
        node = (wxNodeBase *)NULL;
    }

    return node;
}

#if WXWIN_COMPATIBILITY_2_4

// ----------------------------------------------------------------------------
// wxHashTableLong
// ----------------------------------------------------------------------------

wxHashTableLong::~wxHashTableLong()
{
    Destroy();
}

void wxHashTableLong::Init(size_t size)
{
    m_hashSize = size;
    m_values = new wxArrayLong *[size];
    m_keys = new wxArrayLong *[size];

    for ( size_t n = 0; n < m_hashSize; n++ )
    {
        m_values[n] =
        m_keys[n] = (wxArrayLong *)NULL;
    }

    m_count = 0;
}

void wxHashTableLong::Create(size_t size)
{
    Init(size);
}

void wxHashTableLong::Destroy()
{
    for ( size_t n = 0; n < m_hashSize; n++ )
    {
        delete m_values[n];
        delete m_keys[n];
    }

    delete [] m_values;
    delete [] m_keys;
    m_hashSize = 0;
    m_count = 0;
}

void wxHashTableLong::Put(long key, long value)
{
    wxCHECK_RET( m_hashSize, _T("must call Create() first") );

    size_t slot = (size_t)abs((int)(key % (long)m_hashSize));

    if ( !m_keys[slot] )
    {
        m_keys[slot] = new wxArrayLong;
        m_values[slot] = new wxArrayLong;
    }

    m_keys[slot]->Add(key);
    m_values[slot]->Add(value);

    m_count++;
}

long wxHashTableLong::Get(long key) const
{
    wxCHECK_MSG( m_hashSize, wxNOT_FOUND, _T("must call Create() first") );

    size_t slot = (size_t)abs((int)(key % (long)m_hashSize));

    wxArrayLong *keys = m_keys[slot];
    if ( keys )
    {
        size_t count = keys->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( keys->Item(n) == key )
            {
                return m_values[slot]->Item(n);
            }
        }
    }

    return wxNOT_FOUND;
}

long wxHashTableLong::Delete(long key)
{
    wxCHECK_MSG( m_hashSize, wxNOT_FOUND, _T("must call Create() first") );

    size_t slot = (size_t)abs((int)(key % (long)m_hashSize));

    wxArrayLong *keys = m_keys[slot];
    if ( keys )
    {
        size_t count = keys->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( keys->Item(n) == key )
            {
                long val = m_values[slot]->Item(n);

                keys->RemoveAt(n);
                m_values[slot]->RemoveAt(n);

                m_count--;

                return val;
            }
        }
    }

    return wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// wxStringHashTable: more efficient than storing strings in a list
// ----------------------------------------------------------------------------

wxStringHashTable::wxStringHashTable(size_t sizeTable)
{
    m_keys = new wxArrayLong *[sizeTable];
    m_values = new wxArrayString *[sizeTable];

    m_hashSize = sizeTable;
    for ( size_t n = 0; n < m_hashSize; n++ )
    {
        m_values[n] = (wxArrayString *)NULL;
        m_keys[n] = (wxArrayLong *)NULL;
    }
}

wxStringHashTable::~wxStringHashTable()
{
    Destroy();
}

void wxStringHashTable::Destroy()
{
    for ( size_t n = 0; n < m_hashSize; n++ )
    {
        delete m_values[n];
        delete m_keys[n];
    }

    delete [] m_values;
    delete [] m_keys;
    m_hashSize = 0;
}

void wxStringHashTable::Put(long key, const wxString& value)
{
    wxCHECK_RET( m_hashSize, _T("must call Create() first") );

    size_t slot = (size_t)abs((int)(key % (long)m_hashSize));

    if ( !m_keys[slot] )
    {
        m_keys[slot] = new wxArrayLong;
        m_values[slot] = new wxArrayString;
    }

    m_keys[slot]->Add(key);
    m_values[slot]->Add(value);
}

wxString wxStringHashTable::Get(long key, bool *wasFound) const
{
    wxCHECK_MSG( m_hashSize, wxEmptyString, _T("must call Create() first") );

    size_t slot = (size_t)abs((int)(key % (long)m_hashSize));

    wxArrayLong *keys = m_keys[slot];
    if ( keys )
    {
        size_t count = keys->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( keys->Item(n) == key )
            {
                if ( wasFound )
                    *wasFound = true;

                return m_values[slot]->Item(n);
            }
        }
    }

    if ( wasFound )
        *wasFound = false;

    return wxEmptyString;
}

bool wxStringHashTable::Delete(long key) const
{
    wxCHECK_MSG( m_hashSize, false, _T("must call Create() first") );

    size_t slot = (size_t)abs((int)(key % (long)m_hashSize));

    wxArrayLong *keys = m_keys[slot];
    if ( keys )
    {
        size_t count = keys->GetCount();
        for ( size_t n = 0; n < count; n++ )
        {
            if ( keys->Item(n) == key )
            {
                keys->RemoveAt(n);
                m_values[slot]->RemoveAt(n);
                return true;
            }
        }
    }

    return false;
}

#endif // WXWIN_COMPATIBILITY_2_4

// ----------------------------------------------------------------------------
// old not type safe wxHashTable
// ----------------------------------------------------------------------------

wxHashTable::wxHashTable (int the_key_type, int size)
{
  n = 0;
  hash_table = (wxList**) NULL;
  Create(the_key_type, size);
  m_count = 0;
  m_deleteContents = false;
/*
  n = size;
  current_position = -1;
  current_node = (wxNode *) NULL;

  key_type = the_key_type;
  hash_table = new wxList *[size];
  int i;
  for (i = 0; i < size; i++)
    hash_table[i] = (wxList *) NULL;
*/
}

wxHashTable::~wxHashTable ()
{
  Destroy();
}

void wxHashTable::Destroy()
{
  if (!hash_table) return;
  int i;
  for (i = 0; i < n; i++)
    if (hash_table[i])
      delete hash_table[i];
  delete[] hash_table;
  hash_table = NULL;
}

bool wxHashTable::Create(int the_key_type, int size)
{
  Destroy();

  n = size;
  current_position = -1;
  current_node = (wxNode *) NULL;

  key_type = the_key_type;
  hash_table = new wxList *[size];
  int i;
  for (i = 0; i < size; i++)
    hash_table[i] = (wxList *) NULL;
  return true;
}


void wxHashTable::DoCopy(const wxHashTable& table)
{
  n = table.n;
  m_count = table.m_count;
  current_position = table.current_position;
  current_node = NULL; // doesn't matter - Next() will reconstruct it
  key_type = table.key_type;

  hash_table = new wxList *[n];
  for (int i = 0; i < n; i++) {
    if (table.hash_table[i] == NULL)
      hash_table[i] = NULL;
    else {
      hash_table[i] = new wxList(key_type);
      *hash_table[i] = *(table.hash_table[i]);
    }
  }
}

void wxHashTable::Put (long key, long value, wxObject * object)
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
  {
    hash_table[position] = new wxList (wxKEY_INTEGER);
    if (m_deleteContents) hash_table[position]->DeleteContents(true);
  }

  hash_table[position]->Append (value, object);
  m_count++;
}

void wxHashTable::Put (long key, const wxChar *value, wxObject * object)
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
  {
    hash_table[position] = new wxList (wxKEY_STRING);
    if (m_deleteContents) hash_table[position]->DeleteContents(true);
  }

  hash_table[position]->Append (value, object);
  m_count++;
}

void wxHashTable::Put (long key, wxObject * object)
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
  {
    hash_table[position] = new wxList (wxKEY_INTEGER);
    if (m_deleteContents) hash_table[position]->DeleteContents(true);
  }

  hash_table[position]->Append (k, object);
  m_count++;
}

void wxHashTable::Put (const wxChar *key, wxObject * object)
{
  int position = (int) (MakeKey (key) % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
  {
    hash_table[position] = new wxList (wxKEY_STRING);
    if (m_deleteContents) hash_table[position]->DeleteContents(true);
  }

  hash_table[position]->Append (key, object);
  m_count++;
}

wxObject *wxHashTable::Get (long key, long value) const
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (value);
      if (node)
        return node->GetData ();
      else
        return (wxObject *) NULL;
    }
}

wxObject *wxHashTable::Get (long key, const wxChar *value) const
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (value);
      if (node)
        return node->GetData ();
      else
        return (wxObject *) NULL;
    }
}

wxObject *wxHashTable::Get (long key) const
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (k);
      return node ? node->GetData () : (wxObject*)NULL;
    }
}

wxObject *wxHashTable::Get (const wxChar *key) const
{
  int position = (int) (MakeKey (key) % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (key);
      return node ? node->GetData () : (wxObject*)NULL;
    }
}

wxObject *wxHashTable::Delete (long key)
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (k);
      if (node)
        {
          wxObject *data = node->GetData ();
          delete node;
          m_count--;
          return data;
        }
      else
        return (wxObject *) NULL;
    }
}

wxObject *wxHashTable::Delete (const wxChar *key)
{
  int position = (int) (MakeKey (key) % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (key);
      if (node)
        {
          wxObject *data = node->GetData ();
          delete node;
          m_count--;
          return data;
        }
      else
        return (wxObject *) NULL;
    }
}

wxObject *wxHashTable::Delete (long key, int value)
{
  // Should NEVER be
  long k = (long) key;

  int position = (int) (k % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (value);
      if (node)
        {
          wxObject *data = node->GetData ();
          delete node;
          m_count--;
          return data;
        }
      else
        return (wxObject *) NULL;
    }
}

wxObject *wxHashTable::Delete (long key, const wxChar *value)
{
  int position = (int) (key % n);
  if (position < 0) position = -position;

  if (!hash_table[position])
    return (wxObject *) NULL;
  else
    {
      wxNode *node = hash_table[position]->Find (value);
      if (node)
        {
          wxObject *data = node->GetData ();
          delete node;
          m_count--;
          return data;
        }
      else
        return (wxObject *) NULL;
    }
}

long wxHashTable::MakeKey (const wxChar *string) const
{
  long int_key = 0;

  while (*string)
    int_key += (wxUChar) *string++;

  return int_key;
}

void wxHashTable::BeginFind ()
{
  current_position = -1;
  current_node = (wxNode *) NULL;
}

wxHashTable::Node* wxHashTable::Next ()
{
  wxNode *found = (wxNode *) NULL;
  bool end = false;
  while (!end && !found)
    {
      if (!current_node)
        {
          current_position++;
          if (current_position >= n)
            {
              current_position = -1;
              current_node = (wxNode *) NULL;
              end = true;
            }
          else
            {
              if (hash_table[current_position])
                {
                  current_node = hash_table[current_position]->GetFirst ();
                  found = current_node;
                }
            }
        }
      else
        {
          current_node = current_node->GetNext ();
          found = current_node;
        }
    }
  return found;
}

void wxHashTable::DeleteContents (bool flag)
{
  int i;
  m_deleteContents = flag;
  for (i = 0; i < n; i++)
    {
      if (hash_table[i])
        hash_table[i]->DeleteContents (flag);
    }
}

void wxHashTable::Clear ()
{
    int i;
    if (hash_table)
    {
        for (i = 0; i < n; i++)
        {
            if (hash_table[i])
                hash_table[i]->Clear ();
        }
    }
  m_count = 0;
}

#else // if !wxUSE_OLD_HASH_TABLE

wxHashTableBase_Node::wxHashTableBase_Node( long key, void* value,
                                            wxHashTableBase* table )
    : m_value( value ), m_hashPtr( table )
{
    m_key.integer = key;
}

wxHashTableBase_Node::wxHashTableBase_Node( const wxChar* key, void* value,
                                            wxHashTableBase* table )
    : m_value( value ), m_hashPtr( table )
{
    m_key.string = wxStrcpy( new wxChar[wxStrlen( key ) + 1], key );
}

wxHashTableBase_Node::~wxHashTableBase_Node()
{
    if( m_hashPtr ) m_hashPtr->DoRemoveNode( this );
}

//

wxHashTableBase::wxHashTableBase()
    : m_size( 0 ), m_count( 0 ), m_table( NULL ), m_keyType( wxKEY_NONE ),
      m_deleteContents( false )
{
}

void wxHashTableBase::Create( wxKeyType keyType, size_t size )
{
    m_keyType = keyType;
    m_size = size;
    m_table = new wxHashTableBase_Node*[ m_size ];

    for( size_t i = 0; i < m_size; ++i )
        m_table[i] = NULL;
}

void wxHashTableBase::Clear()
{
    for( size_t i = 0; i < m_size; ++i )
    {
        Node* end = m_table[i];

        if( end == NULL )
            continue;

        Node *curr, *next = end->GetNext();

        do
        {
            curr = next;
            next = curr->GetNext();

            DoDestroyNode( curr );

            delete curr;
        }
        while( curr != end );

        m_table[i] = NULL;
    }

    m_count = 0;
}

void wxHashTableBase::DoRemoveNode( wxHashTableBase_Node* node )
{
    size_t bucket = ( m_keyType == wxKEY_INTEGER ?
                      node->m_key.integer        :
                      MakeKey( node->m_key.string ) ) % m_size;

    if( node->GetNext() == node )
    {
        // single-node chain (common case)
        m_table[bucket] = NULL;
    }
    else
    {
        Node *start = m_table[bucket], *curr;
        Node* prev = start;

        for( curr = prev->GetNext(); curr != node;
             prev = curr, curr = curr->GetNext() ) ;

        DoUnlinkNode( bucket, node, prev );
    }

    DoDestroyNode( node );
}

void wxHashTableBase::DoDestroyNode( wxHashTableBase_Node* node )
{
    // if it is called from DoRemoveNode, node has already been
    // removed, from other places it does not matter
    node->m_hashPtr = NULL;

    if( m_keyType == wxKEY_STRING )
        delete[] node->m_key.string;
    if( m_deleteContents )
        DoDeleteContents( node );
}

void wxHashTableBase::Destroy()
{
    Clear();

    delete[] m_table;

    m_table = NULL;
    m_size = 0;
}

void wxHashTableBase::DoInsertNode( size_t bucket, wxHashTableBase_Node* node )
{
    if( m_table[bucket] == NULL )
    {
        m_table[bucket] = node->m_next = node;
    }
    else
    {
        Node *prev = m_table[bucket];
        Node *next = prev->m_next;

        prev->m_next = node;
        node->m_next = next;
        m_table[bucket] = node;
    }

    ++m_count;
}

void wxHashTableBase::DoPut( long key, long hash, void* data )
{
    wxASSERT( m_keyType == wxKEY_INTEGER );

    size_t bucket = size_t(hash) % m_size;
    Node* node = new wxHashTableBase_Node( key, data, this );

    DoInsertNode( bucket, node );
}

void wxHashTableBase::DoPut( const wxChar* key, long hash, void* data )
{
    wxASSERT( m_keyType == wxKEY_STRING );

    size_t bucket = size_t(hash) % m_size;
    Node* node = new wxHashTableBase_Node( key, data, this );

    DoInsertNode( bucket, node );
}

void* wxHashTableBase::DoGet( long key, long hash ) const
{
    wxASSERT( m_keyType == wxKEY_INTEGER );

    size_t bucket = size_t(hash) % m_size;

    if( m_table[bucket] == NULL )
        return NULL;

    Node *first = m_table[bucket]->GetNext(),
         *curr = first;

    do
    {
        if( curr->m_key.integer == key )
            return curr->m_value;

        curr = curr->GetNext();
    }
    while( curr != first );

    return NULL;
}

void* wxHashTableBase::DoGet( const wxChar* key, long hash ) const
{
    wxASSERT( m_keyType == wxKEY_STRING );

    size_t bucket = size_t(hash) % m_size;

    if( m_table[bucket] == NULL )
        return NULL;

    Node *first = m_table[bucket]->GetNext(),
         *curr = first;

    do
    {
        if( wxStrcmp( curr->m_key.string, key ) == 0 )
            return curr->m_value;

        curr = curr->GetNext();
    }
    while( curr != first );

    return NULL;
}

void wxHashTableBase::DoUnlinkNode( size_t bucket, wxHashTableBase_Node* node,
                                    wxHashTableBase_Node* prev )
{
    if( node == m_table[bucket] )
        m_table[bucket] = prev;

    if( prev == node && prev == node->GetNext() )
        m_table[bucket] = NULL;
    else
        prev->m_next = node->m_next;

    DoDestroyNode( node );
    --m_count;
}

void* wxHashTableBase::DoDelete( long key, long hash )
{
    wxASSERT( m_keyType == wxKEY_INTEGER );

    size_t bucket = size_t(hash) % m_size;

    if( m_table[bucket] == NULL )
        return NULL;

    Node *first = m_table[bucket]->GetNext(),
         *curr = first,
         *prev = m_table[bucket];

    do
    {
        if( curr->m_key.integer == key )
        {
            void* retval = curr->m_value;
            curr->m_value = NULL;

            DoUnlinkNode( bucket, curr, prev );
            delete curr;

            return retval;
        }

        prev = curr;
        curr = curr->GetNext();
    }
    while( curr != first );

    return NULL;
}

void* wxHashTableBase::DoDelete( const wxChar* key, long hash )
{
    wxASSERT( m_keyType == wxKEY_STRING );

    size_t bucket = size_t(hash) % m_size;

    if( m_table[bucket] == NULL )
        return NULL;

    Node *first = m_table[bucket]->GetNext(),
         *curr = first,
         *prev = m_table[bucket];

    do
    {
        if( wxStrcmp( curr->m_key.string, key ) == 0 )
        {
            void* retval = curr->m_value;
            curr->m_value = NULL;

            DoUnlinkNode( bucket, curr, prev );
            delete curr;

            return retval;
        }

        prev = curr;
        curr = curr->GetNext();
    }
    while( curr != first );

    return NULL;
}

long wxHashTableBase::MakeKey( const wxChar *str )
{
    long int_key = 0;

    while( *str )
        int_key += (wxUChar)*str++;

    return int_key;
}

// ----------------------------------------------------------------------------
// wxHashTable
// ----------------------------------------------------------------------------

wxHashTable::wxHashTable( const wxHashTable& table )
           : wxHashTableBase()
{
    DoCopy( table );
}

const wxHashTable& wxHashTable::operator=( const wxHashTable& table )
{
    Destroy();
    DoCopy( table );

    return *this;
}

void wxHashTable::DoCopy( const wxHashTable& WXUNUSED(table) )
{
    Create( m_keyType, m_size );

    wxFAIL;
}

void wxHashTable::DoDeleteContents( wxHashTableBase_Node* node )
{
    delete ((wxHashTable_Node*)node)->GetData();
}

void wxHashTable::GetNextNode( size_t bucketStart )
{
    for( size_t i = bucketStart; i < m_size; ++i )
    {
        if( m_table[i] != NULL )
        {
            m_curr = ((Node*)m_table[i])->GetNext();
            m_currBucket = i;
            return;
        }
    }

    m_curr = NULL;
    m_currBucket = 0;
}

wxHashTable::Node* wxHashTable::Next()
{
    if( m_curr == NULL )
        GetNextNode( 0 );
    else
    {
        m_curr = m_curr->GetNext();

        if( m_curr == ( (Node*)m_table[m_currBucket] )->GetNext() )
            GetNextNode( m_currBucket + 1 );
    }

    return m_curr;
}

#endif // !wxUSE_OLD_HASH_TABLE
