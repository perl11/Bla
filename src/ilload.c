// ilload.c: load an intermediate file

#include "il.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define P printf

uchar *b=NIL,*begin=NIL;

long geti() {
  long v;
  uchar x=*b++;
  char neg=x&64;
  v=x&63;
  int sh=6;
  while(x&128) { v=v|(((x=*b++)&127)<<sh); sh+=7; };
  if(neg) v=-v;
  return v;
};

int isend(uchar *end) { return b>=end; };
void setreadbuf(uchar *x) { b=x; };		// also used to return anywhere
uchar *getreadbuf() { return b; };
int getnumprocessed() { return b-begin; };
uchar getch() { return *b++; };
char *getstr() { long l=geti(); char *r=(char *)b; b+=l; return r; };
int get2() { return (*b++<<8)+*b++; };
int get4() { return (((((*b++<<8)+*b++)<<8)+*b++)<<8)+*b++; };
int geth4(uchar *a) { int v=0; for(int i=0;i<4;i++) v=(v<<8)+*a++; return v; };
#define getheader(v) { b+=4; if(get2()>v) { P("you need a newer version of this program"); return NIL; }; chunklen=get4(); }

iinfo *process(uchar *buf,int l) {
  setreadbuf(buf); begin=buf;
  iinfo *i=NIL,*ci=NIL;
  uchar *end=b+l;
  while(b<end) {				// process chunks
    int chunklen;
    if(strncmp((char *)b,"INFO",4)==0) {
      ci=(i?ci->next:i)=new iinfo();
      getheader(ci->version);
      ci->numids=geti();			// build table later
      ci->srcname=getstr();
    } else if(strncmp((char *)b,"FUND",4)==0) {
      if(!ci) return NIL;
      ifun *f=new ifun(); f->next=ci->f; ci->f=f;
      getheader(f->version);
      f->funid=geti();
      f->funname=getstr();
      f->nargs=geti();
      f->envsize=geti();
      f->flags=getch();
      f->numlabels=geti();
      f->labs=new int[f->numlabels];
      for(int nl=0;nl<f->numlabels;nl++) { f->labs[nl]=geti(); };
      f->numinstr=geti();
      f->clen=geti();
      f->code=b;
      b+=f->clen;
    } else if(strncmp((char *)b,"FUNR",4)==0) {
      if(!ci) return NIL;
      iref *r=new iref(); r->next=ci->r; ci->r=r;
      getheader(r->version);
      r->funid=geti();
      r->funname=getstr();
    } else {
      P("unknown ILFF chunk: `%4.4s'\n",b);
      return NIL;
    };
  };
  if(b==end && i) return i;
  return NIL;
};

iinfo *ilload(char *ilname) {
  FILE *fh;
  uchar head[12];
  if((fh=fopen(ilname,"rb"))) {
    if(fread(head,12,1,fh)==1) {
      if(strncmp((char *)head,"Bla ILFF",8)==0) {
        int fl=geth4(head+8);
        uchar *buf=new uchar[fl];
        if(fread(buf,fl,1,fh)==1 || 1) {	// fails?!?
          return process(buf,fl);
        } else P("file structure corrupt\n");
      } else P("no Bla ILFF file\n");
    } else P("short file\n");
  } else P("no such file\n");
  return NIL;
}
