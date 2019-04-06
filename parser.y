%debug
%{ /* Parser */
#include "ast.h"
#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char* msg) {fprintf(stderr, "%s\n", msg);}
int yylex();
extern int line;
extern char filename[256];
char* print_kw(int token);
int add_stg_class(struct sym *entry);
int enter_scope(int type);
int exit_scope();
int id_type = 0;
int stg;
struct ast_node *identi;
struct ast_node *head;
struct ast_node *tail;
struct sym_tab *curr_scope;
%}
%error-verbose
%union{ /* YYLVAL */
  struct identifier {
      char *name;
  } ident;
	struct stringlit{
      char word[4096];
      int length;
  } string;
	struct number{
      unsigned long long intval;
      long double floatval;
      int sign;
      int type;
  } num;
	char charlit;
  struct ast_node *astn;
}

%token <num.intval> IDENT CHARLIT STRING NUMBER INDSEL PLUSPLUS MINUSMINUS SHL SHR LTEQ GTEQ EQEQ NOTEQ LOGAND LOGOR ELLIPSIS TIMESEQ DIVEQ MODEQ PLUSEQ MINUSEQ SHLEQ SHREQ ANDEQ OREQ XOREQ AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE ENUM EXTERN FLOAT FOR GOTO IF INLINE INT LONG REGISTER RESTRICT RETURN SHORT SIGNED SIZEOF STATIC STRUCT SWITCH TYPEDEF UNION UNSIGNED VOID VOLATILE WHILE _BOOL _COMPLEX _IMAGINARY

%type <astn> primary_expr constant_expr parenthesized_expr
%type <astn> postfix_expr subscript_expr component_expr function_call postincrement_expr postdecrement_expr
%type <astn> expr_list cast_expr unary_expr sizeof_expr unary_minus unary_plus log_neg_expr bit_neg_expr addr_expr deref_expr
%type <astn> preincr_expr predecr_expr mult_expr add_expr shift_expr relation_expr equal_expr
%type <astn> bit_or_expr bit_xor_expr bit_and_expr log_or_expr log_and_expr conditional_expr assignment_expr expr
%type <num.intval> assignment_op
%type <astn> expr_statement
%type <astn> decl_func_list decl_func func decl_stmt_list decl_stmt compound_stmt decl decl_specs type_spec type_qual stg_spec direct_decl declarator pointer type_qual_list decl_list
%type <astn> stmt iter_stmt for_stmt while_stmt init_clause switch_stmt return_stmt continue_stmt break_stmt goto_stmt label named_label case_label labeled_stmt

%left ','
%right TIMESEQ
%right '='
%right '?' ':'
%left LOGOR
%left LOGAND
%left '|'
%left '^'
%left '&'
%left EQEQ NOTEQ
%left '>' '<' LTEQ GTEQ
%left SHL SHR
%left '+' '-'
%left '*' '/' '%'
%left PLUSPLUS MINUSMINUS
%left '.'
%left '[' ']'
%left '(' ')'

%start decl_func_list

%%

decl_func_list : decl_func {}
               | decl_func_list decl_func {}
               ;

decl_func      : decl_stmt {}
               | func {}
               ;

func           : decl_specs declarator {
                struct sym *search = NULL;
                if(head->u.ident.name != NULL){
                  search = search_all(curr_scope, head->u.ident.name, ID_FUNC);
                }
                if(search == NULL){
                  ast_node_link(&head, &tail, $1);
                  struct sym *n = add_sym(head, curr_scope, filename, line);
                  add_stg_class(n);
                  head = (struct ast_node *)NULL;
                  tail = (struct ast_node *)NULL;
                  print_sym(n, 0);
                } else if(search->e.func.complete == 1){
                  /* previously defined func */
                } else{
                  print_sym(search, 0);
                  head = (struct ast_node *)NULL;
                  tail = (struct ast_node *)NULL;
                }
                enter_scope(SCOPE_FUNC);
               } '{'  decl_stmt_list '}' {
                struct sym_tab *tmp = curr_scope;
                exit_scope();
                curr_scope->symsE->e.func.complete = 1;
                fprintf(stdout, "AST Dump for function %s\n LIST {\n",curr_scope->symsE->name);
                print_ast($5, 2);
                fprintf(stdout, " }\n");
                /* PRINT AST DUMP FOR FUNC */
               }
               ;

