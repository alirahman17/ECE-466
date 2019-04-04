%option noyywrap
%{
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
/*
#include "lexer.h"
#include "tokens-manual.h"
*/
#include "def.h"
#include "parser.tab.h"
char filename[256] = "<stdin>";
int mode = 0;
int line = 1;
void numInfo(char *buf, int sign, int type, int numType);
void stringAppend(char *buf, char c);
void yywarn(char *filename, int line, char *msg);
void yyerrorL(char *filename, int line, char *msg);
%}
%x CHARS
%x STRINGS
%x ESCS
%x COMMENTS
%x INFOS

INT	  [1-9]+[0-9]*
OCT	  0[0-7]*
HEX	  0[xX][a-fA-F0-9]+
FLO   [0-9]*\.[0-9]*
EXP   [eE][+-]?{INT}+
P     [pP][+-]?{INT}+
%%

  /* LINE NUMBERS */
#                           {line = 1; BEGIN(INFOS);}
<INFOS>^[0-9]+              {line = (int)strtol(yytext, (char **)NULL, 10);}
<INFOS>\"(\\.|[^\\\"])*\"   {  /* " */
                            strncpy(filename, yytext + 1, strlen(yytext) - 2);
                            filename[strlen(yytext) - 2] = '\0';
                            }
<INFOS>.                    {;}
<INFOS>\n                   {BEGIN(INITIAL);}

  /* WHITESPACES */
[\t\v\f ]+   {;}

  /* KEYWORDS */
"auto"          {return AUTO;}
"break"         {return BREAK;}
"case"          {return CASE;}
"char"          {return CHAR;}
"const"         {return CONST;}
"continue"      {return CONTINUE;}
"default"       {return DEFAULT;}
"do"            {return DO;}
"double"        {return DOUBLE;}
"else"          {return ELSE;}
"enum"          {return ENUM;}
"extern"        {return EXTERN;}
"float"         {return FLOAT;}
"for"           {return FOR;}
"goto"          {return GOTO;}
"if"            {return IF;}
"inline"        {return INLINE;}
"int"           {return INT;}
"long"          {return LONG;}
"register"      {return REGISTER;}
"restrict"      {return RESTRICT;}
"return"        {return RETURN;}
"short"         {return SHORT;}
"signed"        {return SIGNED;}
"sizeof"        {return SIZEOF;}
"static"        {return STATIC;}
"struct"        {return STRUCT;}
"switch"        {return SWITCH;}
"typedef"       {return TYPEDEF;}
"union"         {return UNION;}
"unsigned"      {return UNSIGNED;}
"void"          {return VOID;}
"volatile"      {return VOLATILE;}
"while"         {return WHILE;}
"_Bool"         {return _BOOL;}
"_Complex"      { return _COMPLEX;}
"_Imaginary"    {return _IMAGINARY;}

  /* IDENTIFIERS */
[a-zA-Z_][a-zA-Z0-9_]*  {yylval.ident.name = strdup(yytext); return IDENT;}

  /* CHARLIT */
L?\'                    {mode = CHARLIT; BEGIN(CHARS);}

<CHARS>[^\n\\\']+       {
                          if(yyleng > 1){
                            yywarn(filename, line, "Unsupported multibyte character literal truncated to first byte");
                          }
                          yylval.charlit = yytext[0];
                        }
<CHARS>\\               {
                          BEGIN(ESCS);
                        }
<CHARS>\n               {
                          yyerrorL(filename, line, "Unexpected newline");
                          exit(-1);
                        }
<CHARS>\'               {
                          BEGIN(INITIAL);
                          mode = 0;
                          return CHARLIT;
                        }

  /* STRING */
\"                      {
                          yylval.string.length = 0;
                          mode = STRING;
                          BEGIN(STRINGS);
                        }
<STRINGS>[^\n\\\"]*     {
                          stringAppend(yytext, '\0');
                        }
<STRINGS>\\             {
                          BEGIN(ESCS);
                        }
<STRINGS>\"             {
                          BEGIN(INITIAL);
                          mode = 0;
                          return STRING;
                        }

  /* ESCAPE */
<ESCS>[0abfnrtv'"\\?]	  { /* "' */
                          char esc;
                          switch(yytext[0]){
                            case '0': 	esc = '\0'; break;
                            case 'a': 	esc = '\a'; break;
                            case 'b': 	esc = '\b'; break;
                            case 'f': 	esc = '\f'; break;
                            case 'n': 	esc = '\n'; break;
                            case 'r': 	esc = '\r'; break;
                            case 't': 	esc = '\t'; break;
                            case 'v': 	esc = '\v'; break;
                            case '\'': 	esc = '\''; break;
                            case '"': 	esc = '\"'; break;
                            case '\\': 	esc = '\\'; break;
                            case '?':	  esc = '\?'; break;
                          }
                          switch(mode){
                            case CHARLIT:	yylval.charlit = esc;
                                          BEGIN(CHARS);
                                          break;

                            case STRING:	stringAppend((char *)NULL, esc);
                                          BEGIN(STRINGS);
                          }
                        }
