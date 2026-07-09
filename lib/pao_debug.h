/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_debug_H
#define PAO_debug_H

#include <stdio.h>

#define PAO_debug_print(str)                                  \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" str "\n");                       \
      } while (0);

#define PAO_debug_printFmt(fmt, ...)                          \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" fmt "\n", __VA_ARGS__);          \
      } while (0);

#define PAO_debug_fatal(str)                                  \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" str "\n");                       \
        abort();                                              \
      } while (0);

#define PAO_debug_fatalFmt(fmt, ...)                          \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" fmt "\n", __VA_ARGS__);          \
        abort();                                              \
      } while (0);
#endif