decl_stmt_list : decl_stmt {$$ = $1;}
               | decl_stmt_list decl_stmt {
                 $$ = ast_node_alloc(AST_TOP_EXPR_ST);
                 $$->u.top_expr_st.left = $1;
                 $$->u.top_expr_st.right = $2;
               }
               ;

decl_stmt      : decl {$$ = $1;}
               | stmt {$$ = $1;}
               ;

init_clause    : expr
               | decl
               ;

stmt           : expr_statement {$$ = $1;}
               | compound_stmt {$$ = $1;}
               | iter_stmt {$$ = $1;}
               | switch_stmt {$$ = $1;}
               | return_stmt {$$ = $1;}
               | continue_stmt {$$ = $1;}
               | break_stmt {$$ = $1;}
               | goto_stmt {$$ = $1;}
               | labeled_stmt {$$ = $1;}
               | ';' {
                 /* NULL STATEMENT */
               }
               ;

switch_stmt    : SWITCH '(' expr ')' stmt {
                struct ast_node *n = ast_node_alloc(AST_SWITCH);
                n->u.nswitch.expr = $3;
                n->u.nswitch.body = $5;
                $$ = n;
               }
               ;

return_stmt    : RETURN expr ';' {
                struct ast_node *n = ast_node_alloc(AST_RETURN);
                n->u.nreturn.expr = $2;
                $$ = n;
               }
               | RETURN ';' {
                struct ast_node *n = ast_node_alloc(AST_RETURN);
                n->u.nreturn.expr = ast_node_alloc(AST_NULL);
                $$ = n;
               }
               ;

continue_stmt  : CONTINUE ';' {
                struct ast_node *n = ast_node_alloc(AST_CONTINUE);
                $$ = n;
               }
               ;

break_stmt     : BREAK ';' {
                struct ast_node *n = ast_node_alloc(AST_BREAK);
                $$ = n;
               }
               ;

goto_stmt      : GOTO named_label ';' {
                struct ast_node *n = ast_node_alloc(AST_GOTO);
                n->u.ngoto.label = $2;
                $$ = n;
               }
               ;

named_label    : IDENT {
                struct ast_node *n = ast_node_alloc(AST_LABEL);
                n->u.nlabel.label = strdup((const char *)$1);
                n->u.nlabel.icase = 1;
                $$ = n;
               }
               ;

labeled_stmt   : label ':' stmt {
                struct ast_node *n = ast_node_alloc(AST_LSTMT);
                n->u.lstmt.label = $1;
                n->u.lstmt.stmt = $3;
                $$ = n;
               }
               ;

case_label     : CASE conditional_expr {
                struct ast_node *n = ast_node_alloc(AST_CASE);
                n->u.lcase.label = $2;
                $$ = n;
               }
               ;

label          : named_label {$$ = $1;}
               | case_label {$$ = $1;}
               | DEFAULT {
                struct ast_node *n = ast_node_alloc(AST_LABEL);
                n->u.nlabel.icase = 2;
                $$ = n;
               }
               ;


compound_stmt  : {
                enter_scope(SCOPE_BLOCK);
               } '{' decl_stmt_list '}' {
                exit_scope();
                $$ = $3;
               }
               ;

decl           : decl_specs ';' {}
               | decl_specs init_decl_list ';' {}
               ;

decl_list      : decl_specs
               | decl_list ',' decl_specs
               ;

decl_specs     : type_spec {
                $$ = $1;
               }
               | type_spec decl_specs {
                $1->next = $2;
                $$ = $1;
               }
               | type_qual {
                $$ = $1;
               }
               | type_qual decl_specs {
                 $1->next = $2;
                 $$ = $1;
               }
               | stg_spec decl_specs {
                 $$ = $2;
               }
               ;

