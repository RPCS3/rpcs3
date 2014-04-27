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
// A sparse hashtable is a particular implementation of
// a hashtable: one that is meant to minimize memory use.
// It does this by using a *sparse table* (cf sparsetable.h),
// which uses between 1 and 2 bits to store empty buckets
// (we may need another bit for hashtables that support deletion).
//
// When empty buckets are so cheap, an appealing hashtable
// implementation is internal probing, in which the hashtable
// is a single table, and collisions are resolved by trying
// to insert again in another bucket.  The most cache-efficient
// internal probing schemes are linear probing (which suffers,
// alas, from clumping) and quadratic probing, which is what
// we implement by default.
//
// Deleted buckets are a bit of a pain.  We have to somehow mark
// deleted buckets (the probing must distinguish them from empty
// buckets).  The most principled way is to have another bitmap,
// but that's annoying and takes up space.  Instead we let the
// user specify an "impossible" key.  We set deleted buckets
// to have the impossible key.
//
// Note it is possible to change the value of the delete key
// on the fly; you can even remove it, though after that point
// the hashtable is insert_only until you set it again.
//
// You probably shouldn't use this code directly.  Use
// <google/sparse_hash_table> or <google/sparse_hash_set> instead.
//
// You can modify the following, below:
// HT_OCCUPANCY_FLT            -- how full before we double size
// HT_EMPTY_FLT                -- how empty before we halve size
// HT_MIN_BUCKETS              -- smallest bucket size
// HT_DEFAULT_STARTING_BUCKETS -- default bucket size at construct-time
//
// You can also change enlarge_resize_percent (which defaults to
// HT_OCCUPANCY_FLT), and shrink_resize_percent (which defaults to
// HT_EMPTY_FLT) with set_resizing_parameters().
//
// How to decide what values to use?
// shrink_resize_percent's default of .4 * OCCUPANCY_FLT, is probably good.
// HT_MIN_BUCKETS is probably unnecessary since you can specify
// (indirectly) the starting number of buckets at construct-time.
// For enlarge_resize_percent, you can use this chart to try to trade-off
// expected lookup time to the space taken up.  By default, this
// code uses quadratic probing, though you can change it to linear
// via _JUMP below if you really want to.
//
// From http://www.augustana.ca/~mohrj/courses/1999.fall/csc210/lecture_notes/hashing.html
// NUMBER OF PROBES / LOOKUP       Successful            Unsuccessful
// Quadratic collision resolution   1 - ln(1-L) - L/2    1/(1-L) - L - ln(1-L)
// Linear collision resolution     [1+1/(1-L)]/2         [1+1/(1-L)2]/2
//
// -- enlarge_resize_percent --         0.10  0.50  0.60  0.75  0.80  0.90  0.99
// QUADRATIC COLLISION RES.
//    probes/successful lookup    1.05  1.44  1.62  2.01  2.21  2.85  5.11
//    probes/unsuccessful lookup  1.11  2.19  2.82  4.64  5.81  11.4  103.6
// LINEAR COLLISION RES.
//    probes/successful lookup    1.06  1.5   1.75  2.5   3.0   5.5   50.5
//    probes/unsuccessful lookup  1.12  2.5   3.6   8.5   13.0  50.0  5000.0
//
// The value type is required to be copy constructible and default
// constructible, but it need not be (and commonly isn't) assignable.

#ifndef _SPARSEHASHTABLE_H_
#define _SPARSEHASHTABLE_H_

#ifndef SPARSEHASH_STAT_UPDATE
#define SPARSEHASH_STAT_UPDATE(x) ((void) 0)
#endif

// The probing method
// Linear probing
// #define JUMP_(key, num_probes)    ( 1 )
// Quadratic-ish probing
#define JUMP_(key, num_probes)    ( num_probes )


// Hashtable class, used to implement the hashed associative containers
// hash_set and hash_map.

#include <google/sparsehash/sparseconfig.h>
#include <assert.h>
#include <algorithm>              // For swap(), eg
#include <iterator>               // for facts about iterator tags
#include <utility>                // for pair<>
#include <google/sparsetable>     // Since that's basically what we are

_START_GOOGLE_NAMESPACE_

using STL_NAMESPACE::pair;

// Alloc is completely ignored.  It is present as a template parameter only
// for the sake of being compatible with the old SGI hashtable interface.
// TODO(csilvers): is that the right thing to do?

template <class Value, class Key, class HashFcn,
          class ExtractKey, class EqualKey, class Alloc>
class sparse_hashtable;

template <class V, class K, class HF, class ExK, class EqK, class A>
struct sparse_hashtable_iterator;

template <class V, class K, class HF, class ExK, class EqK, class A>
struct sparse_hashtable_const_iterator;

