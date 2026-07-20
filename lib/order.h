/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_ORDER_H
#define PAO_ORDER_H

typedef enum {
  order_LESS,
  order_EQUAL,
  order_GREATER,
} Order;

static inline
Order order_invert(Order a) {
  if (a == order_LESS) {
    return order_GREATER;
  }
  if (a == order_GREATER) {
    return order_LESS;
  }
  return order_EQUAL;
}

#endif