init_decl_list : declarator {
                ast_node_link(&head, &tail,$<astn>0);
                struct sym *n = add_sym(head, curr_scope, filename, line);
                add_stg_class(n);
                print_sym(n, 0);
                head = (struct ast_node *)NULL;
                tail = (struct ast_node *)NULL;
               }
               | init_decl_list ',' declarator {
                 ast_node_link(&head, &tail,$<astn>0);
                 struct sym *n = add_sym(head, curr_scope, filename, line);
                 add_stg_class(n);
                 print_sym(n, 0);
                 head = (struct ast_node *)NULL;
                 tail = (struct ast_node *)NULL;
               }
               ;

stg_spec       : EXTERN {
                stg = STG_EXTERN;
               }
               | STATIC {
                stg = STG_STATIC;
               }
               | REGISTER {
                stg = STG_REG;
               }
               | AUTO {
                stg = STG_AUTO;
               }
               ;

type_spec      : VOID {
                struct ast_node *n = ast_node_alloc(AST_SCALAR);
                n->u.scalar.type = 315;
                $$ = n;
               }
               | CHAR {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 287;
                 $$ = n;
               }
               | SHORT {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 306;
                 $$ = n;
               }
               | INT {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 301;
                 $$ = n;
               }
               | LONG {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 302;
                 $$ = n;
               }
               | FLOAT {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 296;
                 $$ = n;
               }
               | DOUBLE {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 292;
                 $$ = n;
               }
               | SIGNED {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 307;
                 $$ = n;
               }
               | UNSIGNED {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 314;
                 $$ = n;
               }
               | _BOOL {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 318;
                 $$ = n;
               }
               | _COMPLEX {
                 struct ast_node *n = ast_node_alloc(AST_SCALAR);
                 n->u.scalar.type = 319;
                 $$ = n;
               }
               ;

type_qual      : CONST {
                struct ast_node *n = ast_node_alloc(AST_QUAL);
                n->u.scalar.qual = 288;
                n->u.scalar.type = 301;
                $$ = n;
               }
               | RESTRICT {
                 struct ast_node *n = ast_node_alloc(AST_QUAL);
                 n->u.scalar.qual = 304;
                 n->u.scalar.type = 301;
                 $$ = n;
               }
               | VOLATILE {
                 struct ast_node *n = ast_node_alloc(AST_QUAL);
                 n->u.scalar.qual = 316;
                 n->u.scalar.type = 301;
                 $$ = n;
               }
               ;

iter_stmt      : while_stmt
               | for_stmt
               ;

while_stmt     : WHILE '(' expr ')' stmt {
                struct ast_node *n = ast_node_alloc(AST_WHILE);
                n->u.nwhile.expr = $3;
                n->u.nwhile.body = $5;
                $$ = n;
               }
               ;

for_stmt       : FOR '(' init_clause ';' expr ';' expr ')' stmt {
                struct ast_node *n = ast_node_alloc(AST_FOR);
                n->u.nfor.init = $3;
                n->u.nfor.cond = $5;
                n->u.nfor.body = $9;
                n->u.nfor.incr = $7;
                $$ = n;
               }
               | FOR '(' ';' expr ';' expr ')' stmt {
                 struct ast_node *n = ast_node_alloc(AST_FOR);
                 n->u.nfor.init = ast_node_alloc(AST_NULL);
                 n->u.nfor.cond = $4;
                 n->u.nfor.body = $8;
                 n->u.nfor.incr = $6;
                 $$ = n;
               }
               | FOR '(' init_clause ';'  ';' expr ')' stmt {
                 struct ast_node *n = ast_node_alloc(AST_FOR);
                 n->u.nfor.init = $3;
                 n->u.nfor.cond = ast_node_alloc(AST_NULL);
                 n->u.nfor.body = $8;
                 n->u.nfor.incr = $6;
                 $$ = n;
               }
               | FOR '(' init_clause ';' expr ';' ')' stmt {
                 struct ast_node *n = ast_node_alloc(AST_FOR);
                 n->u.nfor.init = $3;
                 n->u.nfor.cond = $5;
                 n->u.nfor.body = $8;
                 n->u.nfor.incr = ast_node_alloc(AST_NULL);
                 $$ = n;
               }
               | FOR '(' init_clause ';' ';' ')' stmt {
                 struct ast_node *n = ast_node_alloc(AST_FOR);
                 n->u.nfor.init = $3;
                 n->u.nfor.cond = ast_node_alloc(AST_NULL);
                 n->u.nfor.body = $7;
                 n->u.nfor.incr = ast_node_alloc(AST_NULL);
                 $$ = n;
               }
               | FOR '(' ';' ';' expr ')' stmt {
                 struct ast_node *n = ast_node_alloc(AST_FOR);
                 n->u.nfor.init = ast_node_alloc(AST_NULL);
                 n->u.nfor.cond = ast_node_alloc(AST_NULL);
                 n->u.nfor.body = $7;
                 n->u.nfor.incr = $5;
                 $$ = n;
               }
               | FOR '(' ';' expr ';' ')' stmt {
                 struct ast_node *n = ast_node_alloc(AST_FOR);
                 n->u.nfor.init = ast_node_alloc(AST_NULL);
                 n->u.nfor.cond = $4;
                 n->u.nfor.body = $7;
                 n->u.nfor.incr = ast_node_alloc(AST_NULL);
                 $$ = n;
               }
               | FOR '(' ';' ';' ')' stmt {
                 struct ast_node *n = ast_node_alloc(AST_FOR);
                 n->u.nfor.init = ast_node_alloc(AST_NULL);
                 n->u.nfor.cond = ast_node_alloc(AST_NULL);
                 n->u.nfor.body = $6;
                 n->u.nfor.incr = ast_node_alloc(AST_NULL);
                 $$ = n;
               }
               ;

