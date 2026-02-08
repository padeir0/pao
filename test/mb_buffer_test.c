#include "../src/mb_buffer.h"
#include "common.h"

void _writeOrBurst(mb_Buffer* buff, char* strLit) {
  usize written = mb_buffer_writeLiteral(buff, strLit);
  if (written == 0) {
    printf("wrote zero bytes (%s)\n", strLit);
    abort();
  }
}

byte g_mb_memory1[DEFAULT_SIZE];
byte g_mb_memory2[DEFAULT_SIZE];

// tests if unwritten buffers are equal.
bool test_buffer_equals_1(void) {
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);
  mb_Buffer buff2 = mb_buffer_create(g_mb_memory2, DEFAULT_SIZE);
  return mb_buffer_equals(&buff1, &buff2);
}

bool test_buffer_equals_2(void) {
  char* s1 = "Hello World!\n";
  char* s2 = "World!\n";
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);
  mb_Buffer buff2 = mb_buffer_create(g_mb_memory2, DEFAULT_SIZE);

  usize written = mb_buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }
  written = mb_buffer_writeLiteral(&buff2, s2);
  if (written == 0) {
    return false;
  }

  byte b = 0;
  while (mb_buffer_readByte(&buff1, &b) && b != ' ');

  return mb_buffer_equals(&buff1, &buff2);
}

bool test_buffer_reset(void) {
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);
  mb_Buffer buff2 = mb_buffer_create(g_mb_memory2, DEFAULT_SIZE);

  char* s1 = "Hello World!\n";
  usize written = mb_buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }
  mb_buffer_reset(&buff1);

  return mb_buffer_equals(&buff1, &buff2);
}

bool test_buffer_hasSpace(void) {
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, 1);
  if (mb_buffer_hasSpace(&buff1, 1) == false) {
    return false;
  }
  usize written = mb_buffer_writeByte(&buff1, 'o');
  if (written != 1) {
    return false;
  }
  if (mb_buffer_hasSpace(&buff1, 1) == true) {
    return false;
  }
  if (mb_buffer_hasSpace(&buff1, 0) == false) {
    return false;
  }
  mb_buffer_reset(&buff1);
  if (mb_buffer_hasSpace(&buff1, 1) == false) {
    return false;
  }
  return true;
}

bool test_buffer_bzero(void) {
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);
  char* s1 = "Hello World!\n";
  usize written = mb_buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }

  mb_buffer_bzero(&buff1);

  usize i = 0;
  while (i < buff1.len) {
    if (buff1.ptr[i] != 0) {
      return false;
    }
    i++;
  }

  return true;
}

bool test_buffer_writeLiteral_1(void) {
  char* s = "Hello\n";
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);
  usize written = mb_buffer_writeLiteral(&buff1, s);
  if (written == 0) {
    return false;
  }

  return strncmp((char*)buff1.ptr, s, buff1.len) == 0 ;
}

bool test_buffer_writeLiteral_2(void) {
  char* s1 = "Hello ";
  char* s2 = "World!\n";
  char* s3 = "Hello World!\n";
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);

  usize written = mb_buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }
  written = mb_buffer_writeLiteral(&buff1, s2);
  if (written == 0) {
    return false;
  }
  return strncmp((char*)buff1.ptr, s3, buff1.len) == 0 ;
}

bool test_buffer_writeString_1(void) {
  char* s = "Hello";
  usize length = strlen(s);
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);
  usize written = mb_buffer_writeString(&buff1, s, length);
  if (written < length) {
    return false;
  }
  return strncmp((char*)buff1.ptr, s, length) == 0 ;
}

bool test_buffer_writeString_2(void) {
  char* s = "Hello";
  usize length = strlen(s);
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);
  mb_Buffer buff2 = mb_buffer_create(g_mb_memory2, DEFAULT_SIZE);

  usize written = mb_buffer_writeString(&buff1, s, length);
  if (written < length) {
    return false;
  }
  written = mb_buffer_writeLiteral(&buff2, s);
  if (written == 0) {
    return false;
  }

  return mb_buffer_equals(&buff1, &buff2);
}

bool test_buffer_readWriteByte_1(void) {
  const byte C = 'c';
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);

  usize written = mb_buffer_writeByte(&buff1, C);
  if (written != 1) {
    return false;
  }
  byte out;
  bool ok = mb_buffer_readByte(&buff1, &out);
  if (ok == false) {
    return false;
  }

  return C == out;
}

bool test_buffer_readWriteByte_2(void) {
  char* s = "this is spectacular!\n";
  usize length = strlen(s);
  usize i;
  mb_Buffer buff1 = mb_buffer_create(g_mb_memory1, DEFAULT_SIZE);

  i = 0;
  while (i < length) {
    usize written = mb_buffer_writeByte(&buff1, (byte)s[i]);
    if (written != 1) {
      return false;
    }
    i++;
  }

  i = 0;
  while (i < length) {
    byte out;
    bool ok = mb_buffer_readByte(&buff1, &out);
    if (ok == false) {
      return false;
    }

    if (out != (byte)s[i]) {
      return false;
    }
    i++;
  }
  
  return true;
}

Tester tests[] = {
  {"test_buffer_equals_1", test_buffer_equals_1},
  {"test_buffer_equals_2", test_buffer_equals_2},
  {"test_buffer_reset", test_buffer_reset},
  {"test_buffer_hasSpace", test_buffer_hasSpace},
  {"test_buffer_bzero", test_buffer_bzero},
  {"test_buffer_readWriteByte_1", test_buffer_readWriteByte_1},
  {"test_buffer_readWriteByte_2", test_buffer_readWriteByte_2},
  {"test_buffer_writeLiteral_1", test_buffer_writeLiteral_1},
  {"test_buffer_writeLiteral_2", test_buffer_writeLiteral_2},
  {"test_buffer_writeString_1", test_buffer_writeString_1},
  {"test_buffer_writeString_2", test_buffer_writeString_2},
};

#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  run_tests(tests, TEST_LEN);
}
