#include <stdio.h>
#include "../lib/pao_basicTypes.h"
#include "../lib/pao_status.h"
#include <strings.h>
#include <string.h>
#include "../lib/pao_colors.h"

#ifndef PAO_TEST_COMMON

#define PAO_TEST_COMMON 1

#define DEFAULT_SIZE 2048

void checkStatus(pao_Status ns) {
  if (ns != PAO_status_ok) {
    printf("fail: %d\n", ns);
    abort();
  }
}

typedef struct {
  char* name;
  bool (*func)(void);
} Tester;

static
char print_buff[DEFAULT_SIZE];

void test(Tester t) {
  bzero(print_buff, DEFAULT_SIZE);
  if (t.func()) {
    strcat(print_buff, PAO_colors_blue "OK" PAO_colors_reset ": ");
  } else {
    strcat(print_buff, PAO_colors_red "FAIL" PAO_colors_reset ": ");
  }
  strcat(print_buff, t.name);
  strcat(print_buff, "\n");
  printf("%s", print_buff);
}

void run_tests(Tester* tests, int length) {
  int i = 0;
  while (i < length) {
    test(tests[i]);
    i++;
  }
}

#endif
