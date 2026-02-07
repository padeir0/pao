/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_basicTypes_H
#define MB_basicTypes_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
// sorry, i got used to small integer names :)

typedef uint8_t  byte;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uintptr_t uptr;
typedef size_t    usize;

const u8 BYTE_MAX = UINT8_MAX;

const u8  U8_MAX  = UINT8_MAX;
const u16 U16_MAX = UINT16_MAX;
const u32 U32_MAX = UINT32_MAX;
const u64 U64_MAX = UINT64_MAX;

const i8  I8_MAX  = INT8_MAX;
const i16 I16_MAX = INT16_MAX;
const i32 I32_MAX = INT32_MAX;
const i64 I64_MAX = INT64_MAX;

const i8  I8_MIN  = INT8_MIN;
const i16 I16_MIN = INT16_MIN;
const i32 I32_MIN = INT32_MIN;
const i64 I64_MIN = INT64_MIN;

const uptr UPTR_MAX  = UINTPTR_MAX;
const usize USIZE_MAX = SIZE_MAX;

#endif
