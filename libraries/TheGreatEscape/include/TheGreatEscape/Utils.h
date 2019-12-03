/**
 * Utils.h
 *
 * This file is part of "The Great Escape in C".
 *
 * This project recreates the 48K ZX Spectrum version of the prison escape
 * game "The Great Escape" in portable C code. It is free software provided
 * without warranty in the interests of education and software preservation.
 *
 * "The Great Escape" was created by Denton Designs and published in 1986 by
 * Ocean Software Limited.
 *
 * The original game is copyright (c) 1986 Ocean Software Ltd.
 * The original game design is copyright (c) 1986 Denton Designs Ltd.
 * The recreated version is copyright (c) 2012-2019 David Thomas
 */

#ifndef UTILS_H
#define UTILS_H

/* ----------------------------------------------------------------------- */

/** Used to specify variable-length arrays. */
#define UNKNOWN 1

#define NELEMS(a) ((int) (sizeof(a) / sizeof(a[0])))

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/**
 * Given a pointer p, its parent structure type t and structure member name f,
 * returns the parent structure.
 */
#define structof(p,t,f) ((t*)((uintptr_t)(p) - offsetof(t,f)))

/* ----------------------------------------------------------------------- */

/**
 * The NEVER_RETURNS macro is placed after calls which are not expected to
 * return (calls which ultimately invoke squash_stack_goto_main()).
 */
#define NEVER_RETURNS assert("Unexpected return." == NULL); return

/* ----------------------------------------------------------------------- */

/* Z80 instruction simulator macros. */

/**
 * Shift left arithmetic.
 */
#define SLA(r)      \
do {                \
  carry = (r) >> 7; \
  (r) <<= 1;        \
} while (0)

/**
 * Shift right logical.
 */
#define SRL(r)      \
do {                \
  carry = (r) & 1;  \
  (r) >>= 1;        \
} while (0)

/**
 * Rotate left.
 */
#define RL(r)                 \
do {                          \
  int carry_out;              \
                              \
  carry_out = (r) >> 7;       \
  (r) = ((r) << 1) | (carry); \
  carry = carry_out;          \
} while (0)

/**
 * Rotate right through carry.
 */
#define RR(r)                       \
do {                                \
  int carry_out;                    \
                                    \
  carry_out = (r) & 1;              \
  (r) = ((r) >> 1) | (carry << 7);  \
  carry = carry_out;                \
} while (0)

/**
 * Rotate right.
 */
#define RRC(r)                      \
do {                                \
  carry = (r) & 1;                  \
  (r) = ((r) >> 1) | (carry << 7);  \
} while (0)

/**
 * Swap two variables (replacing EX/EXX).
 */
#define SWAP(T,a,b) \
do {                \
  T tmp = a;        \
  a = b;            \
  b = tmp;          \
} while (0)

/* ----------------------------------------------------------------------- */

#endif /* UTILS_H */

// vim: ts=8 sts=2 sw=2 et
