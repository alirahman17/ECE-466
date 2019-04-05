#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "symtab.h"
#include "parser.tab.h"

struct sym_tab *new_sym_table(int scopetype, int line){
  struct sym_tab *new_table = (struct sym_tab *) malloc(sizeof(struct sym_tab));
  if(!new_table){
    fprintf(stderr, "No space for new sym table\n");
    exit(1);
  }
  new_table->parent = NULL;
  new_table->type = scopetype;
  new_table->line = line;
  new_table->symsS = NULL;
  new_table->symsE = NULL;
  return new_table;
}

struct sym *new_sym(int type, char *name, struct sym_tab *curr_tab, struct sym *next, char *fname, int line){
  struct sym *newsym;
  newsym = malloc(sizeof(struct sym));
  if(!newsym){
    fprintf(stderr, "No space for new symbol\n");
    exit(1);
  }
  newsym->name = strdup(name);
  newsym->ident = type;
  newsym->curr_tab = curr_tab;
  newsym->next = next;
  newsym->fname = strdup(fname);
  newsym->line = line;
  return newsym;
}
struct sym *search_sym(struct sym_tab *curr_tab, char *ident, int type){
  if(curr_tab->symsS != NULL){
    struct sym *curr_sym = curr_tab->symsS;
    struct sym *i = NULL;
    while(curr_sym != NULL){
      if(!strcmp(curr_sym->name, ident)){
        switch(type){
          default: {
            i = curr_sym;
            break;
          }
        }
        break;
      }
      curr_sym = curr_sym->next;
      if(curr_sym == NULL){
        break;
      }
    }
    if(i != NULL)
      return i;
    }
    else
      return NULL;
}

struct sym *search_all(struct sym_tab *curr_tab, char *ident, int type){
  struct sym *i;
  i = search_sym(curr_tab, ident, type);
  if(i != NULL){
    return i;
  } else{
    while(curr_tab != NULL){
      if(curr_tab->parent != NULL){
        i = search_sym(curr_tab->parent, ident, type);
        if(i != NULL){
          return i;
        }
      } else{
        return NULL;
      }
    }
  }
  return NULL;
}


int print_scope(struct sym *entry){
  switch(entry->curr_tab->type){
    case SCOPE_GLOB: {
      printf("[in global scope starting at %d] ", entry->line);
      break;
    }
    case SCOPE_FUNC: {
      printf("[in function scope starting at %d] ", entry->line);
      break;
    }
    case SCOPE_BLOCK: {
      printf("[in blocking scope starting at %d] ", entry->line);
      break;
    }
    case SCOPE_PROTO: {
      break;
    }
    default: {
      fprintf(stderr, "ERROR: CURRENT SCOPE UNDEFINED\n");
      return 1;
    }
  }
  return 0;
}

int traceback(struct sym *entry){
  int indent = 0;
  struct ast_node *n = entry->n;
  while(n != NULL){
    int curr_ind = indent;
    printf("%.*s", indent, "                                                                              ");
    int node_type = n->node_type;
    switch(node_type){
      case AST_SCALAR: {
        switch(n->u.scalar.type) {
          case CHAR: printf("CHAR\n"); break;
          case SHORT: printf("SHORT\n"); break;
          case INT: printf("INT\n"); break;
          case LONG: printf("LONG\n"); break;
          case UNSIGNED: {
            if(n->next != NULL){
              printf("UNSIGNED\n");
            } else{
              printf("UNSIGNED\n");
              indent++;
              printf("%.*s", indent, "                                                                              ");
              printf("INT\n");
            }
            break;
          }
          case FLOAT: printf("FLOAT\n"); break;
          case DOUBLE: printf("DOUBLE\n"); break;
          case VOID: printf("VOID\n"); break;
          case SIGNED: {
            if(n->next != NULL){
              printf("SIGNED\n");
            } else{
              printf("SIGNED\n");
              indent++;
              printf("%.*s", indent, "                                                                              ");
              printf("INT\n");
            }
            break;
          }
          default: printf("ERROR: UNKNOWN SCALAR\n"); break;
        }
        break;
      }
      case AST_QUAL: {
        switch(n->u.scalar.qual) {
          case CONST: printf("CONST\n"); break;
          case RESTRICT: printf("RESTRICT\n"); break;
          case VOLATILE: printf("VOLATILE\n"); break;
          default: break;
        }
        break;
      }
      case AST_POINTER: {
        printf("pointer to \n");
        break;
      }
      case AST_ARR: {
        printf("array with %d elements of type\n", n->u.arr.num);
        break;
      }
      default: {
        fprintf(stderr, "ERROR: Unknown AST Node\n");
      }
    }
    n = n->next;
    indent = indent + 1;
  }
  return 0;
}

int print_stg(int stg){
  switch(stg){
    case STG_AUTO: printf("auto "); break;
    case STG_EXTERN: printf("extern "); break;
    case STG_STATIC: printf("static "); break;
    case STG_REG: printf("register "); break;
    default: fprintf(stderr, "ERROR: Undefined storage class type\n"); break;
  }
}

int print_sym(struct sym *entry, int step){
  switch(entry->ident){
    case ID_VAR: {
      printf("%s is declared at file %s at line %d ", entry->name, entry->fname, entry->line);
      print_scope(entry);
      printf("as a variable with stg ");
      print_stg(entry->e.var.stg);
      printf("of type:\n");
      traceback(entry);
      break;
    }
    case ID_FUNC: {
      printf("%s is declared at file %s at line %d ", entry->name, entry->fname, entry->line);
      print_scope(entry);
      printf("as a ");
      print_stg(entry->e.func.stg);
      printf("function returning ");
      traceback(entry);
      break;
    }
    default: {
      fprintf(stderr, "ERROR: UNDEFINED ENTRY TYPE\n");
      break;
    }
  }
  return 0;
}

int print_table(struct sym_tab *table){
  int counter = 0;
  struct sym *n = table->symsS;
  while(n != NULL){
    print_sym(n, 1);
    counter++;
    n = n->next;
  }
  return counter;
}

//struct sym *get_sym(struct sym_tab *table, char *name, enum namespace_type){
//}

struct sym *install_sym(struct sym_tab *curr_tab, struct sym *entry, int line){
  struct sym *find = search_sym(curr_tab, entry->name, entry->ident);
  if(find != NULL){
    return find;
  } else{
    if(curr_tab->symsS == NULL){
      curr_tab->symsS = entry;
      curr_tab->symsE = entry;
    } else{
      curr_tab->symsE->next = entry;
      curr_tab->symsE = entry;
    }
  }
  return NULL;
}

struct sym *add_sym(struct ast_node *node, struct sym_tab *curr_tab, char *fname, int line){
  switch(node->u.ident.type) {
    case ID_VAR: {
      struct sym *n = new_sym(ID_VAR, node->u.ident.name, curr_tab, NULL, fname, line);
      struct sym *i = install_sym(curr_tab, n, line);
      if(i != NULL && curr_tab->parent != NULL){
        exit(5);
      } else if(i != NULL && curr_tab->parent == NULL){
        i->n = node->next;
        return i;
      } else {
        n->n = node->next;
        return n;
      }

    }
    case ID_FUNC: {
      struct sym *n = new_sym(ID_FUNC, node->u.ident.name, curr_tab, NULL, fname, line);
      n->e.func.complete = 0;
      struct sym *i = install_sym(curr_tab, n, line);
      n->n = node->next;
      return n;
    }
  }
  return NULL;
}
