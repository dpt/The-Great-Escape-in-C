/* Types.h
 *
 * ISO C99 compatibility for The Great Esape.
 *
 * Copyright (c) David Thomas, 2020. <dave@davespace.co.uk>
 */

#ifndef C99_TYPES_H
#define C99_TYPES_H

#if __STDC_VERSION__ >= 199901 || defined(_MSC_VER)
#include <stdint.h>
#else
typedef signed   char  int8_t;
typedef unsigned char  uint8_t;
typedef signed   short int16_t;
typedef unsigned short uint16_t;
typedef signed   int   int32_t;
typedef unsigned int   uint32_t;
typedef signed   int   intptr_t;
typedef unsigned int   uintptr_t;
#endif

#endif /* C99_TYPES_H */

// vim: ts=8 sts=2 sw=2 et
