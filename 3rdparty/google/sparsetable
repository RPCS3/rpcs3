// Copyright (c) 2005, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Craig Silverstein
//
// A sparsetable is a random container that implements a sparse array,
// that is, an array that uses very little memory to store unassigned
// indices (in this case, between 1-2 bits per unassigned index).  For
// instance, if you allocate an array of size 5 and assign a[2] = <big
// struct>, then a[2] will take up a lot of memory but a[0], a[1],
// a[3], and a[4] will not.  Array elements that have a value are
// called "assigned".  Array elements that have no value yet, or have
// had their value cleared using erase() or clear(), are called
// "unassigned".
//
// Unassigned values seem to have the default value of T (see below).
// Nevertheless, there is a difference between an unassigned index and
// one explicitly assigned the value of T().  The latter is considered
// assigned.
//
// Access to an array element is constant time, as is insertion and
// deletion.  Insertion and deletion may be fairly slow, however:
// because of this container's memory economy, each insert and delete
// causes a memory reallocation.
//
// See /usr/(local/)?doc/sparsehash-0.1/sparsetable.html
// for information about how to use this class.

#ifndef _SPARSETABLE_H_
#define _SPARSETABLE_H_

#include <google/sparsehash/sparseconfig.h>
#include <stdlib.h>             // for malloc/free
#include <stdio.h>              // to read/write tables
#ifdef HAVE_STDINT_H
#include <stdint.h>             // the normal place uint16_t is defined
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>          // the normal place u_int16_t is defined
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>           // a third place for uint16_t or u_int16_t
#endif
#include <assert.h>             // for bounds checking
#include <iterator>             // to define reverse_iterator for me
#include <algorithm>            // equal, lexicographical_compare, swap,...
#include <memory>               // uninitialized_copy
#include <vector>               // a sparsetable is a vector of groups
#include <google/type_traits.h> // for true_type, integral_constant, etc.

#if STDC_HEADERS
#include <string.h>             // for memcpy
#else
#if !HAVE_MEMCPY
#define memcpy(d, s, n)   bcopy ((s), (d), (n))
#endif
#endif

_START_GOOGLE_NAMESPACE_

#ifndef HAVE_U_INT16_T
# if defined HAVE_UINT16_T
    typedef uint16_t u_int16_t;    // true on solaris, possibly other C99 libc's
# elif defined HAVE___UINT16
    typedef __int16 int16_t;       // true on vc++7
    typedef unsigned __int16 u_int16_t;
# else
    // Cannot find a 16-bit integer type.  Hoping for the best with "short"...
    typedef short int int16_t;
    typedef unsigned short int u_int16_t;
# endif
#endif

using STL_NAMESPACE::vector;
using STL_NAMESPACE::uninitialized_copy;

// The smaller this is, the faster lookup is (because the group bitmap is
// smaller) and the faster insert is, because there's less to move.
// On the other hand, there are more groups.  Since group::size_type is
// a short, this number should be of the form 32*x + 16 to avoid waste.
static const u_int16_t DEFAULT_SPARSEGROUP_SIZE = 48;   // fits in 1.5 words


// A NOTE ON ASSIGNING:
// A sparse table does not actually allocate memory for entries
// that are not filled.  Because of this, it becomes complicated
// to have a non-const iterator: we don't know, if the iterator points
// to a not-filled bucket, whether you plan to fill it with something
// or whether you plan to read its value (in which case you'll get
// the default bucket value).  Therefore, while we can define const
// operations in a pretty 'normal' way, for non-const operations, we
// define something that returns a helper object with operator= and
// operator& that allocate a bucket lazily.  We use this for table[]
// and also for regular table iterators.

template <class tabletype>
class table_element_adaptor {
 public:
  typedef typename tabletype::value_type value_type;
  typedef typename tabletype::size_type size_type;
  typedef typename tabletype::reference reference;
  typedef typename tabletype::pointer pointer;

  table_element_adaptor(tabletype *tbl, size_type p)
    : table(tbl), pos(p) { }
  table_element_adaptor& operator= (const value_type &val) {
    table->set(pos, val);
    return *this;
  }
  operator value_type() { return table->get(pos); }   // we look like a value
  pointer operator& () { return &table->mutating_get(pos); }

 private:
  tabletype* table;
  size_type pos;
};

// Our iterator as simple as iterators can be: basically it's just
// the index into our table.  Dereference, the only complicated
// thing, we punt to the table class.  This just goes to show how
// much machinery STL requires to do even the most trivial tasks.
//
// By templatizing over tabletype, we have one iterator type which
// we can use for both sparsetables and sparsebins.  In fact it
// works on any class that allows size() and operator[] (eg vector),
// as long as it does the standard STL typedefs too (eg value_type).

template <class tabletype>
class table_iterator {
 public:
  typedef table_iterator iterator;

  typedef STL_NAMESPACE::random_access_iterator_tag iterator_category;
  typedef typename tabletype::value_type value_type;
  typedef typename tabletype::difference_type difference_type;
  typedef typename tabletype::size_type size_type;
  typedef table_element_adaptor<tabletype> reference;
  typedef table_element_adaptor<tabletype>* pointer;

  // The "real" constructor
  table_iterator(tabletype *tbl, size_type p)
    : table(tbl), pos(p) { }
  // The default constructor, used when I define vars of type table::iterator
  table_iterator() : table(NULL), pos(0) { }
  // The copy constructor, for when I say table::iterator foo = tbl.begin()
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // The main thing our iterator does is dereference.  If the table entry
  // we point to is empty, we return the default value type.
  // This is the big different function from the const iterator.
  reference operator*()              {
    return table_element_adaptor<tabletype>(table, pos);
  }
  pointer operator->()               { return &(operator*()); }

  // Helper function to assert things are ok; eg pos is still in range
  void check() const {
    assert(table);
    assert(pos <= table->size());
  }

  // Arithmetic: we just do arithmetic on pos.  We don't even need to
  // do bounds checking, since STL doesn't consider that it's job.  :-)
  iterator& operator+=(size_type t) { pos += t; check(); return *this; }
  iterator& operator-=(size_type t) { pos -= t; check(); return *this; }
  iterator& operator++()            { ++pos; check(); return *this; }
  iterator& operator--()            { --pos; check(); return *this; }
  iterator operator++(int)          { iterator tmp(*this);     // for x++
                                      ++pos; check(); return tmp; }
  iterator operator--(int)          { iterator tmp(*this);     // for x--
                                      --pos; check(); return tmp; }
  iterator operator+(difference_type i) const  { iterator tmp(*this);
                                                 tmp += i; return tmp; }
  iterator operator-(difference_type i) const  { iterator tmp(*this);
                                                 tmp -= i; return tmp; }
  difference_type operator-(iterator it) const {      // for "x = it2 - it"
    assert(table == it.table);
    return pos - it.pos;
  }
  reference operator[](difference_type n) const {
    return *(*this + n);            // simple though not totally efficient
  }

