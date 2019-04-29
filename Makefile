parser: parser.tab.o ast.o lex.yy.o symtab.o quad.o
	gcc -o parser parser.tab.o ast.o lex.yy.o symtab.o quad.o

lex.yy.o: lexer.l parser.tab.h def.h
	flex lexer.l
	gcc -c lex.yy.c

parser.tab.o: parser.y ast.h
	bison -vd parser.y
	gcc -c parser.tab.c

symtab.o: symtab.h
	gcc -c symtab.c

ast.o: ast.c ast.h
	gcc -c ast.c

quad.o: quad.c quad.h
	gcc -c quad.c

clean:
		rm -f *.exe *.o *.stackdump parser.tab.* parser.output lex.yy.c *~
