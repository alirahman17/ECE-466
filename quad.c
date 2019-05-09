#include "quad.h"
#include "parser.tab.h"

extern int line;
extern char filename[256];
struct basic_block *curr_bb;
struct basic_block_list *curr_bb_list;
struct loop *curr_loop;
struct sym_tab *sym_tab;
int fn = 1;
int bbN = 1;
int temp = 0;
int string_number = 0;
char string_buffer[4096];
int stackSpace = 0;
int if_else_flag = 0;
FILE *outfile;

struct quad *emit(int opcode, struct ast_node *src1, struct ast_node *src2, struct ast_node *dest){
  struct quad *q = malloc(sizeof(struct quad));
  if(q == NULL){
    fprintf(stderr, "ERROR: Unable to allocate new quad %s\n", strerror(errno));
    return NULL;
  }
  q->opcode = opcode;
  q->src1 = src1;
  q->src2 = src2;
  q->dest = dest;

  quad_list_append(q, curr_bb->q_list);
  return q;
}

struct ast_node *new_temp(){
  struct ast_node *n = ast_node_alloc(AST_TEMP);
  n->u.temp.number = ++temp;
  return n;
}

struct ast_node *gen_lvalue(struct ast_node *node, int *m){
    switch (node->node_type) {
      case AST_IDENT: *m = DIRECT; return node;
      case AST_NUMBER:
      case AST_CHARLIT:
      case AST_STRING: return NULL;
      case AST_UNOP:
                    if(node->u.unop.operator == '*'){
                      *m = INDIRECT;
                      return gen_rvalue(node->u.unop.left, NULL);
                    } else {
                      break;
                    }
    }
}

