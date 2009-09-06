/////////////////////////////////////////////////////////////////////////////
// Name:        wx/hash.h
// Purpose:     wxHashTable class
// Author:      Julian Smart
// Modified by: VZ at 25.02.00: type safe hashes with WX_DECLARE_HASH()
// Created:     01/02/97
// RCS-ID:      $Id: hash.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HASH_H__
#define _WX_HASH_H__

#include "wx/defs.h"

#if !wxUSE_STL && WXWIN_COMPATIBILITY_2_4
    #define wxUSE_OLD_HASH_TABLE 1
#else
    #define wxUSE_OLD_HASH_TABLE 0
#endif

#if !wxUSE_STL
    #include "wx/object.h"
#else
    class WXDLLIMPEXP_BASE wxObject;
#endif
#if wxUSE_OLD_HASH_TABLE
    #include "wx/list.h"
#endif
#if WXWIN_COMPATIBILITY_2_4
    #include "wx/dynarray.h"
#endif

// the default size of the hash
#define wxHASH_SIZE_DEFAULT     (1000)

/*
 * A hash table is an array of user-definable size with lists
 * of data items hanging off the array positions.  Usually there'll
 * be a hit, so no search is required; otherwise we'll have to run down
 * the list to find the desired item.
*/

// ----------------------------------------------------------------------------
// this is the base class for object hashes: hash tables which contain
// pointers to objects
// ----------------------------------------------------------------------------

#if wxUSE_OLD_HASH_TABLE

class WXDLLIMPEXP_BASE wxHashTableBase : public wxObject
{
public:
    wxHashTableBase();

    void Create(wxKeyType keyType = wxKEY_INTEGER,
                size_t size = wxHASH_SIZE_DEFAULT);
    void Destroy();

    size_t GetSize() const { return m_hashSize; }
    size_t GetCount() const { return m_count; }

    void DeleteContents(bool flag);

protected:
    // find the node for (key, value)
    wxNodeBase *GetNode(long key, long value) const;

    // the array of lists in which we store the values for given key hash
    wxListBase **m_hashTable;

    // the size of m_lists array
    size_t m_hashSize;

    // the type of indexing we use
    wxKeyType m_keyType;

    // the total number of elements in the hash
    size_t m_count;

    // should we delete our data?
    bool m_deleteContents;

private:
    // no copy ctor/assignment operator (yet)
    DECLARE_NO_COPY_CLASS(wxHashTableBase)
};

#else // if !wxUSE_OLD_HASH_TABLE

#if !defined(wxENUM_KEY_TYPE_DEFINED)
#define wxENUM_KEY_TYPE_DEFINED

enum wxKeyType
{
    wxKEY_NONE,
    wxKEY_INTEGER,
    wxKEY_STRING
};

#endif

union wxHashKeyValue
{
    long integer;
    wxChar *string;
};

// for some compilers (AIX xlC), defining it as friend inside the class is not
// enough, so provide a real forward declaration
class WXDLLIMPEXP_FWD_BASE wxHashTableBase;

class WXDLLIMPEXP_BASE wxHashTableBase_Node
{
    friend class WXDLLIMPEXP_FWD_BASE wxHashTableBase;
    typedef class WXDLLIMPEXP_FWD_BASE wxHashTableBase_Node _Node;
public:
    wxHashTableBase_Node( long key, void* value,
                          wxHashTableBase* table );
    wxHashTableBase_Node( const wxChar* key, void* value,
                          wxHashTableBase* table );
    ~wxHashTableBase_Node();

    long GetKeyInteger() const { return m_key.integer; }
    const wxChar* GetKeyString() const { return m_key.string; }

    void* GetData() const { return m_value; }
    void SetData( void* data ) { m_value = data; }

protected:
    _Node* GetNext() const { return m_next; }

protected:
    // next node in the chain
    wxHashTableBase_Node* m_next;

    // key
    wxHashKeyValue m_key;

    // value
    void* m_value;

    // pointer to the hash containing the node, used to remove the
    // node from the hash when the user deletes the node iterating
    // through it
    // TODO: move it to wxHashTable_Node (only wxHashTable supports
    //       iteration)
    wxHashTableBase* m_hashPtr;
};

