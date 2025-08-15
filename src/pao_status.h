/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_status_H
#define PAO_status_H

typedef enum __attribute__((__packed__)) {
  PAO_status_ok,
  PAO_status_divisionByZero,
  PAO_status_outOfMemory,
  PAO_status_invalidDigit,
  PAO_status_naturalNumberOverflow,
} pao_status;

#endif
