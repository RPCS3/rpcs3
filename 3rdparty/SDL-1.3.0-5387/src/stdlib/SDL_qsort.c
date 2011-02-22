/* qsort.c
 * (c) 1998 Gareth McCaughan
 *
 * This is a drop-in replacement for the C library's |qsort()| routine.
 *
 * Features:
 *   - Median-of-three pivoting (and more)
 *   - Truncation and final polishing by a single insertion sort
 *   - Early truncation when no swaps needed in pivoting step
 *   - Explicit recursion, guaranteed not to overflow
 *   - A few little wrinkles stolen from the GNU |qsort()|.
 *   - separate code for non-aligned / aligned / word-size objects
 *
 * This code may be reproduced freely provided
 *   - this file is retained unaltered apart from minor
 *     changes for portability and efficiency
 *   - no changes are made to this comment
 *   - any changes that *are* made are clearly flagged
 *   - the _ID string below is altered by inserting, after
 *     the date, the string " altered" followed at your option
 *     by other material. (Exceptions: you may change the name
 *     of the exported routine without changing the ID string.
 *     You may change the values of the macros TRUNC_* and
 *     PIVOT_THRESHOLD without changing the ID string, provided
 *     they remain constants with TRUNC_nonaligned, TRUNC_aligned
 *     and TRUNC_words/WORD_BYTES between 8 and 24, and
 *     PIVOT_THRESHOLD between 32 and 200.)
 *
 * You may use it in anything you like; you may make money
 * out of it; you may distribute it in object form or as
 * part of an executable without including source code;
 * you don't have to credit me. (But it would be nice if
 * you did.)
 *
 * If you find problems with this code, or find ways of
 * making it significantly faster, please let me know!
 * My e-mail address, valid as of early 1998 and certainly
 * OK for at least the next 18 months, is
 *    gjm11@dpmms.cam.ac.uk
 * Thanks!
 *
 * Gareth McCaughan   Peterhouse   Cambridge   1998
 */
#include "SDL_config.h"

/*
#include <assert.h>
#include <stdlib.h>
#include <string.h>
*/
#include "SDL_stdinc.h"

#ifdef assert
#undef assert
#endif
#define assert(X)
#ifdef malloc
#undef malloc
#endif
#define malloc	SDL_malloc
#ifdef free
#undef free
#endif
#define free	SDL_free
#ifdef memcpy
#undef memcpy
#endif
#define memcpy	SDL_memcpy
#ifdef memmove
#undef memmove
#endif
#define memmove	SDL_memmove
#ifdef qsort
#undef qsort
#endif
#define qsort	SDL_qsort


#ifndef HAVE_QSORT

static const char _ID[] = "<qsort.c gjm 1.12 1998-03-19>";

/* How many bytes are there per word? (Must be a power of 2,
 * and must in fact equal sizeof(int).)
 */
#define WORD_BYTES sizeof(int)

/* How big does our stack need to be? Answer: one entry per
 * bit in a |size_t|.
 */
#define STACK_SIZE (8*sizeof(size_t))

/* Different situations have slightly different requirements,
 * and we make life epsilon easier by using different truncation
 * points for the three different cases.
 * So far, I have tuned TRUNC_words and guessed that the same
 * value might work well for the other two cases. Of course
 * what works well on my machine might work badly on yours.
 */
#define TRUNC_nonaligned	12
#define TRUNC_aligned		12
#define TRUNC_words		12*WORD_BYTES   /* nb different meaning */

/* We use a simple pivoting algorithm for shortish sub-arrays
 * and a more complicated one for larger ones. The threshold
 * is PIVOT_THRESHOLD.
 */
#define PIVOT_THRESHOLD 40

typedef struct
{
    char *first;
    char *last;
} stack_entry;
#define pushLeft {stack[stacktop].first=ffirst;stack[stacktop++].last=last;}
#define pushRight {stack[stacktop].first=first;stack[stacktop++].last=llast;}
#define doLeft {first=ffirst;llast=last;continue;}
#define doRight {ffirst=first;last=llast;continue;}
#define pop {if (--stacktop<0) break;\
  first=ffirst=stack[stacktop].first;\
  last=llast=stack[stacktop].last;\
  continue;}

