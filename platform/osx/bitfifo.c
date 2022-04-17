/* bitfifo.c -- fifo which stores bits */

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

//#include "base/utils.h"
/**
 * Divide while rounding upwards.
 */
#define DIVIDE_ROUNDING_UP(n, m) (((n) + (m) - 1) / (m))
/**
 * Return the minimum of (a,b).
 */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))



#include "bitfifo.h"

/* A type to hold a chunk of bits. */
typedef unsigned int bitfifo_T;

#define BITFIFOT_WIDTH     (sizeof(unsigned int) * CHAR_BIT)
#define BITFIFOT_1         1u
#define BITFIFOT_LOG2WIDTH 5

/* memcpy() for bit-sized regions */
static void bitcpy(bitfifo_T       *dst,
                   unsigned int     dstbitoff,
                   const bitfifo_T *src,
                   unsigned int     srcbitoff,
                   size_t           nbits)
{
  size_t    nwords;
  bitfifo_T mask;

  if (nbits == 0)
    return;

  assert(dst);
  assert(dstbitoff < BITFIFOT_WIDTH);
  assert(src);
  assert(srcbitoff < BITFIFOT_WIDTH);

  if (dstbitoff == srcbitoff)
  {
    /* source and destination offsets are aligned: easy case */

    /* align to a word boundary */
    if (dstbitoff)
    {
      mask = ~0u << dstbitoff;
      *dst = (*dst & ~mask) | (*src & mask);
      nbits -= dstbitoff;
    }

    /* copy whole words */
    nwords = nbits >> BITFIFOT_LOG2WIDTH;
    if (nwords)
    {
      memcpy(dst, src, nwords * sizeof(*src));
      dst += nwords;
      src += nwords;
    }

    /* then copy trailing bits */
    nbits &= BITFIFOT_WIDTH - 1;
    if (nbits)
    {
      mask = (BITFIFOT_1 << nbits) - 1;
      *dst = (*dst & ~mask) | (*src & mask);
    }
  }
  else
  {
    /* source and destination offsets are not aligned: hard case */

    /* this processes a single bit per iteration (slow) */
    while (nbits--)
    {
      int          dstmask;
      int          srcmask;
      unsigned int dstbits;
      unsigned int srcbits;

      dstmask = BITFIFOT_1 << dstbitoff;
      srcmask = BITFIFOT_1 << srcbitoff;

      dstbits = *dst & ~dstmask;
      srcbits = *src & srcmask;

      if (srcbitoff > dstbitoff)
        *dst = dstbits | (srcbits >> (srcbitoff - dstbitoff));
      else
        *dst = dstbits | (srcbits << (dstbitoff - srcbitoff));

      dstbitoff = (dstbitoff + 1) & (BITFIFOT_WIDTH - 1); if (dstbitoff == 0) dst++;
      srcbitoff = (srcbitoff + 1) & (BITFIFOT_WIDTH - 1); if (srcbitoff == 0) src++;
    }
  }
}

/* ----------------------------------------------------------------------- */

struct bitfifo
{
  unsigned int head;      /* where to put incoming bits; a bit offset */
  unsigned int tail;      /* where to take outgoing bits from: a bit offset */
  unsigned int nbits;     /* buffer size, in bits (capacity is one bit less) */
  bitfifo_T    buffer[1]; /* sized when allocated */
};

bitfifo_t *bitfifo_create(int nbits)
{
  size_t     bufsz;
  bitfifo_t *fifo;

  /* allocate one more bit than requested so we can represent buffer full correctly */
  nbits++;

  bufsz = offsetof(bitfifo_t, buffer) + DIVIDE_ROUNDING_UP(nbits, BITFIFOT_WIDTH) * sizeof(fifo->buffer[0]);
  fifo = malloc(bufsz);
  if (fifo == NULL)
    return NULL;

  fifo->head  = fifo->tail = 0;
  fifo->nbits = nbits;

  return fifo;
}

void bitfifo_destroy(bitfifo_t *doomed)
{
  free(doomed);
}

/* ----------------------------------------------------------------------- */