// As far as iterating, we're basically just a sparsetable
// that skips over deleted elements.
template <class V, class K, class HF, class ExK, class EqK, class A>
struct sparse_hashtable_iterator {
 public:
  typedef sparse_hashtable_iterator<V,K,HF,ExK,EqK,A>       iterator;
  typedef sparse_hashtable_const_iterator<V,K,HF,ExK,EqK,A> const_iterator;
  typedef typename sparsetable<V>::nonempty_iterator        st_iterator;

  typedef STL_NAMESPACE::forward_iterator_tag iterator_category;
  typedef V value_type;
  typedef ptrdiff_t difference_type;
  typedef size_t size_type;
  typedef V& reference;                // Value
  typedef V* pointer;

  // "Real" constructor and default constructor
  sparse_hashtable_iterator(const sparse_hashtable<V,K,HF,ExK,EqK,A> *h,
                            st_iterator it, st_iterator it_end)
    : ht(h), pos(it), end(it_end)   { advance_past_deleted(); }
  sparse_hashtable_iterator() { }      // not ever used internally
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // Happy dereferencer
  reference operator*() const { return *pos; }
  pointer operator->() const { return &(operator*()); }

  // Arithmetic.  The only hard part is making sure that
  // we're not on a marked-deleted array element
  void advance_past_deleted() {
    while ( pos != end && ht->test_deleted(*this) )
      ++pos;
  }
  iterator& operator++()   {
    assert(pos != end); ++pos; advance_past_deleted(); return *this;
  }
  iterator operator++(int) { iterator tmp(*this); ++*this; return tmp; }

  // Comparison.
  bool operator==(const iterator& it) const { return pos == it.pos; }
  bool operator!=(const iterator& it) const { return pos != it.pos; }


  // The actual data
  const sparse_hashtable<V,K,HF,ExK,EqK,A> *ht;
  st_iterator pos, end;
};

// Now do it all again, but with const-ness!
template <class V, class K, class HF, class ExK, class EqK, class A>
struct sparse_hashtable_const_iterator {
 public:
  typedef sparse_hashtable_iterator<V,K,HF,ExK,EqK,A>       iterator;
  typedef sparse_hashtable_const_iterator<V,K,HF,ExK,EqK,A> const_iterator;
  typedef typename sparsetable<V>::const_nonempty_iterator  st_iterator;

  typedef STL_NAMESPACE::forward_iterator_tag iterator_category;
  typedef V value_type;
  typedef ptrdiff_t difference_type;
  typedef size_t size_type;
  typedef const V& reference;                // Value
  typedef const V* pointer;

  // "Real" constructor and default constructor
  sparse_hashtable_const_iterator(const sparse_hashtable<V,K,HF,ExK,EqK,A> *h,
                                  st_iterator it, st_iterator it_end)
    : ht(h), pos(it), end(it_end)   { advance_past_deleted(); }
  // This lets us convert regular iterators to const iterators
  sparse_hashtable_const_iterator() { }      // never used internally
  sparse_hashtable_const_iterator(const iterator &it)
    : ht(it.ht), pos(it.pos), end(it.end) { }
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // Happy dereferencer
  reference operator*() const { return *pos; }
  pointer operator->() const { return &(operator*()); }

  // Arithmetic.  The only hard part is making sure that
  // we're not on a marked-deleted array element
  void advance_past_deleted() {
    while ( pos != end && ht->test_deleted(*this) )
      ++pos;
  }
  const_iterator& operator++() {
    assert(pos != end); ++pos; advance_past_deleted(); return *this;
  }
  const_iterator operator++(int) { const_iterator tmp(*this); ++*this; return tmp; }

  // Comparison.
  bool operator==(const const_iterator& it) const { return pos == it.pos; }
  bool operator!=(const const_iterator& it) const { return pos != it.pos; }


  // The actual data
  const sparse_hashtable<V,K,HF,ExK,EqK,A> *ht;
  st_iterator pos, end;
};

// And once again, but this time freeing up memory as we iterate
template <class V, class K, class HF, class ExK, class EqK, class A>
struct sparse_hashtable_destructive_iterator {
 public:
  typedef sparse_hashtable_destructive_iterator<V,K,HF,ExK,EqK,A> iterator;
  typedef typename sparsetable<V>::destructive_iterator     st_iterator;

  typedef STL_NAMESPACE::forward_iterator_tag iterator_category;
  typedef V value_type;
  typedef ptrdiff_t difference_type;
  typedef size_t size_type;
  typedef V& reference;                // Value
  typedef V* pointer;

  // "Real" constructor and default constructor
  sparse_hashtable_destructive_iterator(const
                                        sparse_hashtable<V,K,HF,ExK,EqK,A> *h,
                                        st_iterator it, st_iterator it_end)
    : ht(h), pos(it), end(it_end)   { advance_past_deleted(); }
  sparse_hashtable_destructive_iterator() { }          // never used internally
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // Happy dereferencer
  reference operator*() const { return *pos; }
  pointer operator->() const { return &(operator*()); }

