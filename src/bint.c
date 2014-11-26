// bint.c: interpret an IL file

#include "il.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int debug=0;
int unsafe_print=0;

#define P printf

#define MAXSTACK 10000
ifun **funtab;
long stackmem[MAXSTACK];

#define jump(a) setreadbuf(a)
void show(long x, long l=0);

//--------------------environments--------------------------

class tagged { public: long tag; };	// Env/Functionclosure/Conscell/Tuple

#define MAX_VARS 25			// dom

class env : public tagged { public:
  env *prev, *parent;
  ifun *fun;
  long *args;		// ptr in stack, also stacklevel
  uchar *pc, *end;	// ptr in codebuf
  long v[MAX_VARS];
  env(env *p,env *par,ifun *f,long *sp) {
    if(debug) { printf("enter: %s [",f->funname); for(int i=-1;i>-4;i--) { show(sp[i]); P(" "); }; P("]\n"); };
    prev=p; parent=par; fun=f; args=sp;
    jump(f->code);
    end=fun->code+fun->clen;
    tag='E';
  };
};

void error(char *s,char *o=0) { P("bint: %s",s); if(o) P(" `%s'",o); P("\n"); exit(1); };

void isenv(void *e, char *s) { if(!e) error("NIL isenv",s); if(((env *)e)->tag!='E') error("not an env!",s); };

//----------------------closures---------------------------------------

class closure : public tagged { public: long e, f; };

long clos(ifun *f,env *e) {		// construct closure value
  closure *c=new closure();
  c->e=(long)e;
  c->f=(long)f;
  c->tag='F';
  return (long)c;
};

env *eclos(long c) { return (env *)((closure *)c)->e; };
ifun *fclos(long c) { return (ifun *)((closure *)c)->f; };

//-----------------cons-cells-----------------------------------------------

class conscell : public tagged { public: long h, t; };

long cons(long h,long t) {
  conscell *c=new conscell();
  c->h=h;
  c->t=t;
  c->tag='C';
  return (long)c;
};

long car(long c, char *f) { if(((conscell *)c)->tag!='C') error("illegal car in",f); return ((conscell *)c)->h; };
long cdr(long c, char *f) { if(((conscell *)c)->tag!='C') error("illegal cdr in",f); return ((conscell *)c)->t; };

//-----------------tuples------------------------------------------------------

class tuplet : public tagged { public: int n; long e[2]; };

long tuple(int n) {
  tuplet *t=(tuplet *)new char[sizeof(tuplet)+sizeof(long)*(n-2)];
  t->tag='T';
  t->n=n;
  return (long)t;
};

void puttuple(long t, int n, long v) { ((tuplet *)t)->e[n]=v; };
long gettuple(long tu, int n, char *f) { tuplet *t=(tuplet *)tu; if(t->tag!='T') error("not a tuple in",f); return t->e[n]; };

//--------------printing-------------------------------------------

#define UNSAFE_LIMIT 100000

void show(long x, long l) {
  if(l>2) { P("..."); return; };
  if(unsafe_print && ((x>UNSAFE_LIMIT) || (x<-UNSAFE_LIMIT))) {
    switch(((tagged *)x)->tag) {
      case 'C': { conscell *c=(conscell *)x; P("["); show(c->h,l+1); P("|"); show(c->t,l+1); P("]"); }; return;
      case 'F': { P("<%s>",((ifun *)((closure *)x)->f)->funname); }; return;
      case 'T': { tuplet *t=(tuplet *)x; int n=1; P("("); show(t->e[0],l+1); while(n<t->n) { P(","); show(t->e[n++],l+1); }; P(")"); }; return;
      case 'E': { env *e=(env *)x; P("%s()",e->fun->funname); }; return;
    };
  };
  P("%ld",x);
};

//-------------env-&-stack-access----------------------------------------

long &anyenv(env *e,int n) {
  if(n<0 || n>=MAX_VARS) error("env deref out of bounds");
  isenv(e,NIL);
  return e->v[n];
};

long popsp(long *&sp) {
  if(sp<=stackmem) error("stack underflow");
  return *--sp;
};

long &topsp(long *sp) {
  if(sp<=stackmem) error("stack underflow (top)");
  return *(sp-1);
};

void pushsp(long *&sp,long v) {
 if(sp>=(stackmem+MAXSTACK)) error("stack overflow");
 *sp++=v;
};

ifun *getifun(int n,iinfo *i) {
  if(n<0 || n>=i->numids) error("funid out of range");
  ifun *f=funtab[n];
  if(!f) error("NIL ifun");
  //if(f->builtin_num) error("ref instead of function");
  return f;
};

long &argadr(env *e,int n) {
  if(n<0 || n>=e->fun->nargs) error("arg num out of bounds");
  return e->args[-e->fun->nargs+n];
};

uchar *getlabel(int nl, env *e) {
  if(nl<0 || nl>=e->fun->numlabels) error("label num out of bounds");
  return e->fun->code+e->fun->labs[nl];
};