struct ast_node *gen_rvalue(struct ast_node *node, struct ast_node *target){
  struct ast_node *tmp, *left, *right, *addr, *n;
  struct sym *s;
  switch(node->node_type) {
    case AST_IDENT:   s = search_all(sym_tab, node->u.ident.name, node->u.ident.type);
                      n = s->n;
                      if(n->node_type == AST_ARR){
                          tmp = new_temp();
                          struct quad *q = emit(LEA, n, NULL, tmp);
                          //printf("%%T%05d, %d\n", q->dest->u.temp.number, q->opcode);
                          return tmp;
                      } else{
                        return node;
                      }
    case AST_NUMBER:  return node;
    case AST_CHARLIT: return node;
    case AST_STRING:
                    node->u.string.number = string_number;
                    string_number++;
                    sprintf(string_buffer + strlen(string_buffer), "\t.section .rodata\n");
                    sprintf(string_buffer + strlen(string_buffer), ".string_ro_%d:\n", node->u.string.number);
                    sprintf(string_buffer + strlen(string_buffer), "\t.string \"");
                    for(int i = 0; i < node->u.string.length; i++){
                      char c = node->u.string.word[i];
                      switch(c)
                      {
                        case '\0':	sprintf(string_buffer + strlen(string_buffer), "\\0"); break;
                        case '\a': 	sprintf(string_buffer + strlen(string_buffer), "\\a"); break;
                        case '\b': 	sprintf(string_buffer + strlen(string_buffer), "\\b"); break;
                        case '\f': 	sprintf(string_buffer + strlen(string_buffer), "\\f"); break;
                        case '\n': 	sprintf(string_buffer + strlen(string_buffer), "\\n"); break;
                        case '\r': 	sprintf(string_buffer + strlen(string_buffer), "\\r"); break;
                        case '\t': 	sprintf(string_buffer + strlen(string_buffer), "\\t"); break;
                        case '\v': 	sprintf(string_buffer + strlen(string_buffer), "\\v"); break;
                        case '\'': 	sprintf(string_buffer + strlen(string_buffer), "\\\'"); break;
                        case '\"': 	sprintf(string_buffer + strlen(string_buffer), "\\\""); break;
                        case '\\': 	sprintf(string_buffer + strlen(string_buffer), "\\\\"); break;
                        default:	if (c > 127 || c < 32)
                                sprintf(string_buffer + strlen(string_buffer), "\\%03o", (unsigned char)c);
                              else
                                sprintf(string_buffer + strlen(string_buffer), "%c", c);
                              break;
                      }
                    }
                    sprintf(string_buffer + strlen(string_buffer), "\"\n");
                    sprintf(string_buffer + strlen(string_buffer), "\t.data\n");
                    sprintf(string_buffer + strlen(string_buffer), "\t.align 4\n");
                    sprintf(string_buffer + strlen(string_buffer), ".string_%d:\n", node->u.string.number);
                    sprintf(string_buffer + strlen(string_buffer), "\t.long .string_ro_%d\n", node->u.string.number);
                    return node;
    case AST_UNOP:
                    if(node->u.unop.operator == '*'){
                      addr = gen_rvalue(node->u.unop.left, NULL);
                      if(target == NULL){
                        target = new_temp();
                      }
                      struct quad *q = emit(LOAD,addr,NULL,target);
                      return target;
                    }

                    if(target == NULL)
                      target = new_temp();
                    struct ast_node *unop = gen_rvalue(node->u.unop.left, target);
                    if(node->u.unop.operator == '&')
                      emit(LEA, unop, NULL, target);
                    else if(node->u.unop.operator == PLUSPLUS){
                      emit(MOV, unop, NULL, target);
                      struct ast_node *num = ast_node_alloc(AST_NUMBER);
                      num->u.num.intval = 1;
                      emit('+', unop, num, unop);
                    } else if(node->u.unop.operator == MINUSMINUS){
                      emit(MOV, unop, NULL, target);
                      struct ast_node *num = ast_node_alloc(AST_NUMBER);
                      num->u.num.intval = 1;
                      emit('-', unop, num, unop);
                    }
                    return target;
    case AST_BINOP:
                    left = gen_rvalue(node->u.binop.left, NULL);
                    right = gen_rvalue(node->u.binop.right, NULL);

                    if(node->u.binop.operator == '+' || node->u.binop.operator == '-'){
                      struct ast_node *num = ast_node_alloc(AST_NUMBER);
                      struct sym *sl = NULL;
                      struct sym *sr = NULL;

                      if(node->u.binop.left->node_type == AST_IDENT)
                        sl = search_all(sym_tab, node->u.binop.left->u.ident.name, node->u.binop.left->u.ident.type);
                      if(node->u.binop.right->node_type == AST_IDENT)
                        sr = search_all(sym_tab, node->u.binop.right->u.ident.name, node->u.binop.right->u.ident.type);

                      if(sl && (sl->n->node_type == AST_POINTER || sl->n->node_type == AST_ARR) && (node->u.binop.right->node_type == AST_NUMBER || node->u.binop.right->node_type == AST_IDENT)){
                        tmp = new_temp();
                        if (sl->n->node_type == AST_ARR){
                          struct ast_node *a = sl->n;
                          while(a != NULL){
                            num->u.num.intval = get_sizeof(a, NULL);
                            if(a->node_type != AST_ARR)
                              break;
                            a = a->next;
                          }
                        }
                        else
                          num->u.num.intval = 4;
                        struct quad *q = emit('*', right, num, tmp);
                        //printf("%%T%05d, %d: %d * %d\n", q->dest->u.temp.number, q->opcode, q->src1->u.num.intval, q->src2->u.num.intval);
                        right = tmp;
                      } else if(sr && (sr->n->node_type == AST_POINTER || sr->n->node_type == AST_ARR) && (node->u.binop.left->node_type == AST_NUMBER || node->u.binop.left->node_type == AST_IDENT)){
                        tmp = new_temp();
                        if (sr->n->node_type == AST_ARR){
                          struct ast_node *a = sr->n;
                          while(a != NULL){
                            num->u.num.intval = get_sizeof(a, NULL);
                            if(a->node_type != AST_ARR)
                              break;
                            a = a->next;
                          }
                        }
                        else
                          num->u.num.intval = 4;
                        emit('*', left, num, tmp);
                        left = tmp;
                      } else if(sl && (sl->n->node_type == AST_POINTER || sl->n->node_type == AST_ARR) && sr && (sr->n->node_type == AST_POINTER || sr->n->node_type == AST_ARR) ){
                        if(node->u.binop.operator == '+'){
                          fprintf(stderr, "ERROR: Pointer Addition not allowed!\n");
                          exit(-1);
                        }

                        struct ast_node *ptrl = sl->n;
                        struct ast_node *ptrr = sr->n;

                        while(ptrl->node_type == ptrr->node_type){
                          if(ptrl->node_type == AST_SCALAR){
                            if (ptrl->u.scalar.type == ptrr->u.scalar.type)
                              break;
                            else{
                              fprintf(stderr, "ERROR: Conflicting pointer types!\n");
                              exit(-1);
                            }

                            ptrl = ptrl->next;
                            ptrr = ptrr->next;

                            if (ptrl->node_type != ptrr->node_type){
                              fprintf(stderr, "ERROR: Conflicting pointer types!\n");
                              exit(-1);
                            }
                          }
                        }

                        tmp = new_temp();
                        emit(node->u.binop.operator, left, right, tmp);
                        struct ast_node *size = ast_node_alloc(AST_NUMBER);
                        size->u.num.intval = get_sizeof(ptrl, NULL);
                        emit('/', tmp, size, target);
                        return target;
                      }
                    }

                    if(target == NULL){
                      target = new_temp();
                    }

                    int b_op = node->u.binop.operator;

                    if ( (b_op == '<') || (b_op == LTEQ) || (b_op == '>') || (b_op == GTEQ) || (b_op == EQEQ) || (b_op == NOTEQ) ){
                      struct basic_block *bt = new_bb();
                      struct basic_block *bf = new_bb();
                      struct basic_block *bn = new_bb();
                      gen_cond(node, bt, bf);
                      curr_bb = bt;

                      struct ast_node *tr = ast_node_alloc(AST_NUMBER);
                      tr->u.num.intval = 1;
                      emit(MOV, tr, NULL, target);
                      bb_link(curr_bb, ALWAYS, bn, NULL);

                      curr_bb = bf;
                      struct ast_node *fa = ast_node_alloc(AST_NUMBER);
                      fa->u.num.intval = 0;
                      emit(MOV, fa, NULL, target);
                      bb_link(curr_bb, ALWAYS, bn, NULL);

                      curr_bb = bn;

                      return target;
                    }



                    struct quad *q = emit(node->u.binop.operator, left, right, target);
                    //printf("%%T%05d, %d: %d + %d\n", q->dest->u.temp.number, q->opcode, q->src1->u.temp.number, q->src2->u.temp.number);
                    return target;
    case AST_ARR:
                  tmp = new_temp();
                  emit(LEA, node, NULL, tmp);
                  return tmp;
    case AST_SIZEOF:
                  if (target == NULL)
                    target = new_temp();
                  struct ast_node *sizeof_expr = ast_node_alloc(AST_NUMBER);
                  sizeof_expr->u.num.intval = get_sizeof(node->u.size_of.left, NULL);
                  emit(MOV, sizeof_expr, NULL, target);
                  return target;
    case AST_FUNC:
                  if (target == NULL)
    								target = new_temp();
    							gen_fncall(node, target);
    							return target;
  }
  fprintf(stderr, "ERROR: No proper rvalue %s:%d\n", filename, line);
  exit(-1);
}

