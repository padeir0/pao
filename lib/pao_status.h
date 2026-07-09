/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_status_H
#define PAO_status_H

typedef enum {
  PAO_status_ok,
  PAO_status_divisionByZero,
  PAO_status_outOfMemory,
  PAO_status_naturalNumberOverflow,
  PAO_status_bufferTooSmall,
  PAO_status_invalidUTF8,
  PAO_status_eof,
  PAO_status_outOfBounds,
  /* indicates the pointer to be freed is unaligned with chunk boundaries */
  PAO_status_badAlignment,
  /* indicates given chunk size is too small (minimum is sizeof(node)) */
  PAO_status_badSize, 
  /* indicates given buffer is too small (minimum is sizeof(pool) + chunksize) */
  /* indicates the given buffer is null */
  PAO_status_nullBuffer,
  PAO_status_failedFree
} pao_Status;

#define PAO_status_check if (st != PAO_status_ok) { return st; }

#endif