/* Some comments on the implementation.
 * 1. When we finish partitioning the array into "low"
 *    and "high", we forget entirely about short subarrays,
 *    because they'll be done later by insertion sort.
 *    Doing lots of little insertion sorts might be a win
 *    on large datasets for locality-of-reference reasons,
 *    but it makes the code much nastier and increases
 *    bookkeeping overhead.
 * 2. We always save the shorter and get to work on the
 *    longer. This guarantees that every time we push
 *    an item onto the stack its size is <= 1/2 of that
 *    of its parent; so the stack can't need more than
 *    log_2(max-array-size) entries.
 * 3. We choose a pivot by looking at the first, last
 *    and middle elements. We arrange them into order
 *    because it's easy to do that in conjunction with
 *    choosing the pivot, and it makes things a little
 *    easier in the partitioning step. Anyway, the pivot
 *    is the middle of these three. It's still possible
 *    to construct datasets where the algorithm takes
 *    time of order n^2, but it simply never happens in
 *    practice.
 * 3' Newsflash: On further investigation I find that
 *    it's easy to construct datasets where median-of-3
 *    simply isn't good enough. So on large-ish subarrays
 *    we do a more sophisticated pivoting: we take three
 *    sets of 3 elements, find their medians, and then
 *    take the median of those.
 * 4. We copy the pivot element to a separate place
 *    because that way we can always do our comparisons
 *    directly against a pointer to that separate place,
 *    and don't have to wonder "did we move the pivot
 *    element?". This makes the inner loop better.
 * 5. It's possible to make the pivoting even more
 *    reliable by looking at more candidates when n
 *    is larger. (Taking this to its logical conclusion
 *    results in a variant of quicksort that doesn't
 *    have that n^2 worst case.) However, the overhead
 *    from the extra bookkeeping means that it's just
 *    not worth while.
 * 6. This is pretty clean and portable code. Here are
 *    all the potential portability pitfalls and problems
 *    I know of:
 *      - In one place (the insertion sort) I construct
 *        a pointer that points just past the end of the
 *        supplied array, and assume that (a) it won't
 *        compare equal to any pointer within the array,
 *        and (b) it will compare equal to a pointer
 *        obtained by stepping off the end of the array.
 *        These might fail on some segmented architectures.
 *      - I assume that there are 8 bits in a |char| when
 *        computing the size of stack needed. This would
 *        fail on machines with 9-bit or 16-bit bytes.
 *      - I assume that if |((int)base&(sizeof(int)-1))==0|
 *        and |(size&(sizeof(int)-1))==0| then it's safe to
 *        get at array elements via |int*|s, and that if
 *        actually |size==sizeof(int)| as well then it's
 *        safe to treat the elements as |int|s. This might
 *        fail on systems that convert pointers to integers
 *        in non-standard ways.
 *      - I assume that |8*sizeof(size_t)<=INT_MAX|. This
 *        would be false on a machine with 8-bit |char|s,
 *        16-bit |int|s and 4096-bit |size_t|s. :-)
 */

/* The recursion logic is the same in each case: */
#define Recurse(Trunc)				\
      { size_t l=last-ffirst,r=llast-first;	\
        if (l<Trunc) {				\
          if (r>=Trunc) doRight			\
          else pop				\
        }					\
        else if (l<=r) { pushLeft; doRight }	\
        else if (r>=Trunc) { pushRight; doLeft }\
        else doLeft				\
      }

/* and so is the pivoting logic: */
#define Pivot(swapper,sz)			\
  if ((size_t)(last-first)>PIVOT_THRESHOLD*sz) mid=pivot_big(first,mid,last,sz,compare);\
  else {	\
    if (compare(first,mid)<0) {			\
      if (compare(mid,last)>0) {		\
        swapper(mid,last);			\
        if (compare(first,mid)>0) swapper(first,mid);\
      }						\
    }						\
    else {					\
      if (compare(mid,last)>0) swapper(first,last)\
      else {					\
        swapper(first,mid);			\
        if (compare(mid,last)>0) swapper(mid,last);\
      }						\
    }						\
    first+=sz; last-=sz;			\
  }

#ifdef DEBUG_QSORT
#include <stdio.h>
#endif

/* and so is the partitioning logic: */
#define Partition(swapper,sz) {			\
  int swapped=0;				\
  do {						\
    while (compare(first,pivot)<0) first+=sz;	\
    while (compare(pivot,last)<0) last-=sz;	\
    if (first<last) {				\
      swapper(first,last); swapped=1;		\
      first+=sz; last-=sz; }			\
    else if (first==last) { first+=sz; last-=sz; break; }\
  } while (first<=last);			\
  if (!swapped) pop				\
}

