/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_DEBUG_H
#define PAO_DEBUG_H

#include <stdio.h>

#define debug_PRINT(str)                                  \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" str "\n");                       \
      } while (0);

#define debug_PRINTFMT(fmt, ...)                          \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" fmt "\n", __VA_ARGS__);          \
      } while (0);

#define debug_FATAL(str)                                  \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" str "\n");                       \
        abort();                                              \
      } while (0);

#define debug_FATALFMT(fmt, ...)                          \
      do {                                                    \
        fprintf(stderr, "[DEBUG] %s:%s:%d\n",                 \
                __FILE__, __func__, __LINE__);                \
        fprintf(stderr, "\t" fmt "\n", __VA_ARGS__);          \
        abort();                                              \
      } while (0);
#endif
