// various objects

#include "lex.h"
#include "object.h"
#include "error.h"
#include <stdio.h>
#include <string.h>

#define P printf

type *anyt=new type(ANY);
type *intt=new type(INT);
type *realt=new type(REAL);
type *boolt=new type(BOOL);
type *stringt=new type(STRING);

obj *curshowobj=NIL;

int indent=0;

void dump() {
  DOLLIST((&topscope->objs),obj *,y)
    y->show();
    P("\n");
  };
};

void scope::dd(feature *f) { if(findany(f->name)) error("double declaration:",f->name); };

feature *scope::find(char *n, llist *l) {
  DOLLIST(l,feature *,f)
    if(strcmp(f->name,n)==0) return f;
  };
  return NIL;
};

void showlay(slist *s) {
  indent+=2;
  FORSLIST(s,elem *,y) { for(int a=0;a<indent;a++) P(" "); y->show(); if(s->lastel()!=y) P("\n"); };
  indent-=2;
};

void showlay(llist *s) {
  indent+=2;
  DOLLIST(s,exp *,y)
    for(int a=0;a<indent;a++) P(" "); y->show(); if(s->lastel()!=y) P("\n");
  };
  indent-=2;
};

void showsep(slist *s,char *sep) {
  FORSLIST(s,elem *,y) { y->show(); if(s->lastel()!=y) P(sep); };
};

void showtypes(llist *s,char *l, char *r) {
  P(l);
  DOLLIST(s,type *,y) y->show(); if(s->lastel()!=y) P(","); };
  P(r);
};

void type::show() {
  if(this) {
    switch(t) {
      case IND:    ((typepar *)this)->par->show(); break;
      case VAR:    P("_"); break;
      case ANY:    P("any"); break;
      case INT:    P("int"); break;
      case REAL:   P("real"); break;
      case BOOL:   P("bool"); break;
      case STRING: P("string"); break;
      case LIST:   P("["); ((typepar *)this)->par->show(); P("]"); break;
      case VECTOR: P("<"); ((typepar *)this)->par->show(); P(">"); break;
      case TUPLE:  showtypes((&((typetuple *)this)->tl),"(",")"); break;
      case CLASST: {
        typeobj *to=(typeobj *)this;
        P("%s",to->name);
        if(to->tl.getnum()) showtypes(&to->tl,"[","]");
        break;
      };
      case PAR: {
        generic *g=NIL;
        if(curshowobj) g=curshowobj->find_generic(NIL,(typepar *)this);
        P("%s",g?g->name:"*");
        break;
      };
      case FUNCTION: {
        typefun *tf=(typefun *)this;
        showtypes((&tf->pars),"(",")");
        P("->");
        if(tf->ret) { tf->ret->show(); } else { P("()"); };
        break;
      };
      default: P("<TYPE %d>",t);
    };
  } else {
    P("<NIL>");
  };
};

#define T() if(t) { P(":"); t->show(); }

// all virtual, but not repeated

void elem::show() { P("<ELEM>"); };
void slist::show() { showlay(this); };
void llist::show() { showlay(this); };
void generic::show() { P("%s",name); if(par->par!=anyt) { P(" < "); par->par->show(); } };
void subtype::show() { P("<SUPER>"); };
void param::show() { P("%s",name); };
void exp::show() { P("<EXP>"); };
void feature::show() { P("<FEATURE>"); };

void obj::show() {
  obj *bo=curshowobj; curshowobj=this;
  P("%s:",name); t->show();
  if(generics) { P(" ["); showsep(generics,","); P("]"); };
  P(" ("); showsep((&pars),","); P(")");
  if(supers) { P(" "); showsep(supers," "); };
  if(ret) { P(" = "); ret->show(); };
  if(sc->objs.getnum() || sc->vars.getnum()) {
    P(" where\n");
    showlay(&sc->vars);
    if(sc->vars.getnum() && sc->objs.getnum()) P("\n");
    showlay(&sc->objs);
  };
  curshowobj=bo;
};

void var::show() { P("%s",name); T(); };

void op::show() {
  P("("); left->show();
  switch(opt) {
    case KEY_AND: P(" and "); break;
    case KEY_OR: P(" or "); break;
    case LEX_ASSIGN: P(":="); break;
    case LEX_UNEQ: P("<>"); break;
    case LEX_HIGHEQ: P(">="); break;
    case LEX_LOWEQ: P("<="); break;
    default: P((opt<256)?"%c":"%d",opt);
  };
  right->show(); P(")"); T();
};

void unify::show() { P("("); e->show(); P(" <=> "); p->show(); P(")"); };
void idexp::show() { if(env) { env->show(); T(); P("."); }; P("%s",name); T(); };
void app::show() { fun->show(); P("("); showsep((&args),","); P(")"); };
void aselect::show() { P("<ASELECT>"); };

void clos::show() {
  if(head) head->show();
  if(sc && (sc->objs.getnum() || sc->vars.getnum())) {
    P(" where\n");
    showlay((&sc->vars));
    if(sc->objs.getnum() && sc->vars.getnum()) P("\n");
    showlay((&sc->objs));
    if(el) P("\n");
  };
  if(el) { P(" do\n"); showlay(el); };
};

void intc::show() { P("%d",value); };
void stringc::show() { char *s=str,c; P("\'"); while((c=*s++)) if(c>=32) { P("%c",c); } else { P("\\%c",c); }; P("\'"); };
void unary::show() { P("<unary>"); };

void ifthen::show() { 
  if(switchexp) { switchexp->show(); P(" | "); };
  FORSLIST((&cc),condconseq *,c) {
    c->cond->show(); P(" -> "); c->conseq->show();
    if(((condconseq *)cc.lastel())!=c) P(" | ");
  };
  if(def) { P(" | "); def->show(); };
};

void loop::show() { P("<loop>"); };
void retexp::show() { P("<retexp>"); };
void self::show() { P("self"); };
void parent::show() { P("parent"); };
void boolval::show() { P(boo?"true":"false"); };
void nil::show() { P("nil"); };

void lisplist::show() {
  P("[");
  if(head) { head->show(); } else { P("<NIL>"); };
  if(tail) { P("|"); tail->show(); };
  P("]");
  T();
};

void tuple::show() { P("<TUPLE>"); };
void pat::show() { P("<pat>"); };
void allpat::show() { P("_"); };
void nilpat::show() { P("nil"); };
void twopat::show() { P("<twopat>"); };
void intpat::show() { P("%d",value); };
void strpat::show() { P("<strpat>"); };
void idpat::show() { P("%s",name); };
void constrpat::show() { P("<constrpat>"); };
void tuplepat::show() { P("<tuplepat>"); };
void listnilpat::show() { P("[]"); };

void listpat::show() {
  P("[");
  if(head) { head->show(); } else { P("<NIL>"); };
  if(tail) { P("|"); tail->show(); };
  P("]");
};

void vectorpat::show() { P("<vectorpat>"); };
