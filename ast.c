#include "ast.h"

struct ast_node * ast_node_alloc(int node_type){
  struct ast_node *n = malloc(sizeof(struct ast_node));
  if(n == NULL){
    fprintf(stderr, "ERROR: Unable to allocate new AST Node: %s\n", strerror(errno));
    return NULL;
  }
  n->node_type = node_type;
  return n;
}

void print_ast(struct ast_node *root, int level){
  if(root->node_type != AST_EXPR_LIST){
    for(int i = 0; i < level; i++){
      fprintf(stdout, "  ");
    }
  }

  if(root == NULL){
    fprintf(stdout, "NULL\n");
    return;
  }
  switch(root->node_type){
    case AST_ASSIGN: fprintf(stdout, "ASSIGNMENT\n");
                     print_ast(root->u.assign.left, level + 1);
                     print_ast(root->u.assign.right, level + 1);
                     break;

    case AST_UNOP:   fprintf(stdout, "UNARY OP %d", root->u.unop.operator);
                     if(root->u.unop.operator < 255)
                      fprintf(stdout, " (%c)\n",  (unsigned char)root->u.unop.operator);
                     else
                      fprintf(stdout, " (%s)\n",  print_kw(root->u.unop.operator));
                     print_ast(root->u.unop.left, level + 1);
                     break;

    case AST_BINOP:  fprintf(stdout, "BINARY OP %d", root->u.binop.operator);
                     if(root->u.binop.operator < 255)
                      fprintf(stdout, " (%c)\n", (unsigned char)root->u.binop.operator);
                     else
                      fprintf(stdout, " (%s)\n", print_kw(root->u.binop.operator));
                     print_ast(root->u.binop.left, level + 1);
                     print_ast(root->u.binop.right, level + 1);
                     break;

    case AST_IDENT:  fprintf(stdout, "IDENT %s\n", root->u.ident.name);
                     break;

    case AST_NUMBER: fprintf(stdout, "NUMBER: (numtype = ");
                     switch(root->u.num.type){
                        case INT_T:   fprintf(stdout, "int) %lld\n", root->u.num.intval);    break;
                        case LONG_T:  fprintf(stdout, "long) %lld\n", root->u.num.intval);   break;
                        case LONGLONG_T:   fprintf(stdout, "long long) %lld\n", root->u.num.intval); break;
                        case DOUBLE_T:   fprintf(stdout, "double) %Lf\n", root->u.num.floatval); break;
                        case LONGDOUBLE_T:   fprintf(stdout, "long double) %Lf\n", root->u.num.floatval); break;
                        case FLOAT_T: fprintf(stdout, "float) %Lg\n", root->u.num.floatval); break;
                        default:      fprintf(stdout, "unknown) \n");                        break;
                     }
                     break;

    case AST_CHARLIT: fprintf(stdout, "CHARLIT %c\n", root->u.charlit.c);
                      break;

    case AST_STRING: fprintf(stdout, "STRING %s\n", root->u.string.word);
                     break;

    case AST_FUNC:   fprintf(stdout, "FUNCTION CALL\n");
                     print_ast(root->u.func.name, level + 1);
                     print_ast(root->u.func.args, level + 1);
                     break;

    case AST_SIZEOF: fprintf(stdout, "SIZEOF\n");
                     print_ast(root->u.size_of.left, level + 1);
                     break;

    case AST_COMP_SELECT: fprintf(stdout, "COMPONENT SELECTION\n");
                     print_ast(root->u.comp_select.name, level + 1);
                     print_ast(root->u.comp_select.member, level + 1);
                     break;

    case AST_EXPR_LIST: print_ast(root->u.expr_list.omember, level);
                     print_ast(root->u.expr_list.nmember, level);
                     break;

    case AST_TOP_EXPR:  print_ast(root->u.top_expr.left, level);
                        break;

    case AST_IF_ELSE:   fprintf(stdout, "IF CONDITION\n");
                        print_ast(root->u.if_else.cond, level + 1);
                        fprintf(stdout, "THEN:\n");
                        print_ast(root->u.if_else.if_true, level + 1);
                        fprintf(stdout, "ELSE:\n");
                        print_ast(root->u.if_else.if_false, level + 1);
  }
}

void ast_node_link(struct ast_node **head, struct ast_node **tail, struct ast_node *ins){
  if(*head == NULL){
    *head = ins;
  }
  if(*tail == NULL){
    *tail = ins;
  } else {
    (*tail)->next = ins;
    *tail = ins;
  }
  while((*tail)->next != NULL){
    *tail = (*tail)->next;
  }
}
