/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_status_H
#define MB_status_H

typedef enum {
  MB_status_ok,
  MB_status_divisionByZero,
  MB_status_outOfMemory,
  MB_status_naturalNumberOverflow,
  MB_status_bufferTooSmall,
  MB_status_invalidUTF8,
  MB_status_eof,
} mb_Status;

#define MB_status_check if (st != MB_status_ok) { return st; }

#endif
