// il.h: generating/loading/saving intermediate language files

#include "bla.h"

enum {
  IL_LD,IL_ST,IL_PARLD,IL_PARST,IL_INDLD,IL_INDST,IL_ARGLD,IL_ARGST,	// 0
  IL_VAL,IL_VALN,							// 8
  IL_IDX,IL_IDXC,IL_IDXS,IL_IDXSC,					// 10
  IL_CLOS,IL_CONS,IL_HDTL,IL_SELF,IL_PARENT,				// 14
  IL_JSR,IL_JSRCL,IL_JSRM,IL_JSRME,IL_SYS,				// 19
  IL_LAB,IL_BRA,IL_BT,IL_BF,IL_RET,IL_JTAB,				// 24
  IL_RAISE,IL_TRY,IL_ENDT,						// 30
  IL_DUP,IL_DROP,IL_SWAP,IL_PICK,IL_ROT,				// 33
  IL_NOT,IL_NEG,							// 38
  IL_ADD,IL_SUB,IL_MUL,IL_DIV,						// 40
  IL_EQ,IL_UNEQ,IL_HIGHER,IL_LOWER,IL_HIGHEQ,IL_LOWEQ,			// 44
  IL_AND,IL_OR,								// 50
  IL_CLOSE,IL_STRC,IL_TTYPE,IL_TUPLE,IL_TUPD
  // add new cg_ also
};

typedef unsigned char uchar;

class buf {
public:
  const int bufsize=64;
  buf *next;
  int used;
  uchar b[64];
  buf() { next=NIL; used=0; };
  buf *full()  { return (used>=bufsize)  ?(next=new buf()):this; };
  buf *full4() { return (used+4>=bufsize)?(next=new buf()):this; };
  void putch(uchar c) { b[used++]=c; };
  uchar *curpos() { return &b[used]; };
};

class iref {
public:
  const int version = 0;
  iref *next;
  int funid;
  char *funname;
  char builtin_num;	// for use by interpreter, 0=no_builtin
  iref() { next=NIL; funid=0; funname=""; builtin_num=0; };
};

class label {
public:
  label *next;
  int lab, lnum;
  label(label *n, int l, int num) { lab=l; next=n; lnum=num; };
};

class ifun : public iref {
public:
  int nargs, envsize, flags;
  int numlabels, numinstr, clen;
  int *labs;		// while saving, label *
  uchar *code;
  buf *cbuf;
  ifun() { labs=NIL; nargs=envsize=flags=numlabels=numinstr=clen=0; code=(uchar *)""; cbuf=NIL; };
  void addlabel(int l, int num) { labs=(int *)new label((label *)labs,l,num); };
};

class iinfo {		// a file is a list of these
public:
  const int version = 0;
  iinfo *next;
  iref *r;
  ifun *f;
  int numids;		// funs+refs
  char *srcname;
  iinfo() { next=NIL; r=NIL; f=NIL; numids=0; srcname=""; };
};

iinfo *newiinfo();
ifun *newifun();
iref *newiref();
buf *combinebufs(char *srcfilename,int numfunid);
int save_il(char *filename,buf *b);
int getcurbuflen();
int getcurnuminstr();

iinfo *ilload(char *ilname);
long geti();
int isend(uchar *end);
void setreadbuf(uchar *x);
uchar *getreadbuf();
int getnumprocessed();
uchar getch();
char *getstr();

void genld(int n);
void genst(int n);
void genparld(int n);
void genparst(int n);
void genindld(int n);
void genindst(int n);
void genargld(int n);
void genargst(int n);

void genval(int n);
void genvaln();

void genidx();
void genidxc();
void genidxs();
void genidxsc();

void genclos(int n,int l);
void gencons();
void genhdtl();
void genself();
void genparent();

void genjsr(int n);
void genjsrcl();
void genjsrm(int n);
void genjsrme(int n);
void gensys(int n);

void genlab(int n);
void genbra(int n);
void genbt(int n);
void genbf(int n);
void genret();
void genjtab(int n);

void genraise();
void gentry(int n);
void genendt();

void gendup();
void gendrop();
void genswap();
void genpick(int n);
void genrot();

void gennot();
void genneg();

void genadd();
void gensub();
void genmul();
void gendiv();

void geneq();
void genuneq();
void genhigher();
void genlower();
void genhigheq();
void genloweq();

void genand();
void genor();

void genclose(int n);
void genstrc(char *s);
void genttype(int n);
void gentuple(int n);
void gentupd(int n);