#define kees(x) break; case x:
#define pop() popsp(sp)
#define top() topsp(sp)
#define push(v) pushsp(sp,v)
#define enva(n) anyenv(curenv,n)
#define parentenv(n) anyenv(curenv->parent,n)
#define arga(n) argadr(curenv,n)
#define is_env(x) isenv((void *)(x),curenv->fun->funname)

//-----------------builtins-------------------------------------------------

char *builtin_names[] = {		// starting from 1
  "stdout","stdin","stderr",
  "getc","putc",
  "strcmp","puts",NIL
};

long try_builtin(long *&sp,long cl) {
  ifun *f=fclos(cl);
  //if(debug) { printf("trying: %s %d %d\n",f->funname,f->funid,f->builtin_num); };
  switch(f->builtin_num) {
    case 0: return FALSE;
    case 1: push((long)stdout); break;
    case 2: push((long)stdin); break;
    case 3: push((long)stderr); break;
    case 4: { long f=pop(); push(fgetc((FILE *)f)); }; break;
    case 5: { long f=pop(); long c=pop(); push(fputc(c,(FILE *)f)); }; break;
    case 6: { long a=pop(); long b=pop(); push(strcmp((char *)a,(char *)b)==0); }; break;
    case 7: { long f=pop(); long s=pop(); push(fputs((char *)s,(FILE *)f)); }; break;
    default: error("unimplemented builtin");
  };
  return TRUE;
};

//-------------the-interpreter-----------------------------------

void iloop(iinfo *ii,ifun *mainf) {
  long opc;
  long *sp=stackmem, *reclevel=NIL;
  env *curenv=new env(NIL,NIL,mainf,sp);
  readeval:
  while(!isend(curenv->end)) {
    switch(opc=getch()) {
      case IL_LD:     push(enva(geti()));
      kees(IL_ST)     enva(geti())=pop();
      kees(IL_PARLD)  push(parentenv(geti()));
      kees(IL_PARST)  parentenv(geti())=pop();
      kees(IL_INDLD)  { long p=pop(); is_env(p); push(anyenv((env *)p,geti())); };
      kees(IL_INDST)  { long e=pop(); is_env(e); anyenv((env *)e,geti())=pop(); };
      kees(IL_ARGLD)  push(arga(geti()));
      kees(IL_ARGST)  arga(geti())=pop();
      kees(IL_VAL)    push(geti());
      kees(IL_VALN)   push(NIL);
      kees(IL_STRC)   push((long)getstr());
      kees(IL_IDX)    error("not implemented:\tidx\n");
      kees(IL_IDXC)   error("not implemented:\tidxc\n");
      kees(IL_IDXS)   error("not implemented:\tidxs\n");
      kees(IL_IDXSC)  error("not implemented:\tidxsc\n");
      kees(IL_CLOS)   { long fi=geti(); long l=geti(); env *e=curenv; while(l--) { if(!e) error("no env: no parent"); e=e->parent; }; push(clos(getifun(fi,ii),e)); };
      kees(IL_CLOSE)  { long e=pop(); is_env(e); push(clos(getifun(geti(),ii),(env *)e)); };
      kees(IL_CONS)   { long h=pop(); long t=pop(); push(cons(h,t)); };
      kees(IL_HDTL)   { long c=pop(); push(cdr(c,curenv->fun->funname)); push(car(c,curenv->fun->funname)); };
      kees(IL_TUPLE)  { long n; long t=tuple(n=geti()); while(n--) puttuple(t,n,pop()); push(t); };
      kees(IL_TUPD)   { long n=geti(),t=pop(); while(n--) push(gettuple(t,n,curenv->fun->funname)); };
      kees(IL_SELF)   push((long)curenv);
      kees(IL_PARENT) error("not implemented:\tparent\n");
      kees(IL_TTYPE)  { ifun *f=getifun(geti(),ii); env *e=(env *)pop(); is_env(e); push(e->fun==f); };
      kees(IL_JSR)    error("not implemented:\tjsr\n");
      kees(IL_JSRCL)  { curenv->pc=getreadbuf(); long cl=pop(); if(!try_builtin(sp,cl)) { curenv=new env(curenv,eclos(cl),fclos(cl),sp); }; };
      kees(IL_JSRM)   error("not implemented:\tjsrm\n");
      kees(IL_JSRME)  error("not implemented:\tjsrme\n");
      kees(IL_SYS)    error("not implemented:\tsys\n");
      kees(IL_LAB)    geti();
      kees(IL_BRA)    jump(getlabel(geti(),curenv));
      kees(IL_BT)     { long l=geti(); if(pop())  jump(getlabel(l,curenv)); };
      kees(IL_BF)     { long l=geti(); if(!pop()) jump(getlabel(l,curenv)); };
      kees(IL_RET)    goto returnfun;
      kees(IL_JTAB)   error("not implemented:\tjtab\t"); { long i,n; n=geti(); P("%ld",n); for(i=0;i<n;i++) P(",%ld",geti()); P("\n"); };
      kees(IL_RAISE)  error("not implemented:\traise\n");
      kees(IL_TRY)    error("not implemented:\ttry\n");
      kees(IL_ENDT)   error("not implemented:\tendt\n");
      kees(IL_DUP)    { long v=pop(); push(v); push(v); };
      kees(IL_DROP)   pop();
      kees(IL_SWAP)   { long b=pop(); long a=pop(); push(b); push(a); };
      kees(IL_PICK)   error("not implemented:\tpick\n");
      kees(IL_ROT)    { long c=pop(); long b=pop(); long a=pop(); push(b); push(c); push(a); };
      kees(IL_NOT)    { long v=pop(); push(!v); };
      kees(IL_NEG)    { long v=pop(); push(-v); };
      kees(IL_ADD)    { long a=pop(); top()+=a; };
      kees(IL_SUB)    { long a=pop(); top()-=a; };
      kees(IL_MUL)    { long a=pop(); top()*=a; };
      kees(IL_DIV)    { long a=pop(); top()/=a; };
      kees(IL_EQ)     { long f=pop(); long a=pop(); push(a==f); };
      kees(IL_UNEQ)   { long f=pop(); long a=pop(); push(a!=f); };
      kees(IL_HIGHER) { long f=pop(); long a=pop(); push(a>f);  };
      kees(IL_LOWER)  { long f=pop(); long a=pop(); push(a<f);  };
      kees(IL_HIGHEQ) { long f=pop(); long a=pop(); push(a>=f); };
      kees(IL_LOWEQ)  { long f=pop(); long a=pop(); push(a<=f); };
      kees(IL_AND)    { long a=pop(); top()&=a; };
      kees(IL_OR)     { long a=pop(); top()|=a; };
      break;
      default:        { char b[100]; sprintf(b,"%ld",opc); error("unknown opcode:",b); };
    };
    if(debug>1) { printf("opc: %ld:  \t",opc); long *sd=sp-3; if(sd<=stackmem) { sd=stackmem; } else { P("..."); }; for(;sd<sp;sd++) { P(" "); show(*sd); }; printf("\n"); };
  };
  returnfun:
  if(debug) { printf("exit: %s\n",curenv->fun->funname); };
  switch(sp-curenv->args) {
    case 0:
      if(debug) { P("warning: %s returned no value\n",curenv->fun->funname); };
      sp-=curenv->fun->nargs;
      push(NIL);
      break;
    case 1: {
      long v=pop();
      sp-=curenv->fun->nargs;
      push(v);
      break;
    };
    default:
      error("more than 1 value returned by",curenv->fun->funname);
  };
  if((curenv=curenv->prev)) {
    jump(curenv->pc);
    goto readeval;
  };
};

