// the typechecker/inferencer (and related activities)

#include "lex.h"
#include "object.h"
#include "error.h"
#include <stdio.h>
#include <string.h>

exp *dummy_exp;

type *unify_types(exp *eo, type *a, type *b, int a_is_upper_limit=TRUE);
type *exact_unify(exp *eo, type *a, type *b);

inline type *newvar() { return new typepar(VAR,NIL); };

inline type *unwind(type *x) {
  while(x->t==IND) { x=((typepar *)x)->par; };
  return x;
};

/*---------------------------------------scopes---------------------------*/

scope *cursc=NIL;		// for tc phase
obj *curf=NIL;
int feature_level;

feature *findscdown(scope *sc, char *n) {
  feature *f;
  feature_level=0;
  obj *o=sc->o;
  while(sc) {
    if((f=sc->findany(n))) return f;
    sc=sc->parent;
    if(sc && sc->o!=o) { feature_level++; o=sc->o; };
  };
  return NIL;
};

obj *findscdown_obj(scope *sc, char *n) {
  feature *f;
  while(sc) { if((f=sc->findobj(n))) return (obj *)f; sc=sc->parent; };
  return NIL;
};

feature *findsc(exp *errorobj, char *n,obj *here=NIL) {
  feature *f;
  if(here) {
    if((!here->secondobj) && (!here->nextdecl))		// very ad hoc
      if((f=findscdown(here->sc,n))) return f;
  } else if((f=findscdown(cursc,n))) {
    return f;
  };
  error("unknown identifier:",n,errorobj);
  return NIL;
};

obj *find_cursc_obj_err(exp *eo, char *n) {
  obj *o=findscdown_obj(cursc,n);	//topscope?
  if(!o) error("no such type:",n,eo);
  return o;
};

void ensure_objtype(exp *eo, typeobj *t) { if(!t->o) t->o=find_cursc_obj_err(eo,t->name); };

/*---------------------------scope-and-pm-merging-------------------------*/

void obj::ensurecoherancy() {};

char *temp_ids[] = { "a","b","c","d","e","f","g","h","i","j","k","l","m",
                     "n","o","p","q","r","s","t","u","v","w","x","y","z",NIL };

char *temp_par[] = { "T","U","V","W","X","Y","Z",
                     "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S",NIL };

generic *obj::find_generic(char *n, typepar *t) {	// find based on either criteria
  for(obj *o=this;o;o=o->parent) {
    if(o->generics) {
      FORSLIST(o->generics,generic *,g) {
        if((n && (strcmp(n,g->name)==0)) || (t && (t==g->par))) return g;
      };
    };
  };
  return NIL;
};

char **temp_par_cur;

void make_generic(type *t, obj *o, int final=FALSE) {
  typepar *tp=(typepar *)unwind(t);
  switch(tp->t) {
    case VAR: tp->t=PAR; tp->par=anyt; break;
    case LIST: case VECTOR: case IND: make_generic(tp->par,o,final); break;
    case TUPLE: { DOLLIST((&((typetuple *)tp)->tl),type *,tt) make_generic(tt,o,final); }; break; };
    case CLASST: { DOLLIST((&((typeobj *)tp)->tl),type *,tt) make_generic(tt,o,final); }; break; };
    case FUNCTION: {
      typefun *tf=(typefun *)tp;
      DOLLIST((&tf->pars),type *,tt) make_generic(tt,o,final); };
      if(tf->ret) make_generic(tf->ret,o,final);
      break;
    };
    case PAR: {
      //printf("par!\n");
      if(final) {
        //printf("par final! %d\n",tp);
        if(!o->find_generic(NIL,tp)) {
          do {
            temp_par_cur++;
            if(!*temp_par_cur) intern("temp_par_ids");
          } while(o->find_generic(*temp_par_cur));
          generic *g=o->newgeneric(tp);
          g->name=*temp_par_cur;
          //printf("par final name! %s\n",g->name);
        };
      } else {
        make_generic(tp->par,o,final);
      };
      break;
    };
  };
};

