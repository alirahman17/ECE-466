#include "quad.h"

struct basic_block *curr_bb;

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
  struct ast_node *temp, *left, *right, *addr;
  switch(node->node_type) {
    case AST_IDENT:
    case AST_NUMBER:
    case AST_CHARLIT:
    case AST_STRING: return node;
    case AST_UNOP:
                    if(node->u.unop.operator == '*'){
                      addr = gen_rvalue(node->u.unop.left, NULL);
                      if(!target){
                        target = new_temp();
                      }
                      emit(LOAD,addr,NULL,target);
                      return target;
                    }
    case AST_BINOP:
                    left = gen_rvalue(node->u.binop.left, NULL);
                    right = gen_rvalue(node->u.binop.right, NULL);
                    if(!target){
                      target = new_temp();
                    }
                    emit(node->u.binop.operator, left, right, target);
                    return target;
  }
}

struct ast_node *gen_assign(struct ast_node *node){
  struct ast_node *dest, *t1;
  int dest_mode;
  dest = gen_lvalue(node->u.assign.left, &dest_mode);

  if(dest == NULL){
    fprintf(stderr, "ERROR: Invalid assigment\n");
  }
  if(dest_mode == DIRECT){
    gen_rvalue(node->u.assign.right, dest);
  } else{
    t1 = gen_rvalue(node->u.assign.right, NULL);
    emit(STORE, t1, dest, NULL);
  }
}

struct quad_list * quad_list_append(struct quad *q, struct quad_list *l){
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

void print_quad(struct quad *q){

}
