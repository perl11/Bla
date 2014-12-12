// the main bla program

#include "bla.h"
#include "lex.h"
#include "parse.h" // also object.h
#include "error.h"
#include <stdio.h>
#include <string.h>

int numfunids=0;
scope *topscope=NIL;
int debugoutput=0;		// 0=none, 1=smallmessages, 2=normal, 3=heavy

int main(int argc, char **argv) {
  printf("Bla Compiler v0.1, (c) 95/96 Wouter\n");
  char *basename=NIL;
  for(int an=1;an<argc;an++) {
    char *as=argv[an];
    if(*as=='-') {
      as++;
      switch(*as++) {
        case 'd': debugoutput=1; if(*as>='0' && *as<='9') debugoutput=*as++-'0'; break;
        case 'h': printf("usage: bla [options] sourcefile\n"
                         "\t-d[n]\tshow debug information (at verbosity level `n')\n"
                         "\t-h\tthis help\n"); cleanup(); break;
        default: fatal("unknown option",argv[an]);
      };
      if(*as) fatal("error in option",argv[an]);
    } else {
      basename=as;
    };
  };
  if(!basename) fatal("no input file specified (try `bla -h' for help)");
  char fname[100]; strcpy(fname,basename); strcat(fname,".bla");
  char ilname[100]; strcpy(ilname,basename); strcat(ilname,".il");
  lex_readsource(fname);
  parse_toplevel();		// ends up in topscope
  lex_end();
  if(debugoutput>=3) dump();
  type_check();
  if(debugoutput>=2) dump();
  generate_code(ilname,fname);
  cleanup();
  return 0;
};