  // Arithmetic.  The only hard part is making sure that
  // we're not on a marked-deleted array element
  void advance_past_deleted() {
    while ( pos != end && ht->test_deleted(*this) )
      ++pos;
  }
  iterator& operator++()   {
    assert(pos != end); ++pos; advance_past_deleted(); return *this;
  }
  iterator operator++(int) { iterator tmp(*this); ++*this; return tmp; }

  // Comparison.
  bool operator==(const iterator& it) const { return pos == it.pos; }
  bool operator!=(const iterator& it) const { return pos != it.pos; }


  // The actual data
  const sparse_hashtable<V,K,HF,ExK,EqK,A> *ht;
  st_iterator pos, end;
};


template <class Value, class Key, class HashFcn,
          class ExtractKey, class EqualKey, class Alloc>
class sparse_hashtable {
 public:
  typedef Key key_type;
  typedef Value value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;

  typedef size_t            size_type;
  typedef ptrdiff_t         difference_type;
  typedef value_type*       pointer;
  typedef const value_type* const_pointer;
  typedef value_type&       reference;
  typedef const value_type& const_reference;
  typedef sparse_hashtable_iterator<Value, Key, HashFcn,
                                    ExtractKey, EqualKey, Alloc>
  iterator;

  typedef sparse_hashtable_const_iterator<Value, Key, HashFcn,
                                          ExtractKey, EqualKey, Alloc>
  const_iterator;

  typedef sparse_hashtable_destructive_iterator<Value, Key, HashFcn,
                                                ExtractKey, EqualKey, Alloc>
  destructive_iterator;


  // How full we let the table get before we resize.  Knuth says .8 is
  // good -- higher causes us to probe too much, though saves memory
  static const float HT_OCCUPANCY_FLT; // = 0.8f;

  // How empty we let the table get before we resize lower.
  // It should be less than OCCUPANCY_FLT / 2 or we thrash resizing
  static const float HT_EMPTY_FLT; // = 0.4 * HT_OCCUPANCY_FLT;

  // Minimum size we're willing to let hashtables be.
  // Must be a power of two, and at least 4.
  // Note, however, that for a given hashtable, the minimum size is
  // determined by the first constructor arg, and may be >HT_MIN_BUCKETS.
  static const size_t HT_MIN_BUCKETS = 4;

  // By default, if you don't specify a hashtable size at
  // construction-time, we use this size.  Must be a power of two, and
  // at least HT_MIN_BUCKETS.
  static const size_t HT_DEFAULT_STARTING_BUCKETS = 32;

  // ITERATOR FUNCTIONS
  iterator begin()             { return iterator(this, table.nonempty_begin(),
                                                 table.nonempty_end()); }
  iterator end()               { return iterator(this, table.nonempty_end(),
                                                 table.nonempty_end()); }
  const_iterator begin() const { return const_iterator(this,
                                                       table.nonempty_begin(),
                                                       table.nonempty_end()); }
  const_iterator end() const   { return const_iterator(this,
                                                       table.nonempty_end(),
                                                       table.nonempty_end()); }

  // This is used when resizing
  destructive_iterator destructive_begin() {
    return destructive_iterator(this, table.destructive_begin(),
                                table.destructive_end());
  }
  destructive_iterator destructive_end() {
    return destructive_iterator(this, table.destructive_end(),
                                table.destructive_end());
  }


  // ACCESSOR FUNCTIONS for the things we templatize on, basically
  hasher hash_funct() const { return hash; }
  key_equal key_eq() const  { return equals; }

  // We need to copy values when we set the special marker for deleted
  // elements, but, annoyingly, we can't just use the copy assignment
  // operator because value_type might not be assignable (it's often
  // pair<const X, Y>).  We use explicit destructor invocation and
  // placement new to get around this.  Arg.
 private:
  void set_value(value_type* dst, const value_type src) {
    dst->~value_type();   // delete the old value, if any
    new(dst) value_type(src);
  }

  // This is used as a tag for the copy constructor, saying to destroy its
  // arg We have two ways of destructively copying: with potentially growing
  // the hashtable as we copy, and without.  To make sure the outside world
  // can't do a destructive copy, we make the typename private.
  enum MoveDontCopyT {MoveDontCopy, MoveDontGrow};


