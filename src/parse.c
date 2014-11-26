// the Bla parser!

#include "lex.h"
#include "error.h"
#include "parse.h"   // has "object.h" as well
#include "exp.h"     // needs ^
#include <stdio.h>
#include <string.h>

int sym=0;
scope *curscope=NIL;
obj *curobj=NIL;
char parsetemp[10];

void push_scope(scope *sc) {
  sc->parent=curscope;
  curscope=sc;
};

void pop_scope() {
  curscope=curscope->parent;			// pop scope
};

extern void match(int t) {
  if(t!=sym) {
    char *tok;
    switch(t) {
      case LEX_ASSIGN: tok=":="; break;
      case LEX_DOTDOT: tok=".."; break;
      case LEX_ARROW: tok="->"; break;
      case LEX_HIGHEQ: tok=">="; break;
      case LEX_LOWEQ: tok="<="; break;
      case LEX_UNEQ: tok="<>"; break;
      case LEX_UNIF: tok="<=>"; break;
      case LEX_EOF: tok="<EOF>"; break;
      case LEX_IDENT: tok="identifier"; break;
      case LEX_INTEGER: tok="integer constant"; break;
      case LEX_REAL: tok="real constant"; break;
      case LEX_STRING: tok="string constant"; break;
      case LEX_PLUSPLUS: tok="++"; break;
      case LEX_MINMIN: tok="--"; break;
      default: sprintf(parsetemp,t<256?"%c":"%d",t); tok=parsetemp;
    };
    error("expected",tok);
  };
  nextsym;
};

void recover() {
  // reads tokens until ";"? what about exps etc.?
};

type *parse_type() {
  switch(sym) {
    case KEY_INT:    nextsym; return intt;
    case KEY_BOOL:   nextsym; return boolt;
    case KEY_REAL:   nextsym; return realt;
    case KEY_STRING: nextsym; return stringt;
    case KEY_ANY:    nextsym; return anyt;
    case '[': {
      nextsym;
      typepar *tl=new typepar(LIST,parse_type());
      match(']');
      return  tl;
    };
    case '<': {
      nextsym;
      typepar *tv=new typepar(VECTOR,parse_type());
      match('>');
      return tv;
    };
    case '(': {
      typetuple *tt=new typetuple();
      nextsym;
      tt->tl.insert(parse_type());
      match(',');
      tt->tl.insert(parse_type());
      while(sym==',') { nextsym; tt->tl.insert(parse_type()); };
      match(')');
      return tt;
    };
    case LEX_IDENT: {
      generic *g;
      char *in=lex_getinfoasc();
      if(curobj && (g=curobj->find_generic(in))) {
        nextsym;
        return g->par;
      } else {
        typeobj *to=new typeobj(in);
        nextsym;
        if(sym=='[') {
          nextsym;
          to->tl.insert(parse_type());
          while(sym==',') { nextsym; to->tl.insert(parse_type()); };
          match(']');
        };
        return to;
      };
    };
    default: error("illegal type spec"); return anyt;
  };
};

var *parse_sdecl(var *v) {
  if(sym==':') {				// parse :typespec
    nextsym;
    v->t=parse_type();
  };
  if(sym=='=') {
    nextsym;
    v->defaultv=parse_exp();
  };
  return v;
};

void parse_par(obj *o,short parnum) {
  param *par=new param();
  par->name="__par__";
  if(sym==LEX_IDENT) {
    char *n=lex_getinfoasc();
    nextsym;
    par->p=parse_patternid(par,n);
  } else {
    par->p=parse_pattern(par);			// pat
  };
  o->pars.insert(par);
  if(par->p->tag()==tag_idpat) o->patternmatch++;
};

void parse_decllistcomma() {			// for loop
  for(;;) {
    parse_decl(FALSE,FALSE,TRUE);
    if(sym!=',') break;
    nextsym;
  };
};


void parse_decllist(char allowst=TRUE,char allowvar=TRUE) {
  do {
    parse_decl(TRUE,allowst,allowvar);
    if(sym=='}') break;
    match(';');
  } while(sym!='}');
};

slist *parse_explist() {
  slist *el=new slist();
  do {
    el->insert(parse_exp());
    if(sym=='}') break;
    match(';');
  } while(sym!='}');
  return el;
};

extern clos *parse_closure(exp *head) {		// called like that for historical reasons
  clos *cl=new clos();
  cl->head=head;
  scope *sc=new scope(curobj); push_scope(sc);
  if(sym==KEY_WHERE) {
    lex_startlayout();	// eats '{'
    nextsym;
    parse_decllist();
    match('}');
  };
  if(sym==KEY_DO) {
    lex_startlayout();
    nextsym;
    cl->el=parse_explist();
    match('}');
  };
  pop_scope();
  cl->sc=sc;
  return cl;
};