void gen_gen_rec(obj *o) {
  make_generic(o->t,o,TRUE);
  { DOLLIST((&o->sc->vars),var *,v) make_generic(v->t,o,TRUE); }; };
  { DOLLIST((&o->sc->objs),obj *,f) gen_gen_rec(f); }; };
};

void generate_generics(llist *objs) {
  DOLLIST(objs,obj *,o)
    temp_par_cur=temp_par-1;
    gen_gen_rec(o);
  };
};

exp *addexp(exp *e,param *p,var *pv) {
  auto u=new unify();
  u->t=boolt;
  u->e=pv;
  u->p=(pat *)p->p;
  u->p->pm(u->e->t);
  if(e) { auto o=new op(); o->t=boolt; o->opt=KEY_AND; o->left=e; o->right=u; e=o; } else { e=u; };
  return e;
};

void maketempids(obj *o) {
  char **s=temp_ids-1;
  FORSLIST((&o->pars),param *,r) {		// assign temp ids
    if(*(r->name)=='_') {
      int notf;
      do {
        s++;
        if(!*s) intern("temp_ids");
        notf=TRUE;
        FORSLIST((&o->pars),param *,p) if(strcmp(*s,p->name)==0) { notf=FALSE; break; };
      } while(!notf);
      r->name=*s;
      ((var *)r->p)->name=*s;
    };
  };
};

void obj::mergedefs() {
  auto it=new ifthen();		// the resulting ifthen
  exp *e=NIL;				// a conditional exp
  short parnum=0;
  if(patternmatch) {
    if(extern_def) error("pattern matching used in extern declaration of",name);
    FORSLIST((&pars),param *,p) {
      auto v=new var();
      v->name=p->name;
      v->vnum=--parnum;
      v->t=p->p->t;
      e=addexp(e,p,v);
      p->p=v;
    };
    obj *ooh=this;
    for(;;) {
      auto cc=new condconseq();
      it->cc.insert(cc);
      cc->cond=e?e:dummy_exp;
      cc->conseq=ooh->ret?ooh->ret:dummy_exp;
      cc->sc=ooh->sc; ooh->sc=new scope(ooh);
      sc->parent=cc->sc->parent; cc->sc->parent=sc;
      if(!(ooh=ooh->nextdecl)) break;			// end of the loop here
      if(pars.getnum()!=ooh->pars.getnum()) {
        error("#of arguments unequal to previous definition for",ooh->name,ooh);
      };
      unify_types(ooh,((typefun *)t)->ret,((typefun *)ooh->t)->ret,FALSE);
      e=NIL;
      param *pv=(param *)pars.reset();
      eleml *te=((typefun *)t)->pars.reset();
      FORSLIST((&ooh->pars),param *,p) {
        te->x=unify_types(ooh,(type *)te->x,p->p->t,FALSE);
        if((*(pv->name)=='_') && (*(p->name)!='_')) ((var *)pv->p)->name=pv->name=p->name;
        e=addexp(e,p,(var*)pv->p);
        pv=(param *)pars.get(pv);
        te=((typefun *)t)->pars.get(te);
      };
    };
    ret=it;
  } else {
    if(nextdecl) error("no patternmatch for",name,this);
    if(!extern_def) {
      clos *cl=NIL;
      FORSLIST((&pars),param *,p) {	// copy of above
        auto v=new var();
        v->name=p->name;
        v->vnum=--parnum;
        v->t=p->p->t;
        // move over arguments to env!
        if(!cl) { cl=new clos(); cl->head=ret; ret=cl; cl->el=new slist(); };
        auto ass=new op();
        ass->opt=LEX_ASSIGN;
        ass->left=((idpat *)p->p)->v;
        ass->right=v;
        cl->el->insert(ass);
        p->p=v;
      };
    };
  };
  maketempids(this);
  make_generic(t,this);
  nextdecl=NIL;
};


/*-------------------------------type-checking-------------------------*/

void type_check() {
  dummy_exp=new nil();
  dummy_exp->t=anyt;
  DOLLIST((&topscope->objs),exp *,e)
    e->tc();
    temp_par_cur=temp_par-1; gen_gen_rec((obj *)e);
  };
  //generate_generics(&topscope->objs);
};

