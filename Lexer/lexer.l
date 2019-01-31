%option noyywrap
%{
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "lexer.h"
#include "tokens-manual.h"

char filename[64] = "<stdin>";
int mode = 0;
int line = 1;
void numInfo(char *buf, int sign, int type, int numType);
void stringAppend(char *buf, char c);
void yywarn(char *filename, int line, char *msg);
void yyerror(char *filename, int line, char *msg);
%}
%x CHARS
%x STRINGS
%x ESCS
%x COMMENTS
%x INFOS

INT	  [1-9]+[0-9]*
OCT	  0[0-7]*
HEX	  0[xX][a-fA-F0-9]+
FLO 	[0-9]*[.]?[0-9]*
EXP 	[eE][+-]?{INT}+
P 	  [pP][+-]?{INT}+
%%

  /* LINE NUMBERS */
#                           {BEGIN(INFOS);}

<INFOS>^[0-9]+              {line = (int)strtol(yytext, (char **)NULL, 10);}
<INFOS>\"(\\.|[^\\\"])*\"		{  /* " */
                						strncpy(filename, yytext + 1, strlen(yytext) - 2);
                						filename[strlen(yytext) - 2] = '\0';
					                  }
<INFOS>.				            {;}
<INFOS>\n 			       	    {BEGIN(INITIAL);}

  /* WHITESPACES */
[\t\v\f ]+     {;}

  /* KEYWORDS */
"auto"			    {yylval.key = "AUTO"; return AUTO;}
"break"			    {yylval.key = "BREAK"; return BREAK;}
"case"		   	  {yylval.key = "CASE"; return CASE;}
"char"			    {yylval.key = "CHAR"; return CHAR;}
"const"			    {yylval.key = "CONST"; return CONST;}
"continue"	    {yylval.key = "CONTINUE"; return CONTINUE;}
"default"		    {yylval.key = "DEFAULT"; return DEFAULT;}
"do"			      {yylval.key = "DO"; return DO;}
"double"		    {yylval.key = "DOUBLE"; return DOUBLE;}
"else"			    {yylval.key = "ELSE"; return ELSE;}
"enum"			    {yylval.key = "ENUM"; return ENUM;}
"extern"		    {yylval.key = "EXTERN"; return EXTERN;}
"float"			    {yylval.key = "FLOAT"; return FLOAT;}
"for"			      {yylval.key = "FOR"; return FOR;}
"goto"			    {yylval.key = "GOTO"; return GOTO;}
"if"			      {yylval.key = "IF"; return IF;}
"inline"		    {yylval.key = "INLINE"; return INLINE;}
"int"			      {yylval.key = "INT"; return INT;}
"long"			    {yylval.key = "LONG"; return LONG;}
"register"	    {yylval.key = "REGISTER"; return REGISTER;}
"restrict"	    {yylval.key = "RESTRICT"; return RESTRICT;}
"return"		    {yylval.key = "RETURN"; return RETURN;}
"short"			    {yylval.key = "SHORT"; return SHORT;}
"signed"		    {yylval.key = "SIGNED"; return SIGNED;}
"sizeof"		    {yylval.key = "SIZEOF"; return SIZEOF;}
"static"		    {yylval.key = "STATIC"; return STATIC;}
"struct"		    {yylval.key = "STRUCT"; return STRUCT;}
"switch"		    {yylval.key = "SWITCH"; return SWITCH;}
"typedef"		    {yylval.key = "TYPEDEF"; return TYPEDEF;}
"union"			    {yylval.key = "UNION"; return UNION;}
"unsigned"	    {yylval.key = "UNSIGNED"; return UNSIGNED;}
"void"			    {yylval.key = "VOID"; return VOID;}
"volatile"	    {yylval.key = "VOLATILE"; return VOLATILE;}
"while"			    {yylval.key = "WHILE"; return WHILE;}
"_Bool"			    {yylval.key = "_BOOL"; return _BOOL;}
"_Complex"	    {yylval.key = "_COMPLEX"; return _COMPLEX;}
"_Imaginary"		{yylval.key = "_IMAGINARY"; return _IMAGINARY;}

  /* IDENTIFIERS */
[a-zA-Z_][a-zA-Z0-9_]*  {yylval.ident.name = strdup(yytext); return IDENT;}

  /* CHARLIT */
L?\'					          {mode = CHARLIT; BEGIN(CHARS);}

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
                          yyerror(filename, line, "Unexpected newline");
                          exit(-1);
                        }
<CHARS>\'               {
                          BEGIN(INITIAL);
                          mode = 0;
                          return CHARLIT;
                        }

  /* STRING */
\"				             	{
    						          yylval.string.length = 0;
    						          mode = STRING;
    						          BEGIN(STRINGS);
  					            }
<STRINGS>[^\n\\\"]*		  {
  						            stringAppend(yytext, (char)NULL);
  					            }
<STRINGS>\\			        {
  						            BEGIN(ESCS);
   					            }