<ESCS>[0-7]{1,3}        {
                          int oct;
                          sscanf(yytext, "%o", &oct);
                          if (oct > 255){
                            fprintf(stderr, "%s: %d: Warning: Octal escape sequence \\%s out of range\n", filename, line, yytext);
                            oct = 255;
                          }
                          char octC = (char)oct;
                          switch(mode){
                            case CHARLIT:	yylval.charlit = octC;
                                          BEGIN(CHARS);
                                          break;

                            case STRING:	stringAppend((char *)NULL, octC);
                                          BEGIN(STRINGS);
                          }
                        }
<ESCS>[xX][0-9a-fA-F]+  {
                          int hex;
                          sscanf(yytext + 1, "%x", &hex);
                          if (hex > 255){
                            fprintf(stderr, "%s: %d: Warning: Hex escape sequence \\%s out of range\n", filename, line, yytext);
                            hex = 255;
                          }
                          char hexC = (char)hex;
                          switch(mode){
                            case CHARLIT:	yylval.charlit = hexC;
                                          BEGIN(CHARS);
                                          break;

                            case STRING:	stringAppend((char *)NULL, hexC);
                                          BEGIN(STRINGS);
                          }
                        }

  /* OPERATORS */
[-~!%^&*+=\|<.>/?]	{return yytext[0];}
"->"            {return INDSEL;}
"++"            {return PLUSPLUS;}
"--"            {return MINUSMINUS;}
"<<"            {return SHL;}
">>"            {return SHR;}
"<="            {return LTEQ;}
">="            {return GTEQ;}
"=="            {return EQEQ;}
"!="            {return NOTEQ;}
"&&"            {return LOGAND;}
"||"            {return LOGOR;}
"..."           {return ELLIPSIS;}
"*="            {return TIMESEQ;}
"/="            {return DIVEQ;}
"%="            {return MODEQ;}
"+="            {return PLUSEQ;}
"-="            {return MINUSEQ;}
"<<="           {return SHLEQ;}
">>="           {return SHREQ;}
"&="            {return ANDEQ;}
"|="            {return OREQ;}
"^="            {return XOREQ;}

  /* PUNCTUATION */
[](){}[:;,]     {return yytext[0];}

  /* NUMBER */
{INT}           {
                  numInfo(yytext, SIGNED_T, INT_T, INTQ);
                  return NUMBER;
                }
{INT}"L"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, SIGNED_T, LONG_T, INTQ);
                  return NUMBER;
                }
{INT}"LL"       {
                  yytext[yyleng - 2] = '\0';
                  numInfo(yytext, SIGNED_T, LONGLONG_T, INTQ);
                  return NUMBER;
                }
{INT}"U"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, UNSIGNED_T, INT_T, INTQ);
                  return NUMBER;
                }
{INT}"LLU"      {
                  yytext[yyleng - 3] = '\0';
                  numInfo(yytext, UNSIGNED_T, LONGLONG_T, INTQ);
                  return NUMBER;
                }
{OCT}           {
                  numInfo(yytext, SIGNED_T, INT_T, OCT);
                  return NUMBER;
                }
{OCT}"L"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, SIGNED_T, LONG_T, OCT);
                  return NUMBER;
                }
{OCT}"LL"       {
                  yytext[yyleng - 2] = '\0';
                  numInfo(yytext, SIGNED_T, LONGLONG_T, OCT);
                  return NUMBER;
                }
{OCT}"U"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, UNSIGNED_T, INT_T, OCT);
                  return NUMBER;
                }
{OCT}"LLU"      {
                  yytext[yyleng - 3] = '\0';
                  numInfo(yytext, UNSIGNED_T, LONGLONG_T, OCT);
                  return NUMBER;
                }
{HEX}           {
                  numInfo(yytext, SIGNED_T, INT_T, HEX);
                  return NUMBER;
                }
{HEX}"L"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, SIGNED_T, LONG_T, HEX);
                  return NUMBER;
                }
{HEX}"LL"       {
                  yytext[yyleng - 2] = '\0';
                  numInfo(yytext, SIGNED_T, LONGLONG_T, HEX);
                  return NUMBER;
                }
{HEX}"U"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, UNSIGNED_T, INT_T, HEX);
                  return NUMBER;
                }
{HEX}"LLU"      {
                  yytext[yyleng - 3] = '\0';
                  numInfo(yytext, UNSIGNED_T, LONGLONG_T, HEX);
                  return NUMBER;
                }
{HEX}{P}        {
                  numInfo(yytext, SIGNED_T, DOUBLE_T, FLO);
                  return NUMBER;
                }
{FLO}           {
                  numInfo(yytext, SIGNED_T, DOUBLE_T, FLO);
                  return NUMBER;
                }
{FLO}"L"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, SIGNED_T, LONGDOUBLE_T, FLO);
                  return NUMBER;
                }
{FLO}"f"        {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, SIGNED_T, FLOAT_T, FLO);
                  return NUMBER;
                }
