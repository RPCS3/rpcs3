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
// This is just a very thin wrapper over densehashtable.h, just
// like sgi stl's stl_hash_set is a very thin wrapper over
// stl_hashtable.  The major thing we define is operator[], because
// we have a concept of a data_type which stl_hashtable doesn't
// (it only has a key and a value).
//
// This is more different from dense_hash_map than you might think,
// because all iterators for sets are const (you obviously can't
// change the key, and for sets there is no value).
//
// NOTE: this is exactly like sparse_hash_set.h, with the word
// "sparse" replaced by "dense", except for the addition of
// set_empty_key().
//
//   YOU MUST CALL SET_EMPTY_KEY() IMMEDIATELY AFTER CONSTRUCTION.
//
// Otherwise your program will die in mysterious ways.
//
// In other respects, we adhere mostly to the STL semantics for
// hash-set.  One important exception is that insert() invalidates
// iterators entirely.  On the plus side, though, erase() doesn't
// invalidate iterators at all, or even change the ordering of elements.
//
// Here are a few "power user" tips:
//
//    1) set_deleted_key():
//         If you want to use erase() you must call set_deleted_key(),
//         in addition to set_empty_key(), after construction.
//         The deleted and empty keys must differ.
//
//    2) resize(0):
//         When an item is deleted, its memory isn't freed right
//         away.  This allows you to iterate over a hashtable,
//         and call erase(), without invalidating the iterator.
//         To force the memory to be freed, call resize(0).
//
//    3) set_resizing_parameters(0.0, 0.8):
//         Setting the shrink_resize_percent to 0.0 guarantees
//         that the hash table will never shrink.
//
// Guide to what kind of hash_set to use:
//   (1) dense_hash_set: fastest, uses the most memory
//   (2) sparse_hash_set: slowest, uses the least memory
//   (3) hash_set (STL): in the middle
// Typically I use sparse_hash_set when I care about space and/or when
// I need to save the hashtable on disk.  I use hash_set otherwise.  I
// don't personally use dense_hash_set ever; the only use of
// dense_hash_set I know of is to work around malloc() bugs in some
// systems (dense_hash_set has a particularly simple allocation scheme).
//
// - dense_hash_set has, typically, a factor of 2 memory overhead (if your
//   data takes up X bytes, the hash_set uses X more bytes in overhead).
// - sparse_hash_set has about 2 bits overhead per entry.
// - sparse_hash_map can be 3-7 times slower than the others for lookup and,
//   especially, inserts.  See time_hash_map.cc for details.
//
// See /usr/(local/)?doc/sparsehash-0.1/dense_hash_set.html
// for information about how to use this class.

#ifndef _DENSE_HASH_SET_H_
#define _DENSE_HASH_SET_H_

#include <google/sparsehash/sparseconfig.h>
#include <stdio.h>                   // for FILE * in read()/write()
#include <algorithm>                 // for the default template args
#include <functional>                // for equal_to
#include <memory>                    // for alloc<>
#include <utility>                   // for pair<>
#include HASH_FUN_H                  // defined in config.h
#include <google/sparsehash/densehashtable.h>


_START_GOOGLE_NAMESPACE_

using STL_NAMESPACE::pair;

template <class Value,
          class HashFcn = SPARSEHASH_HASH<Value>,  // defined in sparseconfig.h
          class EqualKey = STL_NAMESPACE::equal_to<Value>, 
          class Alloc = STL_NAMESPACE::allocator<Value> >
class dense_hash_set {
 private:
  // Apparently identity is not stl-standard, so we define our own
  struct Identity {
    Value& operator()(Value& v) const { return v; }
    const Value& operator()(const Value& v) const { return v; }
  };

  // The actual data
  typedef dense_hashtable<Value, Value, HashFcn, Identity, EqualKey, Alloc> ht;
  ht rep;

