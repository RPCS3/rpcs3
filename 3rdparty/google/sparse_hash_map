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
// This is just a very thin wrapper over sparsehashtable.h, just
// like sgi stl's stl_hash_map is a very thin wrapper over
// stl_hashtable.  The major thing we define is operator[], because
// we have a concept of a data_type which stl_hashtable doesn't
// (it only has a key and a value).
//
// We adhere mostly to the STL semantics for hash-map.  One important
// exception is that insert() invalidates iterators entirely.  On the
// plus side, though, delete() doesn't invalidate iterators at all, or
// even change the ordering of elements.
//
// Here are a few "power user" tips:
//
//    1) set_deleted_key():
//         Unlike STL's hash_map, if you want to use erase() you
//         must call set_deleted_key() after construction.
//
//    2) resize(0):
//         When an item is deleted, its memory isn't freed right
//         away.  This is what allows you to iterate over a hashtable
//         and call erase() without invalidating the iterator.
//         To force the memory to be freed, call resize(0).
//
//    3) set_resizing_parameters(0.0, 0.8):
//         Setting the shrink_resize_percent to 0.0 guarantees
//         that the hash table will never shrink.
//
// Guide to what kind of hash_map to use:
//   (1) dense_hash_map: fastest, uses the most memory
//   (2) sparse_hash_map: slowest, uses the least memory
//   (3) hash_map (STL): in the middle
// Typically I use sparse_hash_map when I care about space and/or when
// I need to save the hashtable on disk.  I use hash_map otherwise.  I
// don't personally use dense_hash_map ever; the only use of
// dense_hash_map I know of is to work around malloc() bugs in some
// systems (dense_hash_map has a particularly simple allocation scheme).
//
// - dense_hash_map has, typically, a factor of 2 memory overhead (if your
//   data takes up X bytes, the hash_map uses X more bytes in overhead).
// - sparse_hash_map has about 2 bits overhead per entry.
// - sparse_hash_map can be 3-7 times slower than the others for lookup and,
//   especially, inserts.  See time_hash_map.cc for details.
//
// See /usr/(local/)?doc/sparsehash-0.1/sparse_hash_map.html
// for information about how to use this class.

#ifndef _SPARSE_HASH_MAP_H_
#define _SPARSE_HASH_MAP_H_

#include <google/sparsehash/sparseconfig.h>
#include <stdio.h>                   // for FILE * in read()/write()
#include <algorithm>                 // for the default template args
#include <functional>                // for equal_to
#include <memory>                    // for alloc<>
#include <utility>                   // for pair<>
#include HASH_FUN_H                  // defined in config.h
#include <google/sparsehash/sparsehashtable.h>


_START_GOOGLE_NAMESPACE_

using STL_NAMESPACE::pair;

template <class Key, class T,
          class HashFcn = SPARSEHASH_HASH<Key>,   // defined in sparseconfig.h
          class EqualKey = STL_NAMESPACE::equal_to<Key>,
          class Alloc = STL_NAMESPACE::allocator<T> >
class sparse_hash_map {
 private:
  // Apparently select1st is not stl-standard, so we define our own
  struct SelectKey {
    const Key& operator()(const pair<const Key, T>& p) const {
      return p.first;
    }
  };

  // The actual data
  typedef sparse_hashtable<pair<const Key, T>, Key, HashFcn,
                           SelectKey, EqualKey, Alloc> ht;
  ht rep;

 public:
  typedef typename ht::key_type key_type;
  typedef T data_type;
  typedef T mapped_type;
  typedef typename ht::value_type value_type;
  typedef typename ht::hasher hasher;
  typedef typename ht::key_equal key_equal;

  typedef typename ht::size_type size_type;
  typedef typename ht::difference_type difference_type;
  typedef typename ht::pointer pointer;
  typedef typename ht::const_pointer const_pointer;
  typedef typename ht::reference reference;
  typedef typename ht::const_reference const_reference;

  typedef typename ht::iterator iterator;
  typedef typename ht::const_iterator const_iterator;

  // Iterator functions
  iterator begin()                    { return rep.begin(); }
  iterator end()                      { return rep.end(); }
  const_iterator begin() const        { return rep.begin(); }
  const_iterator end() const          { return rep.end(); }