/* and so is the pre-insertion-sort operation of putting
 * the smallest element into place as a sentinel.
 * Doing this makes the inner loop nicer. I got this
 * idea from the GNU implementation of qsort().
 */
#define PreInsertion(swapper,limit,sz)		\
  first=base;					\
  last=first + (nmemb>limit ? limit : nmemb-1)*sz;\
  while (last!=base) {				\
    if (compare(first,last)>0) first=last;	\
    last-=sz; }					\
  if (first!=base) swapper(first,(char*)base);

/* and so is the insertion sort, in the first two cases: */
#define Insertion(swapper)			\
  last=((char*)base)+nmemb*size;		\
  for (first=((char*)base)+size;first!=last;first+=size) {	\
    char *test;					\
    /* Find the right place for |first|.	\
     * My apologies for var reuse. */		\
    for (test=first-size;compare(test,first)>0;test-=size) ;	\
    test+=size;					\
    if (test!=first) {				\
      /* Shift everything in [test,first)	\
       * up by one, and place |first|		\
       * where |test| is. */			\
      memcpy(pivot,first,size);			\
      memmove(test+size,test,first-test);	\
      memcpy(test,pivot,size);			\
    }						\
  }

#define SWAP_nonaligned(a,b) { \
  register char *aa=(a),*bb=(b); \
  register size_t sz=size; \
  do { register char t=*aa; *aa++=*bb; *bb++=t; } while (--sz); }

#define SWAP_aligned(a,b) { \
  register int *aa=(int*)(a),*bb=(int*)(b); \
  register size_t sz=size; \
  do { register int t=*aa;*aa++=*bb; *bb++=t; } while (sz-=WORD_BYTES); }

#define SWAP_words(a,b) { \
  register int t=*((int*)a); *((int*)a)=*((int*)b); *((int*)b)=t; }

/* ---------------------------------------------------------------------- */

static char *
pivot_big(char *first, char *mid, char *last, size_t size,
          int compare(const void *, const void *))
{
    size_t d = (((last - first) / size) >> 3) * size;
    char *m1, *m2, *m3;
    {
        char *a = first, *b = first + d, *c = first + 2 * d;
#ifdef DEBUG_QSORT
        fprintf(stderr, "< %d %d %d\n", *(int *) a, *(int *) b, *(int *) c);
#endif
        m1 = compare(a, b) < 0 ?
            (compare(b, c) < 0 ? b : (compare(a, c) < 0 ? c : a))
            : (compare(a, c) < 0 ? a : (compare(b, c) < 0 ? c : b));
    }
    {
        char *a = mid - d, *b = mid, *c = mid + d;
#ifdef DEBUG_QSORT
        fprintf(stderr, ". %d %d %d\n", *(int *) a, *(int *) b, *(int *) c);
#endif
        m2 = compare(a, b) < 0 ?
            (compare(b, c) < 0 ? b : (compare(a, c) < 0 ? c : a))
            : (compare(a, c) < 0 ? a : (compare(b, c) < 0 ? c : b));
    }
    {
        char *a = last - 2 * d, *b = last - d, *c = last;
#ifdef DEBUG_QSORT
        fprintf(stderr, "> %d %d %d\n", *(int *) a, *(int *) b, *(int *) c);
#endif
        m3 = compare(a, b) < 0 ?
            (compare(b, c) < 0 ? b : (compare(a, c) < 0 ? c : a))
            : (compare(a, c) < 0 ? a : (compare(b, c) < 0 ? c : b));
    }
#ifdef DEBUG_QSORT
    fprintf(stderr, "-> %d %d %d\n", *(int *) m1, *(int *) m2, *(int *) m3);
#endif
    return compare(m1, m2) < 0 ?
        (compare(m2, m3) < 0 ? m2 : (compare(m1, m3) < 0 ? m3 : m1))
        : (compare(m1, m3) < 0 ? m1 : (compare(m2, m3) < 0 ? m3 : m2));
}

/* ---------------------------------------------------------------------- */

