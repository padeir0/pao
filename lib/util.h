/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_UTIL_H
#define PAO_UTIL_H

#include "basicTypes.h"

static inline
u32 util_minU32(u32 a, u32 b) {
  if (a <= b) {
    return a;
  }
  return b;
}

static inline
u32 util_maxU32(u32 a, u32 b) {
  if (a <= b) {
    return b;
  }
  return a;
}

static inline
i32 util_absI32(i32 a) {
  if (a < 0) {
    return -a;
  } else {
    return a;
  }
}

static inline
i64 util_absI64(i64 a) {
  if (a < 0) {
    return -a;
  } else {
    return a;
  }
}

static inline
uptr util_distanceU8Ptr(u8* a, u8* b) {
  if (a > b) {
    return (uptr)a-(uptr)b;
  } else {
    return (uptr)b-(uptr)a;
  }
}
#endif