struct ast_node *gen_assign(struct ast_node *node){
  struct ast_node *dest, *t1;
  int dest_mode;
  dest = gen_lvalue(node->u.assign.left, &dest_mode);
  if(dest == NULL){
    fprintf(stderr, "ERROR: Invalid assigment\n");
  }
  if(dest_mode == DIRECT){
    t1 = gen_rvalue(node->u.assign.right, dest);
    emit(MOV, t1, NULL, dest);
  } else{
    t1 = gen_rvalue(node->u.assign.right, NULL);
    emit(STORE, t1, dest, NULL);
  }
  return dest;
}

struct quad_list *quad_list_append(struct quad *q, struct quad_list *l){
  if(l->size == 0){
    l->head = q;
    l->tail = q;
  } else{
    l->tail->next = q;
    l->tail = q;
  }
  l->size++;

  return l;
}

struct quad_list *new_quad_list(void){
  struct quad_list *new_list = malloc(sizeof(struct quad_list));
  if(new_list == NULL){
    fprintf(stderr, "ERROR: Unable to allocate new quad list: %s\n", strerror(errno));
    return NULL;
  } else{
    new_list->size = 0;
    new_list->head = NULL;
    new_list->tail = NULL;
  }
  return new_list;
}

int get_sizeof(struct ast_node *node, struct sym_tab *syms){
  if(sym_tab == NULL){
    sym_tab = syms;
  }
  struct sym *s;
  struct ast_node *n;
  switch(node->node_type){
    case AST_CHARLIT: 	return 1;
    case AST_NUMBER: 	  return 4;
    case AST_POINTER:   return 4;
    case AST_UNOP:      if(node->u.unop.operator == '&')
     	                    return 4;
    case AST_SCALAR:    if(node->u.scalar.type == CHAR)
                          return 1;
                        else
                          return 4;
    case AST_IDENT:     s = search_all(sym_tab, node->u.ident.name, node->u.ident.type);
                        n = s->n;
                        if(n->node_type == AST_POINTER)
                          return 4;

                        if(n->node_type == AST_SCALAR)
                          return get_sizeof(n, sym_tab);

                        if(n->node_type == AST_ARR){
                            return (get_sizeof(n->next, sym_tab) * n->u.arr.num);
                        }
    case AST_ARR:       return (get_sizeof(node->next, sym_tab) * node->u.arr.num);

  }
  return 256;
}