  // DELETE HELPER FUNCTIONS
  // This lets the user describe a key that will indicate deleted
  // table entries.  This key should be an "impossible" entry --
  // if you try to insert it for real, you won't be able to retrieve it!
  // (NB: while you pass in an entire value, only the key part is looked
  // at.  This is just because I don't know how to assign just a key.)
 private:
  void squash_deleted() {           // gets rid of any deleted entries we have
    if ( num_deleted ) {            // get rid of deleted before writing
      sparse_hashtable tmp(MoveDontGrow, *this);
      swap(tmp);                    // now we are tmp
    }
    assert(num_deleted == 0);
  }

 public:
  void set_deleted_key(const value_type &val) {
    // It's only safe to change what "deleted" means if we purge deleted guys
    squash_deleted();
    use_deleted = true;
    set_value(&delval, val);        // save the key (and rest of val too)
  }
  void clear_deleted_key() {
    squash_deleted();
    use_deleted = false;
  }

  // These are public so the iterators can use them
  // True if the item at position bucknum is "deleted" marker
  bool test_deleted(size_type bucknum) const {
    // The num_deleted test is crucial for read(): after read(), the ht values
    // are garbage, and we don't want to think some of them are deleted.
    return (use_deleted && num_deleted > 0 && table.test(bucknum) &&
            equals(get_key(delval), get_key(table.get(bucknum))));
  }
  bool test_deleted(const iterator &it) const {
    return (use_deleted && num_deleted > 0 &&
            equals(get_key(delval), get_key(*it)));
  }
  bool test_deleted(const const_iterator &it) const {
    return (use_deleted && num_deleted > 0 &&
            equals(get_key(delval), get_key(*it)));
  }
  bool test_deleted(const destructive_iterator &it) const {
    return (use_deleted && num_deleted > 0 &&
            equals(get_key(delval), get_key(*it)));
  }
  // Set it so test_deleted is true.  true if object didn't used to be deleted
  // See below (at erase()) to explain why we allow const_iterators
  bool set_deleted(const_iterator &it) {
    assert(use_deleted);             // bad if set_deleted_key() wasn't called
    bool retval = !test_deleted(it);
    // &* converts from iterator to value-type
    set_value(const_cast<value_type*>(&(*it)), delval);
    return retval;
  }
  // Set it so test_deleted is false.  true if object used to be deleted
  bool clear_deleted(const_iterator &it) {
    assert(use_deleted);             // bad if set_deleted_key() wasn't called
    // happens automatically when we assign something else in its place
    return test_deleted(it);
  }


  // FUNCTIONS CONCERNING SIZE
  size_type size() const      { return table.num_nonempty() - num_deleted; }
  // Buckets are always a power of 2
  size_type max_size() const  { return (size_type(-1) >> 1U) + 1; }
  bool empty() const          { return size() == 0; }
  size_type bucket_count() const      { return table.size(); }
  size_type max_bucket_count() const  { return max_size(); }

 private:
  // Because of the above, size_type(-1) is never legal; use it for errors
  static const size_type ILLEGAL_BUCKET = size_type(-1);

 private:
  // This is the smallest size a hashtable can be without being too crowded
  // If you like, you can give a min #buckets as well as a min #elts
  size_type min_size(size_type num_elts, size_type min_buckets_wanted) {
    size_type sz = HT_MIN_BUCKETS;
    while ( sz < min_buckets_wanted || num_elts >= sz * enlarge_resize_percent )
      sz *= 2;
    return sz;
  }

  // Used after a string of deletes
  void maybe_shrink() {
    assert(table.num_nonempty() >= num_deleted);
    assert((bucket_count() & (bucket_count()-1)) == 0); // is a power of two
    assert(bucket_count() >= HT_MIN_BUCKETS);

    // If you construct a hashtable with < HT_DEFAULT_STARTING_BUCKETS,
    // we'll never shrink until you get relatively big, and we'll never
    // shrink below HT_DEFAULT_STARTING_BUCKETS.  Otherwise, something
    // like "dense_hash_set<int> x; x.insert(4); x.erase(4);" will
    // shrink us down to HT_MIN_BUCKETS buckets, which is too small.
    if (shrink_threshold > 0
        && (table.num_nonempty()-num_deleted) < shrink_threshold &&
        bucket_count() > HT_DEFAULT_STARTING_BUCKETS ) {
      size_type sz = bucket_count() / 2;    // find how much we should shrink
      while ( sz > HT_DEFAULT_STARTING_BUCKETS &&
              (table.num_nonempty() - num_deleted) <= sz *
              shrink_resize_percent )
        sz /= 2;                            // stay a power of 2
      sparse_hashtable tmp(MoveDontCopy, *this, sz);
      swap(tmp);                            // now we are tmp
    }
    consider_shrink = false;                // because we just considered it
  }

