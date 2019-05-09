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

void print_ast(struct ast_node *node, int level, int fn, struct sym_tab *tab){
  struct ast_node *root = node;
  if(root->node_type != AST_EXPR_LIST && root->node_type != AST_TOP_EXPR && root->node_type != AST_TOP_EXPR_ST && root->node_type != AST_CASE){
    fprintf(stdout, "%*s", level, "");
  }

  if(root == NULL){
    fprintf(stdout, "NULL\n");
    return;
  }
  switch(root->node_type){
    case AST_ASSIGN: fprintf(stdout, "ASSIGNMENT\n");
                     print_ast(root->u.assign.left, level + 1, fn, tab);
                     print_ast(root->u.assign.right, level + 1, fn, tab);
                     break;

    case AST_UNOP:   fprintf(stdout, "UNARY OP");
                     if(root->u.unop.operator < 255)
                      fprintf(stdout, " (%c)\n",  (unsigned char)root->u.unop.operator);
                     else
                      fprintf(stdout, " (%s)\n",  print_kw(root->u.unop.operator));
                     print_ast(root->u.unop.left, level + 1, fn, tab);
                     break;

    case AST_BINOP:  fprintf(stdout, "BINARY OP");
                     if(root->u.binop.operator < 255)
                      fprintf(stdout, " (%c)\n", (unsigned char)root->u.binop.operator);
                     else
                      fprintf(stdout, " (%s)\n", print_kw(root->u.binop.operator));
                     print_ast(root->u.binop.left, level + 1, fn, tab);
                     print_ast(root->u.binop.right, level + 1, fn, tab);
                     break;

    case AST_IDENT:  {
                       if(root->u.ident.fname == NULL){
                        fprintf(stderr, "ERROR: Unknown symbol %s\n", root->u.ident.name);
                        exit(-5);
                       }
                       if(fn == 1){
                         struct sym *n = search_all(tab, root->u.ident.name, ID_FUNC);
                         if(n == NULL){
                           n = add_sym(root, tab, root->u.ident.fname, root->u.ident.line);
                         } else{
                           root->u.ident.fname = n->fname;
                           root->u.ident.line = n->line;
                         }
                         fprintf(stdout, "stab_fn name=%s def @%s:%d\n", root->u.ident.name, root->u.ident.fname, root->u.ident.line);
                       } else {
                         struct sym *n = search_all(tab, root->u.ident.name, ID_VAR);
                         if(n == NULL){
                           fprintf(stderr, "ERROR: Unknown symbol %s\n", root->u.ident.name);
                           exit(-5);
                         }
                         root->u.ident.fname = n->fname;
                         root->u.ident.line = n->line;
                         fprintf(stdout, "stab_var name=%s def @%s:%d\n", root->u.ident.name, root->u.ident.fname, root->u.ident.line);
                       }
                       break;
                     }

    case AST_NUMBER: fprintf(stdout, "NUMBER: (type = ");
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
                     print_ast(root->u.func.name, level + 1, 1, tab);
                     if(root->u.func.args != NULL){
                       //fprintf(stderr, "NUMARGS: %d", root->u.func.numargs);
                       fprintf(stdout, "%*s", level, "");
                       fprintf(stdout, "FUNCTION ARGS: %d\n", root->u.func.numargs);
                       print_ast(root->u.func.args, level + 1, 0, tab);
                     }
                     break;

    case AST_SIZEOF: fprintf(stdout, "SIZEOF\n");
                     print_ast(root->u.size_of.left, level + 1, fn, tab);
                     break;

    case AST_COMP_SELECT: fprintf(stdout, "COMPONENT SELECTION\n");
                     print_ast(root->u.comp_select.name, level + 1, fn, tab);
                     print_ast(root->u.comp_select.member, level + 1, fn, tab);
                     break;

    case AST_EXPR_LIST: print_ast(root->u.expr_list.omember, level, fn, tab);
                     print_ast(root->u.expr_list.nmember, level, fn, tab);
                     break;

    case AST_TOP_EXPR:  print_ast(root->u.top_expr.left, level, fn, tab);
                        break;

    case AST_TOP_EXPR_ST:  print_ast(root->u.top_expr_st.left, level, fn, tab);
                           print_ast(root->u.top_expr_st.right, level, fn, tab);
                           break;

    case AST_IF_ELSE:   fprintf(stdout, "IF CONDITION\n");
                        print_ast(root->u.if_else.cond, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "THEN:\n");
                        print_ast(root->u.if_else.if_true, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "ELSE:\n");
                        print_ast(root->u.if_else.if_false, level + 1, fn, tab);
                        break;
    case AST_FOR:       fprintf(stdout, "FOR\n");
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "INIT:\n");
                        print_ast(root->u.nfor.init, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "COND:\n");
                        print_ast(root->u.nfor.cond, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "BODY:\n");
                        fprintf(stdout, "%*s", level+1, "");
                        fprintf(stdout, "LIST:\n");
                        print_ast(root->u.nfor.body, level + 2, fn, tab);
                        fprintf(stdout, "%*s", level+1, "");
                        fprintf(stdout, "}\n");
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "INCR:\n");
                        print_ast(root->u.nfor.incr, level + 1, fn, tab);
                        break;
    case AST_WHILE:     fprintf(stdout, "WHILE\n");
                        fprintf(stdout, "%*s", level + 1, "");
                        fprintf(stdout, "EXPR\n");
                        print_ast(root->u.nwhile.expr, level + 2, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "BODY:\n");
                        fprintf(stdout, "%*s", level+1, "");
                        fprintf(stdout, "LIST:\n");
                        print_ast(root->u.nwhile.body, level + 2, fn, tab);
                        fprintf(stdout, "%*s", level+1, "");
                        fprintf(stdout, "}\n");
                        break;
    case AST_NULL:      fprintf(stdout, "NULL\n");
                        break;
    case AST_SWITCH:    fprintf(stdout, "SWITCH, EXPR\n");
                        print_ast(root->u.nswitch.expr, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "BODY:\n");
                        fprintf(stdout, "%*s", level+1, "");
                        fprintf(stdout, "LIST{\n");
                        print_ast(root->u.nswitch.body, level + 2, fn, tab);
                        fprintf(stdout, "%*s", level+1, "");
                        fprintf(stdout, "}\n");
                        break;
    case AST_RETURN:    fprintf(stdout, "RETURN\n");
                        print_ast(root->u.nreturn.expr, level + 1, fn, tab);
                        break;
    case AST_BREAK:     fprintf(stdout, "BREAK\n");
                        break;
    case AST_CONTINUE:  fprintf(stdout, "CONTINUE\n");
                        break;
    case AST_GOTO:      fprintf(stdout, "GOTO %s\n", root->u.ngoto.label->u.nlabel.label);
                        break;
    case AST_LSTMT:     {
                          if(root->u.lstmt.label->u.nlabel.icase == 1){
                            fprintf(stdout, "Label (%s):\n", root->u.lstmt.label->u.nlabel.label);
                            print_ast(root->u.lstmt.stmt, level + 1, fn, tab);
                          } else if(root->u.lstmt.label->u.nlabel.icase == 2){
                            fprintf(stdout, "DEFAULT\n");
                            print_ast(root->u.lstmt.stmt, level + 1, fn, tab);
                          } else{
                            print_ast(root->u.lstmt.label, level + 1, fn, tab);
                            fprintf(stdout, "%*s", level, "");
                            fprintf(stdout, "STMT\n");
                            print_ast(root->u.lstmt.stmt, level + 1, fn, tab);
                          }
                        }
                        break;
    case AST_CASE:      fprintf(stdout, "CASE\n");
                        fprintf(stdout, "%*s", level + 1, "");
                        fprintf(stdout, "EXPR\n");
                        print_ast(root->u.lcase.label, level + 1, fn, tab);
                        break;
    case AST_IF:        fprintf(stdout, "IF:\n");
                        print_ast(root->u.nif.expr, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "THEN:\n");
                        print_ast(root->u.nif.stmt, level + 1, fn, tab);
                        break;
    case AST_IF_T_ELSE: fprintf(stdout, "IF:\n");
                        print_ast(root->u.if_t_else.expr, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "THEN:\n");
                        print_ast(root->u.if_t_else.tstmt, level + 1, fn, tab);
                        fprintf(stdout, "%*s", level, "");
                        fprintf(stdout, "ELSE:\n");
                        print_ast(root->u.if_t_else.estmt, level + 1, fn, tab);
                        break;
    default:            //print_ast(root->next, level + 1);
                        break;

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
    ins->prev = (*tail);
    *tail = ins;
  }
  while((*tail)->next != NULL){
    *tail = (*tail)->next;
  }
}

struct ast_node *reverse(struct ast_node *root, int dir){
    struct ast_node *n = NULL;
    while(root != NULL){
      struct ast_node *temp;
      if(dir == 0){
        temp = root->prev;
        root->prev = n;
      } else if(dir == 1){
        temp = root->next;
        root->next = n;
      }
      n = root;
      root = temp;
    }
    return n;
}

struct ast_node *find_ret_value(struct ast_node *root){
	struct ast_node *n = root;
	while (n != NULL)
	{
		if (n->node_type == AST_SCALAR)
		{
			return n;
		}
		else if (n->node_type == AST_POINTER)
		{
			return n;
		}
		n = n->prev;
	}
	return NULL;
}