void tc_exps(slist *s) { FORSLIST(s,exp *,e) e->tc(); };

// all virtual, but not repeated

void exp::tc() { t=anyt; };
void feature::tc() { t=anyt; };

void var::tc() {
  if(!beingtc) {
    beingtc=1;
    if(defaultv) {
      defaultv->tc();
      if(t) { unify_types(this,t,defaultv->t); } else { t=defaultv->t; };
    };
    if(!t) t=newvar();
    beingtc=2;
  } else {
    if(beingtc==1) { error_rec("recursive variable definition for",name,this); t=newvar(); };
  };
};

void expandselfrefs(obj *o) {
  if(o->selfs) {
    DOLLIST(o->selfs,typeobj *,to)
      if(o->generics) {
        FORSLIST(o->generics,generic *,g) to->tl.insert(g->par);
      };
    };
  };
};

void obj::tc() {
  if(!beingtc) {
    beingtc=1;
    if(!secondobj) {
      if(debugoutput) printf("typechecking %s...\n",name);
      ensurecoherancy();
    };
    scope *bsc=cursc; cursc=sc; obj *bf=curf; curf=this;
    typefun *ft=(typefun *)t;
    FORSLIST((&pars),param *,pp) { pp->p->tc(); ft->pars.insert(pp->p->t); };
    if(!ft->ret) ft->ret=newvar();              // now, all of interface has type
    if((!ret) && (!extern_def)) ret=new self();		// maybe make vars public?
    { DOLLIST((&sc->vars),var *,v) v->tc(); }; };
    { DOLLIST((&sc->objs),obj *,o) o->tc(); }; };
    if(ret) { ret->tc(); unify_types(ret,ft->ret,ret->t); };
    cursc=bsc; curf=bf;
    if(nextdecl) nextdecl->tc();
    if(!secondobj) mergedefs();
    expandselfrefs(this);
    beingtc=2;
  };
};

void op::tc() {
  switch(opt) {
    case '+': case '-': case '*': case '/':
      left->tc(); right->tc();
      unify_types(right,intt,right->t);
      t=unify_types(left,intt,left->t);
      break;
    case '=': case '>': case '<': case LEX_UNEQ: case LEX_HIGHEQ: case LEX_LOWEQ:
      left->tc(); right->tc();
      unify_types(left,left->t,right->t);
      t=boolt;
      break;
    case KEY_OR:
      left->tc(); right->tc();
      t=unify_types(left,left->t,right->t,FALSE);
      break;
    case KEY_AND:
      left->tc(); right->tc();
      t=right->t;
      break;
    case LEX_ASSIGN:
      right->tc();
      t=right->t;
      left->tc();
      unify_types(left,left->t,right->t);
      break;
  };
};

void unify::tc() {
  e->tc();
  p->tc();
  unify_types(p,p->t,e->t);
  p->pm(e->t);
  t=boolt;
};

type *find_cur_tparam(typepar *par,typeobj *env) {
  if(env && env->o->generics) {
    int num=0, num2=0;
    FORSLIST(env->o->generics,generic *,g) { if(g->par==par) break; num++; };
    if(env->o->generics->getnum()==num) return NIL;
    DOLLIST((&env->tl),type *,t) if(num2++==num) return t; };
    //printf("(%d,%d,`%s',%d)",num,num2,env->o->name,env->o->generics->getnum());
    warn("unbound type parameter?");
  };
  return NIL;
};

type *clone_tparam(type *, typeobj *, int &);
type *findbind_cur(typepar *);

llist *clone_tparam_llist(llist *sl, typeobj *env, int &ischanged) {
  if(!sl) { ischanged=FALSE; return NIL; };
  type *tempt[MAX_TYPE_PAR];
  int num=0, isch;
  ischanged=FALSE;
  DOLLIST(sl,type *,t)
    if(num==MAX_TYPE_PAR) intern("max_type_par");
    tempt[num++]=clone_tparam(t,env,isch);
    if(isch) ischanged=TRUE;
  };
  llist *nl=NIL;
  if(ischanged) {
    nl=new llist();
    for(int a=0;a<num;a++) nl->insert(tempt[a]);
  };
  return nl?nl:sl;
};