  // We'll let you resize a hashtable -- though this makes us copy all!
  // When you resize, you say, "make it big enough for this many more elements"
  void resize_delta(size_type delta) {
    if ( consider_shrink )                   // see if lots of deletes happened
      maybe_shrink();
    if ( bucket_count() >= HT_MIN_BUCKETS &&
         (table.num_nonempty() + delta) <= enlarge_threshold )
      return;                                // we're ok as we are

    // Sometimes, we need to resize just to get rid of all the
    // "deleted" buckets that are clogging up the hashtable.  So when
    // deciding whether to resize, count the deleted buckets (which
    // are currently taking up room).  But later, when we decide what
    // size to resize to, *don't* count deleted buckets, since they
    // get discarded during the resize.
    const size_type needed_size = min_size(table.num_nonempty() + delta, 0);
    if ( needed_size > bucket_count() ) {      // we don't have enough buckets
      const size_type resize_to = min_size(table.num_nonempty() - num_deleted
                                           + delta, 0);
      sparse_hashtable tmp(MoveDontCopy, *this, resize_to);
      swap(tmp);                             // now we are tmp
    }
  }

  // Used to actually do the rehashing when we grow/shrink a hashtable
  void copy_from(const sparse_hashtable &ht, size_type min_buckets_wanted) {
    clear();            // clear table, set num_deleted to 0

    // If we need to change the size of our table, do it now
    const size_type resize_to = min_size(ht.size(), min_buckets_wanted);
    if ( resize_to > bucket_count() ) {      // we don't have enough buckets
      table.resize(resize_to);               // sets the number of buckets
      reset_thresholds();
    }

    // We use a normal iterator to get non-deleted bcks from ht
    // We could use insert() here, but since we know there are
    // no duplicates and no deleted items, we can be more efficient
    assert( (bucket_count() & (bucket_count()-1)) == 0);      // a power of two
    for ( const_iterator it = ht.begin(); it != ht.end(); ++it ) {
      size_type num_probes = 0;              // how many times we've probed
      size_type bucknum;
      const size_type bucket_count_minus_one = bucket_count() - 1;
      for (bucknum = hash(get_key(*it)) & bucket_count_minus_one;
           table.test(bucknum);                          // not empty
           bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one) {
        ++num_probes;
        assert(num_probes < bucket_count()); // or else the hashtable is full
      }
      table.set(bucknum, *it);               // copies the value to here
    }
  }

  // Implementation is like copy_from, but it destroys the table of the
  // "from" guy by freeing sparsetable memory as we iterate.  This is
  // useful in resizing, since we're throwing away the "from" guy anyway.
  void move_from(MoveDontCopyT mover, sparse_hashtable &ht,
                 size_type min_buckets_wanted) {
    clear();            // clear table, set num_deleted to 0

    // If we need to change the size of our table, do it now
    size_t resize_to;
    if ( mover == MoveDontGrow )
      resize_to = ht.bucket_count();         // keep same size as old ht
    else                                     // MoveDontCopy
      resize_to = min_size(ht.size(), min_buckets_wanted);
    if ( resize_to > bucket_count() ) {      // we don't have enough buckets
      table.resize(resize_to);               // sets the number of buckets
      reset_thresholds();
    }

    // We use a normal iterator to get non-deleted bcks from ht
    // We could use insert() here, but since we know there are
    // no duplicates and no deleted items, we can be more efficient
    assert( (bucket_count() & (bucket_count()-1)) == 0);      // a power of two
    // THIS IS THE MAJOR LINE THAT DIFFERS FROM COPY_FROM():
    for ( destructive_iterator it = ht.destructive_begin();
          it != ht.destructive_end(); ++it ) {
      size_type num_probes = 0;              // how many times we've probed
      size_type bucknum;
      for ( bucknum = hash(get_key(*it)) & (bucket_count()-1);  // h % buck_cnt
            table.test(bucknum);                          // not empty
            bucknum = (bucknum + JUMP_(key, num_probes)) & (bucket_count()-1) ) {
        ++num_probes;
        assert(num_probes < bucket_count()); // or else the hashtable is full
      }
      table.set(bucknum, *it);               // copies the value to here
    }
  }


  // Required by the spec for hashed associative container
 public:
  // Though the docs say this should be num_buckets, I think it's much
  // more useful as num_elements.  As a special feature, calling with
  // req_elements==0 will cause us to shrink if we can, saving space.
  void resize(size_type req_elements) {       // resize to this or larger
    if ( consider_shrink || req_elements == 0 )
      maybe_shrink();
    if ( req_elements > table.num_nonempty() )    // we only grow
      resize_delta(req_elements - table.num_nonempty());
  }

  // Change the value of shrink_resize_percent and
  // enlarge_resize_percent.  The description at the beginning of this
  // file explains how to choose the values.  Setting the shrink
  // parameter to 0.0 ensures that the table never shrinks.
  void set_resizing_parameters(float shrink, float grow) {
    assert(shrink >= 0.0);
    assert(grow <= 1.0);
    assert(shrink <= grow/2.0);
    shrink_resize_percent = shrink;
    enlarge_resize_percent = grow;
    reset_thresholds();
  }

