/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_STATUS_H
#define PAO_STATUS_H

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
  status_FAILEDFREE,
  /* used in the lexer: reasons for an invalid token */
  status_BADCHAR,
  status_BADESCAPE,
  status_UNTERMINATEDSTRING,
  status_BADNUMBER,
  /* used in the literals: the number is well formed, but there is nothing yet
     to hold its value (a decimal number, or a float that does not fit) */
  status_UNSUPPORTEDNUMBER,
  /* used in the parser: the reasons for a source that is lexically valid
     but is not a program. */
  status_UNEXPECTEDTOKEN,
  status_BADINDENT,
  status_BADDEDENT,
  status_MAXDEPTH
} Status;

#define status_CHECK if (st != status_OK) { return st; }

#endif
