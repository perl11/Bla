// parse.h

#include "object.h"

void parse_toplevel();
type *parse_type();
extern obj *parse_fpart(char *name=NIL);
extern clos *parse_closure(exp *head=NIL);
extern feature *parse_decl(char allowpr=TRUE,char allowst=TRUE,char allowvar=TRUE);
void match(int);
void push_scope(scope *sc);
void pop_scope();

#define nextsym (sym=lex())

extern int sym;
extern scope *curscope;
extern obj *curobj;