declarator     : direct_decl {
                $$ = $1;
               }
               | pointer direct_decl {
                ast_node_link(&head, &tail, $1);
                $$ = head;
               }
               ;

direct_decl    : IDENT {
                 struct ast_node *n = ast_node_alloc(AST_IDENT);
                 n->u.ident.name = strdup((const char *)$1);
                 n->u.ident.type = ID_VAR;
                 ast_node_link(&head, &tail, n);
                 $$ = head;
               }
               | '(' declarator ')' {
                 $$ = head;
               }
               | direct_decl '[' ']' {
                 struct ast_node *n = ast_node_alloc(AST_ARR);
                 n->u.arr.num = 0;
                 ast_node_link(&head, &tail, n);
                 $$ = head;
               }
               | direct_decl '[' NUMBER ']' {
                 struct ast_node *n = ast_node_alloc(AST_ARR);
                 n->u.arr.num = $3;
                 ast_node_link(&head, &tail, n);
                 $$ = head;
               }
               | direct_decl '(' ')' {
                 $1->u.ident.type = ID_FUNC;
                 $$ = head;
               }
               | direct_decl '(' decl_list ')' {
                 $1->u.ident.type = ID_FUNC;
                 $$ = head;
               }
               ;

pointer        : '*' {
                 struct ast_node *n = ast_node_alloc(AST_POINTER);
                 $$ = n;
               }
               | '*' type_qual_list {
                 struct ast_node *n = ast_node_alloc(AST_POINTER);
                 $$ = n;
               }
               | '*' pointer {
                 struct ast_node *n = ast_node_alloc(AST_POINTER);
                 n->next = $2;
                 $$ = n;
               }
               | '*' type_qual_list pointer {
                 struct ast_node *n = ast_node_alloc(AST_POINTER);
                 n->next = $3;
                 $$ = n;
               }
               ;

type_qual_list : type_qual {
                $$ = $1;
               }
               | type_qual_list type_qual {

               }
               ;

expr_statement : expr ';' {
                $$ = ast_node_alloc(AST_TOP_EXPR);
                $$->u.top_expr.left = $1;
                //fprintf(stdout, "\n\n-------------- LINE %d --------------\n", line);
                //print_ast($$, 0);
               }
               | expr_statement expr ';' {
                $$ = ast_node_alloc(AST_TOP_EXPR_ST);
                $$->u.top_expr_st.left = $1;
                $$->u.top_expr_st.right = $2;
                //fprintf(stdout, "\n\n-------------- LINE %d --------------\n", line);
                //print_ast($$, 0);
               }
               //| compound_stmt {}
               ;

