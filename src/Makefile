# makefile for Bla compiler and related utilities (bla/bint/bdsm)

#CXX := g++ -g
CXX:=clang++-mp-3.4 -std=c++11 -g -Wno-c++11-compat-deprecated-writable-strings \
  -Wno-format-security
#CC = mcppc
OPT = 
OBJECTS = main.o lex.o error.o parse.o object.o exp.o typec.o codegen.o ilsave.o tools.o

all:		bla bdsm bint

bla:		$(OBJECTS)
		$(CXX) $(OPT) -o bla $(OBJECTS)

main.o:		bla.h main.c lex.h parse.h object.h error.h
		$(CXX) $(OPT) -o main.o main.c -c

lex.o:		bla.h lex.c lex.h error.h
		$(CXX) $(OPT) -o lex.o lex.c -c

object.o:	bla.h object.c object.h
		$(CXX) $(OPT) -o object.o object.c -c

parse.o:	bla.h parse.c parse.h error.h object.h lex.h exp.h
		$(CXX) $(OPT) -o parse.o parse.c -c

exp.o:		bla.h exp.c exp.h error.h object.h lex.h parse.h
		$(CXX) $(OPT) -o exp.o exp.c -c

error.o:	bla.h error.c error.h lex.h
		$(CXX) $(OPT) -o error.o error.c -c

typec.o:	bla.h typec.c object.h error.h lex.h
		$(CXX) $(OPT) -o typec.o typec.c -c

codegen.o:	bla.h codegen.c object.h il.h error.h
		$(CXX) $(OPT) -o codegen.o codegen.c -c

ilsave.o:	bla.h il.h ilsave.c
		$(CXX) $(OPT) -o ilsave.o ilsave.c -c

# other Bla utils

tools.o:	tools.c
		$(CXX) $(OPT) -o tools.o tools.c -c

ilload.o:	bla.h il.h ilload.c
		$(CXX) $(OPT) -o ilload.o ilload.c -c

ILOBJECTS = ilload.o tools.o

bdsm:		bla.h il.h bdsm.c ilload.c $(ILOBJECTS)
		$(CXX) $(OPT) -o bdsm bdsm.c $(ILOBJECTS)

bint:		bla.h il.h bint.c ilload.c $(ILOBJECTS)
		$(CXX) $(OPT) -o bint bint.c $(ILOBJECTS)

clean:		
		rm *.o bla core