void *gen_stmt(struct ast_node *node){
  if(if_else_flag){
    if_else_flag = 0;
    //return NULL;
  }
  switch(node->node_type){
    case AST_TOP_EXPR_ST: gen_stmt(node->u.top_expr_st.left);
                          gen_stmt(node->u.top_expr_st.right);
                          break;
    case AST_TOP_EXPR: gen_stmt(node->u.top_expr.left); break;
    case AST_ASSIGN:    gen_assign(node); break;
    case AST_IF:        gen_if(node); break;
    case AST_IF_T_ELSE: gen_if(node); break;
    case AST_WHILE:     /*gen_while(node);*/ break;
    case AST_FOR:       gen_for(node); break;
    case AST_RETURN:    gen_return(node); break;
    case AST_BREAK:     gen_break(node); break;
    case AST_CONTINUE:  gen_continue(node); break;
    case AST_UNOP:      if((node->u.unop.operator == PLUSPLUS) || (node->u.unop.operator == MINUSMINUS))
                          gen_unop(node);
                        break;
    case AST_FUNC:      gen_fncall(node, NULL); break;

  }
}
struct ast_node *gen_if(struct ast_node *node){
  struct basic_block *bt = new_bb();
  struct basic_block *bf = new_bb();
  struct basic_block *next;

  if((node->node_type == AST_IF_T_ELSE) && (node->u.if_t_else.estmt != NULL)){
    //fprintf(stderr, "HERE IF_ELSE\n");
    next = new_bb();
  } else{
    next = bf;
  }

  if(node->node_type == AST_IF_T_ELSE){
    //fprintf(stderr, "HERE IF_ELSE\n");
    gen_cond(node->u.if_t_else.expr, bt, bf);
    curr_bb = bt;
    gen_stmt(node->u.if_t_else.tstmt);
  } else if(node->node_type == AST_IF){
    gen_cond(node->u.nif.expr, bt, bf);
    curr_bb = bt;
    gen_stmt(node->u.nif.stmt);
  }

  bb_link(curr_bb, ALWAYS, next, NULL);

  if((node->node_type == AST_IF_T_ELSE) && (node->u.if_t_else.estmt != NULL)){
    //fprintf(stderr, "HERE IF_ELSE\n");
    curr_bb = bf;
    gen_stmt(node->u.if_t_else.estmt);
    if_else_flag = 1;
    bb_link(curr_bb, ALWAYS, next, NULL);
  }
  curr_bb = next;
}
struct ast_node *gen_cond(struct ast_node *node, struct basic_block *bbt, struct basic_block *bbf){
  struct ast_node *left, *right;
  switch(node->node_type){
    case AST_ASSIGN:  left = gen_assign(node);
                      right = ast_node_alloc(AST_NUMBER);
                      right->u.num.intval = 0;
                      emit(CMP, left, right, NULL);
                      bb_link(curr_bb, COND_NEQ, bbt, bbf);
                      break;
    case AST_UNOP:    left = gen_rvalue(node, NULL);
                      right = ast_node_alloc(AST_NUMBER);
                      right->u.num.intval = 0;
                      emit(CMP, left, right, NULL);
                      bb_link(curr_bb, COND_NEQ, bbt, bbf);
                      break;
    case AST_BINOP:   left = gen_rvalue(node->u.binop.left, NULL);
                      right = gen_rvalue(node->u.binop.right, NULL);
                      emit(CMP, left, right, NULL);
                      switch(node->u.binop.operator){
                        case '<': 		bb_link(curr_bb, COND_LT, bbt, bbf); 	break;
                        case LTEQ: 		bb_link(curr_bb, COND_LTEQ, bbt, bbf); break;
                        case '>': 		bb_link(curr_bb, COND_GT, bbt, bbf); 	break;
                        case GTEQ: 		bb_link(curr_bb, COND_GTEQ, bbt, bbf); break;
                        case EQEQ: 		bb_link(curr_bb, COND_EQ, bbt, bbf); 	break;
                        case NOTEQ: 	bb_link(curr_bb, COND_NEQ, bbt, bbf); 	break;
                        default:      break;
                      }
                      break;
    case AST_NUMBER:
    case AST_CHARLIT:
    case AST_IDENT:   left = gen_rvalue(node, NULL);
                      right = ast_node_alloc(AST_NUMBER);
                      right->u.num.intval = 0;
                      emit(CMP, left, right, NULL);
                      bb_link(curr_bb, COND_NEQ, bbt, bbf);
                      break;
  }
  return NULL;
}


struct loop *new_loop(void){
  struct loop *new_loop = malloc(sizeof(struct loop));
  if(new_loop == NULL){
    fprintf(stderr, "ERROR: Unable to allocate new loop: %s\n", strerror(errno));
    return NULL;
  } else{
    new_loop->prev = curr_loop;
  }
  return new_loop;
}


