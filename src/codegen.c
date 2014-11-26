/*--------------------------the-code-generator-----------------------------*/

#include "il.h"
#include "lex.h"
#include "object.h"
#include "error.h"

void fcg(obj *o);

void pm_setup();
int pm_getlab();
void pm_finish();

int lab;
llist nocg_funs;

void addfun(obj *o) { if(!o->codegen_done) nocg_funs.insert(o); };

void NI(exp *eo=NIL) { error("cannot generate code for this construct: not yet implemented",NIL,eo); };

void generate_code(char *ilname,char *fname) {
  newiinfo();
  { DOLLIST((&topscope->objs),obj *,e) fcg(e); }; };
  { DOLLIST((&nocg_funs),obj *,e) fcg(e); }; };
  buf *b=combinebufs(fname,numfunids);
  if(!errorcount) {
    printf("no errors... writing to `%s'\n",ilname);
    if(save_il(ilname,b)) fatal("problem writing IL file ",ilname);
  };
};

void fcg(obj *o) {
  ifun *f;
  if(!o->extern_def) {
    DOLLIST((&o->sc->objs),obj *,so) fcg(so); };
    f=newifun();
    lab=0;
    { DOLLIST((&o->sc->vars),var *,sv) sv->cg(FALSE); }; };
    if(o->ret) o->ret->cg(TRUE);		// should be returns_value
    f->nargs=o->pars.getnum();
    f->envsize=o->maxnum;
    if(o->dynamic_self) f->flags|=1;		// bot these need support
    if(o->parent) f->flags|=2;
    f->numlabels=lab;
    f->numinstr=getcurnuminstr();
    f->clen=getcurbuflen();
    o->codegen_done=TRUE;
  } else {
    f=(ifun *)newiref();
  };
  f->funid=o->funid;
  f->funname=o->name;
};

// all virtual, but not repeated

void exp::cg(int rv) { NI(this); };
void feature::cg(int rv) { NI(this); };

void obj::cg(int rv) { if(rv) genclos(funid,0); addfun(this); };

void cg_var(int level, int rv, var *v, int envused=FALSE) {
  if(rv) {
    if(envused) {
      if(level) intern("env_level");
      genindld(v->vnum);
    }
  } else {
    if(level>1) intern("level_2");
    if(level==1) {
      genparld(v->vnum);
    } else {
      if(v->vnum<0) { genargld(-(v->vnum+1)); } else { genld(v->vnum); };
    };
  };
};

void cg_assign_core(int level, exp *right, int rv, var *v, exp *env=NIL) {
  right->cg(TRUE);
  if(rv) gendup();
  if(env) env->cg(TRUE);
  if(env) {
    if(level) intern("env_level_lval");
    genindst(v->vnum);
  } else {
    if(level>1) intern("level_2_lval");
    if(level==1) {
      genparst(v->vnum);
    } else {
      if(v->vnum<0) { genargst(-(v->vnum+1)); } else { genst(v->vnum); };
    };
  };
};

void var::cg(int rv) {
  if(defaultv) {
    cg_assign_core(0,defaultv,rv,this);
  } else {
    cg_var(0,rv,this);
  };
};

void cg_assign(int rv, exp *left, exp *right) {
  exp *env=NIL;
  var *v;
  int lev=0;
  if(left->tag()==tag_idexp) {		// very adhoc: should work for all sorts of expressions
    idexp *i=(idexp *)left;
    v=(var *)i->f;
    lev=i->level;
    if(v->tag()==tag_obj) error("assignment to function");
    env=i->env;
  } else {
    if(left->tag()!=tag_var) error("identifier expression expected (for now)");
    v=(var *)left;
    if(v->defaultv) intern("def_assign",v->name);
  };
  cg_assign_core(lev,right,rv,v,env);
};

void op::cg(int rv) {
  if(opt==LEX_ASSIGN) {
    cg_assign(rv,left,right);
  } else if(opt==KEY_AND || opt==KEY_OR) {
    left->cg(TRUE);
    int l=lab++;
    if(rv) gendup();
    opt==KEY_OR?genbt(l):genbf(l);
    if(rv) gendrop();
    right->cg(rv);
    genlab(l);
  } else {
    left->cg(rv);
    right->cg(rv);
    if(rv) {
      switch(opt) {
        case '+':		genadd();	break;
        case '-':		gensub();	break;
        case '*':		genmul();	break;
        case '/':		gendiv();	break;
        case '=':		geneq();	break;
        case '>':		genhigher();	break;
        case '<':		genlower();	break;
        case LEX_UNEQ:		genuneq();	break;
        case LEX_HIGHEQ:	genhigheq();	break;
        case LEX_LOWEQ:		genloweq();	break;
        default:		intern("op_case");
      };
    };
  };
};

void unify::cg(int rv) {
  int el;
  e->cg(TRUE);
  pm_setup();
  p->cg(rv);
  el=lab++;
  genval(-1);
  genbra(el);
  pm_finish();
  genvaln();
  genlab(el);
};