  // Comparisons.
  bool operator==(const iterator& it) const {
    return table == it.table && pos == it.pos;
  }
  bool operator<(const iterator& it) const {
    assert(table == it.table);              // life is bad bad bad otherwise
    return pos < it.pos;
  }
  bool operator!=(const iterator& it) const { return !(*this == it); }
  bool operator<=(const iterator& it) const { return !(it < *this); }
  bool operator>(const iterator& it) const { return it < *this; }
  bool operator>=(const iterator& it) const { return !(*this < it); }

  // Here's the info we actually need to be an iterator
  tabletype *table;              // so we can dereference and bounds-check
  size_type pos;                 // index into the table
};

// support for "3 + iterator" has to be defined outside the class, alas
template<class T>
table_iterator<T> operator+(typename table_iterator<T>::difference_type i,
                            table_iterator<T> it) {
  return it + i;               // so people can say it2 = 3 + it
}

template <class tabletype>
class const_table_iterator {
 public:
  typedef table_iterator<tabletype> iterator;
  typedef const_table_iterator const_iterator;

  typedef STL_NAMESPACE::random_access_iterator_tag iterator_category;
  typedef typename tabletype::value_type value_type;
  typedef typename tabletype::difference_type difference_type;
  typedef typename tabletype::size_type size_type;
  typedef typename tabletype::const_reference reference;  // we're const-only
  typedef typename tabletype::const_pointer pointer;

  // The "real" constructor
  const_table_iterator(const tabletype *tbl, size_type p)
    : table(tbl), pos(p) { }
  // The default constructor, used when I define vars of type table::iterator
  const_table_iterator() : table(NULL), pos(0) { }
  // The copy constructor, for when I say table::iterator foo = tbl.begin()
  // Also converts normal iterators to const iterators
  const_table_iterator(const iterator &from)
    : table(from.table), pos(from.pos) { }
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // The main thing our iterator does is dereference.  If the table entry
  // we point to is empty, we return the default value type.
  reference operator*() const       { return (*table)[pos]; }
  pointer operator->() const        { return &(operator*()); }

  // Helper function to assert things are ok; eg pos is still in range
  void check() const {
    assert(table);
    assert(pos <= table->size());
  }

  // Arithmetic: we just do arithmetic on pos.  We don't even need to
  // do bounds checking, since STL doesn't consider that it's job.  :-)
  const_iterator& operator+=(size_type t) { pos += t; check(); return *this; }
  const_iterator& operator-=(size_type t) { pos -= t; check(); return *this; }
  const_iterator& operator++()            { ++pos; check(); return *this; }
  const_iterator& operator--()            { --pos; check(); return *this; }
  const_iterator operator++(int)          { const_iterator tmp(*this); // for x++
                                            ++pos; check(); return tmp; }
  const_iterator operator--(int)          { const_iterator tmp(*this); // for x--
                                            --pos; check(); return tmp; }
  const_iterator operator+(difference_type i) const  { const_iterator tmp(*this);
                                                       tmp += i; return tmp; }
  const_iterator operator-(difference_type i) const  { const_iterator tmp(*this);
                                                       tmp -= i; return tmp; }
  difference_type operator-(const_iterator it) const {   // for "x = it2 - it"
    assert(table == it.table);
    return pos - it.pos;
  }
  reference operator[](difference_type n) const {
    return *(*this + n);            // simple though not totally efficient
  }

  // Comparisons.
  bool operator==(const const_iterator& it) const {
    return table == it.table && pos == it.pos;
  }
  bool operator<(const const_iterator& it) const {
    assert(table == it.table);              // life is bad bad bad otherwise
    return pos < it.pos;
  }
  bool operator!=(const const_iterator& it) const { return !(*this == it); }
  bool operator<=(const const_iterator& it) const { return !(it < *this); }
  bool operator>(const const_iterator& it) const { return it < *this; }
  bool operator>=(const const_iterator& it) const { return !(*this < it); }

  // Here's the info we actually need to be an iterator
  const tabletype *table;        // so we can dereference and bounds-check
  size_type pos;                 // index into the table
};

// support for "3 + iterator" has to be defined outside the class, alas
template<class T>
const_table_iterator<T> operator+(typename
                                  const_table_iterator<T>::difference_type i,
                                  const_table_iterator<T> it) {
  return it + i;               // so people can say it2 = 3 + it
}


// ---------------------------------------------------------------------------


/*
// This is a 2-D iterator.  You specify a begin and end over a list
// of *containers*.  We iterate over each container by iterating over
// it.  It's actually simple:
// VECTOR.begin() VECTOR[0].begin()  --------> VECTOR[0].end() ---,
//     |          ________________________________________________/
//     |          \_> VECTOR[1].begin()  -------->  VECTOR[1].end() -,
//     |          ___________________________________________________/
//     v          \_> ......
// VECTOR.end()
//
// It's impossible to do random access on one of these things in constant
// time, so it's just a bidirectional iterator.
//
// Unfortunately, because we need to use this for a non-empty iterator,
// we use nonempty_begin() and nonempty_end() instead of begin() and end()
// (though only going across, not down).
*/

#define TWOD_BEGIN_      nonempty_begin
#define TWOD_END_        nonempty_end
#define TWOD_ITER_       nonempty_iterator
#define TWOD_CONST_ITER_ const_nonempty_iterator

template <class containertype>
class two_d_iterator {
 public:
  typedef two_d_iterator iterator;

  typedef STL_NAMESPACE::bidirectional_iterator_tag iterator_category;
  // apparently some versions of VC++ have trouble with two ::'s in a typename
  typedef typename containertype::value_type _tmp_vt;
  typedef typename _tmp_vt::value_type value_type;
  typedef typename _tmp_vt::difference_type difference_type;
  typedef typename _tmp_vt::reference reference;
  typedef typename _tmp_vt::pointer pointer;

  // The "real" constructor.  begin and end specify how many rows we have
  // (in the diagram above); we always iterate over each row completely.
  two_d_iterator(typename containertype::iterator begin,
                 typename containertype::iterator end,
                 typename containertype::iterator curr)
    : row_begin(begin), row_end(end), row_current(curr), col_current() {
    if ( row_current != row_end ) {
      col_current = row_current->TWOD_BEGIN_();
      advance_past_end();                 // in case cur->begin() == cur->end()
    }
  }
  // If you want to start at an arbitrary place, you can, I guess
  two_d_iterator(typename containertype::iterator begin,
                 typename containertype::iterator end,
                 typename containertype::iterator curr,
                 typename containertype::value_type::TWOD_ITER_ col)
    : row_begin(begin), row_end(end), row_current(curr), col_current(col) {
    advance_past_end();                 // in case cur->begin() == cur->end()
  }
  // The default constructor, used when I define vars of type table::iterator
  two_d_iterator() : row_begin(), row_end(), row_current(), col_current() { }
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // Happy dereferencer
  reference operator*() const    { return *col_current; }
  pointer operator->() const     { return &(operator*()); }

