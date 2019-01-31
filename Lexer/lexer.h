#ifndef __LEXER_H
#define __LEXER_H

enum num_signs {UNSIGNED_T = 0, SIGNED_T = 1};
enum num_types {INT_T = 0, LONG_T, LONGLONG_T, DOUBLE_T, LONGDOUBLE_T, FLOAT_T};
enum typeNUM	 {INTQ = 0, OCT, HEX, FLO};

struct identifier
{
	char *name;
};

struct string_literal
{
	char word[4096];
	char print_word[4096];
	int length;
};

struct number
{
	unsigned long long intval;
	long double floatval;
	int sign;
	int type;
};

typedef union
{
	struct identifier ident;
	struct string_literal string;
	struct number num;
	char charlit;
	char *key;
} YYSTYPE;

YYSTYPE yylval;

#endif
