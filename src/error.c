// error routines

#include <stdio.h>
#include <stdlib.h>
#include "object.h"
#include "error.h"
#include "lex.h"

int maxwarn = 25;
int maxerror = 10;
int lastline = -1;
int currentline = 0;

extern void cleanup() {
  exit(0);
};

extern void intern(char *msg, char* obj, void *e) { fatal(msg,obj,e,TRUE); };

extern void fatal(char *msg, char* obj, void *e, int is_intern) {
  char* s;
  if(e) ((exp *)e)->seterr();
  printf("fatal: %s: ",(s=lex_getname())?s:"Bla");
  if(is_intern) printf("internal error: ");
  if(currentline) printf("%d: ",currentline);
  printf(msg);
  if(obj) printf(" `%s'",obj);
  printf("\n");
  cleanup();
};

char errbuf[100];

void err(char *msg, char* obj) {
  printf("%s: ",lex_getname());
  if(currentline) printf("%d: ",currentline);
  printf(msg);
  if(obj) printf(" `%s'",obj);
  printf("\n");
  int pos=lex_precise(errbuf,100);
  if(pos>=0) {
    printf("%s\n",errbuf);
    int i;
    for(i=0;i<pos;i++) { printf(" "); };
    printf("^\n");
  };
};

int errorcount = 0;

extern void error_rec(char *msg, char* obj, void *e) {
  if(e) ((exp *)e)->seterr();
  if(errorcount++==maxerror) fatal("too many errors");
  if(currentline==lastline) return;		// show only one error per line
  lastline=currentline;
  printf("error: ");
  err(msg,obj);
  if(e) currentline=0;
};

extern void error(char *msg, char* obj, void *e) { error_rec(msg,obj,e); cleanup(); };

int warncount = 0;

extern void warn(char *msg, char* obj, void *e) {
  if(e) ((exp *)e)->seterr();
  printf("warning: ");
  err(msg,obj);
  if(warncount++==maxwarn) fatal("too many warnings");
  if(e) currentline=0;
};