  // Arithmetic: we just do arithmetic on pos.  We don't even need to
  // do bounds checking, since STL doesn't consider that it's job.  :-)
  // NOTE: this is not amortized constant time!  What do we do about it?
  void advance_past_end() {          // used when col_current points to end()
    while ( col_current == row_current->TWOD_END_() ) {  // end of current row
      ++row_current;                                // go to beginning of next
      if ( row_current != row_end )                 // col is irrelevant at end
        col_current = row_current->TWOD_BEGIN_();
      else
        break;                                      // don't go past row_end
    }
  }

  iterator& operator++() {
    assert(row_current != row_end);                 // how to ++ from there?
    ++col_current;
    advance_past_end();                 // in case col_current is at end()
    return *this;
  }
  iterator& operator--() {
    while ( row_current == row_end ||
            col_current == row_current->TWOD_BEGIN_() ) {
      assert(row_current != row_begin);
      --row_current;
      col_current = row_current->TWOD_END_();             // this is 1 too far
    }
    --col_current;
    return *this;
  }
  iterator operator++(int)       { iterator tmp(*this); ++*this; return tmp; }
  iterator operator--(int)       { iterator tmp(*this); --*this; return tmp; }


  // Comparisons.
  bool operator==(const iterator& it) const {
    return ( row_begin == it.row_begin &&
             row_end == it.row_end &&
             row_current == it.row_current &&
             (row_current == row_end || col_current == it.col_current) );
  }
  bool operator!=(const iterator& it) const { return !(*this == it); }


  // Here's the info we actually need to be an iterator
  // These need to be public so we convert from iterator to const_iterator
  typename containertype::iterator row_begin, row_end, row_current;
  typename containertype::value_type::TWOD_ITER_ col_current;
};

// The same thing again, but this time const.  :-(
template <class containertype>
class const_two_d_iterator {
 public:
  typedef const_two_d_iterator iterator;

  typedef STL_NAMESPACE::bidirectional_iterator_tag iterator_category;
  // apparently some versions of VC++ have trouble with two ::'s in a typename
  typedef typename containertype::value_type _tmp_vt;
  typedef typename _tmp_vt::value_type value_type;
  typedef typename _tmp_vt::difference_type difference_type;
  typedef typename _tmp_vt::const_reference reference;
  typedef typename _tmp_vt::const_pointer pointer;

  const_two_d_iterator(typename containertype::const_iterator begin,
                       typename containertype::const_iterator end,
                       typename containertype::const_iterator curr)
    : row_begin(begin), row_end(end), row_current(curr), col_current() {
    if ( curr != end ) {
      col_current = curr->TWOD_BEGIN_();
      advance_past_end();                 // in case cur->begin() == cur->end()
    }
  }
  const_two_d_iterator(typename containertype::const_iterator begin,
                       typename containertype::const_iterator end,
                       typename containertype::const_iterator curr,
                       typename containertype::value_type::TWOD_CONST_ITER_ col)
    : row_begin(begin), row_end(end), row_current(curr), col_current(col) {
    advance_past_end();                 // in case cur->begin() == cur->end()
  }
  const_two_d_iterator()
    : row_begin(), row_end(), row_current(), col_current() {
  }
  // Need this explicitly so we can convert normal iterators to const iterators
  const_two_d_iterator(const two_d_iterator<containertype>& it) :
    row_begin(it.row_begin), row_end(it.row_end), row_current(it.row_current),
    col_current(it.col_current) { }

  typename containertype::const_iterator row_begin, row_end, row_current;
  typename containertype::value_type::TWOD_CONST_ITER_ col_current;


  // EVERYTHING FROM HERE DOWN IS THE SAME AS THE NON-CONST ITERATOR
  reference operator*() const    { return *col_current; }
  pointer operator->() const     { return &(operator*()); }

  void advance_past_end() {          // used when col_current points to end()
    while ( col_current == row_current->TWOD_END_() ) {  // end of current row
      ++row_current;                                // go to beginning of next
      if ( row_current != row_end )                 // col is irrelevant at end
        col_current = row_current->TWOD_BEGIN_();
      else
        break;                                      // don't go past row_end
    }
  }
  iterator& operator++() {
    assert(row_current != row_end);                 // how to ++ from there?
    ++col_current;
    advance_past_end();                 // in case col_current is at end()
    return *this;
  }
  iterator& operator--() {
    while ( row_current == row_end ||
            col_current == row_current->TWOD_BEGIN_() ) {
      assert(row_current != row_begin);
      --row_current;
      col_current = row_current->TWOD_END_();             // this is 1 too far
    }
    --col_current;
    return *this;
  }
  iterator operator++(int)       { iterator tmp(*this); ++*this; return tmp; }
  iterator operator--(int)       { iterator tmp(*this); --*this; return tmp; }

  bool operator==(const iterator& it) const {
    return ( row_begin == it.row_begin &&
             row_end == it.row_end &&
             row_current == it.row_current &&
             (row_current == row_end || col_current == it.col_current) );
  }
  bool operator!=(const iterator& it) const { return !(*this == it); }
};

// We provide yet another version, to be as frugal with memory as
// possible.  This one frees each block of memory as it finishes
// iterating over it.  By the end, the entire table is freed.
// For understandable reasons, you can only iterate over it once,
// which is why it's an input iterator
template <class containertype>
class destructive_two_d_iterator {
 public:
  typedef destructive_two_d_iterator iterator;

  typedef STL_NAMESPACE::input_iterator_tag iterator_category;
  // apparently some versions of VC++ have trouble with two ::'s in a typename
  typedef typename containertype::value_type _tmp_vt;
  typedef typename _tmp_vt::value_type value_type;
  typedef typename _tmp_vt::difference_type difference_type;
  typedef typename _tmp_vt::reference reference;
  typedef typename _tmp_vt::pointer pointer;

  destructive_two_d_iterator(typename containertype::iterator begin,
                             typename containertype::iterator end,
                             typename containertype::iterator curr)
    : row_begin(begin), row_end(end), row_current(curr), col_current() {
    if ( curr != end ) {
      col_current = curr->TWOD_BEGIN_();
      advance_past_end();                 // in case cur->begin() == cur->end()
    }
  }
  destructive_two_d_iterator(typename containertype::iterator begin,
                             typename containertype::iterator end,
                             typename containertype::iterator curr,
                             typename containertype::value_type::TWOD_ITER_ col)
    : row_begin(begin), row_end(end), row_current(curr), col_current(col) {
    advance_past_end();                 // in case cur->begin() == cur->end()
  }
  destructive_two_d_iterator()
    : row_begin(), row_end(), row_current(), col_current() {
  }

  typename containertype::iterator row_begin, row_end, row_current;
  typename containertype::value_type::TWOD_ITER_ col_current;

