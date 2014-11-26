// ilsave.c: save an intermediate file

#include "il.h"
#include <stdio.h>
#include <string.h>

iinfo *curiinfo=NIL;
ifun *curifun=NIL;

iinfo *newiinfo() { return curiinfo=new iinfo(); };

buf *curbuf;
int curbuflen;
int numinstr;

buf *startrbuf() { numinstr=0; curbuflen=0; return curbuf=new buf(); };
void startfbuf() { curifun->cbuf=startrbuf(); };
int getcurbuflen() { return curbuflen; };
int getcurnuminstr() { return numinstr; };

void addbufs(buf *b) {
  curbuf->next=b;
  curbuflen+=b->used;
  while(b->next) { b=b->next; curbuflen+=b->used; };
  curbuf=b;
};

iref *newiref() {
  iref *f=new iref();
  f->next=curiinfo->r; curiinfo->r=f;
  return f;
};

ifun *newifun() {
  ifun *f=curifun=new ifun();
  f->next=curiinfo->f; curiinfo->f=f;
  startfbuf();
  return f;
};

void bufc(uchar c) {
  curbuf=curbuf->full();
  curbuf->putch(c);
  curbuflen++;
};

void bufi(int i) {
  int neg;
  if((neg=(i<0))) i=-i;
  int v=i & 63;
  if((i=i>>6)) v|=128;
  if(neg) v|=64;
  bufc(v);
  while(i) {
    v=i & 127;
    if((i=i>>7)) v|=128;
    bufc(v);
  };
};

void buf2(int i) { bufc((i>>8)&255); bufc(i&255); };
void buf4(int i) { bufc((i>>24)&255); bufc((i>>16)&255); bufc((i>>8)&255); bufc(i&255); };
void strdat(uchar *s) { uchar c; while((c=*s++)) bufc(c); };
void str(char *s) { bufi(strlen(s)+1); strdat((uchar *)s); bufc(0); };
void put4(uchar *a,int v) { a+=4; for(int i=0;i<4;i++) { *(--a)=v&255; v>>=8; } };

uchar *backpatch(int &curlen) {
  curbuf=curbuf->full4();
  uchar *r=curbuf->curpos();
  strdat((uchar *)"BACK");
  curlen=curbuflen;
  return r;
};

void dobackpatch(uchar *bp,int snap) { put4(bp,curbuflen-snap); };

uchar *header(char *s,int version,int &snap) { strdat((uchar *)s); buf2(version); return backpatch(snap); };

buf *combinebufs(char *srcfilename,int numfunid) {
  buf *b=startrbuf();
  strdat((uchar *)"Bla ILFF");
  int filelensnap; uchar *filelenbp=backpatch(filelensnap);
  iinfo *i=curiinfo;
  while(i) {
    int isnap; uchar *ibp=header("INFO",i->version,isnap);
    bufi(numfunid);
    str(srcfilename);
    dobackpatch(ibp,isnap);
    iref *r=i->r;    
    while(r) {
      int rsnap; uchar *rbp=header("FUNR",r->version,rsnap);
      bufi(r->funid); str(r->funname);
      dobackpatch(rbp,rsnap);
      r=r->next;
    };
    ifun *f=i->f;
    while(f) {
      int fsnap; uchar *fbp=header("FUND",f->version,fsnap);
      bufi(f->funid); str(f->funname);
      bufi(f->nargs); bufi(f->envsize); bufc(f->flags);
      bufi(f->numlabels);
      for(int nl=0;nl<f->numlabels;nl++) {
	label *lab=(label *)f->labs;
        for(;lab;lab=lab->next) if(lab->lnum==nl) break;	// not quite O(1) !
        if(!lab) { printf("internal: missing label?\n"); } else { bufi(lab->lab); };
      };
      bufi(f->numinstr); bufi(f->clen);
      addbufs(f->cbuf);
      dobackpatch(fbp,fsnap);
      f=(ifun *)f->next;
    };
    i=i->next;
  };
  dobackpatch(filelenbp,filelensnap);
  return b;
};

int save_il(char *filename,buf *b) {
  FILE *fh;
  int prob=FALSE;
  if((fh=fopen(filename,"wb"))) {
    while(b && !prob) {
      if(fwrite(&b->b[0],b->used,1,fh)!=1) prob=TRUE;
      b=b->next;
    };
    fclose(fh);
  } else {
    prob=TRUE;
  };
  return prob;
};

