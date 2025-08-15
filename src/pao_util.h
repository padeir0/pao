/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "pao_basicTypes.h"

#ifndef PAO_util_H
#define PAO_util_H

u32 pao_util_minU32(u32 a, u32 b) {
  if (a <= b) {
    return a;
  }
  return b;
}

u32 pao_util_maxU32(u32 a, u32 b) {
  if (a <= b) {
    return b;
  }
  return a;
}

#endif
