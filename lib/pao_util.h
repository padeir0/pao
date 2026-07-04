/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "pao_basicTypes.h"

#ifndef PAO_util_H
#define PAO_util_H

static inline
u32 pao_util_minU32(u32 a, u32 b) {
  if (a <= b) {
    return a;
  }
  return b;
}

static inline
u32 pao_util_maxU32(u32 a, u32 b) {
  if (a <= b) {
    return b;
  }
  return a;
}

static inline
i64 pao_util_absI64(i64 a) {
  if (a < 0) {
    return -a;
  } else {
    return a;
  }
}

#endif