[0]?{INT}+{EXP}[f]?   {
                        if(yytext[yyleng - 1] == 'f'){
                          numInfo(yytext, SIGNED_T, FLOAT_T, FLO);
                        } else{
                          numInfo(yytext, SIGNED_T, DOUBLE_T, FLO);
                        }
                        return NUMBER;
                      }
[0]?{FLO}+{EXP}[f]?   {
                        if(yytext[yyleng - 1] == 'f'){
                          numInfo(yytext, SIGNED_T, FLOAT_T, FLO);
                        } else{
                          numInfo(yytext, SIGNED_T, DOUBLE_T, FLO);
                        }
                        return NUMBER;
                      }


  /* Ignore Comments */
\/\*                {BEGIN(COMMENTS);}
<COMMENTS>.|\n      {;}
<COMMENTS>\*\/      {BEGIN(INITIAL);}
\/\/.*\n            {;}

  /* Update line numbers */
\n      {line++;}

  /* Throw error if no appropriate token is found */
.       {fprintf(stderr, "Error: Undefined token: %s\n", yytext); exit(-1);}

%%

void numInfo(char *buf, int sign, int type, int numType){
  if(numType == INTQ){
    yylval.num.intval = strtoull(buf, (char **)NULL, 10);
    yylval.num.sign = sign;
    yylval.num.type = type;
  } else if(numType == OCT){
    sscanf(buf, "%llo", &yylval.num.intval);
    yylval.num.sign = sign;
    yylval.num.type = type;
  } else if(numType == HEX){
    sscanf(buf, "%llx", &yylval.num.intval);
    yylval.num.sign = sign;
    yylval.num.type = type;
  } else if(numType == FLO){
    yylval.num.floatval = strtod(yytext, NULL);
    yylval.num.sign = sign;
    yylval.num.type = type;
  }

}

void stringAppend(char *buf, char c){
  if(buf == (char *)NULL){
    yylval.string.word[yylval.string.length] = c;
    yylval.string.length++;
  }
  else{
    for(int i = 0; i < strlen(buf); i++){
      yylval.string.word[yylval.string.length + i] = buf[i];
    }
    yylval.string.length += strlen(buf);
  }
}

void yywarn(char *filename, int line, char *msg){
  fprintf(stderr, "%s: %d: Warning: %s\n", filename, line, msg);
}

void yyerrorL(char *filename, int line, char *msg){
  fprintf(stderr, "%s: %d: Error: %s\n", filename, line, msg);
}

void specialChar(char c)
{
	switch(c)
	{
		case '\0':	fprintf(stdout, "\\0"); break;
		case '\a': 	fprintf(stdout, "\\a"); break;
		case '\b': 	fprintf(stdout, "\\b"); break;
		case '\f': 	fprintf(stdout, "\\f"); break;
		case '\n': 	fprintf(stdout, "\\n"); break;
		case '\r': 	fprintf(stdout, "\\r"); break;
		case '\t': 	fprintf(stdout, "\\t"); break;
		case '\v': 	fprintf(stdout, "\\v"); break;
		case '\'': 	fprintf(stdout, "\\\'"); break;
		case '\"': 	fprintf(stdout, "\\\""); break;
		case '\\': 	fprintf(stdout, "\\\\"); break;
		default:	if (c > 127 || c < 32){
                fprintf(stdout, "\\%03o", (unsigned char)c);
              } else{
                fprintf(stdout, "%c", c);
              }
              break;
	}
}

