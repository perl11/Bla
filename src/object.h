// object.h

#include "bla.h"
#include <stdio.h>

class exp;
class clos;
class pat;
class type;
class scope;
class obj;
class var;

class elem { public: elem *n; virtual void show(); };

class slist {
  elem *first, *last;
  int num;
public:
  slist() { first=last=NIL; num=0; };
  void insert(elem *x) { num++; x->n=NIL; last=(last)?(last->n=x,x):(first=x); };
  elem *reset() { return first; };
  elem *get(elem *walk) { return walk->n; };
  elem *lastel() { return last; };
  int getnum() { return num; };
  void show();
};

class eleml { public: eleml *n; void *x; void *e() { return x; }; };

class llist {
  eleml *first, *last;
  int num;
public:
  llist() { first=last=NIL; num=0; };
  void insert(void *d) { num++; eleml *e=new eleml(); e->n=NIL; e->x=d; last=(last)?(last->n=e,e):(first=e); };
  eleml *reset() { return first; };
  eleml *get(eleml *walk) { return walk->n; };
  void *lastel() { return last?last->x:NIL; };
  int getnum() { return num; };
  void show();
  void transfer(llist *from) { if(from->num) { (last?last->n:first)=from->first; last=from->last; num+=from->num; from->num=0; from->first=from->last=NIL; }; };
  void move(llist *from) { first=from->first; last=from->last; num=from->num; };
};

#define DOLLIST(list_,type_,elem_) for(eleml *elemn_=list_->reset();elemn_;elemn_=list_->get(elemn_)) { type_ elem_=(type_)elemn_->e();
#define FORSLIST(list_,type_,elem_) for(type_ elem_=(type_)list_->reset();elem_;elem_=(type_)list_->get(elem_))


// some protos

void type_check();
void generate_code(char *ilname,char *fname);
void dump();


enum typeconsts {
  ANY=0,
  INT, REAL, BOOL, STRING,
  LIST,
  VECTOR,
  TUPLE,
  CLASST,
  FUNCTION,
  VAR,
  IND,
  PAR,
};

class type {				// ANY/INT/REAL/BOOL/STRING
public:
  enum typeconsts t;
  type() {};	// silly
  type(enum typeconsts tag) { t=tag; };
  void show();
};

class typepar : public type {		// LIST/VECTOR/VAR/IND/PAR
public:
  type *par;
  typepar(enum typeconsts tag, type *p) { t=tag; par=p; };
};

class typeobj : public type {		// CLASST
public:
  char *name;
  llist tl;
  obj *o;	// NIL
  typeobj(char *n="__fun__") { o=NIL; name=n; t=CLASST; };
};

class typetuple : public type {		// TUPLE
public:
  llist tl;
  typetuple() { t=TUPLE; };
};

class typefun : public type {		// FUNCTION
public:
  type *ret;	// NIL
  llist pars;	// (type *)
  //llist *tpars;	// (type *)
  typefun() { ret=NIL; t=FUNCTION; };
};

const int MAX_TYPE_PAR=25;		// for some code

extern type *anyt;
extern type *intt;
extern type *realt;
extern type *boolt;
extern type *stringt;

#define EXPMETHODS \
  virtual void show(); \
  virtual void tc(); \
  virtual void cg(int rv)

#define expclass(cname,super) const int tag_##cname=__LINE__; class cname : public super { public: virtual int tag() { return tag_##cname; };

extern int currentline;

expclass(exp,elem)
  int line;
  type *t;
  exp() { t=NIL; line=currentline; };
  void seterr() { currentline=line; };
  EXPMETHODS;
};

class generic : public elem {
public:
  char *name;
  typepar *par;		// { PAR, boundtype }
  virtual void show();
};

class subtype : public elem {
public:
  type *t;		// should be app* for inh, char * for subt?
  char getsimpl;
  virtual void show();
};

class param : public elem {
public:
  char *name;
  exp *p;		// first pat*, after tcti var* (argvar)
  virtual void show();
};

enum storageclass { CONST, CLASS, OBJECT };

expclass(feature,exp)
  char* name;
  char privacy, storage;	// TRUE/FALSE, storageclass
  char beingtc;
  feature() {
    name="__noname__";
    beingtc=0;			// 0=notyet, 1=busy, 2=done
  };
  EXPMETHODS;
};

class scope {
public:
  scope *parent;
  obj *o;		// fun that this scope is in; NIL for toplevel
  llist objs, vars;
  scope(obj *in) { parent=NIL; o=in; };
  void addvar(var *v) { dd((feature *)v); vars.insert((feature *)v); };		// silly casts!
  void addobj(obj *o) { dd((feature *)o); objs.insert((feature *)o); };
  var *findvar(char *n) { return (var *)find(n,&vars); };
  obj *findobj(char *n) { return (obj *)find(n,&objs); };
  feature *findany(char *n) { feature *f=(feature *)findvar(n); return f?f:(feature *)findobj(n); };
  void dd(feature *f);
private:
  feature *find(char *n, llist *l);
};

extern scope *topscope;
extern int numfunids;
extern int debugoutput;