class WXDLLIMPEXP_BASE wxHashTableBase
#if !wxUSE_STL
    : public wxObject
#endif
{
    friend class WXDLLIMPEXP_FWD_BASE wxHashTableBase_Node;
public:
    typedef wxHashTableBase_Node Node;

    wxHashTableBase();
    virtual ~wxHashTableBase() { }

    void Create( wxKeyType keyType = wxKEY_INTEGER,
                 size_t size = wxHASH_SIZE_DEFAULT );
    void Clear();
    void Destroy();

    size_t GetSize() const { return m_size; }
    size_t GetCount() const { return m_count; }

    void DeleteContents( bool flag ) { m_deleteContents = flag; }

    static long MakeKey(const wxChar *string);

protected:
    void DoPut( long key, long hash, void* data );
    void DoPut( const wxChar* key, long hash, void* data );
    void* DoGet( long key, long hash ) const;
    void* DoGet( const wxChar* key, long hash ) const;
    void* DoDelete( long key, long hash );
    void* DoDelete( const wxChar* key, long hash );

private:
    // Remove the node from the hash, *only called from
    // ~wxHashTable*_Node destructor
    void DoRemoveNode( wxHashTableBase_Node* node );

    // destroys data contained in the node if appropriate:
    // deletes the key if it is a string and destrys
    // the value if m_deleteContents is true
    void DoDestroyNode( wxHashTableBase_Node* node );

    // inserts a node in the table (at the end of the chain)
    void DoInsertNode( size_t bucket, wxHashTableBase_Node* node );

    // removes a node from the table (fiven a pointer to the previous
    // but does not delete it (only deletes its contents)
    void DoUnlinkNode( size_t bucket, wxHashTableBase_Node* node,
                       wxHashTableBase_Node* prev );

    // unconditionally deletes node value (invoking the
    // correct destructor)
    virtual void DoDeleteContents( wxHashTableBase_Node* node ) = 0;

protected:
    // number of buckets
    size_t m_size;

    // number of nodes (key/value pairs)
    size_t m_count;

    // table
    Node** m_table;

    // key typ (INTEGER/STRING)
    wxKeyType m_keyType;

    // delete contents when hash is cleared
    bool m_deleteContents;

private:
    DECLARE_NO_COPY_CLASS(wxHashTableBase)
};

#endif // wxUSE_OLD_HASH_TABLE

#if !wxUSE_STL

#if WXWIN_COMPATIBILITY_2_4

// ----------------------------------------------------------------------------
// a hash table which stores longs
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxHashTableLong : public wxObject
{
public:
    wxHashTableLong(size_t size = wxHASH_SIZE_DEFAULT)
        { Init(size); }
    virtual ~wxHashTableLong();

    void Create(size_t size = wxHASH_SIZE_DEFAULT);
    void Destroy();

    size_t GetSize() const { return m_hashSize; }
    size_t GetCount() const { return m_count; }

    void Put(long key, long value);
    long Get(long key) const;
    long Delete(long key);

protected:
    void Init(size_t size);

private:
    wxArrayLong **m_values,
                **m_keys;

    // the size of array above
    size_t m_hashSize;

    // the total number of elements in the hash
    size_t m_count;

    // not implemented yet
    DECLARE_NO_COPY_CLASS(wxHashTableLong)
};

// ----------------------------------------------------------------------------
// wxStringHashTable: a hash table which indexes strings with longs
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStringHashTable : public wxObject
{
public:
    wxStringHashTable(size_t sizeTable = wxHASH_SIZE_DEFAULT);
    virtual ~wxStringHashTable();

    // add a string associated with this key to the table
    void Put(long key, const wxString& value);

    // get the string from the key: if not found, an empty string is returned
    // and the wasFound is set to false if not NULL
    wxString Get(long key, bool *wasFound = NULL) const;

    // remove the item, returning true if the item was found and deleted
    bool Delete(long key) const;

    // clean up
    void Destroy();

private:
    wxArrayLong **m_keys;
    wxArrayString **m_values;

    // the size of array above
    size_t m_hashSize;

    DECLARE_NO_COPY_CLASS(wxStringHashTable)
};

