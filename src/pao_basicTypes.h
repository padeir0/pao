/*
Copyright 2025 Artur Iure Vianna Fernandes

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PAO_basicTypes_H
#define PAO_basicTypes_H

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
