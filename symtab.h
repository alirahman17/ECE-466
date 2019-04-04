#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"

enum ident_type {
  ID_VAR = 0,
  ID_FUNC,
  ID_TDEF,
  ID_CONST,
  ID_STAG,
  ID_ETAG,
  ID_UTAG,
  ID_LABEL,
  ID_SMEM,
  ID_UMEM
};

enum namespace_type {
  NS_NAME = 0,
  NS_TAGS,
  NS_MEM,
  NS_LABEL
};

enum scope_type {
  SCOPE_GLOB = 0,
  SCOPE_FUNC,
  SCOPE_BLOCK,
  SCOPE_PROTO
};

enum stg_class {
  STG_AUTO,
  STG_EXTERN,
  STG_REG,
  STG_STATIC
};

struct sym_tab {
	enum scope_type type;
	struct sym_tab *parent;
	struct sym *symsS;   //Start
  struct sym *symsE;   //End
  int line;
};

struct sym_var{
  int stg;
};

struct sym_func{
  int stg;
  int complete;
};

struct sym {
  char *name;
  struct sym_tab *curr_tab;
  struct ast_node *n;
  struct sym *next;
  enum namespace_type ns;
  char *fname;
  int line;
  enum ident_type ident;
  union {
    struct sym_var var;
    struct sym_func func;
  } e;
};

struct sym_tab *global, *current;

struct sym_tab *new_sym_table(int scopetype, int line);
struct sym *new_sym(int type, char *name, struct sym_tab *curr_tab, struct sym *next, char *fname, int line);
struct sym *search_ident(struct sym_tab *curr_tab, char *ident, int type);

int print_sym(struct sym *entry, int step);
int print_table(struct sym_tab *table);

//struct sym *get_sym(struct sym_tab *table, char *name, enum namespace_type);
struct sym *install_sym(struct sym_tab *curr_tab, struct sym *entry, int line);
struct sym *add_sym(struct ast_node *n, struct sym_tab *curr_tab, char *fname, int line);

#endif
