#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../lib/status.h"
#include "../lib/colors.h"

#ifndef PAO_TEST_COMMON_H

#define PAO_TEST_COMMON_H 1

#define DEFAULT_SIZE 2048

void checkStatus(Status ns) {
  if (ns != status_OK) {
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

void run_tests(const char* name, Tester* tests, int length) {
  int sucesses = 0;
  int i = 0;
  while (i < length) {
    Tester t = tests[i];
    bzero(print_buff, DEFAULT_SIZE);
    bool ok = t.func();
    if (ok) {
      sucesses++;
    } else {
      strcat(print_buff, colors_RED "FAIL" colors_RESET ": ");
      strcat(print_buff, t.name);
      strcat(print_buff, "\n");
      printf("%s", print_buff);
    }
    i++;
  }

  printf("%s: %d/%d tests passed.\n", name, sucesses, i);
}

#endif