  // Accessor functions
  hasher hash_funct() const { return rep.hash_funct(); }
  key_equal key_eq() const  { return rep.key_eq(); }


  // Constructors
  explicit sparse_hash_map(size_type expected_max_items_in_table = 0,
                           const hasher& hf = hasher(),
                           const key_equal& eql = key_equal())
    : rep(expected_max_items_in_table, hf, eql) { }

  template <class InputIterator>
  sparse_hash_map(InputIterator f, InputIterator l,
                  size_type expected_max_items_in_table = 0,
                  const hasher& hf = hasher(),
                  const key_equal& eql = key_equal())
    : rep(expected_max_items_in_table, hf, eql) {
    rep.insert(f, l);
  }
  // We use the default copy constructor
  // We use the default operator=()
  // We use the default destructor

  void clear()                        { rep.clear(); }
  void swap(sparse_hash_map& hs)      { rep.swap(hs.rep); }


  // Functions concerning size
  size_type size() const              { return rep.size(); }
  size_type max_size() const          { return rep.max_size(); }
  bool empty() const                  { return rep.empty(); }
  size_type bucket_count() const      { return rep.bucket_count(); }
  size_type max_bucket_count() const  { return rep.max_bucket_count(); }

  void resize(size_type hint)         { rep.resize(hint); }

  void set_resizing_parameters(float shrink, float grow) {
    return rep.set_resizing_parameters(shrink, grow);
  }

  // Lookup routines
  iterator find(const key_type& key)                 { return rep.find(key); }
  const_iterator find(const key_type& key) const     { return rep.find(key); }

  data_type& operator[](const key_type& key) {       // This is our value-add!
    iterator it = find(key);
    if (it != end()) {
      return it->second;
    } else {
      return insert(value_type(key, data_type())).first->second;
    }
  }

  size_type count(const key_type& key) const         { return rep.count(key); }

  pair<iterator, iterator> equal_range(const key_type& key) {
    return rep.equal_range(key);
  }
  pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
    return rep.equal_range(key);
  }

  // Insertion routines
  pair<iterator, bool> insert(const value_type& obj) { return rep.insert(obj); }
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l)      { rep.insert(f, l); }
  void insert(const_iterator f, const_iterator l)    { rep.insert(f, l); }
  // required for std::insert_iterator; the passed-in iterator is ignored
  iterator insert(iterator, const value_type& obj)   { return insert(obj).first; }


  // Deletion routines
  // THESE ARE NON-STANDARD!  I make you specify an "impossible" key
  // value to identify deleted buckets.  You can change the key as
  // time goes on, or get rid of it entirely to be insert-only.
  void set_deleted_key(const key_type& key)   {
    rep.set_deleted_key(value_type(key, data_type()));  // rep wants a value
  }
  void clear_deleted_key()                    { rep.clear_deleted_key(); }

  // These are standard
  size_type erase(const key_type& key)               { return rep.erase(key); }
  void erase(iterator it)                            { rep.erase(it); }
  void erase(iterator f, iterator l)                 { rep.erase(f, l); }


  // Comparison
  bool operator==(const sparse_hash_map& hs) const   { return rep == hs.rep; }
  bool operator!=(const sparse_hash_map& hs) const   { return rep != hs.rep; }


  // I/O -- this is an add-on for writing metainformation to disk
  bool write_metadata(FILE *fp)       { return rep.write_metadata(fp); }
  bool read_metadata(FILE *fp)        { return rep.read_metadata(fp); }
  bool write_nopointer_data(FILE *fp) { return rep.write_nopointer_data(fp); }
  bool read_nopointer_data(FILE *fp)  { return rep.read_nopointer_data(fp); }
};

// We need a global swap as well
template <class Key, class T, class HashFcn, class EqualKey, class Alloc>
inline void swap(sparse_hash_map<Key, T, HashFcn, EqualKey, Alloc>& hm1,
                 sparse_hash_map<Key, T, HashFcn, EqualKey, Alloc>& hm2) {
  hm1.swap(hm2);
}

_END_GOOGLE_NAMESPACE_

#endif /* _SPARSE_HASH_MAP_H_ */