  // CONSTRUCTORS -- as required by the specs, we take a size,
  // but also let you specify a hashfunction, key comparator,
  // and key extractor.  We also define a copy constructor and =.
  // DESTRUCTOR -- the default is fine, surprisingly.
  explicit sparse_hashtable(size_type expected_max_items_in_table = 0,
                            const HashFcn& hf = HashFcn(),
                            const EqualKey& eql = EqualKey(),
                            const ExtractKey& ext = ExtractKey())
    : hash(hf), equals(eql), get_key(ext), num_deleted(0), use_deleted(false),
      delval(), enlarge_resize_percent(HT_OCCUPANCY_FLT),
      shrink_resize_percent(HT_EMPTY_FLT),
      table(expected_max_items_in_table == 0
            ? HT_DEFAULT_STARTING_BUCKETS
            : min_size(expected_max_items_in_table, 0)) {
    reset_thresholds();
  }

  // As a convenience for resize(), we allow an optional second argument
  // which lets you make this new hashtable a different size than ht.
  // We also provide a mechanism of saying you want to "move" the ht argument
  // into us instead of copying.
  sparse_hashtable(const sparse_hashtable& ht,
                   size_type min_buckets_wanted = HT_DEFAULT_STARTING_BUCKETS)
    : hash(ht.hash), equals(ht.equals), get_key(ht.get_key),
      num_deleted(0), use_deleted(ht.use_deleted), delval(ht.delval),
      enlarge_resize_percent(ht.enlarge_resize_percent),
      shrink_resize_percent(ht.shrink_resize_percent),
      table() {
    reset_thresholds();
    copy_from(ht, min_buckets_wanted);   // copy_from() ignores deleted entries
  }
  sparse_hashtable(MoveDontCopyT mover, sparse_hashtable& ht,
                   size_type min_buckets_wanted = HT_DEFAULT_STARTING_BUCKETS)
    : hash(ht.hash), equals(ht.equals), get_key(ht.get_key),
      num_deleted(0), use_deleted(ht.use_deleted), delval(ht.delval),
      enlarge_resize_percent(ht.enlarge_resize_percent),
      shrink_resize_percent(ht.shrink_resize_percent),
      table() {
    reset_thresholds();
    move_from(mover, ht, min_buckets_wanted);  // ignores deleted entries
  }

  sparse_hashtable& operator= (const sparse_hashtable& ht) {
    if (&ht == this)  return *this;        // don't copy onto ourselves
    clear();
    hash = ht.hash;
    equals = ht.equals;
    get_key = ht.get_key;
    use_deleted = ht.use_deleted;
    set_value(&delval, ht.delval);
    copy_from(ht, HT_MIN_BUCKETS);         // sets num_deleted to 0 too
    return *this;
  }

  // Many STL algorithms use swap instead of copy constructors
  void swap(sparse_hashtable& ht) {
    STL_NAMESPACE::swap(hash, ht.hash);
    STL_NAMESPACE::swap(equals, ht.equals);
    STL_NAMESPACE::swap(get_key, ht.get_key);
    STL_NAMESPACE::swap(num_deleted, ht.num_deleted);
    STL_NAMESPACE::swap(use_deleted, ht.use_deleted);
    STL_NAMESPACE::swap(enlarge_resize_percent, ht.enlarge_resize_percent);
    STL_NAMESPACE::swap(shrink_resize_percent, ht.shrink_resize_percent);
    { value_type tmp;     // for annoying reasons, swap() doesn't work
      set_value(&tmp, delval);
      set_value(&delval, ht.delval);
      set_value(&ht.delval, tmp);
    }
    table.swap(ht.table);
    reset_thresholds();
    ht.reset_thresholds();
  }

  // It's always nice to be able to clear a table without deallocating it
  void clear() {
    table.clear();
    reset_thresholds();
    num_deleted = 0;
  }