#endif // WXWIN_COMPATIBILITY_2_4

#endif // !wxUSE_STL

// ----------------------------------------------------------------------------
// for compatibility only
// ----------------------------------------------------------------------------

#if !wxUSE_OLD_HASH_TABLE

class WXDLLIMPEXP_BASE wxHashTable_Node : public wxHashTableBase_Node
{
    friend class WXDLLIMPEXP_FWD_BASE wxHashTable;
public:
    wxHashTable_Node( long key, void* value,
                      wxHashTableBase* table )
        : wxHashTableBase_Node( key, value, table ) { }
    wxHashTable_Node( const wxChar* key, void* value,
                      wxHashTableBase* table )
        : wxHashTableBase_Node( key, value, table ) { }

    wxObject* GetData() const
        { return (wxObject*)wxHashTableBase_Node::GetData(); }
    void SetData( wxObject* data )
        { wxHashTableBase_Node::SetData( data ); }

    wxHashTable_Node* GetNext() const
        { return (wxHashTable_Node*)wxHashTableBase_Node::GetNext(); }
};

// should inherit protectedly, but it is public for compatibility in
// order to publicly inherit from wxObject
class WXDLLIMPEXP_BASE wxHashTable : public wxHashTableBase
{
    typedef wxHashTableBase hash;
public:
    typedef wxHashTable_Node Node;
    typedef wxHashTable_Node* compatibility_iterator;
public:
    wxHashTable( wxKeyType keyType = wxKEY_INTEGER,
                 size_t size = wxHASH_SIZE_DEFAULT )
        : wxHashTableBase() { Create( keyType, size ); BeginFind(); }
    wxHashTable( const wxHashTable& table );

    virtual ~wxHashTable() { Destroy(); }

    const wxHashTable& operator=( const wxHashTable& );

    // key and value are the same
    void Put(long value, wxObject *object)
        { DoPut( value, value, object ); }
    void Put(long lhash, long value, wxObject *object)
        { DoPut( value, lhash, object ); }
    void Put(const wxChar *value, wxObject *object)
        { DoPut( value, MakeKey( value ), object ); }
    void Put(long lhash, const wxChar *value, wxObject *object)
        { DoPut( value, lhash, object ); }

    // key and value are the same
    wxObject *Get(long value) const
        { return (wxObject*)DoGet( value, value ); }
    wxObject *Get(long lhash, long value) const
        { return (wxObject*)DoGet( value, lhash ); }
    wxObject *Get(const wxChar *value) const
        { return (wxObject*)DoGet( value, MakeKey( value ) ); }
    wxObject *Get(long lhash, const wxChar *value) const
        { return (wxObject*)DoGet( value, lhash ); }

    // Deletes entry and returns data if found
    wxObject *Delete(long key)
        { return (wxObject*)DoDelete( key, key ); }
    wxObject *Delete(long lhash, long key)
        { return (wxObject*)DoDelete( key, lhash ); }
    wxObject *Delete(const wxChar *key)
        { return (wxObject*)DoDelete( key, MakeKey( key ) ); }
    wxObject *Delete(long lhash, const wxChar *key)
        { return (wxObject*)DoDelete( key, lhash ); }

    // Construct your own integer key from a string, e.g. in case
    // you need to combine it with something
    long MakeKey(const wxChar *string) const
        { return wxHashTableBase::MakeKey(string); }

    // Way of iterating through whole hash table (e.g. to delete everything)
    // Not necessary, of course, if you're only storing pointers to
    // objects maintained separately
    void BeginFind() { m_curr = NULL; m_currBucket = 0; }
    Node* Next();

    void Clear() { wxHashTableBase::Clear(); }

    size_t GetCount() const { return wxHashTableBase::GetCount(); }
protected:
    // copy helper
    void DoCopy( const wxHashTable& copy );

    // searches the next node starting from bucket bucketStart and sets
    // m_curr to it and m_currBucket to its bucket
    void GetNextNode( size_t bucketStart );
private:
    virtual void DoDeleteContents( wxHashTableBase_Node* node );

