/* bitfifo.h -- fifo which stores bits */

/**
 * \file bitfifo.h
 *
 * A fifo which stores bits.
 */

#ifndef DATASTRUCT_BITFIFO_H
#define DATASTRUCT_BITFIFO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

//#include "base/result.h"
typedef int result_t;
#define result_OK 0

/* ----------------------------------------------------------------------- */

#define result_BASE_BITFIFO 0x495845

#define result_BITFIFO_EMPTY        (result_BASE_BITFIFO + 0)
#define result_BITFIFO_FULL         (result_BASE_BITFIFO + 1)
#define result_BITFIFO_INSUFFICIENT (result_BASE_BITFIFO + 2)

/* ----------------------------------------------------------------------- */

#define T bitfifo_t

/**
 * A bit fifo's type.
 */
typedef struct bitfifo T;

/* ----------------------------------------------------------------------- */

/**
 * Create a bitfifo.
 *
 * \param nbits Minimum number of bits that the bitfifo should store.
 *
 * \return New bitfifo or NULL if OOM.
 */
bitfifo_t *bitfifo_create(int nbits);

/**
 * Destroy a bitfifo.
 *
 * \param doomed bitfifo to destroy.
 */
void bitfifo_destroy(T *doomed);

/* ----------------------------------------------------------------------- */

/* writes bits to 'head' onwards, wrapping around if required */
/* fifo will reject attempts to store more bits than there is space for */
result_t bitfifo_enqueue(T            *fifo,
                         const unsigned int *newbits,
                         unsigned int  newbitsoffset,
                         size_t        nnewbits);

/* reads bits from 'tail' onwards, wrapping around if required */
result_t bitfifo_dequeue(T            *fifo,
                         unsigned int *outbits,
                         size_t        noutbits);

/* ----------------------------------------------------------------------- */

/* empty the specified fifo */
void bitfifo_clear(T *fifo);

/* returns number of used bits in the fifo */
size_t bitfifo_used(const T *fifo);

/* returns non-zero if the fifo is full */
int bitfifo_full(const T *fifo);

/* returns non-zero if the fifo is empty */
int bitfifo_empty(const T *fifo);

/* ----------------------------------------------------------------------- */

#undef T

/* ----------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* DATASTRUCT_BITFIFO_H */

// vim: ts=8 sts=2 sw=2 et
