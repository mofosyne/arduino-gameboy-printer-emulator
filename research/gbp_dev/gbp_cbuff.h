/*
  # Gameboy Printer Circular Buffer
  * Author:  Brian Khuu (2020-08-09)
  * Licence: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  * Purpose: Circular buffer

  ******************************************************************************

*/
#ifndef GBP_CBUFF_H
#define GBP_CBUFF_H
/******************************************************************************/
// # Circular Byte Buffer For Embedded Applications (Index Based)
// Author: Brian Khuu (July 2020) (briankhuu.com) (mofosyne@gmail.com)
// This Gist (Pointer): https://gist.github.com/mofosyne/d7a4a8d6a567133561c18aaddfd82e6f
// This Gist (Index): https://gist.github.com/mofosyne/82020d5c0e1e11af0eb9b05c73734956
#include <stdint.h> // uint8_t
#include <stddef.h> // size_t
#include <stdbool.h> // bool

typedef struct gpb_cbuff_t
{
  size_t capacity; ///< Maximum number of items in the buffer
  size_t count;    ///< Number of items in the buffer
  uint8_t *buffer; ///< Data Buffer
  size_t head;     ///< Head Index
  size_t tail;     ///< Tail Index

#ifdef FEATURE_CHECKSUM_SUPPORTED
  // Temp
  size_t countTemp;    ///< Number of items in the buffer
  size_t headTemp;     ///< Head Index
#endif // FEATURE_CHECKSUM_SUPPORTED
} gpb_cbuff_t;

static inline bool gpb_cbuff_Init(gpb_cbuff_t *cb, size_t capacity, uint8_t *buffPtr)
{
  gpb_cbuff_t emptyCB = {0};
  if ((cb == NULL) || (buffPtr == NULL))
    return false; ///< Failed
  // Init Struct
  *cb = emptyCB;
  cb->capacity = capacity;
  cb->buffer   = buffPtr;
  cb->count    = 0;
  cb->head     = 0;
  cb->tail     = 0;
  return true; ///< Successful
}

static inline bool gpb_cbuff_Reset(gpb_cbuff_t *cb)
{
  cb->count = 0;
  cb->head = 0;
  cb->tail = 0;
  return true; ///< Successful
}

static inline bool gpb_cbuff_Enqueue(gpb_cbuff_t *cb, uint8_t b)
{
  // Full
  if (cb->count >= cb->capacity)
    return false; ///< Failed
  // Push value
  cb->buffer[cb->head] = b;
  // Increment head
  cb->head = (cb->head + 1) % cb->capacity;
  cb->count = cb->count + 1;
  return true; ///< Successful
}

static inline bool gpb_cbuff_Dequeue(gpb_cbuff_t *cb, uint8_t *b)
{
  // Empty
  if (cb->count == 0)
    return false; ///< Failed
  // Pop value
  *b = cb->buffer[cb->tail];
  // Increment tail
  cb->tail = (cb->tail + 1) % cb->capacity;
  cb->count = cb->count - 1;
  return true; ///< Successful
}

static inline bool gpb_cbuff_Dequeue_Peek(gpb_cbuff_t *cb, uint8_t *b, uint32_t offset)
{
  // Empty
  if (cb->count == 0)
    return false; ///< Failed
  if (cb->count < offset)
    return false; ///< Failed
  // Pop value
  *b = cb->buffer[(cb->tail + offset) % cb->capacity];
  return true; ///< Successful
}

static inline size_t gpb_cbuff_Capacity(gpb_cbuff_t *cb) { return cb->capacity;}
static inline size_t gpb_cbuff_Count(gpb_cbuff_t *cb)   { return cb->count;}
static inline bool gpb_cbuff_IsFull(gpb_cbuff_t *cb)    { return (cb->count >= cb->capacity);}
static inline bool gpb_cbuff_IsEmpty(gpb_cbuff_t *cb)   { return (cb->count == 0);}

#ifdef FEATURE_CHECKSUM_SUPPORTED
/* Temp Enqeue */
static inline bool gpb_cbuff_ResetTemp(gpb_cbuff_t *cb)
{
  cb->countTemp = 0;
  cb->headTemp  = cb->head ;
}

static inline bool gpb_cbuff_AcceptTemp(gpb_cbuff_t *cb)
{
  cb->count += cb->countTemp;
  cb->head  = cb->headTemp  ;
  return true; ///< Successful
}

static inline bool gpb_cbuff_EnqueueTemp(gpb_cbuff_t *cb, uint8_t b)
{
  // Full
  if (cb->countTemp >= (cb->capacity - cb->count))
    return false; ///< Failed
  // Push value
  cb->buffer[cb->headTemp] = b;
  // Increment headTemp
  cb->headTemp = (cb->headTemp + 1) % cb->capacity;
  cb->countTemp = cb->countTemp + 1;
  return true; ///< Successful
}
#else
#define gpb_cbuff_EnqueueTemp(CB, B) gpb_cbuff_Enqueue(CB, B)
#endif // FEATURE_CHECKSUM_SUPPORTED

#endif // GBP_CBUFF_H