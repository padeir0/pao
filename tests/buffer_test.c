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

// each byte becomes 2 hex digits followed by a space
bool test_buffer_toHex_1(void) {
  byte in_bytes[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; /* "Hello" */
  Buffer in = buffer_create(in_bytes, sizeof(in_bytes));
  in.len = sizeof(in_bytes);
  Buffer out = buffer_create(g_buffer_memory1, DEFAULT_SIZE);

  usize written = buffer_toHex(&in, &out);
  char* expected = "48 65 6C 6C 6F ";

  if (written != strlen(expected)) {
    return false;
  }
  return strncmp((char*)out.ptr, expected, written) == 0;
}

// an empty input must write nothing and touch neither buffer's contents
bool test_buffer_toHex_empty(void) {
  Buffer in = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  usize written = buffer_toHex(&in, &out);
  return written == 0 && out.len == 0;
}

// out buffer only has room for whole "XX " groups; a group that
// doesn't fully fit must not be partially written
bool test_buffer_toHex_outOfSpace(void) {
  byte in_bytes[] = {0xAB, 0xCD, 0xEF};
  Buffer in = buffer_create(in_bytes, sizeof(in_bytes));
  in.len = sizeof(in_bytes);
  /* room for exactly 2 groups ("AB CD "), not the 3rd */
  Buffer out = buffer_create(g_buffer_memory1, 6);

  usize written = buffer_toHex(&in, &out);
  char* expected = "AB CD ";

  if (written != strlen(expected)) {
    return false; /* NOTE(1) */
  }
  return strncmp((char*)out.ptr, expected, written) == 0;
  /*
   * NOTE(1): buffer_hasSpace gates each iteration on a full
   *          charsPerByte group, so a buffer with room for exactly
   *          2 groups must stop after the 2nd, not emit a truncated
   *          3rd group.
   */
}

// buffer_toHex appends after whatever is already in `out`
bool test_buffer_toHex_appends(void) {
  byte in_bytes[] = {0xFF};
  Buffer in = buffer_create(in_bytes, sizeof(in_bytes));
  in.len = sizeof(in_bytes);
  Buffer out = buffer_create(g_buffer_memory1, DEFAULT_SIZE);

  _writeOrBurst(&out, "prefix:");
  usize written = buffer_toHex(&in, &out);
  char* expected = "prefix:FF ";

  if (written != 3) {
    return false;
  }
  return strncmp((char*)out.ptr, expected, out.len) == 0;
}

/* copies all bytes when output has room */
bool test_buffer_copy_full(void) {
  char* s = "Hello";
  Buffer in = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  buffer_writeLiteral(&in, s);
  usize copied = buffer_copy(&in, &out);

  return copied == out.len &&
         copied == in.len &&
         strncmp((char*)out.ptr, s, copied) == 0;
}

/* output smaller than input: only copy what fits */
bool test_buffer_copy_outSmaller(void) {
  char* s = "Hello";
  Buffer in = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, 3);

  buffer_writeLiteral(&in, s);
  usize copied = buffer_copy(&in, &out);

  return copied == 3 && strncmp((char*)out.ptr, "Hel", 3) == 0;
}

/* copying from empty input writes nothing */
bool test_buffer_copy_empty(void) {
  Buffer in  = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  usize copied = buffer_copy(&in, &out);
  return copied == 0 && out.len == 0;
}

/* buffer_copySome(len) == min(len, in->len, available) */
bool test_buffer_copySome_exact(void) {
  char* s = "Hello";
  Buffer in  = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  buffer_writeLiteral(&in, s);
  usize copied = buffer_copySome(&in, &out, 3);

  return copied == 3 && strncmp((char*)out.ptr, "Hel", 3) == 0;
}

/* requesting more than in->len copies only in->len */
bool test_buffer_copySome_overRequest(void) {
  char* s = "Hi";
  Buffer in  = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  buffer_writeLiteral(&in, s);
  usize copied = buffer_copySome(&in, &out, 999);

  return copied == 2 && strncmp((char*)out.ptr, "Hi", 2) == 0;
}

/* requesting 0 bytes copies nothing */
bool test_buffer_copySome_zero(void) {
  char* s = "Hello";
  Buffer in  = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  buffer_writeLiteral(&in, s);
  usize copied = buffer_copySome(&in, &out, 0);

  return copied == 0 && out.len == 0;
}

/* copy starts at in->start, not in->ptr */
bool test_buffer_copy_afterRead(void) {
  char* s = "Hello";
  Buffer in  = buffer_create(g_buffer_memory1, DEFAULT_SIZE);
  Buffer out = buffer_create(g_buffer_memory2, DEFAULT_SIZE);

  buffer_writeLiteral(&in, s);
  /* consume "Hel", leaving "lo" readable */
  byte b;
  buffer_readByte(&in, &b);
  buffer_readByte(&in, &b);
  buffer_readByte(&in, &b);

  usize copied = buffer_copy(&in, &out);

  return copied == 2 && strncmp((char*)out.ptr, "lo", 2) == 0;
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
  {"test_buffer_toHex_1", test_buffer_toHex_1},
  {"test_buffer_toHex_empty", test_buffer_toHex_empty},
  {"test_buffer_toHex_outOfSpace", test_buffer_toHex_outOfSpace},
  {"test_buffer_toHex_appends", test_buffer_toHex_appends},
  {"test_buffer_copy_full", test_buffer_copy_full},
  {"test_buffer_copy_outSmaller", test_buffer_copy_outSmaller},
  {"test_buffer_copy_empty", test_buffer_copy_empty},
  {"test_buffer_copySome_exact", test_buffer_copySome_exact},
  {"test_buffer_copySome_overRequest", test_buffer_copySome_overRequest},
  {"test_buffer_copySome_zero", test_buffer_copySome_zero},
  {"test_buffer_copy_afterRead", test_buffer_copy_afterRead},
};

#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  run_tests("buffer", tests, TEST_LEN);
}