  // This is the part that destroys
  void advance_past_end() {          // used when col_current points to end()
    while ( col_current == row_current->TWOD_END_() ) {  // end of current row
      row_current->clear();                         // the destructive part
      // It would be nice if we could decrement sparsetable->num_buckets here
      ++row_current;                                // go to beginning of next
      if ( row_current != row_end )                 // col is irrelevant at end
        col_current = row_current->TWOD_BEGIN_();
      else
        break;                                      // don't go past row_end
    }
  }

  // EVERYTHING FROM HERE DOWN IS THE SAME AS THE REGULAR ITERATOR
  reference operator*() const    { return *col_current; }
  pointer operator->() const     { return &(operator*()); }

  iterator& operator++() {
    assert(row_current != row_end);                 // how to ++ from there?
    ++col_current;
    advance_past_end();                 // in case col_current is at end()
    return *this;
  }
  iterator operator++(int)       { iterator tmp(*this); ++*this; return tmp; }

  bool operator==(const iterator& it) const {
    return ( row_begin == it.row_begin &&
             row_end == it.row_end &&
             row_current == it.row_current &&
             (row_current == row_end || col_current == it.col_current) );
  }
  bool operator!=(const iterator& it) const { return !(*this == it); }
};

#undef TWOD_BEGIN_
#undef TWOD_END_
#undef TWOD_ITER_
#undef TWOD_CONST_ITER_




// SPARSE-TABLE
// ------------
// The idea is that a table with (logically) t buckets is divided
// into t/M *groups* of M buckets each.  (M is a constant set in
// GROUP_SIZE for efficiency.)  Each group is stored sparsely.
// Thus, inserting into the table causes some array to grow, which is
// slow but still constant time.  Lookup involves doing a
// logical-position-to-sparse-position lookup, which is also slow but
// constant time.  The larger M is, the slower these operations are
// but the less overhead (slightly).
//
// To store the sparse array, we store a bitmap B, where B[i] = 1 iff
// bucket i is non-empty.  Then to look up bucket i we really look up
// array[# of 1s before i in B].  This is constant time for fixed M.
//
// Terminology: the position of an item in the overall table (from
// 1 .. t) is called its "location."  The logical position in a group
// (from 1 .. M ) is called its "position."  The actual location in
// the array (from 1 .. # of non-empty buckets in the group) is
// called its "offset."

// The weird mod in the offset is entirely to quiet compiler warnings
// as is the cast to int after doing the "x mod 256"
#define PUT_(take_from, offset)  do {                                          \
  if (putc(static_cast<int>(((take_from) >> ((offset) % (sizeof(take_from)*8)))\
                             % 256), fp)                                       \
      == EOF)                                                                  \
    return false;                                                              \
} while (0)

#define GET_(add_to, offset)  do {                                            \
  if ((x=getc(fp)) == EOF)                                                    \
    return false;                                                             \
  else                                                                        \
    add_to |= (static_cast<size_type>(x) << ((offset) % (sizeof(add_to)*8))); \
} while (0)

template <class T, u_int16_t GROUP_SIZE>
class sparsegroup {
 public:
  // Basic types
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef table_iterator<sparsegroup<T, GROUP_SIZE> > iterator;
  typedef const_table_iterator<sparsegroup<T, GROUP_SIZE> > const_iterator;
  typedef table_element_adaptor<sparsegroup<T, GROUP_SIZE> > element_adaptor;
  typedef value_type &reference;
  typedef const value_type &const_reference;
  typedef u_int16_t size_type;                  // max # of buckets
  typedef int16_t difference_type;
  typedef STL_NAMESPACE::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef STL_NAMESPACE::reverse_iterator<iterator> reverse_iterator;

  // These are our special iterators, that go over non-empty buckets in a
  // group.  These aren't const-only because you can change non-empty bcks.
  typedef pointer nonempty_iterator;
  typedef const_pointer const_nonempty_iterator;
  typedef STL_NAMESPACE::reverse_iterator<nonempty_iterator> reverse_nonempty_iterator;
  typedef STL_NAMESPACE::reverse_iterator<const_nonempty_iterator> const_reverse_nonempty_iterator;