primary_expr   : IDENT {
                          struct sym *n = search_all(curr_scope, (char *)$1, ID_VAR);
                          if(n != NULL){
                            $$ = ast_node_alloc(AST_IDENT);
                            $$->u.ident.name = strdup((char *)$1);
                            $$->u.ident.fntype = 0;
                            $$->u.ident.fname = strdup(n->fname);
                            $$->u.ident.line = n->line;
                          } else {
                            $$ = ast_node_alloc(AST_IDENT);
                            $$->u.ident.name = strdup((char *)$1);
                            $$->u.ident.fntype = 0;
                          }
                          identi = $$;
                          id_type = 0;
               }
               | constant_expr
               | parenthesized_expr
               ;

constant_expr  : NUMBER {
                $$ = ast_node_alloc(AST_NUMBER);
                $$->u.num.intval = yylval.num.intval;
                $$->u.num.floatval = yylval.num.floatval;
                $$->u.num.sign = yylval.num.sign;
                $$->u.num.type = yylval.num.type;
               }
               | CHARLIT {
                $$ = ast_node_alloc(AST_CHARLIT);
                $$->u.charlit.c = yylval.charlit;
               }
               | STRING {
                $$ = ast_node_alloc(AST_STRING);
                strncpy($$->u.string.word, yylval.string.word, yylval.string.length);
                $$->u.string.length = yylval.string.length;
               }
               ;

parenthesized_expr  : '(' expr ')'  {$$ = $2;}
                    ;

postfix_expr   : primary_expr {$$ = $1;}
               | subscript_expr
               | component_expr
               | function_call {id_type = 1;}
               | postincrement_expr
               | postdecrement_expr
               ;

subscript_expr : postfix_expr '[' expr ']' {
                $$ = ast_node_alloc(AST_UNOP);
                $$->u.unop.operator = '*';
                struct ast_node *next = ast_node_alloc(AST_BINOP);
                $$->u.unop.left = next;
                next->u.binop.operator = '+';
                next->u.binop.left = $1;
                next->u.binop.right = $3;
               }
               ;

component_expr : postfix_expr '.' IDENT {
                $$ = ast_node_alloc(AST_COMP_SELECT);
                $$->u.comp_select.name = $1;
                $$->u.comp_select.member = ast_node_alloc(AST_IDENT);
                $$->u.comp_select.member->u.ident.name = yylval.ident.name;
               }
               | postfix_expr INDSEL IDENT {
                $$ = ast_node_alloc(AST_COMP_SELECT);
                $$->u.comp_select.member = ast_node_alloc(AST_IDENT);
                $$->u.comp_select.member->u.ident.name = yylval.ident.name;
                struct ast_node *p = ast_node_alloc(AST_UNOP);
                p->u.unop.operator = '*';
                p->u.unop.left = $1;
                $$->u.comp_select.name = p;
               }
               ;

function_call  :  postfix_expr '(' ')' {
                    identi->u.ident.fntype = 1;
                    id_type = 1;
                    $$ = ast_node_alloc(AST_FUNC);
                    $$->u.func.name = $1;
                    $$->u.func.args = NULL;
               }
               |  postfix_expr '(' expr_list ')' {
                   identi->u.ident.fntype = 1;
                   id_type = 1;
                   $$ = ast_node_alloc(AST_FUNC);
                   $$->u.func.name = $1;
                   $$->u.func.args = $3;
               }
               ;

expr_list      : assignment_expr
               | expr_list ',' assignment_expr {
                $$ = ast_node_alloc(AST_EXPR_LIST);
                $$->u.expr_list.omember = $1;
                $$->u.expr_list.nmember = $3;
               }
               ;

postincrement_expr : postfix_expr PLUSPLUS {
                    $$ = ast_node_alloc(AST_UNOP);
                    $$->u.unop.operator = PLUSPLUS;
                    $$->u.unop.left = $1;
                   }
                   ;

postdecrement_expr : postfix_expr MINUSMINUS {
                    $$ = ast_node_alloc(AST_UNOP);
                    $$->u.unop.operator = MINUSMINUS;
                    $$->u.unop.left = $1;
                   }
                   ;


cast_expr      : unary_expr
               ;

