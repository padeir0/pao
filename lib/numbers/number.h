/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_NUMBER_H
#define PAO_NUMBER_H

#include "../basicTypes.h"
#include "integer.h"
#include "rational.h"

typedef union {
  double nativeFloat;
  i64 nativeInt;
  Integer* integer;
  Rational* rational;
} NumberData;

typedef enum {
  numberKind_nativeInt,
  numberKind_nativeFloat,
  numberKind_integer,
  numberKind_rational,
  numberKind_decimal,
} NumberKind;

/* Implementa uma torre numérica.
Leis de conversão devem preservar informação, ie,
tipos menores são transformados em tipos maiores
   nativeInt < integer < rational < decimal
   double < decimal
Decimal não existe ainda, então não vou me preocupar
com a conversão de double pra decimal.

Um nativeInt deve se tornar Integer caso aconteça overflow e deve
virar Rational caso seja dividido por outro inteiro.

Um double vira decimal caso a precisão pedida (globalmente ou na operação
específica) supere a precisão de um double.
*/
typedef struct {
  NumberKind kind;
  NumberData data;
} Number;

/*
TODO: implementar equivalentes (number_*) das funções expostas dentro da linguagem:
  equal? not-equal?
  less?  greater?
  less-or-equals? greater-or-equals?

  add sub mul div
  rem even? odd?
  abs round ceil
  floor pow

  to-string
*/

#endif