  // Iterator functions
  iterator begin()                      { return iterator(this, 0); }
  const_iterator begin() const          { return const_iterator(this, 0); }
  iterator end()                        { return iterator(this, size()); }
  const_iterator end() const            { return const_iterator(this, size()); }
  reverse_iterator rbegin()             { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  reverse_iterator rend()               { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  // We'll have versions for our special non-empty iterator too
  nonempty_iterator nonempty_begin()             { return group; }
  const_nonempty_iterator nonempty_begin() const { return group; }
  nonempty_iterator nonempty_end()               { return group + num_buckets; }
  const_nonempty_iterator nonempty_end() const   { return group + num_buckets; }
  reverse_nonempty_iterator nonempty_rbegin() {
    return reverse_nonempty_iterator(nonempty_end());
  }
  const_reverse_nonempty_iterator nonempty_rbegin() const {
    return const_reverse_nonempty_iterator(nonempty_end());
  }
  reverse_nonempty_iterator nonempty_rend() {
    return reverse_nonempty_iterator(nonempty_begin());
  }
  const_reverse_nonempty_iterator nonempty_rend() const {
    return const_reverse_nonempty_iterator(nonempty_begin());
  }


  // This gives us the "default" value to return for an empty bucket.
  // We just use the default constructor on T, the template type
  const_reference default_value() const {
    static value_type defaultval = value_type();
    return defaultval;
  }


 private:
  // We need to do all this bit manipulation, of course.  ick
  static size_type charbit(size_type i)  { return i >> 3; }
  static size_type modbit(size_type i)   { return 1 << (i&7); }
  int bmtest(size_type i) const    { return bitmap[charbit(i)] & modbit(i); }
  void bmset(size_type i)          { bitmap[charbit(i)] |= modbit(i); }
  void bmclear(size_type i)        { bitmap[charbit(i)] &= ~modbit(i); }

  void* realloc_or_die(void* ptr, size_t num_bytes) {
    void* retval = realloc(ptr, num_bytes);
    if (retval == NULL) {
      // We really should use PRIuS here, but I don't want to have to add
      // a whole new configure option, with concomitant macro namespace
      // pollution, just to print this (unlikely) error message.  So I cast.
      fprintf(stderr, "FATAL ERROR: failed to allocate %lu bytes for ptr %p",
              static_cast<unsigned long>(num_bytes), ptr);
      exit(1);
    }
    return retval;
  }

  value_type* allocate_group(size_t n) {
    return static_cast<value_type*>(realloc_or_die(NULL,
                                                   n * sizeof(value_type)));
  }

  void free_group() {
    // Valid even for empty group, because NULL+0 is defined to be NULL
    value_type* end_it = group + num_buckets;
    for (value_type* p = group; p != end_it; ++p)
      p->~value_type();
    free(group);
    group = NULL;
  }

 public:                         // get_iter() in sparsetable needs it
  // We need a small function that tells us how many set bits there are
  // in positions 0..i-1 of the bitmap.  It uses a big table.
  // We make it static so templates don't allocate lots of these tables
  static size_type pos_to_offset(const unsigned char *bm, size_type pos) {
    // We could make these ints.  The tradeoff is size (eg does it overwhelm
    // the cache?) vs efficiency in referencing sub-word-sized array elements
    static const char bits_in[256] = {      // # of bits set in one char
      0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
      4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
    };
    size_type retval = 0;

    // [Note: condition pos > 8 is an optimization; convince yourself we
    // give exactly the same result as if we had pos >= 8 here instead.]
    for ( ; pos > 8; pos -= 8 )                    // bm[0..pos/8-1]
      retval += bits_in[*bm++];                    // chars we want *all* bits in
    return retval + bits_in[*bm & ((1 << pos)-1)]; // the char that includes pos
  }

  size_type pos_to_offset(size_type pos) const {   // not static but still const
    return pos_to_offset(bitmap, pos);
  }


 public:
  // Constructors -- default and copy -- and destructor
  sparsegroup() : group(0), num_buckets(0) { memset(bitmap, 0, sizeof(bitmap)); }
  sparsegroup(const sparsegroup& x) : group(0), num_buckets(x.num_buckets) {
    if ( num_buckets ) {
      group = allocate_group(x.num_buckets);
      uninitialized_copy(x.group, x.group + x.num_buckets, group);
    }
    memcpy(bitmap, x.bitmap, sizeof(bitmap));
  }
  ~sparsegroup() { free_group(); }

  // Operator= is just like the copy constructor, I guess
  // TODO(austern): Make this exception safe. Handle exceptions in value_type's
  // copy constructor.
  sparsegroup &operator=(const sparsegroup& x) {
    if ( &x == this ) return *this;                    // x = x
    if ( x.num_buckets == 0 ) {
      free_group();
    } else {
      value_type* p = allocate_group(x.num_buckets);
      uninitialized_copy(x.group, x.group + x.num_buckets, p);
      free_group();
      group = p;
    }
    memcpy(bitmap, x.bitmap, sizeof(bitmap));
    num_buckets = x.num_buckets;
    return *this;
  }

  // Many STL algorithms use swap instead of copy constructors
  void swap(sparsegroup& x) {
    STL_NAMESPACE::swap(group, x.group);
    for ( int i = 0; i < sizeof(bitmap) / sizeof(*bitmap); ++i )
      STL_NAMESPACE::swap(bitmap[i], x.bitmap[i]);  // swap not defined on arrays
    STL_NAMESPACE::swap(num_buckets, x.num_buckets);
  }

  // It's always nice to be able to clear a table without deallocating it
  void clear() {
    free_group();
    memset(bitmap, 0, sizeof(bitmap));
    num_buckets = 0;
  }

  // Functions that tell you about size.  Alas, these aren't so useful
  // because our table is always fixed size.
  size_type size() const           { return GROUP_SIZE; }
  size_type max_size() const       { return GROUP_SIZE; }
  bool empty() const               { return false; }
  // We also may want to know how many *used* buckets there are
  size_type num_nonempty() const   { return num_buckets; }


  // get()/set() are explicitly const/non-const.  You can use [] if
  // you want something that can be either (potentially more expensive).
  const_reference get(size_type i) const {
    if ( bmtest(i) )           // bucket i is occupied
      return group[pos_to_offset(bitmap, i)];
    else
      return default_value();  // return the default reference
  }

  // TODO(csilvers): make protected + friend
  reference mutating_get(size_type i) {    // fills bucket i before getting
    if ( !bmtest(i) )
      set(i, default_value());
    return group[pos_to_offset(bitmap, i)];
  }

  // Syntactic sugar.  It's easy to return a const reference.  To
  // return a non-const reference, we need to use the assigner adaptor.
  const_reference operator[](size_type i) const {
    return get(i);
  }

  element_adaptor operator[](size_type i) {
    return element_adaptor(this, i);
  }

 private:
  // Create space at group[offset], assuming value_type has trivial
  // copy constructor and destructor.  (Really, we want it to have
  // "trivial move", because that's what realloc and memmove both do.
  // But there's no way to capture that using type_traits, so we
  // pretend that move(x, y) is equivalent to "x.~T(); new(x) T(y);"
  // which is pretty much correct, if a bit conservative.)
  void set_aux(size_type offset, true_type) {
    group = (value_type *)
            realloc_or_die(group, sizeof(*group) * (num_buckets+1));
    // This is equivalent to memmove(), but faster on my Intel P4,
    // at least with gcc4.1 -O2 / glibc 2.3.6.
    for (size_type i = num_buckets; i > offset; --i)
      memcpy(group + i, group + i-1, sizeof(*group));
  }

  // Create space at group[offset], without special assumptions about value_type
  void set_aux(size_type offset, false_type) {
    // This is valid because 0 <= offset <= num_buckets
    value_type* p = allocate_group(num_buckets + 1);
    uninitialized_copy(group, group + offset, p);
    uninitialized_copy(group + offset, group + num_buckets, p + offset + 1);
    free_group();
    group = p;
  }

 public:
  // This returns a reference to the inserted item (which is a copy of val).
  // TODO(austern): Make this exception safe: handle exceptions from
  // value_type's copy constructor.
  reference set(size_type i, const_reference val) {
    size_type offset = pos_to_offset(bitmap, i);  // where we'll find (or insert)
    if ( bmtest(i) ) {
      // Delete the old value, which we're replacing with the new one
      group[offset].~value_type();
    } else {
      typedef integral_constant<bool,
          (has_trivial_copy<value_type>::value &&
           has_trivial_destructor<value_type>::value)>
          realloc_and_memmove_ok; // we pretend mv(x,y) == "x.~T(); new(x) T(y)"
      set_aux(offset, realloc_and_memmove_ok());
      ++num_buckets;
      bmset(i);
    }
    // This does the actual inserting.  Since we made the array using
    // malloc, we use "placement new" to just call the constructor.
    new(&group[offset]) value_type(val);
    return group[offset];
  }

  // We let you see if a bucket is non-empty without retrieving it
  bool test(size_type i) const {
    return bmtest(i) ? true : false;   // cast an int to a bool
  }
  bool test(iterator pos) const {
    return bmtest(pos.pos) ? true : false;
  }

 private:
  // Shrink the array, assuming value_type has trivial copy
  // constructor and destructor.  (Really, we want it to have "trivial
  // move", because that's what realloc and memmove both do.  But
  // there's no way to capture that using type_traits, so we pretend
  // that move(x, y) is equivalent to ""x.~T(); new(x) T(y);"
  // which is pretty much correct, if a bit conservative.)
  void erase_aux(size_type offset, true_type) {
    // This isn't technically necessary, since we know we have a
    // trivial destructor, but is a cheap way to get a bit more safety.
    group[offset].~value_type();
    // This is equivalent to memmove(), but faster on my Intel P4,
    // at lesat with gcc4.1 -O2 / glibc 2.3.6.
    assert(num_buckets > 0);
    for (size_type i = offset; i < num_buckets-1; ++i)
      memcpy(group + i, group + i+1, sizeof(*group));  // hopefully inlined!
    group = (value_type *)
            realloc_or_die(group, sizeof(*group) * (num_buckets-1));
  }

  // Shrink the array, without any special assumptions about value_type.
  void erase_aux(size_type offset, false_type) {
    // This is valid because 0 <= offset < num_buckets. Note the inequality.
    value_type* p = allocate_group(num_buckets - 1);
    uninitialized_copy(group, group + offset, p);
    uninitialized_copy(group + offset + 1, group + num_buckets, p + offset);
    free_group();
    group = p;
  }

 public:
  // This takes the specified elements out of the group.  This is
  // "undefining", rather than "clearing".
  // TODO(austern): Make this exception safe: handle exceptions from
  // value_type's copy constructor.
  void erase(size_type i) {
    if ( bmtest(i) ) {                           // trivial to erase empty bucket
      size_type offset = pos_to_offset(bitmap,i); // where we'll find (or insert)
      if ( num_buckets == 1 ) {
        free_group();
        group = NULL;
      } else {
        typedef integral_constant<bool,
            (has_trivial_copy<value_type>::value &&
             has_trivial_destructor<value_type>::value)>
            realloc_and_memmove_ok; // pretend mv(x,y) == "x.~T(); new(x) T(y)"
        erase_aux(offset, realloc_and_memmove_ok());
      }
      --num_buckets;
      bmclear(i);
    }
  }

  void erase(iterator pos) {
    erase(pos.pos);
  }

  void erase(iterator start_it, iterator end_it) {
    // This could be more efficient, but to do so we'd need to make
    // bmclear() clear a range of indices.  Doesn't seem worth it.
    for ( ; start_it != end_it; ++start_it )
      erase(start_it);
  }


  // I/O
  // We support reading and writing groups to disk.  We don't store
  // the actual array contents (which we don't know how to store),
  // just the bitmap and size.  Meant to be used with table I/O.
  // Returns true if all was ok
  bool write_metadata(FILE *fp) const {
    assert(sizeof(num_buckets) == 2);     // we explicitly set to u_int16_t
    PUT_(num_buckets, 8);
    PUT_(num_buckets, 0);
    if ( !fwrite(bitmap, sizeof(bitmap), 1, fp) )  return false;
    return true;
  }

  // Reading destroys the old group contents!  Returns true if all was ok
  bool read_metadata(FILE *fp) {
    clear();

    int x;          // the GET_ macro requires an 'int x' to be defined
    GET_(num_buckets, 8);
    GET_(num_buckets, 0);

    if ( !fread(bitmap, sizeof(bitmap), 1, fp) )  return false;

    // We'll allocate the space, but we won't fill it: it will be
    // left as uninitialized raw memory.
    group = allocate_group(num_buckets);
    return true;
  }

  // If your keys and values are simple enough, we can write them
  // to disk for you.  "simple enough" means POD and no pointers.
  // However, we don't try to normalize endianness
  bool write_nopointer_data(FILE *fp) const {
    for ( const_nonempty_iterator it = nonempty_begin();
          it != nonempty_end(); ++it ) {
      if ( !fwrite(&*it, sizeof(*it), 1, fp) )  return false;
    }
    return true;
  }

  // When reading, we have to override the potential const-ness of *it.
  // Again, only meaningful if value_type is a POD.
  bool read_nopointer_data(FILE *fp) {
    for ( nonempty_iterator it = nonempty_begin();
          it != nonempty_end(); ++it ) {
      if ( !fread(reinterpret_cast<void*>(&(*it)), sizeof(*it), 1, fp) )
        return false;
    }
    return true;
  }

  // Comparisons.  Note the comparisons are pretty arbitrary: we
  // compare values of the first index that isn't equal (using default
  // value for empty buckets).
  bool operator==(const sparsegroup& x) const {
    return ( num_buckets == x.num_buckets &&
             memcmp(bitmap, x.bitmap, sizeof(bitmap)) == 0 &&
             STL_NAMESPACE::equal(begin(), end(), x.begin()) ); // from algorithm
  }
  bool operator<(const sparsegroup& x) const {      // also from algorithm
    return STL_NAMESPACE::lexicographical_compare(begin(), end(), 
                                                  x.begin(), x.end());
  }
  bool operator!=(const sparsegroup& x) const { return !(*this == x); }
  bool operator<=(const sparsegroup& x) const { return !(x < *this); }
  bool operator>(const sparsegroup& x) const { return x < *this; }
  bool operator>=(const sparsegroup& x) const { return !(*this < x); }

 private:
  // The actual data
  value_type *group;                            // (small) array of T's
  unsigned char bitmap[(GROUP_SIZE-1)/8 + 1];   // fancy math is so we round up
  size_type num_buckets;                        // limits GROUP_SIZE to 64K
};

// We need a global swap as well
template <class T, u_int16_t GROUP_SIZE>
inline void swap(sparsegroup<T,GROUP_SIZE> &x, sparsegroup<T,GROUP_SIZE> &y) {
  x.swap(y);
}

// ---------------------------------------------------------------------------


template <class T, u_int16_t GROUP_SIZE = DEFAULT_SPARSEGROUP_SIZE>
class sparsetable {
 public:
  // Basic types
  typedef T value_type;                        // stolen from stl_vector.h
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef table_iterator<sparsetable<T, GROUP_SIZE> > iterator;
  typedef const_table_iterator<sparsetable<T, GROUP_SIZE> > const_iterator;
  typedef table_element_adaptor<sparsetable<T, GROUP_SIZE> > element_adaptor;
  typedef value_type &reference;
  typedef const value_type &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef STL_NAMESPACE::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef STL_NAMESPACE::reverse_iterator<iterator> reverse_iterator;

  // These are our special iterators, that go over non-empty buckets in a
  // table.  These aren't const only because you can change non-empty bcks.
  typedef two_d_iterator< vector< sparsegroup<value_type, GROUP_SIZE> > >
     nonempty_iterator;
  typedef const_two_d_iterator< vector< sparsegroup<value_type, GROUP_SIZE> > >
     const_nonempty_iterator;
  typedef STL_NAMESPACE::reverse_iterator<nonempty_iterator> reverse_nonempty_iterator;
  typedef STL_NAMESPACE::reverse_iterator<const_nonempty_iterator> const_reverse_nonempty_iterator;
  // Another special iterator: it frees memory as it iterates (used to resize)
  typedef destructive_two_d_iterator< vector< sparsegroup<value_type, GROUP_SIZE> > >
     destructive_iterator;

  // Iterator functions
  iterator begin()                      { return iterator(this, 0); }
  const_iterator begin() const          { return const_iterator(this, 0); }
  iterator end()                        { return iterator(this, size()); }
  const_iterator end() const            { return const_iterator(this, size()); }
  reverse_iterator rbegin()             { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  reverse_iterator rend()               { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  // Versions for our special non-empty iterator
  nonempty_iterator nonempty_begin()             {
    return nonempty_iterator(groups.begin(), groups.end(), groups.begin());
  }
  const_nonempty_iterator nonempty_begin() const {
    return const_nonempty_iterator(groups.begin(),groups.end(), groups.begin());
  }
  nonempty_iterator nonempty_end() {
    return nonempty_iterator(groups.begin(), groups.end(), groups.end());
  }
  const_nonempty_iterator nonempty_end() const {
    return const_nonempty_iterator(groups.begin(), groups.end(), groups.end());
  }
  reverse_nonempty_iterator nonempty_rbegin() {
    return reverse_nonempty_iterator(nonempty_end());
  }
  const_reverse_nonempty_iterator nonempty_rbegin() const {
    return const_reverse_nonempty_iterator(nonempty_end());
  }
  reverse_nonempty_iterator nonempty_rend() {
    return reverse_nonempty_iterator(nonempty_begin());
  }
  const_reverse_nonempty_iterator nonempty_rend() const {
    return const_reverse_nonempty_iterator(nonempty_begin());
  }
  destructive_iterator destructive_begin() {
    return destructive_iterator(groups.begin(), groups.end(), groups.begin());
  }
  destructive_iterator destructive_end() {
    return destructive_iterator(groups.begin(), groups.end(), groups.end());
  }

 private:
  typedef typename vector< sparsegroup<value_type, GROUP_SIZE> >::reference
    GroupsReference;
  typedef typename
    vector< sparsegroup<value_type, GROUP_SIZE> >::const_reference
    GroupsConstReference;
  typedef typename vector< sparsegroup<value_type, GROUP_SIZE> >::iterator
    GroupsIterator;
  typedef typename vector< sparsegroup<value_type, GROUP_SIZE> >::const_iterator
    GroupsConstIterator;

  // How to deal with the proper group
  static size_type num_groups(size_type num) {   // how many to hold num buckets
    return num == 0 ? 0 : ((num-1) / GROUP_SIZE) + 1;
  }

  u_int16_t pos_in_group(size_type i) const {
    return static_cast<u_int16_t>(i % GROUP_SIZE);
  }
  size_type group_num(size_type i) const {
    return i / GROUP_SIZE;
  }
  GroupsReference which_group(size_type i) {
    return groups[group_num(i)];
  }
  GroupsConstReference which_group(size_type i) const {
    return groups[group_num(i)];
  }

 public:
  // Constructors -- default, normal (when you specify size), and copy
  sparsetable(size_type sz = 0)
    : groups(num_groups(sz)), table_size(sz), num_buckets(0) { }
  // We'll can get away with using the default copy constructor,
  // and default destructor, and hence the default operator=.  Huzzah!

  // Many STL algorithms use swap instead of copy constructors
  void swap(sparsetable& x) {
    STL_NAMESPACE::swap(groups, x.groups);
    STL_NAMESPACE::swap(table_size, x.table_size);
    STL_NAMESPACE::swap(num_buckets, x.num_buckets);
  }

  // It's always nice to be able to clear a table without deallocating it
  void clear() {
    GroupsIterator group;
    for ( group = groups.begin(); group != groups.end(); ++group ) {
      group->clear();
    }
    num_buckets = 0;
  }

  // Functions that tell you about size.
  // NOTE: empty() is non-intuitive!  It does not tell you the number
  // of not-empty buckets (use num_nonempty() for that).  Instead
  // it says whether you've allocated any buckets or not.
  size_type size() const           { return table_size; }
  size_type max_size() const       { return size_type(-1); }
  bool empty() const               { return table_size == 0; }
  // We also may want to know how many *used* buckets there are
  size_type num_nonempty() const   { return num_buckets; }

  // OK, we'll let you resize one of these puppies
  void resize(size_type new_size) {
    groups.resize(num_groups(new_size));
    if ( new_size < table_size) {   // lower num_buckets, clear last group
      if ( pos_in_group(new_size) > 0 )     // need to clear inside last group
        groups.back().erase(groups.back().begin() + pos_in_group(new_size),
                            groups.back().end());
      num_buckets = 0;                       // refigure # of used buckets
      GroupsConstIterator group;
      for ( group = groups.begin(); group != groups.end(); ++group )
        num_buckets += group->num_nonempty();
    }
    table_size = new_size;
  }


  // We let you see if a bucket is non-empty without retrieving it
  bool test(size_type i) const {
    return which_group(i).test(pos_in_group(i));
  }
  bool test(iterator pos) const {
    return which_group(pos.pos).test(pos_in_group(pos.pos));
  }
  bool test(const_iterator pos) const {
    return which_group(pos.pos).test(pos_in_group(pos.pos));
  }

  // We only return const_references because it's really hard to
  // return something settable for empty buckets.  Use set() instead.
  const_reference get(size_type i) const {
    assert(i < table_size);
    return which_group(i).get(pos_in_group(i));
  }

  // TODO(csilvers): make protected + friend element_adaptor
  reference mutating_get(size_type i) {    // fills bucket i before getting
    assert(i < table_size);
    size_type old_numbuckets = which_group(i).num_nonempty();
    reference retval = which_group(i).mutating_get(pos_in_group(i));
    num_buckets += which_group(i).num_nonempty() - old_numbuckets;
    return retval;
  }

  // Syntactic sugar.  As in sparsegroup, the non-const version is harder
  const_reference operator[](size_type i) const {
    return get(i);
  }

  element_adaptor operator[](size_type i) {
    return element_adaptor(this, i);
  }

  // Needed for hashtables, gets as a nonempty_iterator.  Crashes for empty bcks
  const_nonempty_iterator get_iter(size_type i) const {
    assert(test(i));    // how can a nonempty_iterator point to an empty bucket?
    return const_nonempty_iterator(
      groups.begin(), groups.end(),
      groups.begin() + group_num(i),
      (groups[group_num(i)].nonempty_begin() +
       groups[group_num(i)].pos_to_offset(pos_in_group(i))));
  }
  // For nonempty we can return a non-const version
  nonempty_iterator get_iter(size_type i) {
    assert(test(i));    // how can a nonempty_iterator point to an empty bucket?
    return nonempty_iterator(
      groups.begin(), groups.end(),
      groups.begin() + group_num(i),
      (groups[group_num(i)].nonempty_begin() +
       groups[group_num(i)].pos_to_offset(pos_in_group(i))));
  }


  // This returns a reference to the inserted item (which is a copy of val)
  // The trick is to figure out whether we're replacing or inserting anew
  reference set(size_type i, const_reference val) {
    assert(i < table_size);
    size_type old_numbuckets = which_group(i).num_nonempty();
    reference retval = which_group(i).set(pos_in_group(i), val);
    num_buckets += which_group(i).num_nonempty() - old_numbuckets;
    return retval;
  }

  // This takes the specified elements out of the table.  This is
  // "undefining", rather than "clearing".
  void erase(size_type i) {
    assert(i < table_size);
    size_type old_numbuckets = which_group(i).num_nonempty();
    which_group(i).erase(pos_in_group(i));
    num_buckets += which_group(i).num_nonempty() - old_numbuckets;
  }

  void erase(iterator pos) {
    erase(pos.pos);
  }

  void erase(iterator start_it, iterator end_it) {
    // This could be more efficient, but then we'd need to figure
    // out if we spanned groups or not.  Doesn't seem worth it.
    for ( ; start_it != end_it; ++start_it )
      erase(start_it);
  }


  // We support reading and writing tables to disk.  We don't store
  // the actual array contents (which we don't know how to store),
  // just the groups and sizes.  Returns true if all went ok.

 private:
  // Every time the disk format changes, this should probably change too
  static const unsigned long MAGIC_NUMBER = 0x24687531;

  // Old versions of this code write all data in 32 bits.  We need to
  // support these files as well as having support for 64-bit systems.
  // So we use the following encoding scheme: for values < 2^32-1, we
  // store in 4 bytes in big-endian order.  For values > 2^32, we
  // store 0xFFFFFFF followed by 8 bytes in big-endian order.  This
  // causes us to mis-read old-version code that stores exactly
  // 0xFFFFFFF, but I don't think that is likely to have happened for
  // these particular values.
  static bool write_32_or_64(FILE* fp, size_type value) {
    if ( value < 0xFFFFFFFFULL ) {        // fits in 4 bytes
      PUT_(value, 24);
      PUT_(value, 16);
      PUT_(value, 8);
      PUT_(value, 0);
    } else if ( value == 0xFFFFFFFFUL ) {   // special case in 32bit systems
      PUT_(0xFF, 0); PUT_(0xFF, 0); PUT_(0xFF, 0); PUT_(0xFF, 0);  // marker
      PUT_(0, 0); PUT_(0, 0); PUT_(0, 0); PUT_(0, 0);
      PUT_(0xFF, 0); PUT_(0xFF, 0); PUT_(0xFF, 0); PUT_(0xFF, 0);
    } else {
      PUT_(0xFF, 0); PUT_(0xFF, 0); PUT_(0xFF, 0); PUT_(0xFF, 0);  // marker
      PUT_(value, 56);
      PUT_(value, 48);
      PUT_(value, 40);
      PUT_(value, 32);
      PUT_(value, 24);
      PUT_(value, 16);
      PUT_(value, 8);
      PUT_(value, 0);
    }
    return true;
  }

  static bool read_32_or_64(FILE* fp, size_type *value) {  // reads into value
    size_type first4 = 0;
    int x;
    GET_(first4, 24);
    GET_(first4, 16);
    GET_(first4, 8);
    GET_(first4, 0);
    if ( first4 < 0xFFFFFFFFULL ) {
      *value = first4;
    } else {
      GET_(*value, 56);
      GET_(*value, 48);
      GET_(*value, 40);
      GET_(*value, 32);
      GET_(*value, 24);
      GET_(*value, 16);
      GET_(*value, 8);
      GET_(*value, 0);
    }
    return true;
  }

 public:
  bool write_metadata(FILE *fp) const {
    if ( !write_32_or_64(fp, MAGIC_NUMBER) )  return false;
    if ( !write_32_or_64(fp, table_size) )  return false;
    if ( !write_32_or_64(fp, num_buckets) )  return false;

    GroupsConstIterator group;
    for ( group = groups.begin(); group != groups.end(); ++group )
      if ( group->write_metadata(fp) == false )  return false;
    return true;
  }

  // Reading destroys the old table contents!  Returns true if read ok.
  bool read_metadata(FILE *fp) {
    size_type magic_read = 0;
    if ( !read_32_or_64(fp, &magic_read) )  return false;
    if ( magic_read != MAGIC_NUMBER ) {
      clear();                        // just to be consistent
      return false;
    }

    if ( !read_32_or_64(fp, &table_size) )  return false;
    if ( !read_32_or_64(fp, &num_buckets) )  return false;

    resize(table_size);                            // so the vector's sized ok
    GroupsIterator group;
    for ( group = groups.begin(); group != groups.end(); ++group )
      if ( group->read_metadata(fp) == false )  return false;
    return true;
  }

  // This code is identical to that for SparseGroup
  // If your keys and values are simple enough, we can write them
  // to disk for you.  "simple enough" means no pointers.
  // However, we don't try to normalize endianness
  bool write_nopointer_data(FILE *fp) const {
    for ( const_nonempty_iterator it = nonempty_begin();
          it != nonempty_end(); ++it ) {
      if ( !fwrite(&*it, sizeof(*it), 1, fp) )  return false;
    }
    return true;
  }

  // When reading, we have to override the potential const-ness of *it
  bool read_nopointer_data(FILE *fp) {
    for ( nonempty_iterator it = nonempty_begin();
          it != nonempty_end(); ++it ) {
      if ( !fread(reinterpret_cast<void*>(&(*it)), sizeof(*it), 1, fp) )
        return false;
    }
    return true;
  }

  // Comparisons.  Note the comparisons are pretty arbitrary: we
  // compare values of the first index that isn't equal (using default
  // value for empty buckets).
  bool operator==(const sparsetable& x) const {
    return ( table_size == x.table_size &&
             num_buckets == x.num_buckets &&
             groups == x.groups );
  }
  bool operator<(const sparsetable& x) const {      // also from algobase.h
    return STL_NAMESPACE::lexicographical_compare(begin(), end(), 
                                                  x.begin(), x.end());
  }
  bool operator!=(const sparsetable& x) const { return !(*this == x); }
  bool operator<=(const sparsetable& x) const { return !(x < *this); }
  bool operator>(const sparsetable& x) const { return x < *this; }
  bool operator>=(const sparsetable& x) const { return !(*this < x); }


 private:
  // The actual data
  vector< sparsegroup<value_type, GROUP_SIZE> > groups;  // our list of groups
  size_type table_size;                         // how many buckets they want
  size_type num_buckets;                        // number of non-empty buckets
};

// We need a global swap as well
template <class T, u_int16_t GROUP_SIZE>
inline void swap(sparsetable<T,GROUP_SIZE> &x, sparsetable<T,GROUP_SIZE> &y) {
  x.swap(y);
}

#undef GET_
#undef PUT_

_END_GOOGLE_NAMESPACE_

#endif