unary_expr     : postfix_expr {
                  if(identi->u.ident.fntype == 0 && id_type == 0){
                    struct sym *n = search_all(curr_scope, identi->u.ident.name, ID_VAR);

                  } else{
                    struct sym *n = search_all(curr_scope, identi->u.ident.name, ID_FUNC);
                    if(n != NULL){

                    } else{
                      identi->u.ident.line = line;
                      identi->u.ident.fname = strdup(filename);
                      add_sym(identi, curr_scope, filename, line);
                    }

                  } id_type = 0;
                }
               | sizeof_expr
               | unary_minus
               | unary_plus
               | log_neg_expr
               | bit_neg_expr
               | addr_expr
               | deref_expr
               | preincr_expr
               | predecr_expr
               ;

sizeof_expr    : SIZEOF unary_expr {
                $$ = ast_node_alloc(AST_SIZEOF);
                $$->u.size_of.left = $2;
               }
               ;

unary_minus    : '-' cast_expr {
                $$ = ast_node_alloc(AST_UNOP);
                $$->u.unop.operator = '-';
                $$->u.unop.left = $2;
               }
               ;

unary_plus     : '+' cast_expr {
                $$ = ast_node_alloc(AST_UNOP);
                $$->u.unop.operator = '+';
                $$->u.unop.left = $2;
               }
               ;

log_neg_expr   : '!' cast_expr {
                $$ = ast_node_alloc(AST_UNOP);
                $$->u.unop.operator = '!';
                $$->u.unop.left = $2;
               }
               ;

bit_neg_expr   : '~' cast_expr {
                $$ = ast_node_alloc(AST_UNOP);
                $$->u.unop.operator = '~';
                $$->u.unop.left = $2;
               }
               ;

addr_expr      : '&' cast_expr {
                $$ = ast_node_alloc(AST_UNOP);
                $$->u.unop.operator = '&';
                $$->u.unop.left = $2;
               }
               ;

deref_expr     : '*' cast_expr {
                $$ = ast_node_alloc(AST_UNOP);
                $$->u.unop.operator = '*';
                $$->u.unop.left = $2;
               }
               ;

preincr_expr   : PLUSPLUS unary_expr {
                $$ = ast_node_alloc(AST_ASSIGN);
                $$->u.assign.left = $2;
                struct ast_node *binop = ast_node_alloc(AST_BINOP);
                binop->u.binop.operator = '+';
                binop->u.binop.left = $2;
                struct ast_node *one = ast_node_alloc(AST_NUMBER);
                one->u.num.type = INT_T;
                one->u.num.intval = 1;
                binop->u.binop.right = one;
                $$->u.assign.right = binop;
               }
               ;

predecr_expr   : MINUSMINUS unary_expr {
                $$ = ast_node_alloc(AST_ASSIGN);
                $$->u.assign.left = $2;
                struct ast_node *binop = ast_node_alloc(AST_BINOP);
                binop->u.binop.operator = '-';
                binop->u.binop.left = $2;
                struct ast_node *one = ast_node_alloc(AST_NUMBER);
                one->u.num.type = INT_T;
                one->u.num.intval = 1;
                binop->u.binop.right = one;
                $$->u.assign.right = binop;
               }
               ;

mult_expr      : cast_expr
               | mult_expr '*' cast_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '*';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | mult_expr '/' cast_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '/';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | mult_expr '%' cast_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '%';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

add_expr       : mult_expr
               | add_expr '+' mult_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '+';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | add_expr '-' mult_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '-';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

shift_expr     : add_expr
               | shift_expr SHL add_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = SHL;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | shift_expr SHR add_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = SHR;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

relation_expr  : shift_expr
               | relation_expr '<' shift_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '<';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | relation_expr LTEQ shift_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = LTEQ;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | relation_expr '>' shift_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '>';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | relation_expr GTEQ shift_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = GTEQ;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

equal_expr     : relation_expr
               | equal_expr EQEQ relation_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = EQEQ;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               | equal_expr NOTEQ relation_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = NOTEQ;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

bit_or_expr    : bit_xor_expr
               | bit_or_expr '|' bit_xor_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '|';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

