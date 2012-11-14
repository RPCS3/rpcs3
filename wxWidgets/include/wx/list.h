/////////////////////////////////////////////////////////////////////////////
// Name:        wx/list.h
// Purpose:     wxList, wxStringList classes
// Author:      Julian Smart
// Modified by: VZ at 16/11/98: WX_DECLARE_LIST() and typesafe lists added
// Created:     29/01/98
// RCS-ID:      $Id: list.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1998 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*
  All this is quite ugly but serves two purposes:
    1. Be almost 100% compatible with old, untyped, wxList class
    2. Ensure compile-time type checking for the linked lists

  The idea is to have one base class (wxListBase) working with "void *" data,
  but to hide these untyped functions - i.e. make them protected, so they
  can only be used from derived classes which have inline member functions
  working with right types. This achieves the 2nd goal. As for the first one,
  we provide a special derivation of wxListBase called wxList which looks just
  like the old class.
*/

#ifndef _WX_LISTH__
#define _WX_LISTH__

// -----------------------------------------------------------------------------
// headers
// -----------------------------------------------------------------------------

#include "wx/defs.h"
#include "wx/object.h"
#include "wx/string.h"

#if wxUSE_STL
    #include "wx/beforestd.h"
    #include <algorithm>
    #include <iterator>
    #include <list>
    #include "wx/afterstd.h"
#endif

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// type of compare function for list sort operation (as in 'qsort'): it should
// return a negative value, 0 or positive value if the first element is less
// than, equal or greater than the second

extern "C"
{
typedef int (* LINKAGEMODE wxSortCompareFunction)(const void *elem1, const void *elem2);
}

class WXDLLIMPEXP_FWD_BASE wxObjectListNode;
typedef wxObjectListNode wxNode;

//
typedef int (* LINKAGEMODE wxListIterateFunction)(void *current);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#if !defined(wxENUM_KEY_TYPE_DEFINED)
#define wxENUM_KEY_TYPE_DEFINED

enum wxKeyType
{
    wxKEY_NONE,
    wxKEY_INTEGER,
    wxKEY_STRING
};

#endif

#if wxUSE_STL

#define wxLIST_COMPATIBILITY

#define WX_DECLARE_LIST_3(elT, dummy1, liT, dummy2, decl) \
    WX_DECLARE_LIST_WITH_DECL(elT, liT, decl)
#define WX_DECLARE_LIST_PTR_3(elT, dummy1, liT, dummy2, decl) \
    WX_DECLARE_LIST_3(elT, dummy1, liT, dummy2, decl)

#define WX_DECLARE_LIST_2(elT, liT, dummy, decl) \
    WX_DECLARE_LIST_WITH_DECL(elT, liT, decl)
#define WX_DECLARE_LIST_PTR_2(elT, liT, dummy, decl) \
    WX_DECLARE_LIST_2(elT, liT, dummy, decl) \

#define WX_DECLARE_LIST_WITH_DECL(elT, liT, decl) \
    WX_DECLARE_LIST_XO(elT*, liT, decl)

#if !defined( __VISUALC__ )

template<class T>
class WXDLLIMPEXP_BASE wxList_SortFunction
{
public:
    wxList_SortFunction(wxSortCompareFunction f) : m_f(f) { }
    bool operator()(const T& i1, const T& i2)
      { return m_f((T*)&i1, (T*)&i2) < 0; }
private:
    wxSortCompareFunction m_f;
};

#define WX_LIST_SORTFUNCTION( elT, f ) wxList_SortFunction<elT>(f)
#define VC6_WORKAROUND(elT, liT, decl)

#else // if defined( __VISUALC__ )

#define WX_LIST_SORTFUNCTION( elT, f ) std::greater<elT>( f )
#define VC6_WORKAROUND(elT, liT, decl)                                        \
    decl liT;                                                                 \
                                                                              \
    /* Workaround for broken VC6 STL incorrectly requires a std::greater<> */ \
    /* to be passed into std::list::sort() */                                 \
    template <>                                                               \
    struct std::greater<elT>                                                  \
    {                                                                         \
        private:                                                              \
            wxSortCompareFunction m_CompFunc;                                 \
        public:                                                               \
            greater( wxSortCompareFunction compfunc = NULL )                  \
                : m_CompFunc( compfunc ) {}                                   \
            bool operator()(const elT X, const elT Y) const                   \
                {                                                             \
                    return m_CompFunc ?                                       \
                        ( m_CompFunc( X, Y ) < 0 ) :                          \
                        ( X > Y );                                            \
                }                                                             \
    };

#endif // defined( __VISUALC__ )

/*
    Note 1: the outer helper class _WX_LIST_HELPER_##liT below is a workaround
    for mingw 3.2.3 compiler bug that prevents a static function of liT class
    from being exported into dll. A minimal code snippet reproducing the bug:

         struct WXDLLEXPORT Foo
         {
            static void Bar();
            struct SomeInnerClass
            {
              friend class Foo; // comment this out to make it link
            };
            ~Foo()
            {
                Bar();
            }
         };

    The program does not link under mingw_gcc 3.2.3 producing undefined
    reference to Foo::Bar() function


    Note 2: the EmptyList is needed to allow having a NULL pointer-like
    invalid iterator. We used to use just an uninitialized iterator object
    instead but this fails with some debug/checked versions of STL, notably the
    glibc version activated with _GLIBCXX_DEBUG, so we need to have a separate
    invalid iterator.
 */