void *gen_for(struct ast_node *node){
  struct basic_block *bb_cond = new_bb();
  struct basic_block *bb_body = new_bb();
  struct basic_block *bb_incr = new_bb();
  struct basic_block *bb_next = new_bb();

  curr_loop = new_loop();
  curr_loop->bb_cont = bb_incr;
  curr_loop->bb_break = bb_next;

  gen_assign(node->u.nfor.init);
  bb_link(curr_bb, ALWAYS, bb_cond, NULL);
  curr_bb = bb_cond;

  gen_cond(node->u.nfor.cond, bb_body, bb_next);

  curr_bb = bb_body;
  gen_stmt(node->u.nfor.body);
  bb_link(curr_bb, ALWAYS, bb_incr, NULL);

  curr_bb = bb_incr;
  gen_stmt(node->u.nfor.incr);
  bb_link(curr_bb, ALWAYS, bb_cond, NULL);

  curr_bb = bb_next;
  curr_loop = curr_loop->prev;
}

void *gen_break(struct ast_node *node){
  bb_link(curr_bb, ALWAYS, curr_loop->bb_break, NULL);
}

void *gen_continue(struct ast_node *node){
  bb_link(curr_bb, ALWAYS, curr_loop->bb_cont, NULL);
}

void *gen_return(struct ast_node *node){
  struct ast_node *n = NULL;
  if(node->u.nreturn.expr != NULL){
    n = gen_rvalue(node->u.nreturn.expr, NULL);
  }
  emit(RET, n, NULL, NULL);
}

void *gen_unop(struct ast_node *node){
  struct ast_node *n = ast_node_alloc(AST_NUMBER);
  n->u.num.intval = 1;

  if(node->u.unop.operator == PLUSPLUS)
    emit('+', node->u.unop.left, n, node->u.unop.left);
  else if(node->u.unop.operator == MINUSMINUS)
    emit('-', node->u.unop.left, n, node->u.unop.left);
}

void *gen_fncall(struct ast_node *node, struct ast_node *target){
  struct ast_node *n = malloc(sizeof(struct ast_node));
  struct ast_node *n2 = node;
  struct ast_node *arg;
  int n_arg = 1;
  int total_args = 0;
  int sort_arg = 0;

  n2 = n2->u.func.args;
  total_args = node->u.func.numargs;
  sort_arg = total_args;

  if (memcpy(n, node, total_args * sizeof(struct ast_node)) == NULL)
    fprintf(stderr, "Error allocating memory for function arguments in gen_fncall: %s\n", strerror(errno));

  n = n->u.func.args;
  while(n != NULL){
    struct ast_node *num = ast_node_alloc(AST_NUMBER);
    num->u.num.intval = total_args - sort_arg;
    if(n->node_type == AST_BINOP || n->node_type == AST_UNOP)
      arg = gen_rvalue(n, NULL);
    else if(n->node_type == AST_EXPR_LIST){
      n = n->u.expr_list.nmember;
      continue;
    }
    else
      arg = n;
    //fprintf(stdout, "ARG %d, %d\n", num->u.num.intval, arg->node_type);
    emit(ARG, num, arg, NULL);
    n_arg++;
    sort_arg--;
    n = n->prev;
  }

  struct ast_node *num_arg = ast_node_alloc(AST_NUMBER);
  num_arg->u.num.intval = total_args;
  emit(CALL, node->u.func.name, num_arg, target);
}



struct basic_block *new_bb(void){
  struct basic_block *bb = malloc(sizeof(struct basic_block));
  if(bb == NULL){
    fprintf(stderr, "ERROR: Unable to allocate new basic block: %s\n", strerror(errno));
    return NULL;
  }
  char name[256] = {0};
  sprintf(name, ".BB.%d.%d", fn, bbN);
  bbN++;
  bb->name = strdup(name);
  bb->q_list = new_quad_list();
  curr_bb_list = bb_list_append(bb, curr_bb_list);
  return bb;
}

struct basic_block_list *new_bb_list(void){
  struct basic_block_list *new_list = malloc(sizeof(struct basic_block_list));
  if(new_list == NULL){
    fprintf(stderr, "ERROR: Unable to allocate new basic block list: %s\n", strerror(errno));
    return NULL;
  } else{
    new_list->size = 0;
    new_list->head = NULL;
    new_list->tail = NULL;
  }
  return new_list;
}

struct basic_block_list *bb_list_append(struct basic_block *bb, struct basic_block_list *l){
  if(l == NULL)
    l = new_bb_list();

  if(l->size == 0){
    l->head = bb;
    l->tail = bb;
  } else{
    l->tail->next = bb;
    l->tail = bb;
  }
  l->size++;

  return l;
}

struct basic_block *bb_link(struct basic_block *bb, int branch, struct basic_block *left, struct basic_block *right){
  bb->branch = branch;
  bb->left = left;
  bb->right = right;
  return bb;
}