void genld(int n) { numinstr++; bufc(IL_LD); bufi(n); };
void genst(int n) { numinstr++; bufc(IL_ST); bufi(n); };
void genparld(int n) { numinstr++; bufc(IL_PARLD); bufi(n); };
void genparst(int n) { numinstr++; bufc(IL_PARST); bufi(n); };
void genindld(int n) { numinstr++; bufc(IL_INDLD); bufi(n); };
void genindst(int n) { numinstr++; bufc(IL_INDST); bufi(n); };
void genargld(int n) { numinstr++; bufc(IL_ARGLD); bufi(n); };
void genargst(int n) { numinstr++; bufc(IL_ARGST); bufi(n); };

void genval(int n) { numinstr++; bufc(IL_VAL); bufi(n); };
void genvaln() { numinstr++; bufc(IL_VALN); };

void genidx() { numinstr++; bufc(IL_IDX); };
void genidxc() { numinstr++; bufc(IL_IDXC); };
void genidxs() { numinstr++; bufc(IL_IDXS); };
void genidxsc() { numinstr++; bufc(IL_IDXSC); };

void genclos(int n,int l) { numinstr++; bufc(IL_CLOS); bufi(n); bufi(l); };
void gencons() { numinstr++; bufc(IL_CONS); };
void genhdtl() { numinstr++; bufc(IL_HDTL); };
void genself() { numinstr++; bufc(IL_SELF); };
void genparent() { numinstr++; bufc(IL_PARENT); };

void genjsr(int n) { numinstr++; bufc(IL_JSR); bufi(n); };
void genjsrcl() { numinstr++; bufc(IL_JSRCL); };
void genjsrm(int n) { numinstr++; bufc(IL_JSRM); bufi(n); };
void genjsrme(int n) { numinstr++; bufc(IL_JSRME); bufi(n); };
void gensys(int n) { numinstr++; bufc(IL_SYS); bufi(n); };

void genlab(int n) { curifun->addlabel(curbuflen,n); numinstr++; bufc(IL_LAB); bufi(n); };
void genbra(int n) { numinstr++; bufc(IL_BRA); bufi(n); };
void genbt(int n) { numinstr++; bufc(IL_BT); bufi(n); };
void genbf(int n) { numinstr++; bufc(IL_BF); bufi(n); };
void genret() { numinstr++; bufc(IL_RET); };
void genjtab(int n) { numinstr++; bufc(IL_JTAB); bufi(n); };

void genraise() { numinstr++; bufc(IL_RAISE); };
void gentry(int n) { numinstr++; bufc(IL_TRY); bufi(n); };
void genendt() { numinstr++; bufc(IL_ENDT); };

void gendup() { numinstr++; bufc(IL_DUP); };
void gendrop() { numinstr++; bufc(IL_DROP); };
void genswap() { numinstr++; bufc(IL_SWAP); };
void genpick(int n) { numinstr++; bufc(IL_PICK); bufi(n); };
void genrot() { numinstr++; bufc(IL_ROT); };

void gennot() { numinstr++; bufc(IL_NOT); };
void genneg() { numinstr++; bufc(IL_NEG); };

void genadd() { numinstr++; bufc(IL_ADD); };
void gensub() { numinstr++; bufc(IL_SUB); };
void genmul() { numinstr++; bufc(IL_MUL); };
void gendiv() { numinstr++; bufc(IL_DIV); };

void geneq() { numinstr++; bufc(IL_EQ); };
void genuneq() { numinstr++; bufc(IL_UNEQ); };
void genhigher() { numinstr++; bufc(IL_HIGHER); };
void genlower() { numinstr++; bufc(IL_LOWER); };
void genhigheq() { numinstr++; bufc(IL_HIGHEQ); };
void genloweq() { numinstr++; bufc(IL_LOWEQ); };

void genand() { numinstr++; bufc(IL_AND); };
void genor() { numinstr++; bufc(IL_OR); };

void genclose(int n) { numinstr++; bufc(IL_CLOSE); bufi(n); };
void genstrc(char *s) { numinstr++; bufc(IL_STRC); str(s); };
void genttype(int n) { numinstr++; bufc(IL_TTYPE); bufi(n); };
void gentuple(int n) { numinstr++; bufc(IL_TUPLE); bufi(n); };
void gentupd(int n) { numinstr++; bufc(IL_TUPD); bufi(n); };
