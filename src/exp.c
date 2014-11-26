// exp.c: the expression parser

#include "parse.h"	// has "object.h" as well
#include "exp.h"        // needs ^
#include "error.h"
#include "lex.h"

exp *parse_exp(char take_bar);
pat *parse_listpat();

pat *parse_patterntwo(param *par, pat *other) {
  twopat *t=new twopat();
  nextsym;
  t->left=other;
  t->right=parse_pattern(par);
  return t;
};

extern pat *parse_patternid(param *par, char *n) {
  pat *p;
  if(sym=='(') {
    constrpat *c=new constrpat();
    nextsym;
    c->name=n;
    if(sym!=')') {
      c->args=new slist();
      c->args->insert(parse_pattern());
      while(sym==',') { nextsym; c->args->insert(parse_pattern()); };
      match(')');
    } else {
      c->args=NIL;
    };
    p=c;
  } else {
    idpat *i=new idpat(); i->name=n;
    if(par) { par->name=n; };
    p=i;
    var *v=new var(); i->v=v; v->name=n; v->vnum=curobj->bump(); curscope->addvar(v);
    if(sym==':') { nextsym; v->t=parse_type(); };
  };
  return (sym=='/')?parse_patterntwo(par,p):p;
};

extern pat *parse_pattern(param *par) {
  pat *p;
  switch(sym) {
    case LEX_IDENT:
      { char *n=lex_getinfoasc(); nextsym; p=parse_patternid(par,n); break; };
    case LEX_INTEGER:
      { intpat *i=new intpat(); i->value=lex_getinfoint(); nextsym; p=i; break; };
    case KEY_NIL:
      { nilpat *i=new nilpat(); nextsym; p=i; break; };
    case '_':
      { p=new allpat(); nextsym; break; };
    case '[':
      { p=parse_listpat(); break; };
    case '(': {
      tuplepat *t=new tuplepat();
      nextsym; t->pats.insert(parse_pattern());
      match(','); t->pats.insert(parse_pattern());
      while(sym==',') { nextsym; t->pats.insert(parse_pattern()); };
      match(')');
      p=t;
      break;
    };
    case '<':
      error("not impl.");
  };
  return (sym=='/')?parse_patterntwo(par,p):p;
};

pat *parse_listpat() {
  nextsym;
  if(sym==']') {
    listnilpat *i=new listnilpat(); nextsym; return i;
  } else {
    pat *e;
    listpat *l=new listpat(); l->head=parse_pattern(); e=l;
    while(sym==',') {
      listpat *x=new listpat();
      nextsym;
      l->tail=x;
      l=x;
      l->head=parse_pattern();
    };
    if(sym=='|') {
      nextsym; l->tail=parse_pattern();
    } else {
      listnilpat *i=new listnilpat(); l->tail=i;
    };
    match(']');
    return e;
  };
};

exp *parse_list() {
  nextsym;
  if(sym==']') {
    nil *n=new nil(); nextsym; return n;
  } else {
    exp *e;
    lisplist *l=new lisplist(); l->head=parse_exp(FALSE); e=l;
    while(sym==',') {
      lisplist *x=new lisplist();
      nextsym;
      l->tail=x;
      l=x;
      x->head=parse_exp(FALSE);
      //l->tail=new lisplist();
      //l=(lisplist *)l->tail;		// this fails to flush "l" outof a reg under maxonc++ 1.1
      //l->head=parse_exp(FALSE);
    };
    if(sym=='|') {
      nextsym; l->tail=parse_exp(FALSE);
    } else {
      l->tail=new nil();
    };
    match(']');
    return e;
  };
};

exp *parse_loop() {		// to be implemented
  return NIL;
};

exp *parse_tuple() {
  nextsym;
  exp *e=parse_exp();
  tuple *tup=NIL;
  while(sym==',') {
    if(!tup) { tup=new tuple(); tup->exps.insert(e); e=tup; };
    nextsym;
    tup->exps.insert(parse_exp());
  };
  match(')');
  return e;
};

exp *parse_cond(char is_switch) {
  push_scope(new scope(curobj));
  return is_switch?parse_pattern():parse_exp(FALSE);
};

// scope prob: if a->b|c->d|e, then only a->b has no own scope,
// because a already parsed. if a|b->c|.. then no prob.

exp *parse_if(exp *e,char is_switch) {
  int havescope=FALSE;
  ifthen *i=new ifthen();
  if(is_switch) {
    nextsym;
    i->switchexp=e;
    e=parse_cond(is_switch); havescope=TRUE;
  };
  exp *def;
  for(;;) {
    condconseq *cc=new condconseq();
    match(LEX_ARROW);
    i->cc.insert(cc);
    cc->cond=e;
    cc->conseq=parse_exp(FALSE);
    if(havescope) { cc->sc=curscope; pop_scope(); havescope=FALSE; };
    def=NIL;
    if(sym!='|') break;
    nextsym;
    def=e=parse_cond(is_switch); havescope=TRUE;	// vars decl in def lost?
    if(sym!=LEX_ARROW) break;
  };
  i->def=def;
  if(is_switch && def) error_rec("expected","->");
  if(havescope) pop_scope();
  return i;
};

