/* Copyright (C) 1991-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Douglas C. Schmidt (schmidt@ics.uci.edu).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/* If you consider tuning this algorithm, you should consult first:
   Engineering a sort function; Jon Bentley and M. Douglas McIlroy;
   Software - Practice and Experience; Vol. 23 (11), 1249-1265, 1993.  */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Swap SIZE bytes between addresses A and B.  These helpers are provided
   along the generic one as an optimization.  */

typedef void (*swap_t)(void *, void *, size_t);

static inline bool
check_alignment (const void *base, size_t align)
{
  return _STRING_ARCH_unaligned || ((uintptr_t)base & (align - 1)) == 0;
}

static void
swap_u32 (void * restrict a, void * restrict b, size_t size)
{
  uint32_t *ua = a, *ub = b, tmp = *ua;
  *ua = *ub, *ub = tmp;
}

static void
swap_u64 (void * restrict a, void * restrict b, size_t size)
{
  uint64_t *ua = a, *ub = b, tmp = *ua;
  *ua = *ub, *ub = tmp;
}

static void
swap_generic (void * restrict a, void * restrict b, size_t size)
{
  /* Use multiple small memcpys with constant size to enable inlining
     on most targets.  */
  enum {
    SWAP_GENERIC_SIZE = 32
  };
  unsigned char tmp[SWAP_GENERIC_SIZE];
  while (size > SWAP_GENERIC_SIZE)
    {
      memcpy (tmp, a, SWAP_GENERIC_SIZE);
      a = memcpy (a, b, SWAP_GENERIC_SIZE) + SWAP_GENERIC_SIZE;
      b = memcpy (b, tmp, SWAP_GENERIC_SIZE) + SWAP_GENERIC_SIZE;
      size -= SWAP_GENERIC_SIZE;
    }
  memcpy (tmp, a, size);
  memcpy (a, b, size);
  memcpy (b, tmp, size);
}

static inline swap_t
select_swap_func (const void *base, size_t size)
{
  if (size == sizeof (uint32_t)
      && check_alignment (base, _Alignof (uint32_t)))
    return swap_u32;
  else if (size == sizeof (uint64_t)
	   && check_alignment (base, _Alignof (uint64_t)))
    return swap_u64;
  return swap_generic;
}

/* Discontinue quicksort algorithm when partition gets below this size.
   This particular magic number was chosen to work best on a Sun 4/260. */
#define MAX_THRESH 4

/* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct
  {
    char *lo;
    char *hi;
    size_t depth;
  } stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
   log(MAX_THRESH)).  Since total_elements has type size_t, we get as
   upper bound for log (total_elements):
   bits per byte (CHAR_BIT) * sizeof(size_t).  */

enum {
  stack_size = CHAR_BIT * sizeof (size_t)
};

static inline stack_node *
push (stack_node *top, char *lo, char *hi, size_t depth)
{
  top->lo = lo;
  top->hi = hi;
  top->depth = depth;
  return ++top;
}

static inline stack_node *
pop (stack_node *top, char **lo, char **hi, size_t *depth)
{
  --top;
  *lo = top->lo;
  *hi = top->hi;
  *depth = top->depth;
  return top;
}

/* Helper function to calculate the maximum depth before quicksort switches
   to heapsort.  X should be higher than 0.  */
static inline size_t
lg (size_t x)
{
  return sizeof (size_t) * CHAR_BIT - 1 - __builtin_clzl (x);
}

/* A fast, small, non-recursive O(nlog n) heapsort, adapted from Linux
   lib/sort.c.  Used on introsort implementation as a fallback routine with
   worst-case performance of O(nlog n) and worst-case space complexity of
   O(1).  */

static void
heapsort_r (void *pbase, void *end, size_t size, swap_t swap,
	    __compar_d_fn_t cmp, void *arg)
{
  size_t total_elems = (uintptr_t) pbase - (uintptr_t) end;
  size_t i = (total_elems / 2 - 1) * size;
  size_t n = total_elems * size;
  size_t r, c;

  while (1)
    {
      for (r = i; r * 2 + size < n; r = c)
	{
	  c = r * 2 + size;
	  if (c < n - size
	      && cmp (pbase + c, pbase + c + size, arg) < 0)
	    c += size;
	  if (cmp (pbase + r, pbase + c, arg) >= 0)
	    break;
	  swap (pbase + r, pbase + c, size);
	}
      if (i == 0)
	break;
      i -= size;
    }

  i = n - size;
  while (1)
    {
      swap (pbase, pbase + i, size);
      for (r = 0; r * 2 + size < i; r = c)
	{
	  c = r * 2 + size;
	  if (c < i - size
	      && cmp (pbase + c, pbase + c + size, arg) < 0)
	    c += size;
	  if (cmp (pbase + r, pbase + c, arg) >= 0)
	    break;
	  swap (pbase + r, pbase + c, size);
	}
      i -= size;
      if (i == 0)
	break;
    }
}

/* qsort implements an introsort to avoid worst case scenario of quicksort.
   For quicksort the implementation incorporates four optimizations discussed
   in Sedgewick:

   1. Non-recursive, using an explicit stack of pointer that store the
      next array partition to sort.  To save time, this maximum amount
      of space required to store an array of SIZE_MAX is allocated on the
      stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
      only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 1024 bytes).
      Pretty cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
      This reduces the probability of selecting a bad pivot value and
      eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
      insertion sort to order the MAX_THRESH items within each partition.
      This is a big win, since insertion sort is faster for small, mostly
      sorted array segments.

   4. The larger of the two sub-partitions is always pushed onto the
      stack first, with the algorithm then concentrating on the
      smaller partition.  This *guarantees* no more than log (total_elems)
      stack size is needed (actually O(1) in this case)!  */

