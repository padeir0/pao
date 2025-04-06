#include "../src/pao_linearAlloc.h"
#include <stdio.h>
#include <stdlib.h>

#define BUFFSIZE 1024
uint8_t buffer[BUFFSIZE];

void checkres(enum arena_RES r) {
  if (r != arena_OK) {
    printf("ERROR: %s\n", arena_str_res(r));
    abort();
  }
}

void setbuff(uint8_t* obj, size_t size, uint8_t value) {
  size_t i = 0;
  while (i < size) {
    obj[i] = value;
    i++;
  }
}

bool checkbuff(uint8_t* obj, size_t size, uint8_t value) {
  size_t i = 0;
  while (i < size) {
    if (obj[i] != value) {
      return false;
    }
    i++;
  }
  return true;
}

#define BUFFARRAYSIZE 10
uint8_t* buffs[BUFFARRAYSIZE];

int main() {
  enum arena_RES res;
  arena* a = arena_create(buffer, BUFFSIZE, &res);
  uint8_t* buff;
  size_t alloc_size;
  int i;
  checkres(res);

  i = 0;
  while (i < BUFFARRAYSIZE) {
    alloc_size = (i+1)*10;
    buff = (uint8_t*) arena_alloc(a, alloc_size);
    if (buff == NULL) {
      printf("ERROR: out of memory\n");
      abort();
    }
    setbuff(buff, alloc_size, i);
    buffs[i] = buff;
    i++;
  }

  i = 0;
  while (i < BUFFARRAYSIZE) {
    alloc_size = (i+1)*10;
    if (checkbuff(buffs[i], alloc_size, i) == false) {
      printf("ERROR: corrupted memory\n");
      abort();
    }
    i++;
  }

  arena_free_all(a);

  if (arena_empty(a) != true) {
    printf("ERROR: arena is not empty\n");
    abort();
  }

  printf("OK arena\n");
  return 0;
}
