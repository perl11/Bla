// The Bla Lexical Analyzer

#include "bla.h"
#include "lex.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *keywords[] = {
  "and","any",
  "bool",
  "class","close","const",
  "do",
  "except","exit","extends","extern",
  "false",
  "implements","int",
  "lambda","loop",
  "module",
  "next","nil","not",
  "object","or",
  "parent","private","public",
  "raise","real","return",
  "self","string",
  "true",
  "where","while",
  NIL
};

char *p=NIL, *endsrc=NIL, *start=NIL, *lastlf;
char *sname;
char tmpbuf[10];
char *info1=NIL;
int info2=0;

class lexstack;		// silly

lexstack *lexst = NIL;

class lexstack {
public:
  int doeslayout;
  int offs;
  lexstack *prev;
  lexstack(int o,int lay) {
    doeslayout = lay;
    offs = o;
    prev = lexst;
    lexst = this;
  };
  ~lexstack() {
    if(!prev) fatal("underflow?");
  };
  void drop() {
    lexst = prev;
    delete this;
  };
};

extern void lex_readsource(char *name) {
   FILE *f;
   char *mem;
   #define MAX_FILE 100000 // wrong!
   if((mem=(char *)malloc(MAX_FILE))==NIL) fatal("no memory for source");
   *mem++='\n'; *mem++='\n';
   if((f=fopen(name,"r"))==NIL) fatal("could not open file",name);
   int read=fread(mem,1,MAX_FILE-4,f);
   fclose(f);
   currentline=1;
   lastlf=start=p=mem;
   endsrc=p+read;
   *endsrc='\n'; endsrc[1]='\n';
   sname=name;
   new lexstack(0,FALSE);
};

#define inline
extern inline char *lex_getname() { return sname; };
extern inline char *lex_getinfoasc() { return info1; };
extern inline int lex_getinfoint() { return info2; };

extern void lex_end() { currentline=0; p=NIL; };

extern int lex_precise(char *buf, int len) {
  if(p && currentline) {
    char *s=p, *e=p;
    while((*(s-1))!='\n') s--;
    while(*e!='\n') e++;
    int l=p-s;
    int ll=e-s;
    if(ll>=len) ll=len-1;
    strncpy(buf,s,ll);
    buf[ll]=0;
    return l;
  };
  return -1;
};

char *eatnest(char *p) {
  char *orig=p-1;
  p++;
  int nest=1,c;
  for(;;) {
    c=*p++;
    if(c=='\n') { currentline++; lastlf=p; if(p>=endsrc) { p=orig; error("missing `*/' in comment"); }; };
    if(c=='*' && *p=='/') { p++; if(!--nest) break; };
    if(c=='/' && *p=='*') { p++; nest++; };
  };
  return p;
};

void simplewhite() {
  char c;
  lab:
  switch(c=*p++) {
    case '\n': currentline++; lastlf=p; if(p>=endsrc) { p=endsrc; return; }; goto lab;
    case '\t': case ' ': goto lab;
    case '/': if(*p=='*') { p=eatnest(p); goto lab; };
    case '-': if(*p=='-') {
      c=p[-2];
      if(c=='\n' || c=='\t' || c==' ') {
        while(*p!='\n') p++; goto lab;
      };
    };
  };
  p--;
};

extern void lex_startlayout() {
  simplewhite();
  if(*p=='{') { new lexstack(0,FALSE); p++; }
         else { new lexstack(p-lastlf,TRUE); };
};

extern void lex_droplevel() {};

int getstrchar(char end) {
  char c;
  switch(c=*p++) {
    case '\n':
      p--; error_rec("string constant not terminated"); return -1;
    case '\'': case '\"':
      if(c==end) { if(*p==c) { p++; } else { return -1; }; return c; }
    case '\\':
      switch(c=*p++) {
        case '0': return '\0';		// zero (0)
        case 'b': return '\b';		// backspace (8)
        case 'g': return 7;		// beep (7)
        case 't': return '\t';		// tab (9)
        case 'n': return '\n';		// linefeed (10)
        case 'r': return '\r';		// carriage return (13)
        case 'e': return 27;		// escape (27)
        case '\\': return '\\';		// backslash (47)
        default: error_rec("illegal escape character"); return c;
      };
    default:
      return c;
  };
};

extern int lex() {
  char c;
  lab:
  switch(c=*p++) {
    case '\n': {
      currentline++;
      lastlf=p;
      if(p>=endsrc) { p=endsrc; };	// pos will be -1
      if(lexst->doeslayout) {
        simplewhite();
        int pos=p-lastlf;
        if(pos==lexst->offs) return ';';
        if(pos<lexst->offs) {
          p=lastlf-1; currentline--;		// so step will be repeated
          lexst->drop();
          return '}';
        };
      };
      if(p>=endsrc) return LEX_EOF;
      goto lab;
    };
    case '\t':  case ' ':
      goto lab;
    case '`': case '(': case ')': case '|': case ';': case '[': case ']':
    case ',': case '{': case '}': case '=': case '_': case '*':
      return c;
    case '/': if(*p=='*') { p=eatnest(p); goto lab; }; return c;
    case '+': return (*p=='+') ? (p++,LEX_PLUSPLUS) : '+';
    case '.': return (*p=='.') ? (p++,LEX_DOTDOT) : '.';
    case ':': return (*p=='=') ? (p++,LEX_ASSIGN) : ':';
    case '>': return (*p=='=') ? (p++,LEX_HIGHEQ) : '>';
    case '-':
      switch(c=*p) {
        case '>': p++; return LEX_ARROW;
        case '-': p++; c=p[-3]; if(c=='\n' || c=='\t' || c==' ') { while(*p!='\n') p++; goto lab; };
                                return LEX_MINMIN;
        default: return '-';
      };
    case '<':
      switch(c=*p) {
        case '>': p++; return LEX_UNEQ;
        case '=': p++; return (*p=='>') ? (p++,LEX_UNIF) : LEX_LOWEQ;
        default: return '<';
      };
    case '\'': {
      char *st=p;
      while((c=getstrchar('\''))>0);
      info1=new char[p-st];
      p=st; st=info1;
      while((c=getstrchar('\''))>0) *st++=c;
      *st=0;
      return LEX_STRING;
    };
    case '\"': {
      int num=0,n=0;
      while((c=getstrchar('\"'))>0) {
        n=(n<<8)+c;
        if(num++==4) error_rec("too large integer constant");
      };
      info2=n;
      return LEX_INTEGER;
    };
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      {
        int n=c-'0';
        while(isdigit(c=*p)) { p++; n=n*10+c-'0'; };
        info2=n;
        return LEX_INTEGER; // or: LEX_REAL
      };
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
      {
        char *s=p-1;
        while(isalnum(*p) || (*p=='_')) p++;
        int len=p-s;
        char *t=new char[len+1];
        strncpy(t,s,len);
        t[len]=0;
        char **keys=keywords;		// faster in the future
        int knum=KEY_AAA;
        while(*keys) { knum++; if(strcmp(t,*keys++)==0) return knum; };
        info1=t;
        return LEX_IDENT;
      };
    default:
      sprintf(tmpbuf,"\"%c\"",c);
      error_rec("illegal character",tmpbuf);
      goto lab;
  };
};
