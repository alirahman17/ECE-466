#ifndef __QUAD_H
#define __QUAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "symtab.h"

struct quad {
  char *dest;
  struct ast_node *destN;
  char *op;
  char *src1;
  struct ast_node *src1N;
  char *src2;
  struct ast_node *src2N;
};

struct quad *quad_gen(struct ast_node *n);
void print_quad(struct quad *q);

#endif