void parse_generic(obj *o) {
  if(sym!=LEX_IDENT) error("generic type variable expected");
  char *n=lex_getinfoasc();
  if(o->find_generic(n)) error_rec("double type variable declaration for",n);
  typepar *tp=new typepar(PAR,anyt);
  generic *g=o->newgeneric(tp);
  g->name=n;
  nextsym;
  if(sym=='<') { nextsym; tp->par=parse_type(); };
};

extern obj *parse_fpart(char *name) {			// NIL=lambda
  obj *reto, *o, *prev;
  o=reto=new obj();
  o->name=name?name:(char*)"lambda";
  if(name && (prev=(obj *)curscope->objs.lastel()) && (strcmp(prev->name,o->name)==0)) {
    if(!prev->patternmatch) error_rec("unreachable part of function",name);
    while(prev->nextdecl) prev=prev->nextdecl;
    prev->nextdecl=o;
    o->secondobj=TRUE;
    reto=NIL;
  };
  push_scope(o->sc);				// this obj is now curscope
  o->parent=curobj;				// this obj is now curobj
  curobj=o;
  if(sym=='[') {				// parse generics
    nextsym;
    parse_generic(o);
    while(sym==',') { nextsym; parse_generic(o); };
    match(']');
  };
  match('(');
  if(sym!=')') {  				// parse args
    short parnum=-1;
    parse_par(o,parnum);
    while(sym==',') { nextsym; parse_par(o,--parnum); };
    match(')');
  } else {
    nextsym;
  };
  o->patternmatch=(o->patternmatch!=o->pars.getnum());
  if(sym==':') {				// parse return typespec
    nextsym;
    ((typefun *)o->t)->ret=parse_type();
  };
  while(sym==KEY_IMPLEMENTS || sym==KEY_EXTENDS) {	// parse inheritance
    int inh=(sym==KEY_EXTENDS);
    nextsym;
    type *t=parse_type();
    subtype *s=new subtype();
    s->t=t;
    s->getsimpl=inh;
    if(!o->supers) o->supers=new slist();
    o->supers->insert(s);
  };
  if(sym=='=') { nextsym; o->ret=parse_exp(); o->returns_value=TRUE; } else
    if(sym==KEY_DO || sym==KEY_WHERE) { o->ret=parse_closure(); } else
      if(sym==KEY_EXTERN) { nextsym; o->extern_def=TRUE; };
  if(o->ret && (o->ret->tag()==tag_clos) && (!o->patternmatch)) {
    clos *c=(clos *)o->ret;
    if(c->sc) { o->sc->objs.transfer(&c->sc->objs); o->sc->vars.transfer(&c->sc->vars); };
  };
  pop_scope();
  curobj=curobj->parent;
  return reto;
};

extern feature *parse_decl(char allowpr,char allowst,char allowvar) {
  int st=-1,pr=2;
  if(allowpr) { if(sym==KEY_PUBLIC) { nextsym; pr=FALSE; } else
                if(sym==KEY_PRIVATE) { nextsym; pr=TRUE; }; };
  if(allowst) { if(sym==KEY_CONST) { nextsym; st=CONST; } else
                  if(sym==KEY_CLASS) { nextsym; st=CLASS; } else
                    if(sym==KEY_OBJECT) { nextsym; st=OBJECT; }; };
  if(sym!=LEX_IDENT) { error("identifier expected"); };
  char *id=lex_getinfoasc();
  nextsym;
  feature *f=NIL;
  if((sym!='(') && (sym!='[')) {
    if(!allowvar) error("no toplevel variable declarations allowed");
    var *v=new var();
    v->name=id;
    v->vnum=curobj->bump();
    f=parse_sdecl(v);
    curscope->addvar(v);
  } else {
    obj *o=parse_fpart(id);
    if(o) curscope->addobj(o);
    f=o;
  };
  if(f) {
    if(pr!=2) f->privacy=pr;
    if(st!=-1) f->storage=st;
  };
  return f;
}

void treat_module(char *modname) {
  printf("loading module '%s'...\n",modname);
};

void parse_toplevel() {
  lex_startlayout();
  nextsym;
  push_scope(topscope=new scope(NIL));
  while(sym==KEY_MODULE) {
    nextsym;
    for(;;) {
      if(sym!=LEX_STRING) error("module name expected");
      treat_module(lex_getinfoasc());
      nextsym;
      if(sym!=',') break;
      match(',');
    };
    match(';');
  };
  parse_decllist(FALSE,FALSE);
  pop_scope();
  match('}');
  match(LEX_EOF);
};
