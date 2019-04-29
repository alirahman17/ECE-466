#ifndef __QUAD_H
#define __QUAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "symtab.h"

enum quad_opcodes {LOAD = 300, STORE};
enum addr_modes {DIRECT = 0, INDIRECT};

struct quad {
  struct ast_node *dest;
  int opcode;
  struct ast_node *src1;
  struct ast_node *src2;
  struct quad *next;
};

struct quad_list {
	int size;
	struct quad *head;
	struct quad *tail;
};

struct basic_block{
  struct quad_list *q_list;
  struct basic_block *left;
  struct basic_block *right;
  struct basic_block *next;
};

struct quad *emit(int opcode, struct ast_node *src1, struct ast_node *src2, struct ast_node *dest);
struct ast_node *new_temp();
struct ast_node *gen_lvalue(struct ast_node *node, int *m);
struct ast_node *gen_rvalue(struct ast_node *n, struct ast_node *target);
struct ast_node *gen_assign(struct ast_node *node);
struct quad_list * quad_list_append(struct quad *q, struct quad_list *l);
void print_quad(struct quad *q);

#endif