static void
qsort_nonaligned(void *base, size_t nmemb, size_t size,
                 int (*compare) (const void *, const void *))
{

    stack_entry stack[STACK_SIZE];
    int stacktop = 0;
    char *first, *last;
    char *pivot = malloc(size);
    size_t trunc = TRUNC_nonaligned * size;
    assert(pivot != 0);

    first = (char *) base;
    last = first + (nmemb - 1) * size;

    if ((size_t) (last - first) > trunc) {
        char *ffirst = first, *llast = last;
        while (1) {
            /* Select pivot */
            {
                char *mid = first + size * ((last - first) / size >> 1);
                Pivot(SWAP_nonaligned, size);
                memcpy(pivot, mid, size);
            }
            /* Partition. */
            Partition(SWAP_nonaligned, size);
            /* Prepare to recurse/iterate. */
        Recurse(trunc)}
    }
    PreInsertion(SWAP_nonaligned, TRUNC_nonaligned, size);
    Insertion(SWAP_nonaligned);
    free(pivot);
}

static void
qsort_aligned(void *base, size_t nmemb, size_t size,
              int (*compare) (const void *, const void *))
{

    stack_entry stack[STACK_SIZE];
    int stacktop = 0;
    char *first, *last;
    char *pivot = malloc(size);
    size_t trunc = TRUNC_aligned * size;
    assert(pivot != 0);

    first = (char *) base;
    last = first + (nmemb - 1) * size;

    if ((size_t) (last - first) > trunc) {
        char *ffirst = first, *llast = last;
        while (1) {
            /* Select pivot */
            {
                char *mid = first + size * ((last - first) / size >> 1);
                Pivot(SWAP_aligned, size);
                memcpy(pivot, mid, size);
            }
            /* Partition. */
            Partition(SWAP_aligned, size);
            /* Prepare to recurse/iterate. */
        Recurse(trunc)}
    }
    PreInsertion(SWAP_aligned, TRUNC_aligned, size);
    Insertion(SWAP_aligned);
    free(pivot);
}

static void
qsort_words(void *base, size_t nmemb,
            int (*compare) (const void *, const void *))
{

    stack_entry stack[STACK_SIZE];
    int stacktop = 0;
    char *first, *last;
    char *pivot = malloc(WORD_BYTES);
    assert(pivot != 0);

    first = (char *) base;
    last = first + (nmemb - 1) * WORD_BYTES;

    if (last - first > TRUNC_words) {
        char *ffirst = first, *llast = last;
        while (1) {
#ifdef DEBUG_QSORT
            fprintf(stderr, "Doing %d:%d: ",
                    (first - (char *) base) / WORD_BYTES,
                    (last - (char *) base) / WORD_BYTES);
#endif
            /* Select pivot */
            {
                char *mid =
                    first + WORD_BYTES * ((last - first) / (2 * WORD_BYTES));
                Pivot(SWAP_words, WORD_BYTES);
                *(int *) pivot = *(int *) mid;
            }
#ifdef DEBUG_QSORT
            fprintf(stderr, "pivot=%d\n", *(int *) pivot);
#endif
            /* Partition. */
            Partition(SWAP_words, WORD_BYTES);
            /* Prepare to recurse/iterate. */
        Recurse(TRUNC_words)}
    }
    PreInsertion(SWAP_words, (TRUNC_words / WORD_BYTES), WORD_BYTES);
    /* Now do insertion sort. */
    last = ((char *) base) + nmemb * WORD_BYTES;
    for (first = ((char *) base) + WORD_BYTES; first != last;
         first += WORD_BYTES) {
        /* Find the right place for |first|. My apologies for var reuse */
        int *pl = (int *) (first - WORD_BYTES), *pr = (int *) first;
        *(int *) pivot = *(int *) first;
        for (; compare(pl, pivot) > 0; pr = pl, --pl) {
            *pr = *pl;
        }
        if (pr != (int *) first)
            *pr = *(int *) pivot;
    }
    free(pivot);
}

/* ---------------------------------------------------------------------- */

void
qsort(void *base, size_t nmemb, size_t size,
      int (*compare) (const void *, const void *))
{

    if (nmemb <= 1)
        return;
    if (((uintptr_t) base | size) & (WORD_BYTES - 1))
        qsort_nonaligned(base, nmemb, size, compare);
    else if (size != WORD_BYTES)
        qsort_aligned(base, nmemb, size, compare);
    else
        qsort_words(base, nmemb, compare);
}

#endif /* !HAVE_QSORT */
/* vi: set ts=4 sw=4 expandtab: */
