/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_status_H
#define MB_status_H

typedef enum __attribute__((__packed__)) {
  MB_status_ok,
  MB_status_divisionByZero,
  MB_status_outOfMemory,
  MB_status_invalidDigit,
  MB_status_naturalNumberOverflow,
} mb_status;

#endif