result_t bitfifo_enqueue(bitfifo_t       *fifo,
                         const bitfifo_T *newbits,
                         unsigned int     newbitsoffset,
                         size_t           nnewbits)
{
  if (bitfifo_full(fifo))
    return result_BITFIFO_FULL;

  if (fifo->head < fifo->tail)
  {
    /* when head < tail then there's a single free contiguous gap somewhere
     * in the buffer that we can target:
     * e.g. -> head      tail ->
     *             \    /
     *      [######......######]
     */

    /* is the free gap large enough?
     * note: ensure one unused bit before tail */
    if (fifo->tail - 1 - fifo->head < nnewbits)
      return result_BITFIFO_FULL;

    bitcpy(&fifo->buffer[fifo->head >> BITFIFOT_LOG2WIDTH],
           fifo->head & (BITFIFOT_WIDTH - 1),
           newbits,
           newbitsoffset,
           nnewbits);

    fifo->head += nnewbits;
  }
  else
  {
    size_t spareahead;
    size_t sparebehind;
    size_t totalcopied;

    /* when head >= tail then the bits wrap around at the end of the buffer.
     * there can be up to two gaps: one at either end of the buffer:
     * e.g.  tail ---> head
     *           \    /
     *    [......######......]
     * to fill we may have to wrap around at the end of the buffer.
     */

    spareahead  = fifo->nbits - fifo->head;
    sparebehind = fifo->tail - 1; /* ensure one unused bit before tail */

    if (spareahead + sparebehind < nnewbits)
      return result_BITFIFO_FULL;

    totalcopied = 0;

    /* fill from head all the way to the end of the buffer, if needed */
    if (spareahead > 0)
    {
      size_t copy = MIN(nnewbits, spareahead);

      bitcpy(&fifo->buffer[fifo->head >> BITFIFOT_LOG2WIDTH],
             fifo->head & (BITFIFOT_WIDTH - 1),
             newbits,
             newbitsoffset,
             copy);
      nnewbits -= copy;

      newbits += copy >> BITFIFOT_LOG2WIDTH;
      newbitsoffset += copy & (BITFIFOT_WIDTH - 1);
      if (newbitsoffset >= BITFIFOT_WIDTH) /* carried */
      {
        newbitsoffset -= BITFIFOT_WIDTH;
        newbits++;
      }

      totalcopied += copy;
    }

    /* fill from the start of the buffer, if needed */
    if (nnewbits > 0)
    {
      assert(sparebehind >= nnewbits);

      bitcpy(&fifo->buffer[0], 0, newbits, newbitsoffset, nnewbits);

      totalcopied += nnewbits;
    }

    fifo->head = (fifo->head + totalcopied) % fifo->nbits;
  }

  return result_OK;
}

result_t bitfifo_dequeue(bitfifo_t *fifo,
                         bitfifo_T *outbits,
                         size_t     noutbits)
{
  if (bitfifo_empty(fifo))
    return result_BITFIFO_EMPTY;

  if (fifo->head < fifo->tail)
  {
    size_t availabletail;
    size_t availablehead;
    size_t totalcopied;
    int    outbitsoffset = 0;

    /* when head < tail there's two blocks to read from:
     * e.g. -> head      tail ->
     *             \    /
     *      [######......######]
     */

    /* shouldn't need to compensate for the unused bit */
    availabletail = fifo->nbits - fifo->tail;
    availablehead = fifo->head;

    if (availabletail + availablehead < noutbits)
      return result_BITFIFO_INSUFFICIENT;

    totalcopied = 0;

    if (availabletail > 0)
    {
      size_t copy = MIN(noutbits, availabletail);

      bitcpy(outbits,
             0,
             &fifo->buffer[fifo->tail >> BITFIFOT_LOG2WIDTH],
             fifo->tail & (BITFIFOT_WIDTH - 1),
             copy);
      noutbits -= copy;

      outbits += copy >> BITFIFOT_LOG2WIDTH;
      outbitsoffset = copy & (BITFIFOT_WIDTH - 1);

      totalcopied += copy;
    }

    if (noutbits > 0)
    {
      assert(availablehead >= noutbits);

      bitcpy(outbits,
             outbitsoffset,
             &fifo->buffer[fifo->tail >> BITFIFOT_LOG2WIDTH],
             fifo->tail & (BITFIFOT_WIDTH - 1),
             noutbits);

      totalcopied += noutbits;
    }

    fifo->tail = (fifo->tail + totalcopied) % fifo->nbits;
  }
  else
  {
    /* when head >= tail there's a single contiguous block to read from
     * e.g.  tail ---> head
     *           \    /
     *    [......######......]
     */

    if (noutbits > fifo->head - fifo->tail)
      return result_BITFIFO_INSUFFICIENT;

    bitcpy(outbits,
           0,
           &fifo->buffer[fifo->tail >> BITFIFOT_LOG2WIDTH],
           fifo->tail & (BITFIFOT_WIDTH - 1),
           noutbits);

    fifo->tail += noutbits;
  }

  /* if the buffer is completely emptied then reset the head and tail offsets
   * to their initial values */
  if (fifo->tail == fifo->head)
    fifo->head = fifo->tail = 0;

  return result_OK;
}

void bitfifo_clear(bitfifo_t *fifo)
{
  fifo->head = fifo->tail = 0;
}

/* ----------------------------------------------------------------------- */

size_t bitfifo_used(const bitfifo_t *fifo)
{
  if (fifo->head >= fifo->tail)
    return fifo->head - fifo->tail;
  else
    return fifo->head + fifo->nbits - fifo->tail;
}

int bitfifo_full(const bitfifo_t *fifo)
{
  /* the fifo is full if (head + 1) == tail */
  return (fifo->head + 1) % fifo->nbits == fifo->tail;
}

int bitfifo_empty(const bitfifo_t *fifo)
{
  /* fifo is empty if head == tail */
  return fifo->head == fifo->tail;
}

// vim: ts=8 sts=2 sw=2 et