  // LOOKUP ROUTINES
 private:
  // Returns a pair of positions: 1st where the object is, 2nd where
  // it would go if you wanted to insert it.  1st is ILLEGAL_BUCKET
  // if object is not found; 2nd is ILLEGAL_BUCKET if it is.
  // Note: because of deletions where-to-insert is not trivial: it's the
  // first deleted bucket we see, as long as we don't find the key later
  pair<size_type, size_type> find_position(const key_type &key) const {
    size_type num_probes = 0;              // how many times we've probed
    const size_type bucket_count_minus_one = bucket_count() - 1;
    size_type bucknum = hash(key) & bucket_count_minus_one;
    size_type insert_pos = ILLEGAL_BUCKET; // where we would insert
    SPARSEHASH_STAT_UPDATE(total_lookups += 1);
    while ( 1 ) {                          // probe until something happens
      if ( !table.test(bucknum) ) {        // bucket is empty
        SPARSEHASH_STAT_UPDATE(total_probes += num_probes);
        if ( insert_pos == ILLEGAL_BUCKET )  // found no prior place to insert
          return pair<size_type,size_type>(ILLEGAL_BUCKET, bucknum);
        else
          return pair<size_type,size_type>(ILLEGAL_BUCKET, insert_pos);

      } else if ( test_deleted(bucknum) ) {// keep searching, but mark to insert
        if ( insert_pos == ILLEGAL_BUCKET )
          insert_pos = bucknum;

      } else if ( equals(key, get_key(table.get(bucknum))) ) {
        SPARSEHASH_STAT_UPDATE(total_probes += num_probes);
        return pair<size_type,size_type>(bucknum, ILLEGAL_BUCKET);
      }
      ++num_probes;                        // we're doing another probe
      bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one;
      assert(num_probes < bucket_count()); // don't probe too many times!
    }
  }

 public:
  iterator find(const key_type& key) {
    if ( size() == 0 ) return end();
    pair<size_type, size_type> pos = find_position(key);
    if ( pos.first == ILLEGAL_BUCKET )     // alas, not there
      return end();
    else
      return iterator(this, table.get_iter(pos.first), table.nonempty_end());
  }

  const_iterator find(const key_type& key) const {
    if ( size() == 0 ) return end();
    pair<size_type, size_type> pos = find_position(key);
    if ( pos.first == ILLEGAL_BUCKET )     // alas, not there
      return end();
    else
      return const_iterator(this,
                            table.get_iter(pos.first), table.nonempty_end());
  }

  // Counts how many elements have key key.  For maps, it's either 0 or 1.
  size_type count(const key_type &key) const {
    pair<size_type, size_type> pos = find_position(key);
    return pos.first == ILLEGAL_BUCKET ? 0 : 1;
  }

  // Likewise, equal_range doesn't really make sense for us.  Oh well.
  pair<iterator,iterator> equal_range(const key_type& key) {
    const iterator pos = find(key);      // either an iterator or end
    return pair<iterator,iterator>(pos, pos);
  }
  pair<const_iterator,const_iterator> equal_range(const key_type& key) const {
    const const_iterator pos = find(key);      // either an iterator or end
    return pair<iterator,iterator>(pos, pos);
  }


  // INSERTION ROUTINES
 private:
  // If you know *this is big enough to hold obj, use this routine
  pair<iterator, bool> insert_noresize(const value_type& obj) {
    // First, double-check we're not inserting delval
    assert(!use_deleted || !equals(get_key(obj), get_key(delval)));
    const pair<size_type,size_type> pos = find_position(get_key(obj));
    if ( pos.first != ILLEGAL_BUCKET) {      // object was already there
      return pair<iterator,bool>(iterator(this, table.get_iter(pos.first),
                                          table.nonempty_end()),
                                 false);          // false: we didn't insert
    } else {                                 // pos.second says where to put it
      if ( test_deleted(pos.second) ) {      // just replace if it's been del.
        // The set() below will undelete this object.  We just worry about stats
        assert(num_deleted > 0);
        --num_deleted;                       // used to be, now it isn't
      }
      table.set(pos.second, obj);
      return pair<iterator,bool>(iterator(this, table.get_iter(pos.second),
                                          table.nonempty_end()),
                                 true);           // true: we did insert
    }
  }

 public:
  // This is the normal insert routine, used by the outside world
  pair<iterator, bool> insert(const value_type& obj) {
    resize_delta(1);  // adding an object, grow if need be
    return insert_noresize(obj);
  }