void finalid(idexp *i) {
  i->name=lex_getinfoasc();
  nextsym;
  // ++ -- here!
};

exp *parse_factor() {
  exp *e;		// filled with a factor after switch()
  switch(sym) {
    case KEY_SELF:
      { self *s=new self(); nextsym; e=s; break; }
    case KEY_PARENT:
      { parent *p=new parent(); nextsym; e=p; break; }
    case KEY_TRUE: case KEY_FALSE:
      { boolval *b=new boolval(); b->boo=(sym==KEY_TRUE); nextsym; e=b; break; };
    case KEY_NIL:
      { e=new nil(); nextsym; break; };
    case '(':
      e=parse_tuple(); break;
    case '[':
      e=parse_list(); break;
    case LEX_INTEGER:
      { intc *i=new intc(); i->value=lex_getinfoint(); e=i; nextsym; break; };
    case KEY_DO: case KEY_WHERE:
      e=parse_closure(); break;
    case KEY_LOOP: case KEY_WHILE: case KEY_NEXT:
      e=parse_loop();
    case '-': case KEY_NOT:
      { unary *u=new unary(); u->op=sym; nextsym; u->e=parse_factor(); e=u; break; };
    case LEX_IDENT:
      { idexp *i=new idexp(); e=i; i->env=NIL; finalid(i); break; };
    case KEY_LAMBDA:
      nextsym; e=parse_fpart(); break;
    case '`':
      nextsym; e=parse_decl(FALSE,FALSE,TRUE); if(!e) error("illegal exp"); break;
    case KEY_RETURN:
      { retexp *r=new retexp(); nextsym; r->e=parse_exp(); e=r; break; };
    case LEX_STRING:
      { stringc *s=new stringc(); s->str=lex_getinfoasc(); nextsym; e=s; break; };
    case KEY_RAISE:
    case LEX_REAL:
    case LEX_PLUSPLUS: case LEX_MINMIN:
    case '<':
      // parse here!!!
      error("notimpl.");
    default:
      error("syntax error in expression");
  };
  lab: switch(sym) {
    case '(': {
      app *a=new app(); nextsym; a->fun=e;
      if(sym!=')') {
        while(1) { a->args.insert(parse_exp()); if(sym==')') break; match(','); };
        match(')');
      } else { nextsym; };
      a->targs=NIL;
      if(sym=='[') {
        nextsym;
        a->targs=new llist();
        a->targs->insert(parse_type());
        while(sym==',') { nextsym; a->targs->insert(parse_type()); };
        match(']');
      };
      e=a; goto lab;
    };
    case '.': {
      idexp *i=new idexp(); nextsym; i->env=e;
      if(sym!=LEX_IDENT) error("feature ident expected");
      finalid(i);
      e=i;
      goto lab;
    };
    case '[': {
      aselect *a=new aselect(); nextsym;
      a->ptr=e; a->index=parse_exp();
      e=a; match(']');
      goto lab;
    };
    case LEX_UNIF: {
      unify *u=new unify();
      nextsym;
      u->e=e;
      u->p=parse_pattern();
      e=u;
      goto lab;
    };
  };
  return e;
};

short *prec=NIL;

enum { PREC_MUL=1, PREC_ADD, PREC_EQ, PREC_AND, PREC_OR, PREC_ASS };

void init_table() {
  prec=new short[MAX_LEX_SYM];
  int a;
  for(a=0;a<MAX_LEX_SYM;a++) { prec[a]=0; };
  prec['*']=PREC_MUL; prec['/']=PREC_MUL;
  prec['+']=PREC_ADD; prec['-']=PREC_ADD;
  prec['=']=PREC_EQ; prec['>']=PREC_EQ; prec['<']=PREC_EQ;
  prec[LEX_UNEQ]=PREC_EQ; prec[LEX_HIGHEQ]=PREC_EQ; prec[LEX_LOWEQ]=PREC_EQ;
  prec[KEY_AND]=PREC_AND;
  prec[KEY_OR]=PREC_OR;
  prec[LEX_ASSIGN]=PREC_ASS;
};

extern exp *parse_exp(char take_bar) {
  if(!prec) init_table();
  exp *e=parse_factor();
  int numf=1;
  op *o;
  while(prec[sym]) {
    o=new op();
    o->opt=sym; nextsym;
    o->left=e;
    o->right=parse_factor();
    numf++;
    e=o;
  };
  op *x=o;              // do precedence based on numf
  while(numf>2) {
    op *y=(op *)x->left;
    int t=x->opt;
    int swap=(y->opt > t);
    if(y->opt==t) {
      if(prec[t]==PREC_EQ) error("comparison operators are non-associative");
      if(prec[t]==PREC_ASS) swap=TRUE;	// fix right-associativity
    };
    if(swap) {
      x->opt=y->opt; y->opt=t;
      exp *et=y->left;
      y->left=y->right;
      y->right=x->right;
      x->right=y;
      x->left=et;
    } else {
      x=y;
    };
    numf--;
  };
  lab: switch(sym) {
    case KEY_DO: case KEY_WHERE: {
      e=parse_closure(e);
      goto lab;
    };
    case '|': case LEX_ARROW: {
      if((!take_bar) && (sym=='|')) break;
      e=parse_if(e,sym=='|');
    };
  };
  return e;
};
