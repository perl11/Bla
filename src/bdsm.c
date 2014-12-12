// bdsm.c: disassemble an IL file

#include "il.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define P printf

void error(char *s,char *extra=NIL) { P("bdsm: "); P(s,extra); exit(1); };

iref *findfun(long n,iinfo *i) {
  for(iref *r=i->r;r;r=r->next) if(r->funid==n) return r;
  for(ifun *f=i->f;f;f=(ifun *)f->next) if(f->funid==n) return f;
  error("funid %p not found\n",(char*)n); return NULL;
};

void instr(iinfo *ii) {
  int i,n,opc;
  switch(opc=getch()) {
    case IL_LD:     P("\tld\t%ld\n",geti()); break;
    case IL_ST:     P("\tst\t%ld\n",geti()); break;
    case IL_PARLD:  P("\tparld\t%ld\n",geti()); break;
    case IL_PARST:  P("\tparst\t%ld\n",geti()); break;
    case IL_INDLD:  P("\tindld\t%ld\n",geti()); break;
    case IL_INDST:  P("\tindst\t%ld\n",geti()); break;
    case IL_ARGLD:  P("\targld\t%ld\n",geti()); break;
    case IL_ARGST:  P("\targst\t%ld\n",geti()); break;
    case IL_VAL:    P("\tval\t%ld\n",geti()); break;
    case IL_VALN:   P("\tvaln\n"); break;
    case IL_STRC:   P("\tstrc\t'%s'\n",getstr()); break;
    case IL_IDX:    P("\tidx\n"); break;
    case IL_IDXC:   P("\tidxc\n"); break;
    case IL_IDXS:   P("\tidxs\n"); break;
    case IL_IDXSC:  P("\tidxsc\n"); break;
    case IL_CLOS:   { long fi=geti(); P("\tclos\t%s\t%ld\n",findfun(fi,ii)->funname,geti()); }; break;
    case IL_CLOSE:  P("\tclose\t%s\n",findfun(geti(),ii)->funname); break;
    case IL_CONS:   P("\tcons\n"); break;
    case IL_HDTL:   P("\thdtl\n"); break;
    case IL_TUPLE:  P("\ttuple\t%ld\n",geti()); break;
    case IL_TUPD:   P("\ttupd\t%ld\n",geti()); break;
    case IL_SELF:   P("\tself\n"); break;
    case IL_PARENT: P("\tparent\n"); break;
    case IL_TTYPE:  P("\tttype\t%s\n",findfun(geti(),ii)->funname); break;
    case IL_JSR:    P("\tjsr\t%s\n",findfun(geti(),ii)->funname); break;
    case IL_JSRCL:  P("\tjsrcl\n"); break;
    case IL_JSRM:   P("\tjsrm\t%ld\n",geti()); break;
    case IL_JSRME:  P("\tjsrme\t%ld\n",geti()); break;
    case IL_SYS:    P("\tsys\t%ld\n",geti()); break;
    case IL_LAB:    P("%ld:",geti()); break;
    case IL_BRA:    P("\tbra\t%ld\n",geti()); break;
    case IL_BT:     P("\tbt\t%ld\n",geti()); break;
    case IL_BF:     P("\tbf\t%ld\n",geti()); break;
    case IL_RET:    P("\tret\n"); break;
    case IL_JTAB:   P("\tjtab\t"); n=geti(); P("%d",n); for(i=0;i<n;i++) P(",%ld",geti()); P("\n"); break;
    case IL_RAISE:  P("\traise\n"); break;
    case IL_TRY:    P("\ttry\t%ld\n",geti()); break;
    case IL_ENDT:   P("\tendt\n"); break;
    case IL_DUP:    P("\tdup\n"); break;
    case IL_DROP:   P("\tdrop\n"); break;
    case IL_SWAP:   P("\tswap\n"); break;
    case IL_PICK:   P("\tpick\t%ld\n",geti()); break;
    case IL_ROT:    P("\trot\n"); break;
    case IL_NOT:    P("\tnot\n"); break;
    case IL_NEG:    P("\tneg\n"); break;
    case IL_ADD:    P("\tadd\n"); break;
    case IL_SUB:    P("\tsub\n"); break;
    case IL_MUL:    P("\tmul\n"); break;
    case IL_DIV:    P("\tdiv\n"); break;
    case IL_EQ:     P("\teq\n"); break;
    case IL_UNEQ:   P("\tuneq\n"); break;
    case IL_HIGHER: P("\thigher\n"); break;
    case IL_LOWER:  P("\tlower\n"); break;
    case IL_HIGHEQ: P("\thigheq\n"); break;
    case IL_LOWEQ:  P("\tloweq\n"); break;
    case IL_AND:    P("\tand\n"); break;
    case IL_OR:     P("\tor\n"); break;
    default:        P("\t-- unknown opcode %d!\n",opc);
  };
};

void dsm(iinfo *i) {
  while(i) {
    P("-- disassembly of `%s'\n\n",i->srcname);
    for(iref *r=i->r;r;r=r->next) P("-- uses %s()\n",r->funname);
    for(ifun *f=i->f;f;f=(ifun *)f->next) {
      P("\n%s[%d,%d,%c,%c] {\n",f->funname,f->nargs,f->envsize,f->flags&1?'D':'S',f->flags&2?'P':'N');
      setreadbuf(f->code);
      uchar *end=f->code+f->clen;
      while(!isend(end)) instr(i);
      P("\n}\n");
    };
    i=i->next;
  };
};

int main(int argc, char **argv) {
  if(argc!=2) error("missing arg\n");
  char *basename=argv[1];
  char ilname[100]; strcpy(ilname,basename); strcat(ilname,".il");
  iinfo *i=ilload(ilname);
  if(!i) { int o=getnumprocessed(); if(o) P("at offset %d\n",o); error("failed to read file %s\n",ilname); };
  dsm(i);
  return 0;
};
