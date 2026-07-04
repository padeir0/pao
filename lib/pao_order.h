/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_order_H
#define PAO_order_H

typedef enum {
  PAO_order_less,
  PAO_order_equal,
  PAO_order_greater,
} pao_Order;

static inline
pao_Order pao_order_invert(pao_Order a) {
  if (a == PAO_order_less) {
    return PAO_order_greater;
  }
  if (a == PAO_order_greater) {
    return PAO_order_less;
  }
  return PAO_order_equal;
}

#endif