type *clone_tparam(type *s, typeobj *env, int &ischanged) {
  if(!s) { ischanged=FALSE; return NIL; };
  type *subt;
  llist *subl;
  switch(s->t) {
    case IND: {
      subt=clone_tparam(((typepar *)s)->par,env,ischanged);
      return ischanged?subt:s;
    };
    case LIST: case VECTOR: {
      subt=clone_tparam(((typepar *)s)->par,env,ischanged);
      return ischanged?new typepar(s->t,subt):s;
    };
    case TUPLE: {
      subl=clone_tparam_llist(&((typetuple *)s)->tl,env,ischanged);
      if(!ischanged) return s;
      auto  tt=new typetuple();
      tt->tl.move(subl);
      return tt;
    };
    case CLASST: {
      subl=clone_tparam_llist(&((typeobj *)s)->tl,env,ischanged);
      if(!ischanged) return s;
      auto  to=new typeobj();
      to->tl.move(subl);
      to->o=((typeobj *)s)->o;
      to->name=((typeobj *)s)->name;
      return to;
    };
    case FUNCTION: {
      subl=clone_tparam_llist(&((typefun *)s)->pars,env,ischanged);
      int isch;
      subt=clone_tparam(((typefun *)s)->ret,env,isch);
      if(!(ischanged=(ischanged || isch))) return s;
      auto  tf=new typefun();
      tf->pars.move(subl);
      tf->ret=subt;
      return tf;
    };
    case PAR: {
      ischanged=TRUE;
      type *t=findbind_cur((typepar *)s);
      return t?t:((t=find_cur_tparam((typepar *)s,env))?t:newvar());
    };
    default: { ischanged=FALSE; return s; };
  };
  return NIL;
};

idexp *hackidexp=NIL, *thisidexp=NIL;
typeobj *hackenv;

void idexp::tc() {
  typeobj *objt=NIL;
  if(env) {
    env->tc();
    if((objt=(typeobj *)unwind(env->t))->t!=CLASST) { env->t->show(); printf("\n"); error("environment of `.' is not a class type",NIL,env); };
  };
  if(objt) ensure_objtype(env,objt);
  f=findsc(this,name,objt?objt->o:NIL);
  level=feature_level;
  f->tc();
  t=f->t;
  //printf("[%s]",name);
  if(objt) { int dum; t=clone_tparam(t,objt,dum); };
  { hackidexp=this; hackenv=objt; };
};

const int max_tempbind=100;
class tempbind { public: typepar *par; type *val; };
tempbind tb[max_tempbind];
int curbind=0;
void clearbind() { curbind=0; };
void addbind(typepar *p, type *v) {
  if(curbind>=max_tempbind) intern("max_tempbind");
  tb[curbind].par=p;
  tb[curbind].val=v;
  curbind++;
};
void addbinds(exp *eo, llist *ta) {
  if(ta) {
    if((hackidexp!=thisidexp) || (hackidexp->f->tag()!=tag_obj)) {
      error_rec("unapplicable type arguments",NIL,eo);
    } else {
      slist *gs=((obj *)hackidexp->f)->generics;
      generic *g=gs?(generic *)gs->reset():NIL;
      DOLLIST(ta,type *,t)
        if(!g) { error_rec("too many type arguments",NIL,eo); return; };
        addbind(g->par,t);
        g=(generic *)gs->get(g);
      };
      if(g) { error_rec("too few type arguments",NIL,eo); return; };
    };
  };
};
type *findbind_cur(typepar *par) {
  for(int c=0;c<curbind;c++) if(par==tb[c].par) return tb[c].val;
  return NIL;
};
type *findbind(exp *eo, typepar *par, type *other) {
  type *t;
  t=findbind_cur(par);
  //par->show(); other->show(); t->show(); printf("\n");
  if(t) return unify_types(eo,t,other);
  if(hackidexp==thisidexp) if((t=find_cur_tparam(par,hackenv))) return t;
  addbind(par,other);
  return other;
};