void print_bb(struct basic_block *bb){
  if(bb != NULL){
    struct basic_block *bb_t = bb;
    struct quad *q = bb_t->q_list->head;
    printf("%s:\n", bb_t->name);
    while(q != NULL){
      print_quad(q);
      q = q->next;
    }
    printf("\t");
    switch(bb_t->branch){
      case NEVER:     break;
      case ALWAYS:    printf("BR %s\n", bb_t->left->name); break;
      case COND_LT:		printf("BRLT %s %s\n", bb_t->left->name, bb_t->right->name); break;
      case COND_GT:		printf("BRGT %s %s\n", bb_t->left->name, bb_t->right->name); break;
      case COND_LTEQ:	printf("BRLE %s %s\n", bb_t->left->name, bb_t->right->name); break;
      case COND_GTEQ:	printf("BRGE %s %s\n", bb_t->left->name, bb_t->right->name); break;
      case COND_EQ:		printf("BREQ %s %s\n", bb_t->left->name, bb_t->right->name); break;
      case COND_NEQ:	printf("BRNE %s %s\n", bb_t->left->name, bb_t->right->name); break;
      default: break;
    }
    print_bb(bb_t->next);
  }
}

void print_quad(struct quad *q){
  if (q == NULL)
    return;
  printf("\t");
  if (q->dest != NULL){
    if (q->dest->node_type == AST_IDENT){
      struct sym *s = search_all(sym_tab, q->dest->u.ident.name, 0);
      printf("%s", q->dest->u.ident.name);
      if(s->e.var.stg == STG_AUTO)
        printf("{local} = ");
      else
        printf("{globl} = ");
    }
    else if (q->dest->node_type == AST_TEMP){
      printf("%%T%05d = ", q->dest->u.temp.number);
    }
  }
  switch(q->opcode){
    case '+':
          printf("ADD ");
          print_rval(q->src1);
          printf(", ");
          print_rval(q->src2);
          printf("\n");
          break;

    case '-':
          printf("SUB ");
          print_rval(q->src1);
          printf(", ");
          print_rval(q->src2);
          printf("\n");
          break;

    case '*': 	/* MUL */
          printf("MUL ");
          print_rval(q->src1);
          printf(", ");
          print_rval(q->src2);
          printf("\n");
          break;

    case '/': 	/* DIV */
          printf("DIV ");
          print_rval(q->src1);
          printf(", ");
          print_rval(q->src2);
          printf("\n");
          break;

    case LOAD:
            printf("LOAD ");
            if (q->src1->node_type == AST_TEMP)
              printf(" [%%T%05d]", q->src1->u.temp.number);
            else
              print_rval(q->src1);
            printf("\n");
            break;

    case '&':
    case LEA:
            printf("LEA ");
            print_rval(q->src1);
            printf("\n");
            break;

    case MOV:
            printf("MOV ");
            print_rval(q->src1);
            printf("\n");
            break;

    case STORE:
            printf("STORE ");
            print_rval(q->src1);
            printf(", ");
            print_rval(q->src2);
            printf("\n");
            break;

    case CMP:
            printf("CMP ");
            print_rval(q->src1);
            printf(", ");
            print_rval(q->src2);
            printf("\n");
            break;

    case ARG:
            printf("ARG ");
            print_rval(q->src1);
            printf(", ");
            print_rval(q->src2);
            printf("\n");
            break;

    case CALL:
            printf("CALL ");
            print_rval(q->src1);
            printf(", ");
            print_rval(q->src2);
            printf("\n");
            break;

    case RET:
            printf("RETURN ");
            print_rval(q->src1);
            printf("\n");
            break;
    default: break;
  }
}

void print_rval(struct ast_node *node){
  if(node == NULL)
    return;
  if(node->node_type == AST_IDENT){
    struct sym *s = search_all(sym_tab, node->u.ident.name, node->u.ident.type);
    printf("%s", node->u.ident.name);
    if(s->e.var.stg == STG_AUTO)
      printf("{local}");
    else
      printf("{globl}");
  }
  if(node->node_type == AST_TEMP){
    printf("%%T%05d", node->u.temp.number);
  }
  if(node->node_type == AST_NUMBER){
    printf("%llu", node->u.num.intval);
  }
  if(node->node_type == AST_CHARLIT){
    printf("%c", (unsigned char)node->u.charlit.c);
  }
  if(node->node_type == AST_ARR){
    struct ast_node *a = node->u.arr.t;
    while(a != NULL && a->node_type != AST_IDENT){
      a = a->next;
    }
    struct sym *s = search_all(sym_tab, a->u.ident.name, 0);
    printf("%s", s->name);
    if(s->e.var.stg == STG_AUTO)
      printf("{local}");
    else
      printf("{globl}");
  }
  if(node->node_type == AST_STRING){
    for(int i = 0; i < node->u.string.length; i++)
      printf("%c",node->u.string.word[i]);

    gen_rvalue(node, NULL);
  }
}

