#include <stdio.h>
#include "../src/pao_basicTypes.h"
#include "../src/pao_status.h"
#include <strings.h>
#include <string.h>

#define DEFAULT_SIZE 2048

void checkStatus(pao_status ns) {
  if (ns != PAO_status_ok) {
    printf("fail: %d\n", ns);
    abort();
  }
}

typedef struct {
  char* name;
  bool (*func)(void);
} Tester;


// TODO: create a header for colors
#define COLOR_RED   "\x1b[31m"
#define COLOR_BLUE  "\x1b[34m"
#define COLOR_RESET "\x1b[0m"

char print_buff[DEFAULT_SIZE];
void test(Tester t) {
  bzero(print_buff, DEFAULT_SIZE);
  if (t.func()) {
    strcat(print_buff, COLOR_BLUE "OK" COLOR_RESET ": ");
  } else {
    strcat(print_buff, COLOR_RED "FAIL" COLOR_RESET ": ");
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