void app::tc() {
  { FORSLIST((&args),exp *,e) e->tc(); };
  fun->tc();
  typefun *tf=(typefun *)unwind(fun->t);
  if(tf->t==FUNCTION) {		// 95% of the time, treat as special case
    t=tf->ret?tf->ret:newvar();
    if(args.getnum()!=tf->pars.getnum()) { error("incorrect #of args in function application",hackidexp==thisidexp?hackidexp->f->name:NIL,this); };
    thisidexp=(idexp *)fun; clearbind();
    //if(hackidexp==thisidexp) printf("<%s>",thisidexp->name);
    addbinds(this,targs);
    exp *e=(exp *)args.reset();
    DOLLIST((&tf->pars),type *,tt) unify_types(e,tt,e->t); e=(exp *)args.get(e); };
    int dum; t=clone_tparam(t,hackidexp==thisidexp?hackenv:NIL,dum);
    thisidexp=NIL; clearbind();
  } else {			// likely a var
    t=newvar();
    tf=new typefun();
    tf->ret=t;
    FORSLIST((&args),exp *,x) { tf->pars.insert(x->t); };
    unify_types(this,fun->t,tf);
  };
};

void aselect::tc() {
  ptr->tc();
  index->tc();
  unify_types(index,intt,index->t);
  typepar *tp=(typepar *)unwind(ptr->t);
  if(tp->t==STRING) {
    t=intt;
  } else if(tp->t==VECTOR) {
    t=tp->par;
  } else {
    error_rec("[] operates on vectors (and strings) only",NIL,this);
    t=anyt;
  };
};

void clos::tc() {
  scope *bsc;
  if(sc) {
    bsc=cursc; cursc=sc; 
    { DOLLIST((&sc->vars),var *,v) v->tc(); }; };
    { DOLLIST((&sc->objs),obj *,o) o->tc(); }; };
  };
  if(el) { tc_exps(el); };
  if(excl) { tc_exps(excl); };
  if(closel) { tc_exps(closel); };
  if(head) head->tc();
  if(sc) { cursc=bsc; };
  t=head?head->t:newvar();
};

void intc::tc() { t=intt; };
void stringc::tc() { t=stringt; };

void unary::tc() {
  if(op=='-') { e->tc(); t=intt; } else { e->tc(); t=boolt; };
  unify_types(e,t,e->t);
};

void ifthen::tc() {
  if(switchexp) switchexp->tc();
  FORSLIST((&cc),condconseq *,c) {
     scope *bsc;
     if(c->sc) { bsc=cursc; cursc=c->sc; };
     c->cond->tc();
     if(switchexp) unify_types(c->cond,switchexp->t,c->cond->t);
     c->conseq->tc();
     if(t) { unify_types(c->conseq,t,c->conseq->t,FALSE); } else { t=c->conseq->t; };
     if(c->sc) { cursc=bsc; };
  };
  if(def) { def->tc(); unify_types(def,t,def->t,FALSE); }
};

void loop::tc() {
  t=newvar();
};

void retexp::tc() {
  e->tc();
  unify_types(e,((typefun *)curf->t)->ret,e->t);
  t=newvar();
};

void addself(obj *o, type *t) {
  if(!o->selfs) o->selfs=new llist();
  o->selfs->insert(t);
};

void self::tc() {
  t=new typeobj(curf->name);
  ((typeobj *)t)->o=curf;
  addself(curf,t);
};

void parent::tc() {
  if(!curf->parent) {
    error_rec("function has no parent",NIL,this);
    t=anyt;
  } else {
    t=new typeobj(curf->parent->name);
    ((typeobj *)t)->o=curf->parent;
    addself(curf->parent,t);
  };
};

void boolval::tc() { t=boolt; };
void nil::tc() { t=newvar(); };

void lisplist::tc() {
  head->tc();
  tail->tc();
  t=new typepar(LIST,head->t);
  unify_types(this,t,tail->t);
};

void tuple::tc() {
  auto tt=new typetuple();
  FORSLIST((&exps),exp *,e) { e->tc(); tt->tl.insert(e->t); };
  t=tt;
};

void pat::tc() { t=anyt; };
void allpat::tc() { t=newvar(); matches_always=TRUE; };
void nilpat::tc() { t=newvar(); };