 public:
  typedef typename ht::key_type key_type;
  typedef typename ht::value_type value_type;
  typedef typename ht::hasher hasher;
  typedef typename ht::key_equal key_equal;

  typedef typename ht::size_type size_type;
  typedef typename ht::difference_type difference_type;
  typedef typename ht::const_pointer pointer;
  typedef typename ht::const_pointer const_pointer;
  typedef typename ht::const_reference reference;
  typedef typename ht::const_reference const_reference;

  typedef typename ht::const_iterator iterator;
  typedef typename ht::const_iterator const_iterator;


  // Iterator functions -- recall all iterators are const
  iterator begin() const              { return rep.begin(); }
  iterator end() const                { return rep.end(); }


  // Accessor functions
  hasher hash_funct() const { return rep.hash_funct(); }
  key_equal key_eq() const  { return rep.key_eq(); }


  // Constructors
  explicit dense_hash_set(size_type expected_max_items_in_table = 0,
                          const hasher& hf = hasher(),
                          const key_equal& eql = key_equal())
    : rep(expected_max_items_in_table, hf, eql) { }

  template <class InputIterator>
  dense_hash_set(InputIterator f, InputIterator l,
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
  // This clears the hash set without resizing it down to the minimum
  // bucket count, but rather keeps the number of buckets constant
  void clear_no_resize()              { rep.clear_no_resize(); }
  void swap(dense_hash_set& hs)       { rep.swap(hs.rep); }


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
  iterator find(const key_type& key) const           { return rep.find(key); }

  size_type count(const key_type& key) const         { return rep.count(key); }

  pair<iterator, iterator> equal_range(const key_type& key) const {
    return rep.equal_range(key);
  }

  // Insertion routines
  pair<iterator, bool> insert(const value_type& obj) {
    pair<typename ht::iterator, bool> p = rep.insert(obj);
    return pair<iterator, bool>(p.first, p.second);   // const to non-const
  }
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l)      { rep.insert(f, l); }
  void insert(const_iterator f, const_iterator l)    { rep.insert(f, l); }
  // required for std::insert_iterator; the passed-in iterator is ignored
  iterator insert(iterator, const value_type& obj)   { return insert(obj).first; }


  // Deletion and empty routines
  // THESE ARE NON-STANDARD!  I make you specify an "impossible" key
  // value to identify deleted and empty buckets.  You can change the
  // deleted key as time goes on, or get rid of it entirely to be insert-only.
  void set_empty_key(const key_type& key)     { rep.set_empty_key(key); }
  void set_deleted_key(const key_type& key)   { rep.set_deleted_key(key); }
  void clear_deleted_key()                    { rep.clear_deleted_key(); }

  // These are standard
  size_type erase(const key_type& key)               { return rep.erase(key); }
  void erase(iterator it)                            { rep.erase(it); }
  void erase(iterator f, iterator l)                 { rep.erase(f, l); }


  // Comparison
  bool operator==(const dense_hash_set& hs) const    { return rep == hs.rep; }
  bool operator!=(const dense_hash_set& hs) const    { return rep != hs.rep; }


  // I/O -- this is an add-on for writing metainformation to disk
  bool write_metadata(FILE *fp)       { return rep.write_metadata(fp); }
  bool read_metadata(FILE *fp)        { return rep.read_metadata(fp); }
  bool write_nopointer_data(FILE *fp) { return rep.write_nopointer_data(fp); }
  bool read_nopointer_data(FILE *fp)  { return rep.read_nopointer_data(fp); }
};

template <class Val, class HashFcn, class EqualKey, class Alloc>
inline void swap(dense_hash_set<Val, HashFcn, EqualKey, Alloc>& hs1,
                 dense_hash_set<Val, HashFcn, EqualKey, Alloc>& hs2) {
  hs1.swap(hs2);
}

_END_GOOGLE_NAMESPACE_

#endif /* _DENSE_HASH_SET_H_ */
