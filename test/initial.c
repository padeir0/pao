#include "../src/pao_buffer.h"

#define I_PAO_memoryLength 2048
byte g_pao_memory1[I_PAO_memoryLength];
byte g_pao_memory2[I_PAO_memoryLength];

void writeOrBurst(pao_Buffer* buff, char* strLit) {
  usize written = pao_buffer_writeLiteral(buff, strLit);
  if (written == 0) {
    printf("wrote zero bytes (%s)\n", strLit);
    abort();
  }
}

int main(void) {
  pao_Buffer buff1 = pao_buffer_create(g_pao_memory1, I_PAO_memoryLength);
  pao_Buffer buff2 = pao_buffer_create(g_pao_memory2, I_PAO_memoryLength);
  writeOrBurst(&buff1, "Hello, World!\n");
  writeOrBurst(&buff1, "I have many things to say,\n");
  writeOrBurst(&buff1, "none of which you care.\n");
  writeOrBurst(&buff1, "So... anyway... bye World!\n");
  pao_buffer_printStr(&buff1);

  pao_buffer_toHex(&buff1, &buff2);
  writeOrBurst(&buff2, "\n");
  pao_buffer_printStr(&buff2);

  pao_buffer_reset(&buff1);
  pao_buffer_reset(&buff2);

  pao_buffer_writeString(&buff1, "Hello\n Only\n", 6);
  pao_buffer_writeString(&buff2, "Hello\n Only\n", 6);

  if (pao_buffer_equals(&buff1, &buff2)) {
    printf("equals!\n");
  }

  printf("sizeof(pao_Buffer) ==  %ld\n", sizeof(pao_Buffer));
}