void *gen_quad(struct ast_node *func, struct ast_node  *stmt, struct sym_tab *sym_tabl, FILE *f){
  sym_tab = sym_tabl;

  curr_bb_list = new_bb_list();
	curr_bb = new_bb();
	curr_bb->q_list = new_quad_list();

  printf("\n\n---------- QUADS BEGIN ----------\n");

	printf("%s:\n", func->u.ident.name);

  struct ast_node *n = stmt;
  while (n != NULL){
    if (n->node_type == AST_IDENT)
      n = n->next;
    gen_stmt(n);
    n = n->next;
  }
  print_bb(curr_bb_list->head);
  printf("\n---------- QUADS END ----------\n\n");

  // TARGET CODE
  outfile = f;
  asm_setup();
  prologue(func); // THE ENDGAME
  asm_bb(curr_bb_list->head);

  fn++;
	bbN = 1;
	temp = 0;
	memset(string_buffer, 0, 4096);
}

void asm_setup(void){
  if(fn != 1)
    return;
  struct sym *s = sym_tab->parent->symsS;

  while(s != NULL){
    if(s->n->node_type == AST_SCALAR || s->n->node_type == AST_ARR){
      int size = get_sizeof(s->n, NULL);
      fprintf(outfile, "\t.comm %s, %d, %d\n", s->name, size, (size == 1 || size == 2)?size:4);
    }
    else if(s->ident == ID_FUNC){
      fprintf(outfile, "\t.globl %s\n", s->name);
      fprintf(outfile, "\t.type %s, @function\n", s->name);
    }
    s = s->next;
  }

  fprintf(outfile, "%s", string_buffer);
}

void prologue(struct ast_node *func){
  struct sym *s = sym_tab->symsS;
  while (s != NULL){
    if(s->n->node_type == AST_SCALAR || s->n->node_type == AST_IDENT || s->n->node_type == AST_ARR){
      if(s->e.var.stg == STG_EXTERN){
        int size = get_sizeof(s->n, NULL);
        fprintf(outfile, "\t.comm %s, %d, %d\n", s->name, size, (size == 1 || size == 2)?size:4);
      }
      else if(s->e.var.stg == STG_STATIC){
        int size = get_sizeof(s->n, NULL);
        fprintf(outfile, "\t.local%s.%d\n", s->name, fn);
        fprintf(outfile, "\t.comm %s.%d, %d, %d\n", s->name, fn, size, (size == 1 || size == 2)?size:4);
      }
      else
        stackSpace = s->frame_offset;
    }

    s = s->next;
  }

  stackSpace += (temp * 4);
  fprintf(outfile, "\t.text\n");
  fprintf(outfile, "%s:\n", func->u.ident.name);
  fprintf(outfile, "\tpushl %%ebp\n");
  fprintf(outfile, "\tmovl %%esp, %%ebp\n");
  fprintf(outfile, "\tsubl $%d, %%esp\n", stackSpace);
}

void asm_print(struct ast_node *node){
  if(node == NULL){
    return;
  }
  if(node->node_type == AST_IDENT){
    struct sym *s = search_all(sym_tab, node->u.ident.name, node->u.ident.type);
    if(s->e.var.stg == STG_AUTO){
      fprintf(outfile, "-%d(%%ebp)", s->frame_offset);
    } else if(s->e.var.stg == STG_STATIC){
      fprintf(outfile, "%s.%d", s->name, fn);
    } else{
      fprintf(outfile, "%s", s->name);
    }
  }

  if(node->node_type == AST_TEMP){
    int temp_offset = (stackSpace - (4 * temp)) + (4 * node->u.temp.number);
    fprintf(outfile, "-%d(%%ebp)", temp_offset);
  }

  if(node->node_type == AST_NUMBER){
    fprintf(outfile, "$%llu", node->u.num.intval);
  }

  if(node->node_type == AST_CHARLIT){
    fprintf(outfile, "$%d", (unsigned char)node->u.charlit.c);
  }

  if(node->node_type == AST_ARR){
    struct ast_node *a = node->u.arr.t;
    while(a != NULL && a->node_type != AST_IDENT){
      a = a->next;
    }
    struct sym *s = search_all(sym_tab, a->u.ident.name, a->u.ident.type);
    if(s->e.var.stg == STG_AUTO){
      fprintf(outfile, "-%d(%%ebp)", s->frame_offset);
    } else if(s->e.var.stg == STG_STATIC){
      fprintf(outfile, "%s.%d", s->name, fn);
    } else{
      fprintf(outfile, "%s", s->name);
    }
  }

  if(node->node_type == AST_STRING){
    fprintf(outfile, ".string_%d", node->u.string.number);
  }
}

