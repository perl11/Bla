// lex

extern void lex_readsource(char *name);
extern void lex_end();
extern int lex_precise(char *buf, int len);
extern int lex();
extern char *lex_getname();
extern char *lex_getinfoasc();
extern int lex_getinfoint();
extern void lex_startlayout();
extern void lex_droplevel();

enum lextokens {	// ascii version in parse.c/match()
  LEX_ASSIGN=256,
  LEX_DOTDOT,
  LEX_ARROW,
  LEX_HIGHEQ,
  LEX_LOWEQ,
  LEX_UNEQ,
  LEX_UNIF,
  LEX_EOF,
  LEX_IDENT,
  LEX_INTEGER,
  LEX_REAL,
  LEX_STRING,
  LEX_PLUSPLUS,
  LEX_MINMIN
};

enum keyconsts {
  KEY_AAA=300,
  KEY_AND, KEY_ANY,
  KEY_BOOL,
  KEY_CLASS, KEY_CLOSE, KEY_CONST,
  KEY_DO,
  KEY_EXCEPT, KEY_EXIT, KEY_EXTENDS, KEY_EXTERN,
  KEY_FALSE,
  KEY_IMPLEMENTS, KEY_INT,
  KEY_LAMBDA, KEY_LOOP,
  KEY_MODULE,
  KEY_NEXT, KEY_NIL, KEY_NOT,
  KEY_OBJECT, KEY_OR,
  KEY_PARENT, KEY_PRIVATE, KEY_PUBLIC,
  KEY_RAISE, KEY_REAL, KEY_RETURN,
  KEY_SELF, KEY_STRING,
  KEY_TRUE,
  KEY_WHERE, KEY_WHILE
};

const int MAX_LEX_SYM=400;