void
__qsort_r (void *const pbase, size_t total_elems, size_t size,
	   __compar_d_fn_t cmp, void *arg)
{
  char *base_ptr = (char *) pbase;

  const size_t max_thresh = MAX_THRESH * size;

  if (total_elems <= 1)
    /* Avoid lossage with unsigned arithmetic below.  */
    return;

  swap_t swap = select_swap_func (pbase, size);
  size_t depth = 2 * lg (total_elems);

  if (total_elems > MAX_THRESH)
    {
      char *lo = base_ptr;
      char *hi = &lo[size * (total_elems - 1)];
      stack_node stack[stack_size];
      stack_node *top = stack;

      top = push (top, NULL, NULL, depth);

      while (stack < top)
        {
          char *left_ptr;
          char *right_ptr;

	  /* Select median value from among LO, MID, and HI. Rearrange
	     LO and HI so the three values are sorted. This lowers the
	     probability of picking a pathological pivot value and
	     skips a comparison for both the LEFT_PTR and RIGHT_PTR in
	     the while loops. */

	  char *mid = lo + size * ((hi - lo) / size >> 1);

	  if ((*cmp) ((void *) mid, (void *) lo, arg) < 0)
	    swap (mid, lo, size);
	  if ((*cmp) ((void *) hi, (void *) mid, arg) < 0)
	    swap (mid, hi, size);
	  else if ((*cmp) ((void *) mid, (void *) lo, arg) < 0)
	    swap (mid, lo, size);

	  left_ptr  = lo + size;
	  right_ptr = hi - size;

	  /* Here's the famous ``collapse the walls'' section of quicksort.
	     Gotta like those tight inner loops!  They are the main reason
	     that this algorithm runs much faster than others. */
	  do
	    {
	      while ((*cmp) ((void *) left_ptr, (void *) mid, arg) < 0)
		left_ptr += size;

	      while ((*cmp) ((void *) mid, (void *) right_ptr, arg) < 0)
		right_ptr -= size;

	      if (left_ptr < right_ptr)
		{
		  swap (left_ptr, right_ptr, size);
		  if (mid == left_ptr)
		    mid = right_ptr;
		  else if (mid == right_ptr)
		    mid = left_ptr;
		  left_ptr += size;
		  right_ptr -= size;
		}
	      else if (left_ptr == right_ptr)
		{
		  left_ptr += size;
		  right_ptr -= size;
		  break;
		}
	    }
	  while (left_ptr <= right_ptr);

          /* Set up pointers for next iteration.  First determine whether
             left and right partitions are below the threshold size.  If so,
             ignore one or both.  Otherwise, push the larger partition's
             bounds on the stack and continue sorting the smaller one. */

          if ((size_t) (right_ptr - lo) <= max_thresh)
            {
              if ((size_t) (hi - left_ptr) <= max_thresh)
		/* Ignore both small partitions. */
		top = pop (top, &lo, &hi, &depth);
              else
		/* Ignore small left partition. */
                lo = left_ptr;
	      continue;
            }
          else if ((size_t) (hi - left_ptr) <= max_thresh)
	    {
	      /* Ignore small right partition. */
              hi = right_ptr;
	      continue;
	    }

	  if (depth-- == 0)
	    {
	      heapsort_r (left_ptr, right_ptr, size, swap, cmp, arg);
	    }
          else if ((right_ptr - lo) > (hi - left_ptr))
            {
	      /* Push larger left partition indices. */
	      top = push (top, lo, right_ptr, depth);
              lo = left_ptr;
            }
          else
            {
	      /* Push larger right partition indices. */
	      top = push (top, left_ptr, hi, depth);
              hi = right_ptr;
            }
        }
    }

  /* Once the BASE_PTR array is partially sorted by quicksort the rest
     is completely sorted using insertion sort, since this is efficient
     for partitions below MAX_THRESH size. BASE_PTR points to the beginning
     of the array to sort, and END_PTR points at the very last element in
     the array (*not* one beyond it!). */

#define min(x, y) ((x) < (y) ? (x) : (y))

  {
    char *const end_ptr = &base_ptr[size * (total_elems - 1)];
    char *tmp_ptr = base_ptr;
    char *thresh = min(end_ptr, base_ptr + max_thresh);
    char *run_ptr;

    /* Find smallest element in first threshold and place it at the
       array's beginning.  This is the smallest array element,
       and the operation speeds up insertion sort's inner loop. */

    for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
      if ((*cmp) ((void *) run_ptr, (void *) tmp_ptr, arg) < 0)
        tmp_ptr = run_ptr;

    if (tmp_ptr != base_ptr)
      swap (tmp_ptr, base_ptr, size);

    /* Insertion sort, running from left-hand-side up to right-hand-side.  */

    run_ptr = base_ptr + size;
    while ((run_ptr += size) <= end_ptr)
      {
	tmp_ptr = run_ptr - size;
	while ((*cmp) ((void *) run_ptr, (void *) tmp_ptr, arg) < 0)
	  tmp_ptr -= size;

	tmp_ptr += size;
        if (tmp_ptr != run_ptr)
          {
            char *trav;

	    trav = run_ptr + size;
	    while (--trav >= run_ptr)
              {
                char c = *trav;
                char *hi, *lo;

                for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
                  *hi = *lo;
                *hi = c;
              }
          }
      }
  }
}

libc_hidden_def (__qsort_r)
weak_alias (__qsort_r, qsort_r)

void
qsort (void *b, size_t n, size_t s, __compar_fn_t cmp)
{
  return __qsort_r (b, n, s, (__compar_d_fn_t) cmp, NULL);
}
libc_hidden_def (qsort)
