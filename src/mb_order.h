/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_order_H
#define MB_order_H

typedef enum {
  MB_order_less,
  MB_order_equal,
  MB_order_greater,
} mb_Order;

static inline
mb_Order mb_order_invert(mb_Order a) {
  if (a == MB_order_less) {
    return MB_order_greater;
  }
  if (a == MB_order_greater) {
    return MB_order_less;
  }
  return MB_order_equal;
}

#endif