// the real wxList-class declaration
#define WX_DECLARE_LIST_XO(elT, liT, decl)                                    \
    decl _WX_LIST_HELPER_##liT                                                \
    {                                                                         \
        typedef elT _WX_LIST_ITEM_TYPE_##liT;                                 \
    public:                                                                   \
        static void DeleteFunction( _WX_LIST_ITEM_TYPE_##liT X );             \
    };                                                                        \
                                                                              \
    VC6_WORKAROUND(elT, liT, decl)                                            \
    decl liT : public std::list<elT>                                          \
    {                                                                         \
    private:                                                                  \
        typedef std::list<elT> BaseListType;                                  \
        static BaseListType EmptyList;                                        \
                                                                              \
        bool m_destroy;                                                       \
                                                                              \
    public:                                                                   \
        decl compatibility_iterator                                           \
        {                                                                     \
        private:                                                              \
            /* Workaround for broken VC6 nested class name resolution */      \
            typedef std::list<elT>::iterator iterator;                        \
            friend class liT;                                                 \
                                                                              \
            iterator m_iter;                                                  \
            liT * m_list;                                                     \
                                                                              \
        public:                                                               \
            compatibility_iterator()                                          \
                : m_iter(EmptyList.end()), m_list( NULL ) {}                  \
            compatibility_iterator( liT* li, iterator i )                     \
                : m_iter( i ), m_list( li ) {}                                \
            compatibility_iterator( const liT* li, iterator i )               \
                : m_iter( i ), m_list( const_cast< liT* >( li ) ) {}          \
                                                                              \
            compatibility_iterator* operator->() { return this; }             \
            const compatibility_iterator* operator->() const { return this; } \
                                                                              \
            bool operator==(const compatibility_iterator& i) const            \
            {                                                                 \
                wxASSERT_MSG( m_list && i.m_list,                             \
                              wxT("comparing invalid iterators is illegal") ); \
                return (m_list == i.m_list) && (m_iter == i.m_iter);          \
            }                                                                 \
            bool operator!=(const compatibility_iterator& i) const            \
                { return !( operator==( i ) ); }                              \
            operator bool() const                                             \
                { return m_list ? m_iter != m_list->end() : false; }          \
            bool operator !() const                                           \
                { return !( operator bool() ); }                              \
                                                                              \
            elT GetData() const                                               \
                { return *m_iter; }                                           \
            void SetData( elT e )                                             \
                { *m_iter = e; }                                              \
                                                                              \
            compatibility_iterator GetNext() const                            \
            {                                                                 \
                iterator i = m_iter;                                          \
                return compatibility_iterator( m_list, ++i );                 \
            }                                                                 \
            compatibility_iterator GetPrevious() const                        \
            {                                                                 \
                if ( m_iter == m_list->begin() )                              \
                    return compatibility_iterator();                          \
                                                                              \
                iterator i = m_iter;                                          \
                return compatibility_iterator( m_list, --i );                 \
            }                                                                 \
            int IndexOf() const                                               \
            {                                                                 \
                return *this ? std::distance( m_list->begin(), m_iter )       \
                             : wxNOT_FOUND;                                   \
            }                                                                 \
        };                                                                    \
    public:                                                                   \
        liT() : m_destroy( false ) {}                                         \
                                                                              \
        compatibility_iterator Find( const elT e ) const                      \
        {                                                                     \
          liT* _this = const_cast< liT* >( this );                            \
          return compatibility_iterator( _this,                               \
                     std::find( _this->begin(), _this->end(), e ) );          \
        }                                                                     \
                                                                              \
        bool IsEmpty() const                                                  \
            { return empty(); }                                               \
        size_t GetCount() const                                               \
            { return size(); }                                                \
        int Number() const                                                    \
            { return static_cast< int >( GetCount() ); }                      \
                                                                              \
        compatibility_iterator Item( size_t idx ) const                       \
        {                                                                     \
            iterator i = const_cast< liT* >(this)->begin();                   \
            std::advance( i, idx );                                           \
            return compatibility_iterator( this, i );                         \
        }                                                                     \
        elT operator[](size_t idx) const                                      \
        {                                                                     \
            return Item(idx).GetData();                                       \
        }                                                                     \
                                                                              \
        compatibility_iterator GetFirst() const                               \
        {                                                                     \
            return compatibility_iterator( this,                              \
                const_cast< liT* >(this)->begin() );                          \
        }                                                                     \
        compatibility_iterator GetLast() const                                \
        {                                                                     \
            iterator i = const_cast< liT* >(this)->end();                     \
            return compatibility_iterator( this, !empty() ? --i : i );        \
        }                                                                     \
        compatibility_iterator Member( elT e ) const                          \
            { return Find( e ); }                                             \
        compatibility_iterator Nth( int n ) const                             \
            { return Item( n ); }                                             \
        int IndexOf( elT e ) const                                            \
            { return Find( e ).IndexOf(); }                                   \
                                                                              \
        compatibility_iterator Append( elT e )                                \
        {                                                                     \
            push_back( e );                                                   \
            return GetLast();                                                 \
        }                                                                     \
        compatibility_iterator Insert( elT e )                                \
        {                                                                     \
            push_front( e );                                                  \
            return compatibility_iterator( this, begin() );                   \
        }                                                                     \
        compatibility_iterator Insert(const compatibility_iterator &i, elT e) \
        {                                                                     \
            return compatibility_iterator( this, insert( i.m_iter, e ) );     \
        }                                                                     \
        compatibility_iterator Insert( size_t idx, elT e )                    \
        {                                                                     \
            return compatibility_iterator( this,                              \
                                           insert( Item( idx ).m_iter, e ) ); \
        }                                                                     \
                                                                              \
        void DeleteContents( bool destroy )                                   \
            { m_destroy = destroy; }                                          \
        bool GetDeleteContents() const                                        \
            { return m_destroy; }                                             \
        void Erase( const compatibility_iterator& i )                         \
        {                                                                     \
            if ( m_destroy )                                                  \
                _WX_LIST_HELPER_##liT::DeleteFunction( i->GetData() );        \
            erase( i.m_iter );                                                \
        }                                                                     \
        bool DeleteNode( const compatibility_iterator& i )                    \
        {                                                                     \
            if( i )                                                           \
            {                                                                 \
                Erase( i );                                                   \
                return true;                                                  \
            }                                                                 \
            return false;                                                     \
        }                                                                     \
        bool DeleteObject( elT e )                                            \
        {                                                                     \
            return DeleteNode( Find( e ) );                                   \
        }                                                                     \
        void Clear()                                                          \
        {                                                                     \
            if ( m_destroy )                                                  \
                std::for_each( begin(), end(),                                \
                               _WX_LIST_HELPER_##liT::DeleteFunction );       \
            clear();                                                          \
        }                                                                     \
        /* Workaround for broken VC6 std::list::sort() see above */           \
        void Sort( wxSortCompareFunction compfunc )                           \
            { sort( WX_LIST_SORTFUNCTION( elT, compfunc ) ); }                \
        ~liT() { Clear(); }                                                   \
                                                                              \
        /* It needs access to our EmptyList */                                \
        friend decl compatibility_iterator;                                   \
    }

#define WX_DECLARE_LIST(elementtype, listname)                              \
    WX_DECLARE_LIST_WITH_DECL(elementtype, listname, class)
#define WX_DECLARE_LIST_PTR(elementtype, listname)                          \
    WX_DECLARE_LIST(elementtype, listname)

#define WX_DECLARE_EXPORTED_LIST(elementtype, listname)                     \
    WX_DECLARE_LIST_WITH_DECL(elementtype, listname, class WXDLLEXPORT)
#define WX_DECLARE_EXPORTED_LIST_PTR(elementtype, listname)                 \
    WX_DECLARE_EXPORTED_LIST(elementtype, listname)

#define WX_DECLARE_USER_EXPORTED_LIST(elementtype, listname, usergoo)       \
    WX_DECLARE_LIST_WITH_DECL(elementtype, listname, class usergoo)
#define WX_DECLARE_USER_EXPORTED_LIST_PTR(elementtype, listname, usergoo)   \
    WX_DECLARE_USER_EXPORTED_LIST(elementtype, listname, usergoo)

// this macro must be inserted in your program after
//      #include "wx/listimpl.cpp"
#define WX_DEFINE_LIST(name)    "don't forget to include listimpl.cpp!"

#define WX_DEFINE_EXPORTED_LIST(name)      WX_DEFINE_LIST(name)
#define WX_DEFINE_USER_EXPORTED_LIST(name) WX_DEFINE_LIST(name)

#else // if !wxUSE_STL

// due to circular header dependencies this function has to be declared here
// (normally it's found in utils.h which includes itself list.h...)
#if WXWIN_COMPATIBILITY_2_4
extern WXDLLIMPEXP_BASE wxChar* copystring(const wxChar *s);
#endif

// undef it to get rid of old, deprecated functions
#define wxLIST_COMPATIBILITY

// -----------------------------------------------------------------------------
// key stuff: a list may be optionally keyed on integer or string key
// -----------------------------------------------------------------------------

union wxListKeyValue
{
    long integer;
    wxChar *string;
};

// a struct which may contain both types of keys
//
// implementation note: on one hand, this class allows to have only one function
// for any keyed operation instead of 2 almost equivalent. OTOH, it's needed to
// resolve ambiguity which we would otherwise have with wxStringList::Find() and
// wxList::Find(const char *).
class WXDLLIMPEXP_BASE wxListKey
{
public:
    // implicit ctors
    wxListKey() : m_keyType(wxKEY_NONE)
        { }
    wxListKey(long i) : m_keyType(wxKEY_INTEGER)
        { m_key.integer = i; }
    wxListKey(const wxChar *s) : m_keyType(wxKEY_STRING)
        { m_key.string = wxStrdup(s); }
    wxListKey(const wxString& s) : m_keyType(wxKEY_STRING)
        { m_key.string = wxStrdup(s.c_str()); }

    // accessors
    wxKeyType GetKeyType() const { return m_keyType; }
    const wxChar *GetString() const
        { wxASSERT( m_keyType == wxKEY_STRING ); return m_key.string; }
    long GetNumber() const
        { wxASSERT( m_keyType == wxKEY_INTEGER ); return m_key.integer; }

    // comparison
    // Note: implementation moved to list.cpp to prevent BC++ inline
    // expansion warning.
    bool operator==(wxListKeyValue value) const ;

    // dtor
    ~wxListKey()
    {
        if ( m_keyType == wxKEY_STRING )
            free(m_key.string);
    }

private:
    wxKeyType m_keyType;
    wxListKeyValue m_key;
};

// -----------------------------------------------------------------------------
// wxNodeBase class is a (base for) node in a double linked list
// -----------------------------------------------------------------------------

extern WXDLLIMPEXP_DATA_BASE(wxListKey) wxDefaultListKey;

class WXDLLIMPEXP_FWD_BASE wxListBase;

class WXDLLIMPEXP_BASE wxNodeBase
{
friend class wxListBase;
public:
    // ctor
    wxNodeBase(wxListBase *list = (wxListBase *)NULL,
               wxNodeBase *previous = (wxNodeBase *)NULL,
               wxNodeBase *next = (wxNodeBase *)NULL,
               void *data = NULL,
               const wxListKey& key = wxDefaultListKey);

    virtual ~wxNodeBase();

    // FIXME no check is done that the list is really keyed on strings
    const wxChar *GetKeyString() const { return m_key.string; }
    long GetKeyInteger() const { return m_key.integer; }

    // Necessary for some existing code
    void SetKeyString(wxChar* s) { m_key.string = s; }
    void SetKeyInteger(long i) { m_key.integer = i; }

#ifdef wxLIST_COMPATIBILITY
    // compatibility methods, use Get* instead.
    wxDEPRECATED( wxNode *Next() const );
    wxDEPRECATED( wxNode *Previous() const );
    wxDEPRECATED( wxObject *Data() const );
#endif // wxLIST_COMPATIBILITY

protected:
    // all these are going to be "overloaded" in the derived classes
    wxNodeBase *GetNext() const { return m_next; }
    wxNodeBase *GetPrevious() const { return m_previous; }

    void *GetData() const { return m_data; }
    void SetData(void *data) { m_data = data; }

    // get 0-based index of this node within the list or wxNOT_FOUND
    int IndexOf() const;

    virtual void DeleteData() { }
public:
    // for wxList::iterator
    void** GetDataPtr() const { return &(((wxNodeBase*)this)->m_data); }
private:
    // optional key stuff
    wxListKeyValue m_key;

    void        *m_data;        // user data
    wxNodeBase  *m_next,        // next and previous nodes in the list
                *m_previous;

    wxListBase  *m_list;        // list we belong to

    DECLARE_NO_COPY_CLASS(wxNodeBase)
};

// -----------------------------------------------------------------------------
// a double-linked list class
// -----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_BASE wxList;

class WXDLLIMPEXP_BASE wxListBase : public wxObject
{
friend class WXDLLIMPEXP_FWD_BASE wxNodeBase; // should be able to call DetachNode()
friend class wxHashTableBase;   // should be able to call untyped Find()

public:
    // default ctor & dtor
    wxListBase(wxKeyType keyType = wxKEY_NONE)
        { Init(keyType); }
    virtual ~wxListBase();

    // accessors
        // count of items in the list
    size_t GetCount() const { return m_count; }

        // return true if this list is empty
    bool IsEmpty() const { return m_count == 0; }

    // operations

        // delete all nodes
    void Clear();

        // instruct it to destroy user data when deleting nodes
    void DeleteContents(bool destroy) { m_destroy = destroy; }

       // query if to delete
    bool GetDeleteContents() const
        { return m_destroy; }

      // get the keytype
    wxKeyType GetKeyType() const
        { return m_keyType; }

      // set the keytype (required by the serial code)
    void SetKeyType(wxKeyType keyType)
        { wxASSERT( m_count==0 ); m_keyType = keyType; }

#ifdef wxLIST_COMPATIBILITY
    // compatibility methods from old wxList
    wxDEPRECATED( int Number() const );             // use GetCount instead.
    wxDEPRECATED( wxNode *First() const );          // use GetFirst
    wxDEPRECATED( wxNode *Last() const );           // use GetLast
    wxDEPRECATED( wxNode *Nth(size_t n) const );    // use Item

    // kludge for typesafe list migration in core classes.
    wxDEPRECATED( operator wxList&() const );
#endif // wxLIST_COMPATIBILITY

protected:

    // all methods here are "overloaded" in derived classes to provide compile
    // time type checking

    // create a node for the list of this type
    virtual wxNodeBase *CreateNode(wxNodeBase *prev, wxNodeBase *next,
                                   void *data,
                                   const wxListKey& key = wxDefaultListKey) = 0;

// Can't access these from derived classes otherwise (bug in Salford C++?)
#ifdef __SALFORDC__
public:
#endif

    // ctors
        // from an array
    wxListBase(size_t count, void *elements[]);
        // from a sequence of objects
    wxListBase(void *object, ... /* terminate with NULL */);

protected:
    void Assign(const wxListBase& list)
        { Clear(); DoCopy(list); }

        // get list head/tail
    wxNodeBase *GetFirst() const { return m_nodeFirst; }
    wxNodeBase *GetLast() const { return m_nodeLast; }

        // by (0-based) index
    wxNodeBase *Item(size_t index) const;

        // get the list item's data
    void *operator[](size_t n) const
    {
        wxNodeBase *node = Item(n);

        return node ? node->GetData() : (wxNodeBase *)NULL;
    }

    // operations
        // append to end of list
    wxNodeBase *Prepend(void *object)
        { return (wxNodeBase *)wxListBase::Insert(object); }
        // append to beginning of list
    wxNodeBase *Append(void *object);
        // insert a new item at the beginning of the list
    wxNodeBase *Insert(void *object) { return Insert( (wxNodeBase*)NULL, object); }
        // insert a new item at the given position
    wxNodeBase *Insert(size_t pos, void *object)
        { return pos == GetCount() ? Append(object)
                                   : Insert(Item(pos), object); }
        // insert before given node or at front of list if prev == NULL
    wxNodeBase *Insert(wxNodeBase *prev, void *object);

        // keyed append
    wxNodeBase *Append(long key, void *object);
    wxNodeBase *Append(const wxChar *key, void *object);

        // removes node from the list but doesn't delete it (returns pointer
        // to the node or NULL if it wasn't found in the list)
    wxNodeBase *DetachNode(wxNodeBase *node);
        // delete element from list, returns false if node not found
    bool DeleteNode(wxNodeBase *node);
        // finds object pointer and deletes node (and object if DeleteContents
        // is on), returns false if object not found
    bool DeleteObject(void *object);

    // search (all return NULL if item not found)
        // by data
    wxNodeBase *Find(const void *object) const;

        // by key
    wxNodeBase *Find(const wxListKey& key) const;

    // get 0-based index of object or wxNOT_FOUND
    int IndexOf( void *object ) const;

    // this function allows the sorting of arbitrary lists by giving
    // a function to compare two list elements. The list is sorted in place.
    void Sort(const wxSortCompareFunction compfunc);

    // functions for iterating over the list
    void *FirstThat(wxListIterateFunction func);
    void ForEach(wxListIterateFunction func);
    void *LastThat(wxListIterateFunction func);

    // for STL interface, "last" points to one after the last node
    // of the controlled sequence (NULL for the end of the list)
    void Reverse();
    void DeleteNodes(wxNodeBase* first, wxNodeBase* last);
private:

        // common part of all ctors
    void Init(wxKeyType keyType = wxKEY_NONE);

    // helpers
        // common part of copy ctor and assignment operator
    void DoCopy(const wxListBase& list);
        // common part of all Append()s
    wxNodeBase *AppendCommon(wxNodeBase *node);
        // free node's data and node itself
    void DoDeleteNode(wxNodeBase *node);

    size_t m_count;             // number of elements in the list
    bool m_destroy;             // destroy user data when deleting list items?
    wxNodeBase *m_nodeFirst,    // pointers to the head and tail of the list
               *m_nodeLast;

    wxKeyType m_keyType;        // type of our keys (may be wxKEY_NONE)
};

// -----------------------------------------------------------------------------
// macros for definition of "template" list type
// -----------------------------------------------------------------------------

// and now some heavy magic...

// declare a list type named 'name' and containing elements of type 'T *'
// (as a by product of macro expansion you also get wx##name##Node
// wxNode-derived type)
//
// implementation details:
//  1. We define _WX_LIST_ITEM_TYPE_##name typedef to save in it the item type
//     for the list of given type - this allows us to pass only the list name
//     to WX_DEFINE_LIST() even if it needs both the name and the type
//
//  2. We redefine all non-type-safe wxList functions with type-safe versions
//     which don't take any space (everything is inline), but bring compile
//     time error checking.
//
//  3. The macro which is usually used (WX_DECLARE_LIST) is defined in terms of
//     a more generic WX_DECLARE_LIST_2 macro which, in turn, uses the most
//     generic WX_DECLARE_LIST_3 one. The last macro adds a sometimes
//     interesting capability to store polymorphic objects in the list and is
//     particularly useful with, for example, "wxWindow *" list where the
//     wxWindowBase pointers are put into the list, but wxWindow pointers are
//     retrieved from it.
//
//  4. final hack is that WX_DECLARE_LIST_3 is defined in terms of
//     WX_DECLARE_LIST_4 to allow defining classes without operator->() as
//     it results in compiler warnings when this operator doesn't make sense
//     (i.e. stored elements are not pointers)

// common part of WX_DECLARE_LIST_3 and WX_DECLARE_LIST_PTR_3
#define WX_DECLARE_LIST_4(T, Tbase, name, nodetype, classexp, ptrop)        \
    typedef int (*wxSortFuncFor_##name)(const T **, const T **);            \
                                                                            \
    classexp nodetype : public wxNodeBase                                   \
    {                                                                       \
    public:                                                                 \
        nodetype(wxListBase *list = (wxListBase *)NULL,                     \
                 nodetype *previous = (nodetype *)NULL,                     \
                 nodetype *next = (nodetype *)NULL,                         \
                 T *data = (T *)NULL,                                       \
                 const wxListKey& key = wxDefaultListKey)                   \
            : wxNodeBase(list, previous, next, data, key) { }               \
                                                                            \
        nodetype *GetNext() const                                           \
            { return (nodetype *)wxNodeBase::GetNext(); }                   \
        nodetype *GetPrevious() const                                       \
            { return (nodetype *)wxNodeBase::GetPrevious(); }               \
                                                                            \
        T *GetData() const                                                  \
            { return (T *)wxNodeBase::GetData(); }                          \
        void SetData(T *data)                                               \
            { wxNodeBase::SetData(data); }                                  \
                                                                            \
    protected:                                                              \
        virtual void DeleteData();                                          \
                                                                            \
        DECLARE_NO_COPY_CLASS(nodetype)                                     \
    };                                                                      \
                                                                            \
    classexp name : public wxListBase                                       \
    {                                                                       \
    public:                                                                 \
        typedef nodetype Node;                                              \
        classexp compatibility_iterator                                     \
        {                                                                   \
        public:                                                             \
            compatibility_iterator(Node *ptr = NULL) : m_ptr(ptr) { }       \
                                                                            \
            Node *operator->() const { return m_ptr; }                      \
            operator Node *() const { return m_ptr; }                       \
                                                                            \
        private:                                                            \
            Node *m_ptr;                                                    \
        };                                                                  \
                                                                            \
        name(wxKeyType keyType = wxKEY_NONE) : wxListBase(keyType)          \
            { }                                                             \
        name(const name& list) : wxListBase(list.GetKeyType())              \
            { Assign(list); }                                               \
        name(size_t count, T *elements[])                                   \
            : wxListBase(count, (void **)elements) { }                      \
                                                                            \
        name& operator=(const name& list)                                   \
            { Assign(list); return *this; }                                 \
                                                                            \
        nodetype *GetFirst() const                                          \
            { return (nodetype *)wxListBase::GetFirst(); }                  \
        nodetype *GetLast() const                                           \
            { return (nodetype *)wxListBase::GetLast(); }                   \
                                                                            \
        nodetype *Item(size_t index) const                                  \
            { return (nodetype *)wxListBase::Item(index); }                 \
                                                                            \
        T *operator[](size_t index) const                                   \
        {                                                                   \
            nodetype *node = Item(index);                                   \
            return node ? (T*)(node->GetData()) : (T*)NULL;                 \
        }                                                                   \
                                                                            \
        nodetype *Append(Tbase *object)                                     \
            { return (nodetype *)wxListBase::Append(object); }              \
        nodetype *Insert(Tbase *object)                                     \
            { return (nodetype *)Insert((nodetype*)NULL, object); }         \
        nodetype *Insert(size_t pos, Tbase *object)                         \
            { return (nodetype *)wxListBase::Insert(pos, object); }         \
        nodetype *Insert(nodetype *prev, Tbase *object)                     \
            { return (nodetype *)wxListBase::Insert(prev, object); }        \
                                                                            \
        nodetype *Append(long key, void *object)                            \
            { return (nodetype *)wxListBase::Append(key, object); }         \
        nodetype *Append(const wxChar *key, void *object)                   \
            { return (nodetype *)wxListBase::Append(key, object); }         \
                                                                            \
        nodetype *DetachNode(nodetype *node)                                \
            { return (nodetype *)wxListBase::DetachNode(node); }            \
        bool DeleteNode(nodetype *node)                                     \
            { return wxListBase::DeleteNode(node); }                        \
        bool DeleteObject(Tbase *object)                                    \
            { return wxListBase::DeleteObject(object); }                    \
        void Erase(nodetype *it)                                            \
            { DeleteNode(it); }                                             \
                                                                            \
        nodetype *Find(const Tbase *object) const                           \
            { return (nodetype *)wxListBase::Find(object); }                \
                                                                            \
        virtual nodetype *Find(const wxListKey& key) const                  \
            { return (nodetype *)wxListBase::Find(key); }                   \
                                                                            \
        int IndexOf(Tbase *object) const                                    \
            { return wxListBase::IndexOf(object); }                         \
                                                                            \
        void Sort(wxSortCompareFunction func)                               \
            { wxListBase::Sort(func); }                                     \
        void Sort(wxSortFuncFor_##name func)                                \
            { Sort((wxSortCompareFunction)func); }                          \
                                                                            \
    protected:                                                              \
        virtual wxNodeBase *CreateNode(wxNodeBase *prev, wxNodeBase *next,  \
                               void *data,                                  \
                               const wxListKey& key = wxDefaultListKey)     \
            {                                                               \
                return new nodetype(this,                                   \
                                    (nodetype *)prev, (nodetype *)next,     \
                                    (T *)data, key);                        \
            }                                                               \
        /* STL interface */                                                 \
    public:                                                                 \
        typedef size_t size_type;                                           \
        typedef int difference_type;                                        \
        typedef T* value_type;                                              \
        typedef Tbase* base_value_type;                                     \
        typedef value_type& reference;                                      \
        typedef const value_type& const_reference;                          \
        typedef base_value_type& base_reference;                            \
        typedef const base_value_type& const_base_reference;                \
                                                                            \
        classexp iterator                                                   \
        {                                                                   \
            typedef name list;                                              \
        public:                                                             \
            typedef nodetype Node;                                          \
            typedef iterator itor;                                          \
            typedef T* value_type;                                          \
            typedef value_type* ptr_type;                                   \
            typedef value_type& reference;                                  \
                                                                            \
            Node* m_node;                                                   \
            Node* m_init;                                                   \
        public:                                                             \
            typedef reference reference_type;                               \
            typedef ptr_type pointer_type;                                  \
                                                                            \
            iterator(Node* node, Node* init) : m_node(node), m_init(init) {}\
            iterator() : m_node(NULL), m_init(NULL) { }                     \
            reference_type operator*() const                                \
                { return *(pointer_type)m_node->GetDataPtr(); }             \
            ptrop                                                           \
            itor& operator++() { m_node = m_node->GetNext(); return *this; }\
            const itor operator++(int)                                      \
                { itor tmp = *this; m_node = m_node->GetNext(); return tmp; }\
            itor& operator--()                                              \
            {                                                               \
                m_node = m_node ? m_node->GetPrevious() : m_init;           \
                return *this;                                               \
            }                                                               \
            const itor operator--(int)                                      \
            {                                                               \
                itor tmp = *this;                                           \
                m_node = m_node ? m_node->GetPrevious() : m_init;           \
                return tmp;                                                 \
            }                                                               \
            bool operator!=(const itor& it) const                           \
                { return it.m_node != m_node; }                             \
            bool operator==(const itor& it) const                           \
                { return it.m_node == m_node; }                             \
        };                                                                  \
        classexp const_iterator                                             \
        {                                                                   \
            typedef name list;                                              \
        public:                                                             \
            typedef nodetype Node;                                          \
            typedef T* value_type;                                          \
            typedef const value_type& const_reference;                      \
            typedef const_iterator itor;                                    \
            typedef value_type* ptr_type;                                   \
                                                                            \
            Node* m_node;                                                   \
            Node* m_init;                                                   \
        public:                                                             \
            typedef const_reference reference_type;                         \
            typedef const ptr_type pointer_type;                            \
                                                                            \
            const_iterator(Node* node, Node* init)                          \
                : m_node(node), m_init(init) { }                            \
            const_iterator() : m_node(NULL), m_init(NULL) { }               \
            const_iterator(const iterator& it)                              \
                : m_node(it.m_node), m_init(it.m_init) { }                  \
            reference_type operator*() const                                \
                { return *(pointer_type)m_node->GetDataPtr(); }             \
            ptrop                                                           \
            itor& operator++() { m_node = m_node->GetNext(); return *this; }\
            const itor operator++(int)                                      \
                { itor tmp = *this; m_node = m_node->GetNext(); return tmp; }\
            itor& operator--()                                              \
            {                                                               \
                m_node = m_node ? m_node->GetPrevious() : m_init;           \
                return *this;                                               \
            }                                                               \
            const itor operator--(int)                                      \
            {                                                               \
                itor tmp = *this;                                           \
                m_node = m_node ? m_node->GetPrevious() : m_init;           \
                return tmp;                                                 \
            }                                                               \
            bool operator!=(const itor& it) const                           \
                { return it.m_node != m_node; }                             \
            bool operator==(const itor& it) const                           \
                { return it.m_node == m_node; }                             \
        };                                                                  \
        classexp reverse_iterator                                           \
        {                                                                   \
            typedef name list;                                              \
        public:                                                             \
            typedef nodetype Node;                                          \
            typedef T* value_type;                                          \
            typedef reverse_iterator itor;                                  \
            typedef value_type* ptr_type;                                   \
            typedef value_type& reference;                                  \
                                                                            \
            Node* m_node;                                                   \
            Node* m_init;                                                   \
        public:                                                             \
            typedef reference reference_type;                               \
            typedef ptr_type pointer_type;                                  \
                                                                            \
            reverse_iterator(Node* node, Node* init)                        \
                : m_node(node), m_init(init) { }                            \
            reverse_iterator() : m_node(NULL), m_init(NULL) { }             \
            reference_type operator*() const                                \
                { return *(pointer_type)m_node->GetDataPtr(); }             \
            ptrop                                                           \
            itor& operator++()                                              \
                { m_node = m_node->GetPrevious(); return *this; }           \
            const itor operator++(int)                                      \
            { itor tmp = *this; m_node = m_node->GetPrevious(); return tmp; }\
            itor& operator--()                                              \
            { m_node = m_node ? m_node->GetNext() : m_init; return *this; } \
            const itor operator--(int)                                      \
            {                                                               \
                itor tmp = *this;                                           \
                m_node = m_node ? m_node->GetNext() : m_init;               \
                return tmp;                                                 \
            }                                                               \
            bool operator!=(const itor& it) const                           \
                { return it.m_node != m_node; }                             \
            bool operator==(const itor& it) const                           \
                { return it.m_node == m_node; }                             \
        };                                                                  \
        classexp const_reverse_iterator                                     \
        {                                                                   \
            typedef name list;                                              \
        public:                                                             \
            typedef nodetype Node;                                          \
            typedef T* value_type;                                          \
            typedef const_reverse_iterator itor;                            \
            typedef value_type* ptr_type;                                   \
            typedef const value_type& const_reference;                      \
                                                                            \
            Node* m_node;                                                   \
            Node* m_init;                                                   \
        public:                                                             \
            typedef const_reference reference_type;                         \
            typedef const ptr_type pointer_type;                            \
                                                                            \
            const_reverse_iterator(Node* node, Node* init)                  \
                : m_node(node), m_init(init) { }                            \
            const_reverse_iterator() : m_node(NULL), m_init(NULL) { }       \
            const_reverse_iterator(const reverse_iterator& it)              \
                : m_node(it.m_node), m_init(it.m_init) { }                  \
            reference_type operator*() const                                \
                { return *(pointer_type)m_node->GetDataPtr(); }             \
            ptrop                                                           \
            itor& operator++()                                              \
                { m_node = m_node->GetPrevious(); return *this; }           \
            const itor operator++(int)                                      \
            { itor tmp = *this; m_node = m_node->GetPrevious(); return tmp; }\
            itor& operator--()                                              \
                { m_node = m_node ? m_node->GetNext() : m_init; return *this;}\
            const itor operator--(int)                                      \
            {                                                               \
                itor tmp = *this;                                           \
                m_node = m_node ? m_node->GetNext() : m_init;               \
                return tmp;                                                 \
            }                                                               \
            bool operator!=(const itor& it) const                           \
                { return it.m_node != m_node; }                             \
            bool operator==(const itor& it) const                           \
                { return it.m_node == m_node; }                             \
        };                                                                  \
                                                                            \
        wxEXPLICIT name(size_type n, const_reference v = value_type())      \
            { assign(n, v); }                                               \
        name(const const_iterator& first, const const_iterator& last)       \
            { assign(first, last); }                                        \
        iterator begin() { return iterator(GetFirst(), GetLast()); }        \
        const_iterator begin() const                                        \
            { return const_iterator(GetFirst(), GetLast()); }               \
        iterator end() { return iterator(NULL, GetLast()); }                \
        const_iterator end() const { return const_iterator(NULL, GetLast()); }\
        reverse_iterator rbegin()                                           \
            { return reverse_iterator(GetLast(), GetFirst()); }             \
        const_reverse_iterator rbegin() const                               \
            { return const_reverse_iterator(GetLast(), GetFirst()); }       \
        reverse_iterator rend() { return reverse_iterator(NULL, GetFirst()); }\
        const_reverse_iterator rend() const                                 \
            { return const_reverse_iterator(NULL, GetFirst()); }            \
        void resize(size_type n, value_type v = value_type())               \
        {                                                                   \
            while (n < size())                                              \
                pop_back();                                                 \
            while (n > size())                                              \
                push_back(v);                                                \
        }                                                                   \
        size_type size() const { return GetCount(); }                       \
        size_type max_size() const { return INT_MAX; }                      \
        bool empty() const { return IsEmpty(); }                            \
        reference front() { return *begin(); }                              \
        const_reference front() const { return *begin(); }                  \
        reference back() { iterator tmp = end(); return *--tmp; }           \
        const_reference back() const { const_iterator tmp = end(); return *--tmp; }\
        void push_front(const_reference v = value_type())                   \
            { Insert(GetFirst(), (const_base_reference)v); }                \
        void pop_front() { DeleteNode(GetFirst()); }                        \
        void push_back(const_reference v = value_type())                    \
            { Append((const_base_reference)v); }                            \
        void pop_back() { DeleteNode(GetLast()); }                          \
        void assign(const_iterator first, const const_iterator& last)       \
        {                                                                   \
            clear();                                                        \
            for(; first != last; ++first)                                   \
                Append((const_base_reference)*first);                       \
        }                                                                   \
        void assign(size_type n, const_reference v = value_type())          \
        {                                                                   \
            clear();                                                        \
            for(size_type i = 0; i < n; ++i)                                \
                Append((const_base_reference)v);                            \
        }                                                                   \
        iterator insert(const iterator& it, const_reference v = value_type())\
        {                                                                   \
            if ( it == end() )                                              \
                Append((const_base_reference)v);                            \
            else                                                            \
                Insert(it.m_node, (const_base_reference)v);                 \
            iterator itprev(it);                                            \
            return itprev--;                                                \
        }                                                                   \
        void insert(const iterator& it, size_type n, const_reference v = value_type())\
        {                                                                   \
            for(size_type i = 0; i < n; ++i)                                \
                insert(it, v);                                              \
        }                                                                   \
        void insert(const iterator& it, const_iterator first, const const_iterator& last)\
        {                                                                   \
            for(; first != last; ++first)                                   \
                insert(it, *first);                                         \
        }                                                                   \
        iterator erase(const iterator& it)                                  \
        {                                                                   \
            iterator next = iterator(it.m_node->GetNext(), GetLast());      \
            DeleteNode(it.m_node); return next;                             \
        }                                                                   \
        iterator erase(const iterator& first, const iterator& last)         \
        {                                                                   \
            iterator next = last;                                           \
            if ( next != end() )                                            \
                ++next;                                                     \
            DeleteNodes(first.m_node, last.m_node);                         \
            return next;                                                    \
        }                                                                   \
        void clear() { Clear(); }                                           \
        void splice(const iterator& it, name& l, const iterator& first, const iterator& last)\
            { insert(it, first, last); l.erase(first, last); }              \
        void splice(const iterator& it, name& l)                            \
            { splice(it, l, l.begin(), l.end() ); }                         \
        void splice(const iterator& it, name& l, const iterator& first)     \
        {                                                                   \
            if ( it != first )                                              \
            {                                                               \
                insert(it, *first);                                         \
                l.erase(first);                                             \
            }                                                               \
        }                                                                   \
        void remove(const_reference v)                                      \
            { DeleteObject((const_base_reference)v); }                      \
        void reverse()                                                      \
            { Reverse(); }                                                  \
     /* void swap(name& l)                                                  \
        {                                                                   \
            { size_t t = m_count; m_count = l.m_count; l.m_count = t; }     \
            { bool t = m_destroy; m_destroy = l.m_destroy; l.m_destroy = t; }\
            { wxNodeBase* t = m_nodeFirst; m_nodeFirst = l.m_nodeFirst; l.m_nodeFirst = t; }\
            { wxNodeBase* t = m_nodeLast; m_nodeLast = l.m_nodeLast; l.m_nodeLast = t; }\
            { wxKeyType t = m_keyType; m_keyType = l.m_keyType; l.m_keyType = t; }\
        } */                                                                \
    }

#define WX_LIST_PTROP                                                       \
            pointer_type operator->() const                                 \
                { return (pointer_type)m_node->GetDataPtr(); }
#define WX_LIST_PTROP_NONE

#define WX_DECLARE_LIST_3(T, Tbase, name, nodetype, classexp)               \
    WX_DECLARE_LIST_4(T, Tbase, name, nodetype, classexp, WX_LIST_PTROP_NONE)
#define WX_DECLARE_LIST_PTR_3(T, Tbase, name, nodetype, classexp)        \
    WX_DECLARE_LIST_4(T, Tbase, name, nodetype, classexp, WX_LIST_PTROP)

#define WX_DECLARE_LIST_2(elementtype, listname, nodename, classexp)        \
    WX_DECLARE_LIST_3(elementtype, elementtype, listname, nodename, classexp)
#define WX_DECLARE_LIST_PTR_2(elementtype, listname, nodename, classexp)        \
    WX_DECLARE_LIST_PTR_3(elementtype, elementtype, listname, nodename, classexp)

#define WX_DECLARE_LIST(elementtype, listname)                              \
    typedef elementtype _WX_LIST_ITEM_TYPE_##listname;                      \
    WX_DECLARE_LIST_2(elementtype, listname, wx##listname##Node, class)
#define WX_DECLARE_LIST_PTR(elementtype, listname)                              \
    typedef elementtype _WX_LIST_ITEM_TYPE_##listname;                      \
    WX_DECLARE_LIST_PTR_2(elementtype, listname, wx##listname##Node, class)

#define WX_DECLARE_LIST_WITH_DECL(elementtype, listname, decl) \
    typedef elementtype _WX_LIST_ITEM_TYPE_##listname;                      \
    WX_DECLARE_LIST_2(elementtype, listname, wx##listname##Node, decl)

#define WX_DECLARE_EXPORTED_LIST(elementtype, listname)                     \
    WX_DECLARE_LIST_WITH_DECL(elementtype, listname, class WXDLLEXPORT)

#define WX_DECLARE_EXPORTED_LIST_PTR(elementtype, listname)                     \
    typedef elementtype _WX_LIST_ITEM_TYPE_##listname;                      \
    WX_DECLARE_LIST_PTR_2(elementtype, listname, wx##listname##Node, class WXDLLEXPORT)

#define WX_DECLARE_USER_EXPORTED_LIST(elementtype, listname, usergoo)       \
    typedef elementtype _WX_LIST_ITEM_TYPE_##listname;                      \
    WX_DECLARE_LIST_2(elementtype, listname, wx##listname##Node, class usergoo)
#define WX_DECLARE_USER_EXPORTED_LIST_PTR(elementtype, listname, usergoo)       \
    typedef elementtype _WX_LIST_ITEM_TYPE_##listname;                      \
    WX_DECLARE_LIST_PTR_2(elementtype, listname, wx##listname##Node, class usergoo)

// this macro must be inserted in your program after
//      #include "wx/listimpl.cpp"
#define WX_DEFINE_LIST(name)    "don't forget to include listimpl.cpp!"

#define WX_DEFINE_EXPORTED_LIST(name)      WX_DEFINE_LIST(name)
#define WX_DEFINE_USER_EXPORTED_LIST(name) WX_DEFINE_LIST(name)

#endif // !wxUSE_STL

// ============================================================================
// now we can define classes 100% compatible with the old ones
// ============================================================================

// ----------------------------------------------------------------------------
// commonly used list classes
// ----------------------------------------------------------------------------

#if defined(wxLIST_COMPATIBILITY)

// inline compatibility functions

#if !wxUSE_STL

// ----------------------------------------------------------------------------
// wxNodeBase deprecated methods
// ----------------------------------------------------------------------------

inline wxNode *wxNodeBase::Next() const { return (wxNode *)GetNext(); }
inline wxNode *wxNodeBase::Previous() const { return (wxNode *)GetPrevious(); }
inline wxObject *wxNodeBase::Data() const { return (wxObject *)GetData(); }

// ----------------------------------------------------------------------------
// wxListBase deprecated methods
// ----------------------------------------------------------------------------

inline int wxListBase::Number() const { return (int)GetCount(); }
inline wxNode *wxListBase::First() const { return (wxNode *)GetFirst(); }
inline wxNode *wxListBase::Last() const { return (wxNode *)GetLast(); }
inline wxNode *wxListBase::Nth(size_t n) const { return (wxNode *)Item(n); }
inline wxListBase::operator wxList&() const { return *(wxList*)this; }

#endif

// define this to make a lot of noise about use of the old wxList classes.
//#define wxWARN_COMPAT_LIST_USE

// ----------------------------------------------------------------------------
// wxList compatibility class: in fact, it's a list of wxObjects
// ----------------------------------------------------------------------------

WX_DECLARE_LIST_2(wxObject, wxObjectList, wxObjectListNode,
                        class WXDLLIMPEXP_BASE);

class WXDLLIMPEXP_BASE wxList : public wxObjectList
{
public:
#if defined(wxWARN_COMPAT_LIST_USE) && !wxUSE_STL
    wxList() { };
    wxDEPRECATED( wxList(int key_type) );
#elif !wxUSE_STL
    wxList(int key_type = wxKEY_NONE);
#endif

    // this destructor is required for Darwin
   ~wxList() { }

#if !wxUSE_STL
    wxList& operator=(const wxList& list)
        { (void) wxListBase::operator=(list); return *this; }

    // compatibility methods
    void Sort(wxSortCompareFunction compfunc) { wxListBase::Sort(compfunc); }
#endif

#if wxUSE_STL
#else
    wxNode *Member(wxObject *object) const { return (wxNode *)Find(object); }
#endif

private:
#if !wxUSE_STL
    DECLARE_DYNAMIC_CLASS(wxList)
#endif
};

#if !wxUSE_STL

// -----------------------------------------------------------------------------
// wxStringList class for compatibility with the old code
// -----------------------------------------------------------------------------
WX_DECLARE_LIST_2(wxChar, wxStringListBase, wxStringListNode, class WXDLLIMPEXP_BASE);

class WXDLLIMPEXP_BASE wxStringList : public wxStringListBase
{
public:
    // ctors and such
        // default
#ifdef wxWARN_COMPAT_LIST_USE
    wxStringList();
    wxDEPRECATED( wxStringList(const wxChar *first ...) );
#else
    wxStringList();
    wxStringList(const wxChar *first ...);
#endif

        // copying the string list: the strings are copied, too (extremely
        // inefficient!)
    wxStringList(const wxStringList& other) : wxStringListBase() { DeleteContents(true); DoCopy(other); }
    wxStringList& operator=(const wxStringList& other)
        { Clear(); DoCopy(other); return *this; }

    // operations
        // makes a copy of the string
    wxNode *Add(const wxChar *s);

        // Append to beginning of list
    wxNode *Prepend(const wxChar *s);

    bool Delete(const wxChar *s);

    wxChar **ListToArray(bool new_copies = false) const;
    bool Member(const wxChar *s) const;

    // alphabetic sort
    void Sort();

private:
    void DoCopy(const wxStringList&); // common part of copy ctor and operator=

    DECLARE_DYNAMIC_CLASS(wxStringList)
};

#else // if wxUSE_STL

WX_DECLARE_LIST_XO(wxString, wxStringListBase, class WXDLLIMPEXP_BASE);

class WXDLLIMPEXP_BASE wxStringList : public wxStringListBase
{
public:
    compatibility_iterator Append(wxChar* s)
        { wxString tmp = s; delete[] s; return wxStringListBase::Append(tmp); }
    compatibility_iterator Insert(wxChar* s)
        { wxString tmp = s; delete[] s; return wxStringListBase::Insert(tmp); }
    compatibility_iterator Insert(size_t pos, wxChar* s)
    {
        wxString tmp = s;
        delete[] s;
        return wxStringListBase::Insert(pos, tmp);
    }
    compatibility_iterator Add(const wxChar* s)
        { push_back(s); return GetLast(); }
    compatibility_iterator Prepend(const wxChar* s)
        { push_front(s); return GetFirst(); }
};

#endif // wxUSE_STL

#endif // wxLIST_COMPATIBILITY

// delete all list elements
//
// NB: the class declaration of the list elements must be visible from the
//     place where you use this macro, otherwise the proper destructor may not
//     be called (a decent compiler should give a warning about it, but don't
//     count on it)!
#define WX_CLEAR_LIST(type, list)                                            \
    {                                                                        \
        type::iterator it, en;                                               \
        for( it = (list).begin(), en = (list).end(); it != en; ++it )        \
            delete *it;                                                      \
        (list).clear();                                                      \
    }

#endif // _WX_LISTH__
