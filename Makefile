parser: parser.tab.o ast.o lex.yy.o symtab.o quad.o
	gcc -o parser parser.tab.o ast.o lex.yy.o symtab.o quad.o

lex.yy.o: lexer.l parser.tab.h def.h
	flex lexer.l
	gcc -c -std=gnu99 lex.yy.c

parser.tab.o: parser.y ast.h
	bison -vd parser.y
	gcc -c -std=gnu99 parser.tab.c

symtab.o: symtab.h
	gcc -c -std=gnu99 symtab.c

ast.o: ast.c ast.h
	gcc -c -std=gnu99 ast.c

quad.o: quad.c quad.h
	gcc -c -std=gnu99 quad.c

clean:
		rm -f *.exe *.o *.stackdump parser.tab.* parser.output lex.yy.c *~
