/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "mb_basicTypes.h"

#ifndef MB_util_H
#define MB_util_H

u32 mb_util_minU32(u32 a, u32 b) {
  if (a <= b) {
    return a;
  }
  return b;
}

u32 mb_util_maxU32(u32 a, u32 b) {
  if (a <= b) {
    return b;
  }
  return a;
}

#endif