void idexp::cg(int rv) {		// what about rv
  if(env) env->cg(rv);
  if(f->tag()==tag_obj) {
    obj *o=(obj *)f;
    if(env) { genclose(o->funid); } else { genclos(o->funid,level); };
  } else {
    cg_var(level,rv,(var *)f,(long)env);
  };
};

void app::cg(int rv) {
  FORSLIST((&args),exp *,e) e->cg(TRUE);
  fun->cg(TRUE);
  genjsrcl();
  if(!rv) gendrop();
};

void aselect::cg(int rv) { NI(this); };

void clos::cg(int rv) {
  if(sc) {
    DOLLIST((&sc->objs),obj *,so) addfun(so); };
    { DOLLIST((&sc->vars),var *,sv) sv->cg(FALSE); }; };
  };
  if(el) FORSLIST(el,exp *,e) e->cg(FALSE);
  if(head) { head->cg(rv); } else if(rv) { genvaln(); };
};

void intc::cg(int rv) { if(rv) genval(value); };
void stringc::cg(int rv) { genstrc(str); if(!rv) gendrop(); };

void unary::cg(int rv) {
  e->cg(rv);
  if(op=='-') {
    if(rv) genneg();
  } else if(op==KEY_NOT) {
    if(rv) gennot();
  } else {
    intern("cg-unary");
  };
};

void ifthen::cg(int rv) {
  int fl=lab++;
  if(switchexp) {
    switchexp->cg(TRUE);
    int num=0;
    FORSLIST((&cc),condconseq *,c) {
      if(++num!=cc.getnum()) gendup();
      pm_setup();
      c->cond->cg(TRUE);
      if(num!=cc.getnum()) gendrop();
      c->conseq->cg(rv);
      genbra(fl);
      pm_finish();
    };
  } else {
    FORSLIST((&cc),condconseq *,c) {
      int nl=lab++;
      c->cond->cg(TRUE);
      genbf(nl);
      c->conseq->cg(rv);
      genbra(fl);
      genlab(nl);
    };
  };
  if(def) { def->cg(rv); } else { if(rv) genvaln(); };
  genlab(fl);
};

void loop::cg(int rv) { NI(this); };
void retexp::cg(int rv) { e->cg(TRUE); genret(); };
void self::cg(int rv) { if(rv) genself(); };
void parent::cg(int rv) { if(rv) genparent(); };
void boolval::cg(int rv) { if(rv) genval(boo); };
void nil::cg(int rv) { if(rv) genvaln(); };
void lisplist::cg(int rv) { tail->cg(rv); head->cg(rv); if(rv) gencons(); };

void tuple::cg(int rv) {
  FORSLIST((&exps),exp *,e) e->cg(rv);
  if(rv) gentuple(exps.getnum());
};

// these expect a value on the stack, and will jump to one of these labs if match fails

#define PM_MAXLEV 25
int pm_level;
int pm_labs[PM_MAXLEV];

void pm_setup() {
  pm_level=0;
  for(int i=0;i<PM_MAXLEV;i++) pm_labs[i]=-1;
};

int pm_getlab() {
  if(pm_level>=PM_MAXLEV) intern("pm_match_depth");
  return (pm_labs[pm_level]==-1)?(pm_labs[pm_level]=lab++):pm_labs[pm_level];
};

void pm_finish() {
  int started=FALSE;
  for(int i=PM_MAXLEV-1;i>=0;i--) {
    if(pm_labs[i]!=-1) { started=TRUE; genlab(pm_labs[i]); };
    if(started && i) gendrop();
  };
};

void pat::cg(int rv) { NI(this); };			// (matchvalue-bool)
void allpat::cg(int rv) { gendrop(); };
void nilpat::cg(int rv) { genbt(pm_getlab()); };
void twopat::cg(int rv) { gendup(); pm_level++; left->cg(TRUE); pm_level--; right->cg(TRUE); };
void intpat::cg(int rv) { if(value) { genval(value); geneq(); genbf(pm_getlab()); } else { genbt(pm_getlab()); }; };
void strpat::cg(int rv) { NI(this); };
void idpat::cg(int rv) { genst(v->vnum); };

void constrpat::cg(int rv) { 
  gendup();
  pm_level++;
  genttype(o->funid);
  genbf(pm_getlab());
  pm_level--;
  llist *vs=&o->sc->vars;
  eleml *v=vs->reset();
  pm_level++;		// for whole loop
  if(args) FORSLIST(args,pat *,p) {
    gendup();
    cg_var(0,TRUE,(var *)v->e(),TRUE);
    p->cg(TRUE);
    v=vs->get(v);
  };
  gendrop();
  pm_level--;
};

void tuplepat::cg(int rv) {
  gentupd(pats.getnum());
  pm_level+=pats.getnum();
  FORSLIST((&pats),pat *,p) { pm_level--; p->cg(TRUE); };
};

void listnilpat::cg(int rv) { genbt(pm_getlab()); };
void listpat::cg(int rv) { genhdtl(); pm_level++; head->cg(rv); pm_level--; tail->cg(rv); };
void vectorpat::cg(int rv) { NI(this); };