    // current node
    Node* m_curr;

    // bucket the current node belongs to
    size_t m_currBucket;
};

#else // if wxUSE_OLD_HASH_TABLE

class WXDLLIMPEXP_BASE wxHashTable : public wxObject
{
public:
    typedef wxNode Node;
    typedef wxNode* compatibility_iterator;

    int n;
    int current_position;
    wxNode *current_node;

    unsigned int key_type;
    wxList **hash_table;

    wxHashTable(int the_key_type = wxKEY_INTEGER,
                int size = wxHASH_SIZE_DEFAULT);
    virtual ~wxHashTable();

    // copy ctor and assignment operator
    wxHashTable(const wxHashTable& table) : wxObject()
        { DoCopy(table); }
    wxHashTable& operator=(const wxHashTable& table)
        { Clear(); DoCopy(table); return *this; }

    void DoCopy(const wxHashTable& table);

    void Destroy();

    bool Create(int the_key_type = wxKEY_INTEGER,
                int size = wxHASH_SIZE_DEFAULT);

    // Note that there are 2 forms of Put, Get.
    // With a key and a value, the *value* will be checked
    // when a collision is detected. Otherwise, if there are
    // 2 items with a different value but the same key,
    // we'll retrieve the WRONG ONE. So where possible,
    // supply the required value along with the key.
    // In fact, the value-only versions make a key, and still store
    // the value. The use of an explicit key might be required
    // e.g. when combining several values into one key.
    // When doing that, it's highly likely we'll get a collision,
    // e.g. 1 + 2 = 3, 2 + 1 = 3.

    // key and value are NOT necessarily the same
    void Put(long key, long value, wxObject *object);
    void Put(long key, const wxChar *value, wxObject *object);

    // key and value are the same
    void Put(long value, wxObject *object);
    void Put(const wxChar *value, wxObject *object);

    // key and value not the same
    wxObject *Get(long key, long value) const;
    wxObject *Get(long key, const wxChar *value) const;

    // key and value are the same
    wxObject *Get(long value) const;
    wxObject *Get(const wxChar *value) const;

    // Deletes entry and returns data if found
    wxObject *Delete(long key);
    wxObject *Delete(const wxChar *key);

    wxObject *Delete(long key, int value);
    wxObject *Delete(long key, const wxChar *value);

    // Construct your own integer key from a string, e.g. in case
    // you need to combine it with something
    long MakeKey(const wxChar *string) const;

    // Way of iterating through whole hash table (e.g. to delete everything)
    // Not necessary, of course, if you're only storing pointers to
    // objects maintained separately

    void BeginFind();
    Node* Next();

    void DeleteContents(bool flag);
    void Clear();

    // Returns number of nodes
    size_t GetCount() const { return m_count; }

private:
    size_t m_count;             // number of elements in the hashtable
    bool m_deleteContents;

    DECLARE_DYNAMIC_CLASS(wxHashTable)
};

#endif // wxUSE_OLD_HASH_TABLE

#if !wxUSE_OLD_HASH_TABLE

// defines a new type safe hash table which stores the elements of type eltype
// in lists of class listclass
#define _WX_DECLARE_HASH(eltype, dummy, hashclass, classexp)                  \
    classexp hashclass : public wxHashTableBase                               \
    {                                                                         \
    public:                                                                   \
        hashclass(wxKeyType keyType = wxKEY_INTEGER,                          \
                  size_t size = wxHASH_SIZE_DEFAULT)                          \
            : wxHashTableBase() { Create(keyType, size); }                    \
                                                                              \
        virtual ~hashclass() { Destroy(); }                                   \
                                                                              \
        void Put(long key, eltype *data) { DoPut(key, key, (void*)data); }    \
        void Put(long lhash, long key, eltype *data)                          \
            { DoPut(key, lhash, (void*)data); }                               \
        eltype *Get(long key) const { return (eltype*)DoGet(key, key); }      \
        eltype *Get(long lhash, long key) const                               \
            { return (eltype*)DoGet(key, lhash); }                            \
        eltype *Delete(long key) { return (eltype*)DoDelete(key, key); }      \
        eltype *Delete(long lhash, long key)                                  \
            { return (eltype*)DoDelete(key, lhash); }                         \
    private:                                                                  \
        virtual void DoDeleteContents( wxHashTableBase_Node* node )           \
            { delete (eltype*)node->GetData(); }                              \
                                                                              \
        DECLARE_NO_COPY_CLASS(hashclass)                                      \
    }