void twopat::tc() {
  left->tc();
  right->tc();
  t=exact_unify(left,left->t,right->t);
  matches_always=(left->matches_always && right->matches_always);
};

void intpat::tc() { t=intt; };
void strpat::tc() { t=stringt; };
void idpat::tc() { v->tc(); t=v->t; };

void constrpat::tc() {
  o=find_cursc_obj_err(this,name);
  o->tc();
  llist *vs=&o->sc->vars;
  eleml *v=vs->reset();
  if(args) FORSLIST(args,pat *,p) {
    if(!v) error("too many elements for object pattern",o->name,this);
    p->tc();
    unify_types(p,((var *)v->e())->t,p->t);
    v=vs->get(v);
  };
  if(v) error("too few elements for object pattern",o->name,this);
  /*
  typeobj *to=new typeobj(o->name);	// should deal with typepars too (!)
  to->o=o;
  t=to;
  */
  t=((typefun *)o->t)->ret;	// hack: to make it easy to have unified type for bodies
};

void tuplepat::tc() {
  auto tt=new typetuple();
  FORSLIST((&pats),exp *,p) { p->tc(); tt->tl.insert(p->t); };
  t=tt;
};

void listnilpat::tc() { t=new typepar(LIST,newvar()); };

void listpat::tc() {
  head->tc();
  tail->tc();
  t=new typepar(LIST,head->t);
  unify_types(this,t,tail->t);
};

void vectorpat::tc() { t=anyt; };

/*-----------------pattern-match-analysis--------------------------*/

// FALSE is default

int pm_list(slist *s,slist *types) {
  int r=TRUE;
  FORSLIST(s,pat *,p) { p->pm(NIL); if(!p->matches_always) r=FALSE; };
  return r;
};

void pat::pm(type *with) { };
void allpat::pm(type *with) { matches_always=TRUE; };
void nilpat::pm(type *with) { };
void twopat::pm(type *with) { matches_always=(left->matches_always && right->matches_always); };
void intpat::pm(type *with) { };
void strpat::pm(type *with) { };
void idpat::pm(type *with) { matches_always=TRUE; };		// depends on type, really
void constrpat::pm(type *with) { matches_always=TRUE; };	// same here
void tuplepat::pm(type *with) { matches_always=pm_list(&pats,NIL); };
void listnilpat::pm(type *with) { };
void listpat::pm(type *with) { matches_always=(head->matches_always && tail->matches_always); };
void vectorpat::pm(type *with) { matches_always=pm_list(&pats,NIL); };


/*----------------------------type-unifier--------------------------*/

void type_error(exp *eo, type *a, type *b) {
  a->show(); printf(" <=> "); b->show(); printf("\n");
  warn("types don't match!",NIL,eo);
};

void occurs(exp *eo, type *a, type *b);

void occurslist(exp *eo, type *a,llist *x) { DOLLIST(x,type *,y) occurs(eo,a,y); }; };

void occurs(exp *eo, type *a, type *b) {
  b=unwind(b);
  switch(b->t) {
    case VAR: if(a==b) fatal("recursive type detected",NIL,eo); break;
    case LIST: case VECTOR: occurs(eo,a,((typepar *)b)->par); break;
    case CLASST: occurslist(eo,a,&((typeobj *)b)->tl); break;		// also b->o->t?
    case TUPLE: occurslist(eo,a,&((typetuple *)b)->tl); break;
    case FUNCTION: occurs(eo,a,((typefun *)b)->ret); occurslist(eo,a,&((typefun *)b)->pars); break;
  };
};

int is_super_of(obj *super, obj *sub) {		// temp impl.
  if(sub->supers) FORSLIST(sub->supers,subtype *,s) {
    typeobj *to=(typeobj *)s->t;
    if(to->t!=CLASST) error("problem with supertype of",sub->name);
    ensure_objtype(NIL,to);
    if(strcmp(to->o->name,super->name)==0) return TRUE;
    is_super_of(super,to->o);
  };
  return FALSE;
};
type *common_super(obj *a, obj* b) { return anyt; };		// fow now

