#ifndef __QUAD_H
#define __QUAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "symtab.h"

enum quad_opcodes {LOAD = 300, STORE, LEA, MOV, CMP, ARG, CALL, RET};
enum addr_modes {DIRECT = 0, INDIRECT};
enum bb_branches {NEVER = 0, ALWAYS, COND_LT, COND_GT, COND_LTEQ, COND_GTEQ, COND_EQ, COND_NEQ};

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
  char *name;
  int branch;
  struct quad_list *q_list;
  struct basic_block *left;
  struct basic_block *right;
  struct basic_block *next;
};

struct basic_block_list {
	int size;
	struct basic_block *head;
	struct basic_block *tail;
};

struct loop{
  struct basic_block *bb_cont;
  struct basic_block *bb_break;
  struct loop *prev;
};


struct quad *emit(int opcode, struct ast_node *src1, struct ast_node *src2, struct ast_node *dest);
struct ast_node *new_temp();
struct ast_node *gen_lvalue(struct ast_node *node, int *m);
struct ast_node *gen_rvalue(struct ast_node *n, struct ast_node *target);
struct ast_node *gen_assign(struct ast_node *node);
struct quad_list *quad_list_append(struct quad *q, struct quad_list *l);
struct quad_list *new_quad_list(void);

int get_sizeof(struct ast_node *node, struct sym_tab *syms);
void *gen_stmt(struct ast_node *node);
struct ast_node *gen_if(struct ast_node *node);
struct ast_node *gen_cond(struct ast_node *node, struct basic_block *bbt, struct basic_block *bbf);
struct loop *new_loop(void);
void *gen_for(struct ast_node *node);
void *gen_break(struct ast_node *node);
void *gen_continue(struct ast_node *node);
void *gen_return(struct ast_node *node);
void *gen_unop(struct ast_node *node);
void *gen_fncall(struct ast_node *node, struct ast_node *target);

struct basic_block *new_bb(void);
struct basic_block_list *new_bb_list(void);
struct basic_block_list *bb_list_append(struct basic_block *bb, struct basic_block_list *l);
struct basic_block *bb_link(struct basic_block *bb, int branch, struct basic_block *left, struct basic_block *right);
void print_bb(struct basic_block *bb);

void print_quad(struct quad *q);
void print_rval(struct ast_node *node);
void *gen_quad(struct ast_node *func, struct ast_node  *stmt, struct sym_tab *sym_tabl, FILE *f);

#endif
