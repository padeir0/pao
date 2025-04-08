#include "../src/pao_linearAlloc.h"
#include <stdio.h>
#include <stdlib.h>

#define PAO_bufferSize 1024
u8 g_pao_buffer[PAO_bufferSize];

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

#define PAO_buffArraySize 10
u8* g_pao_buffs[PAO_buffArraySize];

int main(void) {
  pao_LinearAlloc* a = pao_linearAlloc_create(g_pao_buffer, PAO_bufferSize);
  checkres((void*)a, "failed to create allocator");
  u8* buff;
  usize alloc_size;
  u32 i;

  i = 0;
  while (i < PAO_buffArraySize && i <= U8_MAX) {
    alloc_size = (i+1)*10;
    buff = (uint8_t*) pao_linearAlloc_alloc(a, alloc_size);
    if (buff == NULL) {
      printf("ERROR: out of memory\n");
      abort();
    }
    // UNSAFE: we can cast u32 to u8 because it's max value is enforced on the loop condition
    setbuff(buff, alloc_size, (u8)i);
    g_pao_buffs[i] = buff;
    i++;
  }

  i = 0;
  while (i < PAO_buffArraySize && i <= U8_MAX) {
    alloc_size = (i+1)*10;
    if (checkbuff(g_pao_buffs[i], alloc_size, (u8)i) == false) {
      printf("ERROR: corrupted memory\n");
      abort();
    }
    i++;
  }

  pao_linearAlloc_freeAll(a);

  if (pao_linearAlloc_empty(a) != true) {
    printf("ERROR: arena is not empty\n");
    abort();
  }

  printf("OK: pao_linearAlloc_test\n");
  return 0;
}
