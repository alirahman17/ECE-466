#ifndef __AST_H
#define __AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum ast_types {AST_ASSIGN = 1, AST_UNOP, AST_BINOP, AST_NUMBER, AST_IDENT, AST_CHARLIT, AST_STRING, AST_FUNC, AST_SIZEOF, AST_COMP_SELECT, AST_EXPR_LIST, AST_TOP_EXPR, AST_IF_ELSE, AST_SCALAR, AST_ARR, AST_POINTER};
//enum scalar_types {CHAR, SHORT, INT, LONG, UNSIGNED, CONST, RESTRICT, VOLATILE};
enum num_signs {UNSIGNED_T = 0, SIGNED_T = 1};
enum num_types {INT_T = 0, LONG_T, LONGLONG_T, DOUBLE_T, LONGDOUBLE_T, FLOAT_T};

struct node_ident{
  char *name;
  int type;
};

struct node_charlit{
  char c;
};

struct node_string{
  char word[4096];
  int length;
};

struct node_number{
  unsigned long long intval;
  long double floatval;
  int sign;
  int type;
};

struct node_if_else{
  struct ast_node *cond;
  struct ast_node *if_true;
  struct ast_node *if_false;
};

struct node_size{
  struct ast_node *left;
};

struct node_top{
  struct ast_node *left;
};

struct node_unop{
  int operator;
  struct ast_node *left;
};

struct node_binop{
  int operator;
  struct ast_node *left;
  struct ast_node *right;
};

struct node_comp{
  struct ast_node *name;
  struct ast_node *member;
};

struct node_func{
  struct ast_node *name;
  struct ast_node *args;
};

struct node_expr_list{
  struct ast_node *omember;
  struct ast_node *nmember;
};

struct node_assign{
  struct ast_node *left;
  struct ast_node *right;
};

struct node_scalar{
  int type;
};

struct node_point{
  ;
};

struct node_arr{
  int num;
};

struct ast_node{
  int node_type;
  union {
    struct node_ident ident;
    struct node_charlit charlit;
    struct node_string string;
    struct node_number num;
    struct node_if_else if_else;
    struct node_size size_of;
    struct node_top top_expr;
    struct node_unop unop;
    struct node_binop binop;
    struct node_comp comp_select;
    struct node_func func;
    struct node_expr_list expr_list;
    struct node_assign assign;

    struct node_scalar scalar;
    struct node_point pointer;
    struct node_arr arr;
  } u;
  struct ast_node *next;
};

struct ast_node *ast_node_alloc(int node_type);

void print_ast(struct ast_node *root, int level);
char* print_kw(int token);
void ast_node_link(struct ast_node **head, struct ast_node **tail, struct ast_node *ins);

#endif
