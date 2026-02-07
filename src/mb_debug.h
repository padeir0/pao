/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_debug_H
#define MB_debug_H

#include <stdio.h>

#define MB_DEBUG \
    fprintf(stderr, "[DEBUG] %s:%s line %d\n", \
            __FILE__, __func__, __LINE__);

#endif