expclass(obj,feature)
  obj *nextdecl;	// nonNIL if >1 body, NIL after tcti
  obj *parent;
  exp *ret;		// or clos. if NIL and not extern_def then objdef short
  slist *generics, *supers;
  slist pars;		// automatically constr.
  scope *sc;
  llist *selfs, *lambdas;
  int funid;
  short curnum, maxnum;
  char secondobj;
  char dynamic_self;	// no analysis so far
  char returns_value;
  char extern_def;	// also `ret' will be NIL
  char patternmatch;	// pattern-matching used and/or more than one body
  char codegen_done;
  obj() {
    parent=nextdecl=NIL; privacy=FALSE; storage=CLASS;
    ret=NIL; generics=supers=NIL;
    curnum=maxnum=0;
    codegen_done=extern_def=secondobj=dynamic_self=returns_value=FALSE;
    patternmatch=0;
    sc=new scope(this);
    selfs=lambdas=NIL;
    funid=numfunids++;
    t=new typefun();
  };
  short bump() { if(++curnum>maxnum) maxnum=curnum; return curnum-1; };
  void back() { curnum--; };
  void ensurecoherancy();
  void mergedefs();
  generic *newgeneric(typepar *par) {
    if(!generics) generics=new slist();
    generic *g=new generic();
    generics->insert(g);
    g->par=par;
    return g;
  };
  generic *find_generic(char *n=NIL, typepar *t=NIL);
  EXPMETHODS;
};

expclass(var,feature)
  exp *defaultv;
  short vnum;
  var() {
    privacy=TRUE;
    storage=OBJECT;
    defaultv=NIL;
    vnum=10000;
  };
  EXPMETHODS;
};

// subclasses of exp

expclass(op,exp)			// := + - * / = <> > < <= >= and or
  int opt;
  exp *left, *right;
  EXPMETHODS;
};

expclass(unify,exp)			// <=>
  exp *e;
  pat *p;
  EXPMETHODS;
};

expclass(idexp,exp)			// ident, env.ident etc.
  char *name;
  exp *env;		// NIL!
  feature *f;		// (filled at tcti)
  char level;		// relative to env/self, 0=here, 1=parent, 2=etc.. (filled at tcti)
  char autodi,pm;	// -1/0/1, "+"/"-"
  EXPMETHODS;
};

expclass(app,exp)			// exp(1), f(1) etc.
  exp *fun;
  slist args;		// what about tagged calls?
  llist *targs;		// NIL
  EXPMETHODS;
};

expclass(aselect,exp)			// a<1>
  exp *ptr, *index;
  EXPMETHODS;
};

expclass(clos,exp)			// where ... do ... except ... close ...
  exp *head;			// NIL
  scope *sc;			// NIL where
  slist *el, *excl, *closel;	// do/except/close
  clos() { sc=NIL; el=excl=closel=NIL; };
  EXPMETHODS;
};

expclass(intc,exp)			// integer, charc
  int value;
  EXPMETHODS;
};

expclass(stringc,exp)			// stringc
  char *str;
  EXPMETHODS;
};

expclass(unary,exp)			// - not
  int op;
  exp *e;
  EXPMETHODS;
};

class condconseq : public elem {
public:
  exp *cond, *conseq;	// if(switchexp) cond==pat
  scope *sc;
  condconseq() { sc=NIL; };
};

expclass(ifthen,exp)			// a|b->c|...|e
  exp *switchexp, *def;
  slist cc;
  ifthen() { switchexp=NIL; def=NIL; };
  EXPMETHODS;
};

expclass(loop,exp)			// loop while next do ...
  scope *sc;
  exp *cond, *next, *body;
  loop() { sc=NIL; };
  EXPMETHODS;
};

expclass(retexp,exp)			// return
  exp *e;
  EXPMETHODS;
};

expclass(self,exp)			// self
  EXPMETHODS;
};

expclass(parent,exp)			// parent
  EXPMETHODS;
};

expclass(boolval,exp)			// true,false
  char boo;				// just one object will do!
  EXPMETHODS;
};

expclass(nil,exp)			// nil
  EXPMETHODS;				// same here.
};

expclass(lisplist,exp)			// [...]
  exp *head, *tail;
  EXPMETHODS;
};

expclass(tuple,exp)			// (x,y)
  slist exps;
  EXPMETHODS;
};

// patterns

#define PATMETHODS EXPMETHODS; \
  void pm(type *with)

expclass(pat,exp)			// pattern base
  char matches_always;
  pat() { matches_always=FALSE; };
  PATMETHODS;
};

expclass(allpat,pat)			// _
  PATMETHODS;
};

expclass(nilpat,pat)			// nil
  PATMETHODS;
};

expclass(twopat,pat)			// X/Y
  pat *left, *right;
  PATMETHODS;
};

expclass(intpat,pat)			// 1
  int value;
  PATMETHODS;
};

expclass(strpat,pat)			// 'a'
  char *s;
  PATMETHODS;
};

expclass(idpat,pat) 			// X
  char *name;
  var *v;
  idpat() { v=NIL; };
  PATMETHODS;
};

expclass(constrpat,pat)			// a(X,Y)
  char *name;
  obj *o;	// filled at tcti
  slist *args;
  PATMETHODS;
};

expclass(tuplepat,pat)			// (X,Y)
  slist pats;
  PATMETHODS;
};

expclass(listnilpat,pat)		// []
  PATMETHODS;
};

expclass(listpat,pat)			// [H|T]
  pat *head, *tail;
  PATMETHODS;
};

expclass(vectorpat,pat)			// <X,Y,Z>
  slist pats;
  PATMETHODS;
};
