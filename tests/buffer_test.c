#include "../lib/buffer.h"
#include "common.h"

void _writeOrBurst(Buffer* buff, char* strLit) {
  usize written = buffer_writeLiteral(buff, strLit);
  if (written == 0) {
    printf("wrote zero bytes (%s)\n", strLit);
    abort();
  }
}

byte g_buffer_memory1[DEFAULT_SIZE];
byte g_buffer_memory2[DEFAULT_SIZE];

// tests if unwritten buffers are equal.
bool test_buffer_equals_1(void) {
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer buff2 = buffer_create(g_buffer_memory2, DEFAULT_SIZE);
  return buffer_equals(&buff1, &buff2);
}

bool test_buffer_equals_2(void) {
  char* s1 = "Hello World!\n";
  char* s2 = "World!\n";
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer buff2 = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  usize written = buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }
  written = buffer_writeLiteral(&buff2, s2);
  if (written == 0) {
    return false;
  }

  byte b = 0;
  while (buffer_readByte(&buff1, &b) && b != ' ');

  return buffer_equals(&buff1, &buff2);
}

bool test_buffer_reset(void) {
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer buff2 = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  char* s1 = "Hello World!\n";
  usize written = buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }
  buffer_reset(&buff1);

  return buffer_equals(&buff1, &buff2);
}

bool test_buffer_hasSpace(void) {
  Buffer buff1 = buffer_create(g_buffer_memory1, 1);
  if (buffer_hasSpace(&buff1, 1) == false) {
    return false;
  }
  usize written = buffer_writeByte(&buff1, 'o');
  if (written != 1) {
    return false;
  }
  if (buffer_hasSpace(&buff1, 1) == true) {
    return false;
  }
  if (buffer_hasSpace(&buff1, 0) == false) {
    return false;
  }
  buffer_reset(&buff1);
  if (buffer_hasSpace(&buff1, 1) == false) {
    return false;
  }
  return true;
}

bool test_buffer_bzero(void) {
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  char* s1 = "Hello World!\n";
  usize written = buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }

  buffer_bzero(&buff1);

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
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  usize written = buffer_writeLiteral(&buff1, s);
  if (written == 0) {
    return false;
  }

  return strncmp((char*)buff1.ptr, s, buff1.len) == 0 ;
}

bool test_buffer_writeLiteral_2(void) {
  char* s1 = "Hello ";
  char* s2 = "World!\n";
  char* s3 = "Hello World!\n";
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);

  usize written = buffer_writeLiteral(&buff1, s1);
  if (written == 0) {
    return false;
  }
  written = buffer_writeLiteral(&buff1, s2);
  if (written == 0) {
    return false;
  }
  return strncmp((char*)buff1.ptr, s3, buff1.len) == 0 ;
}

bool test_buffer_writeString_1(void) {
  char* s = "Hello";
  usize length = strlen(s);
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  usize written = buffer_writeString(&buff1, s, length);
  if (written < length) {
    return false;
  }
  return strncmp((char*)buff1.ptr, s, length) == 0 ;
}

bool test_buffer_writeString_2(void) {
  char* s = "Hello";
  usize length = strlen(s);
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer buff2 = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  usize written = buffer_writeString(&buff1, s, length);
  if (written < length) {
    return false;
  }
  written = buffer_writeLiteral(&buff2, s);
  if (written == 0) {
    return false;
  }

  return buffer_equals(&buff1, &buff2);
}

bool test_buffer_readWriteByte_1(void) {
  const byte C = 'c';
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);

  usize written = buffer_writeByte(&buff1, C);
  if (written != 1) {
    return false;
  }
  byte out;
  bool ok = buffer_readByte(&buff1, &out);
  if (ok == false) {
    return false;
  }

  return C == out;
}

bool test_buffer_readWriteByte_2(void) {
  char* s = "this is spectacular!\n";
  usize length = strlen(s);
  usize i;
  Buffer buff1 = buffer_create(g_buffer_memory1, DEFAULT_SIZE);

  i = 0;
  while (i < length) {
    usize written = buffer_writeByte(&buff1, (byte)s[i]);
    if (written != 1) {
      return false;
    }
    i++;
  }

  i = 0;
  while (i < length) {
    byte out;
    bool ok = buffer_readByte(&buff1, &out);
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
  run_tests("buffer", tests, TEST_LEN);
}
