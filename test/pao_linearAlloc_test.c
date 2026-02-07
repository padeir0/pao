#include "../src/mb_linearAlloc.h"
#include <stdio.h>
#include <stdlib.h>

#define MB_bufferSize 1024
u8 g_mb_buffer[MB_bufferSize];

void checkres(void* obj, char* s) {
  if (obj == NULL) {
    printf("ERROR: %s\n", s);
    abort();
  }
}

void setbuff(u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    obj[i] = value;
    i++;
  }
}

bool checkbuff(u8* obj, usize size, u8 value) {
  size_t i = 0;
  while (i < size) {
    if (obj[i] != value) {
      return false;
    }
    i++;
  }
  return true;
}

#define MB_buffArraySize 10
u8* g_mb_buffs[MB_buffArraySize];

int main(void) {
  mb_LinearAlloc* a = mb_linearAlloc_create(g_mb_buffer, MB_bufferSize);
  checkres((void*)a, "failed to create allocator");
  u8* buff;
  usize alloc_size;
  u32 i;

  i = 0;
  while (i < MB_buffArraySize && i <= U8_MAX) {
    alloc_size = (i+1)*10;
    buff = (uint8_t*) mb_linearAlloc_alloc(a, alloc_size);
    if (buff == NULL) {
      printf("ERROR: out of memory\n");
      abort();
    }
    // UNSAFE: we can cast u32 to u8 because it's max value is enforced on the loop condition
    setbuff(buff, alloc_size, (u8)i);
    g_mb_buffs[i] = buff;
    i++;
  }

  i = 0;
  while (i < MB_buffArraySize && i <= U8_MAX) {
    alloc_size = (i+1)*10;
    if (checkbuff(g_mb_buffs[i], alloc_size, (u8)i) == false) {
      printf("ERROR: corrupted memory\n");
      abort();
    }
    i++;
  }

  mb_linearAlloc_freeAll(a);

  if (mb_linearAlloc_empty(a) != true) {
    printf("ERROR: arena is not empty\n");
    abort();
  }

  printf("OK: mb_linearAlloc_test\n");
  return 0;
}