char* print_kw(int token)
{
	switch(token)
	{
		case INDSEL:		return "INDSEL";
		case PLUSPLUS:		return "PLUSPLUS";
		case MINUSMINUS:	return "MINUSMINUS";
		case SHL:		return "SHL";
		case SHR:		return "SHR";
		case LTEQ:		return "LTEQ";
		case GTEQ:		return "GTEQ";
		case EQEQ:		return "EQEQ";
		case NOTEQ:		return "NOTEQ";
		case LOGAND:		return "LOGAND";
		case LOGOR:		return "LOGOR";
		case ELLIPSIS:		return "ELLIPSIS";
		case TIMESEQ:		return "TIMESEQ";
		case DIVEQ:		return "DIVEQ";
		case MODEQ:		return "MODEQ";
		case PLUSEQ:		return "PLUSEQ";
		case MINUSEQ:		return "MINUSEQ";
		case SHLEQ:		return "SHLEQ";
		case SHREQ:		return "SHREQ";
		case ANDEQ:		return "ANDEQ";
		case OREQ:		return "OREQ";
		case XOREQ:		return "XOREQ";
		case AUTO:		return "AUTO";
		case BREAK:		return "BREAK";
		case CASE:		return "CASE";
		case CHAR:		return "CHAR";
		case CONST:		return "CONST";
		case CONTINUE:		return "CONTINUE";
		case DEFAULT:		return "DEFAULT";
		case DO:		return "DO";
		case DOUBLE:		return "DOUBLE";
		case ELSE:		return "ELSE";
		case ENUM:		return "ENUM";
		case EXTERN:		return "EXTERN";
		case FLOAT:		return "FLOAT";
		case FOR:		return "FOR";
		case GOTO:		return "GOTO";
		case IF:		return "IF";
		case INLINE:		return "INLINE";
		case INT:		return "INT";
		case LONG:		return "LONG";
		case REGISTER:		return "REGISTER";
		case RESTRICT:		return "RESTRICT";
		case RETURN:		return "RETURN";
		case SHORT:		return "SHORT";
		case SIGNED:		return "SIGNED";
		case SIZEOF:		return "SIZEOF";
		case STATIC:		return "STATIC";
		case STRUCT:		return "STRUCT";
		case SWITCH:		return "SWITCH";
		case TYPEDEF:		return "TYPEDEF";
		case UNION:		return "UNION";
		case UNSIGNED:		return "UNSIGNED";
		case VOID:		return "VOID";
		case VOLATILE:		return "VOLATILE";
		case WHILE:		return "WHILE";
		case _BOOL:		return "_BOOL";
		case _COMPLEX:		return "_COMPLEX";
		case _IMAGINARY:	return "_IMAGINARY";
		default:		return "ERROR";
	}
}

void numberType(int c)
{
	switch(c)
	{
    case INT_T:         fprintf(stdout, "INT\n"); break;
    case LONG_T:        fprintf(stdout, "LONG\n"); break;
    case LONGLONG_T: 	  fprintf(stdout, "LONGLONG\n"); break;
    case DOUBLE_T:      fprintf(stdout, "DOUBLE\n"); break;
    case LONGDOUBLE_T: 	fprintf(stdout, "LONGDOUBLE\n"); break;
    case FLOAT_T:       fprintf(stdout, "FLOAT\n"); break;
    default:	break;
	}
}

#if 0
int main(int argc, char **argv)
{
  int c;
  while(c = yylex()){
    if(c < 255){
      fprintf(stdout, "%s\t %d\t %c\n", filename, line, (char)c);
    } else{
      switch(c){
        case IDENT:   fprintf(stdout, "%s\t %d\t IDENT\t %s\n", filename, line, yylval.ident.name);
                      break;
        case CHARLIT: fprintf(stdout, "%s\t %d\t CHARLIT\t ", filename, line);
                      specialChar(yylval.charlit);
                      fprintf(stdout, "\n");
                      break;
        case STRING:  fprintf(stdout, "%s\t %d\t STRING\t\t ", filename, line);
                      for(int i = 0; i < yylval.string.length; i++){
                        specialChar(yylval.string.word[i]);
                      }
                      fprintf(stdout, "\n");
                      break;
        case NUMBER:  fprintf(stdout, "%s\t %d\t NUMBER\t ", filename, line);
                      if(yylval.num.type == INT_T || yylval.num.type == LONG_T || yylval.num.type == LONGLONG_T){
                        fprintf(stdout, "INTEGER\t %lld\t", yylval.num.intval);
                      } else{
                        fprintf(stdout, "REAL\t\t %Lg\t", yylval.num.floatval);
                      }
                      if(yylval.num.sign == UNSIGNED_T){
                        fprintf(stdout, "UNSIGNED,");
                      }
                      numberType(yylval.num.type);
                      break;
        default:      fprintf(stdout, "%s\t %d\n", filename, line);
                      break;
      }
    }
  }
}
#endif