ifun *resolve(iinfo *i) {		// find main() and buitins etc.
  funtab=new ifun *[i->numids];
  ifun *mainf=NIL;
  for(ifun *f=i->f;f;f=(ifun *)f->next) {
    funtab[f->funid]=f;
    if(strcmp(f->funname,"main")==0) mainf=f;
  };
  if(!mainf) error("no main() function!");
  for(iref *r=i->r;r;r=r->next) {
    char **tab=builtin_names;
    int n=1;
    for(;*tab;n++,tab++) if(strcmp(r->funname,*tab)==0) break;
    if(!*tab) error("unknown function referenced:",r->funname);
    funtab[r->funid]=(ifun *)r;
    r->builtin_num=n;
  };
  return mainf;
};

int main(int argc, char **argv) {
  int ac=1;
  while(ac<argc && argv[ac][0]=='-') {
    switch(argv[ac][1]) {
      case 'h': printf("bint [-opt] basefilename: interprets Bla intermediate (EMMER) files\n"
                       "-h\tthis help\n-d\tprint debugging output\n"
                       "-s\tprint heavy debugging output (stacks + opcodes)\n"
                       "-u\tallow unsafe printing code (works on Amiga, might coredump elsewhere)\n"); break;
      case 'd': debug=1; break;
      case 's': debug=2; break;
      case 'u': unsafe_print=1; break;
      default:  error("unknown commandline switch",argv[ac]);
    };
    ac++;
  };
  if(ac>=argc) error("missing EMMER (.il) file");
  char *basename=argv[ac++];
  if(ac!=argc) error("superfluous args");
  char ilname[100]; strcpy(ilname,basename); strcat(ilname,".il");
  iinfo *i=ilload(ilname);
  if(!i) { int o=getnumprocessed(); if(o) P("at offset %d\n",o); error("failed to read file",ilname); };
  if(i->next) error("current interpreter supports only one-source compiled programs");
  ifun *mainf=resolve(i);
  iloop(i,mainf);
  return 0;
};
