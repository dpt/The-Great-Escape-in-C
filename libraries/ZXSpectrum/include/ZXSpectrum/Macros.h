/* Macros.h
 *
 * Utility macros.
 *
 * Copyright (c) David Thomas, 2018. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_MACROS_H
#define ZXSPECTRUM_MACROS_H

/**
 * Return the minimum of (a,b).
 */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/**
 * Return the maximum of (a,b).
 */
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/**
 * Return 'a' clamped to the range [b..c].
 */
#define CLAMP(a,b,c) MIN(MAX(a,b),c)

#endif /* ZXSPECTRUM_MACROS_H */

