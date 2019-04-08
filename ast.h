#ifndef __AST_H
#define __AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

enum ast_types {AST_ASSIGN = 1, AST_UNOP, AST_BINOP, AST_NUMBER, AST_IDENT, AST_CHARLIT,
  AST_STRING, AST_FUNC, AST_SIZEOF, AST_COMP_SELECT, AST_EXPR_LIST, AST_TOP_EXPR,
  AST_IF_ELSE, AST_SCALAR, AST_ARR, AST_POINTER, AST_QUAL, AST_FOR, AST_NULL,
  AST_SWITCH, AST_WHILE, AST_RETURN, AST_CONTINUE, AST_BREAK, AST_CASE, AST_LABEL,
  AST_TOP_EXPR_ST, AST_GOTO, AST_NLABEL, AST_LSTMT, AST_IF, AST_IF_T_ELSE};
//enum scalar_types {CHAR, SHORT, INT, LONG, UNSIGNED, CONST, RESTRICT, VOLATILE};
enum num_signs {UNSIGNED_T = 0, SIGNED_T = 1};
enum num_types {INT_T = 0, LONG_T, LONGLONG_T, DOUBLE_T, LONGDOUBLE_T, FLOAT_T};

struct node_ident{
  char *name;
  int type;
  int line;
  char *fname;
  int fntype;
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

struct node_top_st{
  struct ast_node *left;
  struct ast_node *right;
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
  int type;
  int line;
  char *fname;
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
  int qual;
  int type;
};

struct node_point{
  ;
};

struct node_arr{
  int num;
};

struct node_for{
  struct ast_node *init;
  struct ast_node *cond;
  struct ast_node *body;
  struct ast_node *incr;
};

struct node_switch{
  struct ast_node *expr;
  struct ast_node *body;
};

struct node_return{
  struct ast_node *expr;
};

struct node_while{
  struct ast_node *expr;
  struct ast_node *body;
};

struct node_goto{
  struct ast_node *label;
};

struct node_label{
  char *label;
  int icase;
};

struct node_lstmt{
  struct ast_node *label;
  struct ast_node *stmt;
};

struct node_case{
  struct ast_node *label;
};

struct node_if{
  struct ast_node *expr;
  struct ast_node *stmt;
};

struct node_if_t_e{
  struct ast_node *expr;
  struct ast_node *tstmt;
  struct ast_node *estmt;
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
    struct node_top_st top_expr_st;
    struct node_unop unop;
    struct node_binop binop;
    struct node_comp comp_select;
    struct node_func func;
    struct node_expr_list expr_list;
    struct node_assign assign;

    struct node_scalar scalar;
    struct node_point pointer;
    struct node_arr arr;

    struct node_for nfor;
    struct node_switch nswitch;
    struct node_while nwhile;
    struct node_return nreturn;
    struct node_goto ngoto;
    struct node_label nlabel;
    struct node_lstmt lstmt;
    struct node_case lcase;
    struct node_if nif;
    struct node_if_t_e if_t_else;
  } u;
  struct ast_node *next;
  struct ast_node *prev;
};

struct ast_node *ast_node_alloc(int node_type);

void print_ast(struct ast_node *root, int level);
char* print_kw(int token);
void ast_node_link(struct ast_node **head, struct ast_node **tail, struct ast_node *ins);

#endif