bit_xor_expr   : bit_and_expr
               | bit_xor_expr '^' bit_and_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '^';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

bit_and_expr   : equal_expr
               | bit_and_expr '&' equal_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = '&';
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

log_or_expr    : log_and_expr
               | log_or_expr LOGOR log_and_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = LOGOR;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

log_and_expr   : bit_or_expr
               | log_and_expr LOGAND bit_or_expr {
                $$ = ast_node_alloc(AST_BINOP);
                $$->u.binop.operator = LOGAND;
                $$->u.binop.left = $1;
                $$->u.binop.right = $3;
               }
               ;

conditional_expr : log_or_expr
                 | log_or_expr '?' expr ':' conditional_expr {
                   $$ = ast_node_alloc(AST_IF_ELSE);
                   $$->u.if_else.cond = $1;
                   $$->u.if_else.if_true = $3;
                   $$->u.if_else.if_false = $5;
                 }
                 ;

                 /* INSERT IF and IF-ELSE STATEMENTS HERE */

assignment_expr :  conditional_expr {}
                |  unary_expr assignment_op assignment_expr {
                 $$ = ast_node_alloc(AST_ASSIGN);
                 $$->u.assign.left = $1;
                 if($2 == '=')
                  $$->u.assign.right = $3;
                 else{
                  struct ast_node *binop = ast_node_alloc(AST_BINOP);
                  binop->u.binop.left = $1;
                  switch($2){
                    case PLUSEQ:  binop->u.binop.operator = '+'; break;
                    case MINUSEQ: binop->u.binop.operator = '-'; break;
                    case TIMESEQ: binop->u.binop.operator = '*'; break;
                    case DIVEQ:   binop->u.binop.operator = '/'; break;
                    case MODEQ:   binop->u.binop.operator = '%'; break;
                    case SHLEQ:   binop->u.binop.operator = SHL; break;
                    case SHREQ:   binop->u.binop.operator = SHR; break;
                    case ANDEQ:   binop->u.binop.operator = '&'; break;
                    case XOREQ:   binop->u.binop.operator = '^'; break;
                    case OREQ:    binop->u.binop.operator = '|'; break;
                  }
                  binop->u.binop.right = $3;
                  $$->u.assign.right = binop;
                 }
                }
                ;

assignment_op  : '='      {$$ = '=';}
               | PLUSEQ   {$$ = PLUSEQ;}
               | MINUSEQ  {$$ = MINUSEQ;}
               | TIMESEQ  {$$ = TIMESEQ;}
               | DIVEQ    {$$ = DIVEQ;}
               | MODEQ    {$$ = MODEQ;}
               | SHLEQ    {$$ = SHLEQ;}
               | SHREQ    {$$ = SHREQ;}
               | ANDEQ    {$$ = ANDEQ;}
               | XOREQ    {$$ = XOREQ;}
               | OREQ     {$$ = OREQ;}
               ;

expr           : assignment_expr { id_type = 0;/*print_ast($$, 0);*/}
               | expr ',' assignment_expr {
                 $$ = ast_node_alloc(AST_EXPR_LIST);
                 $$->u.expr_list.omember = $1;
                 $$->u.expr_list.nmember = $3;
               }
               ;



%%

int enter_scope(int type){
  struct sym_tab *new_tab = new_sym_table(type, line);
  new_tab->parent = curr_scope;
  curr_scope = new_tab;
}

int exit_scope(){
  curr_scope = curr_scope->parent;
}

int add_stg_class(struct sym *entry){
  if(stg == 0){
    if(entry->curr_tab->type == SCOPE_GLOB){
      stg = STG_EXTERN;
    } else if(entry->curr_tab->type == SCOPE_FUNC || entry->curr_tab->type == SCOPE_BLOCK){
      stg = STG_AUTO;
    } else{
      // BROKEN
    }
  }
  if(entry->ident == ID_VAR){
    entry->e.var.stg = stg;
  } else if(entry->ident == ID_FUNC){
    entry->e.var.stg = stg;
  }
  stg = 0;
}

int main(int argc, char **argv){
  curr_scope = new_sym_table(SCOPE_GLOB, line);
  return yyparse();
}