type *exact_unify(exp *eo, type *a, type *b) {
  unify_types(eo,a,b);
  return unify_types(eo,b,a);
};	// smarter?

type *unify_par(exp *eo, typepar *a, typepar *b, int a_is_upper_limit) {
  type *t=unify_types(eo,a->par,b->par,a_is_upper_limit);
  return a_is_upper_limit?a:new typepar(a->t,t);
};

type *unify_classt(exp *eo, typeobj *a, typeobj *b, int a_is_upper_limit) {
  int eq;
  if(strcmp(a->name,b->name)!=0) {
    ensure_objtype(eo,a);
    ensure_objtype(eo,b);
    eq=is_super_of(a->o,b->o);
  } else {
    if(a->o) {	// copy obj appropriately
      if(!b->o) { b->o=a->o; }
    } else {
      if(b->o) { a->o=b->o; } else { a->o=b->o=find_cursc_obj_err(eo,a->name); };
    };
    eq=(a->o==b->o);
  };
  if(eq) {
    // check on parametrisation. if fails set eq=FALSE
  };
  if(!eq) {
    if(a_is_upper_limit) type_error(eo,a,b);
    return common_super(a->o,b->o);
  };
  return a;
};

int unify_list(exp *eo, llist *al, llist *bl, llist *cl) {
  if(al->getnum()!=bl->getnum()) { error_rec("wrong tuple/function arity",NIL,eo); return FALSE; };
  eleml *be=bl->reset();
  DOLLIST(al,type *,ae)
    type *c=unify_types(eo,ae,(type *)be->e(),!cl);
    if(cl) cl->insert(c);
    be=bl->get(be);
  };
  return TRUE;
};

type *unify_tuple(exp *eo, typetuple *a, typetuple *b, int a_is_upper_limit) {
  typetuple *tt=a;
  return unify_list(eo,&a->tl,&b->tl,a_is_upper_limit?NIL:&(tt=new typetuple())->tl)?tt:a;
};

type *unify_fun(exp *eo, typefun *a, typefun *b, int a_is_upper_limit) {
  // allow ret to be NIL, too.
  type *c;
  if(a->ret) {
    if(!b->ret) { type_error(eo,a,b); return a; };
    c=unify_types(eo,a->ret,b->ret,a_is_upper_limit);
  } else {
    if(b->ret) {}; // allowed or not?
    c=b->ret;
  };
  typefun *tf=a;
  return unify_list(eo,&a->pars,&b->pars,a_is_upper_limit?NIL:(tf=new typefun(),tf->ret=c,&tf->pars))?tf:a;
};

inline void indirect(exp *eo, typepar *a, type *b) {
  int dum;
  b=clone_tparam(b,NIL,dum);
  occurs(eo,a,b);
  a->t=IND;
  a->par=b;
};

type *unify_types(exp *eo, type *a, type *b, int a_is_upper_limit) {
  if(!(a && b)) { if(a) a->show(); if(b) b->show(); printf(" with <NIL>\n"); intern("type_is_nil"); }
  a=unwind(a);
  b=unwind(b);
  if(a==b) return a;
  if(b->t==PAR) { return findbind(eo,(typepar *)b,a); };
  if(a->t==PAR) { return findbind(eo,(typepar *)a,b); };
  if(b->t==VAR) { indirect(eo,(typepar *)b,a); return a; };
  if(a->t==VAR) { indirect(eo,(typepar *)a,b); return b; };
  if(a->t==ANY) { return a; };		// sortof temp
  if(a->t!=b->t) { 
    if(a_is_upper_limit) { type_error(eo,a,b); return a; } else { return anyt; };
  }
  switch(a->t) {
    case LIST: case VECTOR: return unify_par(eo,(typepar *)a,(typepar *)b,a_is_upper_limit);
    case CLASST: return unify_classt(eo,(typeobj *)a,(typeobj *)b,a_is_upper_limit);
    case TUPLE: return unify_tuple(eo,(typetuple *)a,(typetuple *)b,a_is_upper_limit);
    case FUNCTION: return unify_fun(eo,(typefun *)a,(typefun *)b,a_is_upper_limit);
    default: intern("type_tag");
  };
  return NIL;
};