void translate_quad(struct quad *q){
  if(q == NULL){
    return;
  }
  fprintf(outfile, "\t");
  switch(q->opcode){
    case '+':   fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\taddl ");
                asm_print(q->src2);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tmovl %%ecx, ");
                asm_print(q->dest);
                fprintf(outfile, "\n");
                break;
    case '-':   fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tsubl ");
                asm_print(q->src2);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tmovl %%ecx, ");
                asm_print(q->dest);
                fprintf(outfile, "\n");
                break;
    case '*':   fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%eax\n");
                fprintf(outfile, "\txor %%edx, %%edx\n");
                fprintf(outfile, "\tmovl ");
                asm_print(q->src2);
                fprintf(outfile, ", %%ebx\n");
                fprintf(outfile, "\tmul %%ebx\n");
                fprintf(outfile, "\tmovl %%eax, ");
                asm_print(q->dest);
                fprintf(outfile, "\n");
                break;
    case '/':   fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%eax\n");
                fprintf(outfile, "\txor %%edx, %%edx");
                fprintf(outfile, "\tmovl ");
                asm_print(q->src2);
                fprintf(outfile, ", %%ebx\n");
                fprintf(outfile, "\tdiv %%ebx\n");
                fprintf(outfile, "\tmovl %%eax, ");
                asm_print(q->dest);
                fprintf(outfile, "\n");
                break;
    case LOAD:  fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tmovl (%%ecx), %%ebx\n");
                fprintf(outfile, "\tmovl %%ebx, ");
                asm_print(q->dest);
                fprintf(outfile, "\n");
                break;
    case '&':
    case LEA:   fprintf(outfile, "lea ");
                asm_print(q->src1);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tmovl %%ecx, ");
                asm_print(q->dest);
                fprintf(outfile, "\n");
                break;
    case MOV:   fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tmovl %%ecx, ");
                asm_print(q->dest);
                fprintf(outfile, "\n");
                break;
    case STORE: fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tmovl ");
                asm_print(q->src2);
                fprintf(outfile, ", %%ebx\n");
                fprintf(outfile, "\tmovl %%ecx, (%%ebx)\n");
                break;
    case CMP:   fprintf(outfile, "movl ");
                asm_print(q->src1);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tmovl ");
                asm_print(q->src2);
                fprintf(outfile, ", %%ebx\n");
                fprintf(outfile, "\tcmp %%ebx, %%ecx\n");
                break;
    case ARG:   fprintf(outfile, "movl ");
                asm_print(q->src2);
                fprintf(outfile, ", %%ecx\n");
                fprintf(outfile, "\tpushl %%ecx\n");
                break;
    case CALL:  fprintf(outfile, "call ");
                asm_print(q->src1);
                fprintf(outfile, "\n");
                struct ast_node *n = ast_node_alloc(AST_NUMBER);
                n->u.num.intval = q->src2->u.num.intval * 4;
                fprintf(outfile, "\taddl ");
                asm_print(n);
                fprintf(outfile, ", %%esp\n");
                if(q->dest != NULL){
                  fprintf(outfile, "\tmovl %%eax, ");
                  asm_print(q->dest);
                  fprintf(outfile, "\n");
                }

    case RET:   if (q->src1 != NULL){
                  fprintf(outfile, "movl ");
                  asm_print(q->src1);
                  fprintf(outfile, ", %%eax\n");
                  fprintf(outfile, "\t");
                }
                fprintf(outfile, "leave\n");
                fprintf(outfile, "\tret\n");
                break;
  }
}

void asm_bb(struct basic_block *block){
  if(block != NULL){
    struct basic_block *bb = block;
    struct quad *q = bb->q_list->head;
    fprintf(outfile, "%s:\n", bb->name);
    while (q != NULL){
      translate_quad(q);
      q = q->next;
    }

    fprintf(outfile, "\t");

    switch (bb->branch){
      case NEVER: 		break;

      case ALWAYS:		fprintf(outfile, "jmp %s\n", bb->left->name);
                    break;

      case COND_LT:		fprintf(outfile, "jl %s\n", bb->left->name);
                     fprintf(outfile, "\tjmp %s\n", bb->right->name);
                     break;

      case COND_GT:		fprintf(outfile, "jg %s\n", bb->left->name);
                     fprintf(outfile, "\tjmp %s\n", bb->right->name);
                     break;

      case COND_LTEQ: 	fprintf(outfile, "jle %s\n", bb->left->name);
                       fprintf(outfile, "\tjmp %s\n", bb->right->name);
                       break;

      case COND_GTEQ: 	fprintf(outfile, "jge %s\n", bb->left->name);
                       fprintf(outfile, "\tjmp %s\n", bb->right->name);
                       break;

      case COND_EQ: 		fprintf(outfile, "je %s\n", bb->left->name);
                      fprintf(outfile, "\tjmp %s\n", bb->right->name);
                      break;

      case COND_NEQ: 		fprintf(outfile, "jne %s\n", bb->left->name);
                       fprintf(outfile, "\tjmp %s\n", bb->right->name);
                       break;
    }
    asm_bb(bb->next);
  }
}