<STRINGS>\"			        {
              						BEGIN(INITIAL);
              						mode = 0;
              						return STRING;
  					            }

  /* ESCAPE */
  /* Escape Codes */
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
<ESCS>[0-7]{1,3}	     	{
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
<ESCS>[xX][0-9a-fA-F]+	{
                					int hex;
                					sscanf(yytext + 1, "%x", &hex);
                					if (hex > 255)
                					{
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
"->"            {yylval.key = "INDSEL"; return INDSEL;}
"++"            {yylval.key = "PLUSPLUS"; return PLUSPLUS;}
"--"            {yylval.key = "MINUSMINUS"; return MINUSMINUS;}
"<<"            {yylval.key = "SHL"; return SHL;}
">>"            {yylval.key = "SHR"; return SHR;}
"<="            {yylval.key = "LTEQ"; return LTEQ;}
">="            {yylval.key = "GTEQ"; return GTEQ;}
"=="            {yylval.key = "EQEQ"; return EQEQ;}
"!="            {yylval.key = "NOTEQ"; return NOTEQ;}
"&&"         		{yylval.key = "LOGAND"; return LOGAND;}
"||"         		{yylval.key = "LOGOR"; return LOGOR;}
"..."			      {yylval.key = "ELLIPSIS"; return ELLIPSIS;}
"*="            {yylval.key = "TIMESEQ"; return TIMESEQ;}
"/="            {yylval.key = "DIVEQ"; return DIVEQ;}
"%="            {yylval.key = "MODEQ"; return MODEQ;}
"+="            {yylval.key = "PLUSEQ"; return PLUSEQ;}
"-="            {yylval.key = "MINUSEQ"; return MINUSEQ;}
"<<="           {yylval.key = "SHLEQ"; return SHLEQ;}
">>="           {yylval.key = "SHREQ"; return SHREQ;}
"&="            {yylval.key = "ANDEQ"; return ANDEQ;}
"|="            {yylval.key = "OREQ"; return OREQ;}
"^="            {yylval.key = "XOREQ"; return XOREQ;}

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
{FLO}		        {
  					      numInfo(yytext, SIGNED_T, DOUBLE_T, FLO);
  					      return NUMBER;
				        }
{FLO}"L"		    {
                  yytext[yyleng - 1] = '\0';
  					      numInfo(yytext, SIGNED_T, LONGDOUBLE_T, FLO);
  					      return NUMBER;
				        }
{FLO}"f"			  {
                  yytext[yyleng - 1] = '\0';
                  numInfo(yytext, SIGNED_T, FLOAT_T, FLO);
                  return NUMBER;
                }
[0]?{FLO}+{EXP}[f]?		{
  					            if(yytext[yyleng - 1] == 'f'){
                          numInfo(yytext, SIGNED_T, FLOAT_T, FLO);
                        } else{
                          numInfo(yytext, SIGNED_T, DOUBLE_T, FLO);
                        }
                        return NUMBER;
                      }


  /* Ignore Comments */
\/\*            		{BEGIN(COMMENTS);}
<COMMENTS>.|\n    	{;}
<COMMENTS>\*\/   		{BEGIN(INITIAL);}
\/\/.*\n        		{;}

  /* Update line numbers */
\n 			{line++;}

  /* Throw error if no appropriate token is found */
.				{fprintf(stderr, "Error: Undefined token: %s\n", yytext); exit(-1);}

%%

void numInfo(char *buf, int sign, int type, int numType){
  if(numType == INTQ){
    yylval.num.intval = atoi(buf);
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
    sscanf(buf, "%Lf", &yylval.num.floatval);
    yylval.num.sign = sign;
    yylval.num.type = type;
  }

}

void stringAppend(char *buf, char c){
	if(buf == (char *)NULL)
	{
		yylval.string.word[yylval.string.length] = c;
		yylval.string.length++;
	}
	else
	{
		for(int i = 0; i < strlen(buf); i++){
      yylval.string.word[yylval.string.length + i] = buf[i];
    }
		yylval.string.length += strlen(buf);
	}
}

void yywarn(char *filename, int line, char *msg){
  fprintf(stderr, "%s: %d: Warning: %s\n", filename, line, msg);
}

void yyerror(char *filename, int line, char *msg){
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

void numberType(int c)
{
	switch(c)
	{
    case INT_T:	      	fprintf(stdout, "INT\n"); break;
    case LONG_T:		    fprintf(stdout, "LONG\n"); break;
    case LONGLONG_T: 	  fprintf(stdout, "LONGLONG\n"); break;
    case DOUBLE_T: 		  fprintf(stdout, "DOUBLE\n"); break;
    case LONGDOUBLE_T: 	fprintf(stdout, "LONGDOUBLE\n"); break;
    case FLOAT_T: 		  fprintf(stdout, "FLOAT\n"); break;
		default:	break;
	}
}

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
        case STRING:	fprintf(stdout, "%s\t %d\t STRING\t\t ", filename, line);
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
        default:	    fprintf(stdout, "%s\t %d\t %s\n", filename, line, yylval.key);
						          break;
      }
    }
  }
}
