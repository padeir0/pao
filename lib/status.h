/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_status_H
#define PAO_status_H

typedef enum {
  status_OK,
  status_DIVISIONBYZERO,
  status_OUTOFMEMORY,
  status_NATURALNUMBEROVERFLOW,
  status_BUFFERTOOSMALL,
  status_INVALIDUTF8,
  status_EOF,
  status_OUTOFBOUNDS,
  /* indicates some pointer is badly aligned (not necessarily about word boundaries) */
  status_BADALIGNMENT,
  status_BADSIZE, 
  status_NULLBUFFER,
  /* used in the allocator interface*/
  status_FAILEDFREE
} Status;

#define status_CHECK if (st != status_OK) { return st; }

#endif