  // When inserting a lot at a time, we specialize on the type of iterator
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l) {
    // specializes on iterator type
    insert(f, l, typename STL_NAMESPACE::iterator_traits<InputIterator>::iterator_category());
  }

  // Iterator supports operator-, resize before inserting
  template <class ForwardIterator>
  void insert(ForwardIterator f, ForwardIterator l,
              STL_NAMESPACE::forward_iterator_tag) {
    size_type n = STL_NAMESPACE::distance(f, l);   // TODO(csilvers): standard?
    resize_delta(n);
    for ( ; n > 0; --n, ++f)
      insert_noresize(*f);
  }

  // Arbitrary iterator, can't tell how much to resize
  template <class InputIterator>
  void insert(InputIterator f, InputIterator l,
              STL_NAMESPACE::input_iterator_tag) {
    for ( ; f != l; ++f)
      insert(*f);
  }


  // DELETION ROUTINES
  size_type erase(const key_type& key) {
    // First, double-check we're not erasing delval
    assert(!use_deleted || !equals(key, get_key(delval)));
    const_iterator pos = find(key);   // shrug: shouldn't need to be const
    if ( pos != end() ) {
      assert(!test_deleted(pos));  // or find() shouldn't have returned it
      set_deleted(pos);
      ++num_deleted;
      consider_shrink = true;      // will think about shrink after next insert
      return 1;                    // because we deleted one thing
    } else {
      return 0;                    // because we deleted nothing
    }
  }

  // This is really evil: really it should be iterator, not const_iterator.
  // But...the only reason keys are const is to allow lookup.
  // Since that's a moot issue for deleted keys, we allow const_iterators
  void erase(const_iterator pos) {
    if ( pos == end() ) return;    // sanity check
    if ( set_deleted(pos) ) {      // true if object has been newly deleted
      ++num_deleted;
      consider_shrink = true;      // will think about shrink after next insert
    }
  }

  void erase(const_iterator f, const_iterator l) {
    for ( ; f != l; ++f) {
      if ( set_deleted(f)  )       // should always be true
        ++num_deleted;
    }
    consider_shrink = true;        // will think about shrink after next insert
  }


  // COMPARISON
  bool operator==(const sparse_hashtable& ht) const {
    // We really want to check that the hash functions are the same
    // but alas there's no way to do this.  We just hope.
    return ( num_deleted == ht.num_deleted && table == ht.table );
  }
  bool operator!=(const sparse_hashtable& ht) const {
    return !(*this == ht);
  }


  // I/O
  // We support reading and writing hashtables to disk.  NOTE that
  // this only stores the hashtable metadata, not the stuff you've
  // actually put in the hashtable!  Alas, since I don't know how to
  // write a hasher or key_equal, you have to make sure everything
  // but the table is the same.  We compact before writing.
  bool write_metadata(FILE *fp) {
    squash_deleted();           // so we don't have to worry about delkey
    return table.write_metadata(fp);
  }

  bool read_metadata(FILE *fp) {
    num_deleted = 0;            // since we got rid before writing
    bool result = table.read_metadata(fp);
    reset_thresholds();
    return result;
  }

  // Only meaningful if value_type is a POD.
  bool write_nopointer_data(FILE *fp) {
    return table.write_nopointer_data(fp);
  }

  // Only meaningful if value_type is a POD.
  bool read_nopointer_data(FILE *fp) {
    return table.read_nopointer_data(fp);
  }

 private:
  // The actual data
  hasher hash;                      // required by hashed_associative_container
  key_equal equals;
  ExtractKey get_key;
  size_type num_deleted;        // how many occupied buckets are marked deleted
  bool use_deleted;                          // false until delval has been set
  value_type delval;                         // which key marks deleted entries
  float enlarge_resize_percent;                       // how full before resize
  float shrink_resize_percent;                       // how empty before resize
  size_type shrink_threshold;           // table.size() * shrink_resize_percent
  size_type enlarge_threshold;         // table.size() * enlarge_resize_percent
  sparsetable<value_type> table;      // holds num_buckets and num_elements too
  bool consider_shrink;   // true if we should try to shrink before next insert

  void reset_thresholds() {
    enlarge_threshold = static_cast<size_type>(table.size()
                                               * enlarge_resize_percent);
    shrink_threshold = static_cast<size_type>(table.size()
                                              * shrink_resize_percent);
    consider_shrink = false;   // whatever caused us to reset already considered
  }
};

// We need a global swap as well
template <class V, class K, class HF, class ExK, class EqK, class A>
inline void swap(sparse_hashtable<V,K,HF,ExK,EqK,A> &x,
                 sparse_hashtable<V,K,HF,ExK,EqK,A> &y) {
  x.swap(y);
}

#undef JUMP_

template <class V, class K, class HF, class ExK, class EqK, class A>
const typename sparse_hashtable<V,K,HF,ExK,EqK,A>::size_type
  sparse_hashtable<V,K,HF,ExK,EqK,A>::ILLEGAL_BUCKET;

// How full we let the table get before we resize.  Knuth says .8 is
// good -- higher causes us to probe too much, though saves memory
template <class V, class K, class HF, class ExK, class EqK, class A>
const float sparse_hashtable<V,K,HF,ExK,EqK,A>::HT_OCCUPANCY_FLT = 0.8f;

// How empty we let the table get before we resize lower.
// It should be less than OCCUPANCY_FLT / 2 or we thrash resizing
template <class V, class K, class HF, class ExK, class EqK, class A>
const float sparse_hashtable<V,K,HF,ExK,EqK,A>::HT_EMPTY_FLT = 0.4f *
sparse_hashtable<V,K,HF,ExK,EqK,A>::HT_OCCUPANCY_FLT;

_END_GOOGLE_NAMESPACE_

#endif /* _SPARSEHASHTABLE_H_ */