#else // if wxUSE_OLD_HASH_TABLE

#define _WX_DECLARE_HASH(eltype, listclass, hashclass, classexp)               \
    classexp hashclass : public wxHashTableBase                                \
    {                                                                          \
    public:                                                                    \
        hashclass(wxKeyType keyType = wxKEY_INTEGER,                           \
                  size_t size = wxHASH_SIZE_DEFAULT)                           \
            { Create(keyType, size); }                                         \
                                                                               \
        virtual ~hashclass() { Destroy(); }                                            \
                                                                               \
        void Put(long key, long val, eltype *data) { DoPut(key, val, data); }  \
        void Put(long key, eltype *data) { DoPut(key, key, data); }            \
                                                                               \
        eltype *Get(long key, long value) const                                \
        {                                                                      \
            wxNodeBase *node = GetNode(key, value);                            \
            return node ? ((listclass::Node *)node)->GetData() : (eltype *)0;  \
        }                                                                      \
        eltype *Get(long key) const { return Get(key, key); }                  \
                                                                               \
        eltype *Delete(long key, long value)                                   \
        {                                                                      \
            eltype *data;                                                      \
                                                                               \
            wxNodeBase *node = GetNode(key, value);                            \
            if ( node )                                                        \
            {                                                                  \
                data = ((listclass::Node *)node)->GetData();                   \
                                                                               \
                delete node;                                                   \
                m_count--;                                                     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                data = (eltype *)0;                                            \
            }                                                                  \
                                                                               \
            return data;                                                       \
        }                                                                      \
        eltype *Delete(long key) { return Delete(key, key); }                  \
                                                                               \
    protected:                                                                 \
        void DoPut(long key, long value, eltype *data)                         \
        {                                                                      \
            size_t slot = (size_t)abs((int)(key % (long)m_hashSize));          \
                                                                               \
            if ( !m_hashTable[slot] )                                          \
            {                                                                  \
                m_hashTable[slot] = new listclass(m_keyType);                  \
                if ( m_deleteContents )                                        \
                    m_hashTable[slot]->DeleteContents(true);                   \
            }                                                                  \
                                                                               \
            ((listclass *)m_hashTable[slot])->Append(value, data);             \
            m_count++;                                                         \
        }                                                                      \
                                                                               \
        DECLARE_NO_COPY_CLASS(hashclass)                                       \
    }

#endif // wxUSE_OLD_HASH_TABLE

// this macro is to be used in the user code
#define WX_DECLARE_HASH(el, list, hash) \
    _WX_DECLARE_HASH(el, list, hash, class)

// and this one does exactly the same thing but should be used inside the
// library
#define WX_DECLARE_EXPORTED_HASH(el, list, hash)  \
    _WX_DECLARE_HASH(el, list, hash, class WXDLLEXPORT)

#define WX_DECLARE_USER_EXPORTED_HASH(el, list, hash, usergoo)  \
    _WX_DECLARE_HASH(el, list, hash, class usergoo)

// delete all hash elements
//
// NB: the class declaration of the hash elements must be visible from the
//     place where you use this macro, otherwise the proper destructor may not
//     be called (a decent compiler should give a warning about it, but don't
//     count on it)!
#define WX_CLEAR_HASH_TABLE(hash)                                            \
    {                                                                        \
        (hash).BeginFind();                                                  \
        wxHashTable::compatibility_iterator it = (hash).Next();              \
        while( it )                                                          \
        {                                                                    \
            delete it->GetData();                                            \
            it = (hash).Next();                                              \
        }                                                                    \
        (hash).Clear();                                                      \
    }

#endif
    // _WX_HASH_H__
