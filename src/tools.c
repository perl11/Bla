// tools.c

#include "bla.h"
#include <stdio.h>
#include <new.h>
#include <stdlib.h>

#define FAST_SIZE 10000				/* alloc each time */
char* fast_cur=0;
int fast_left=0;

void memprob() { printf("panic: no memory!"); exit(20); };

void *operator new(size_t size) {
  int realsize=(size+MEM_ALIGN)&(~MEM_ALIGN);			/* align memory*/
  char* r;
  if(realsize>FAST_SIZE) {					/* toolarge: alloc own */
    if(!(r=(char*)malloc(size))) memprob();
    return r;
  } else {
    if(realsize>fast_left) fast_cur=NIL;			/* end of buf */
    if(fast_cur==0) {						/* alloc new buf */
      if(!(fast_cur=(char*)malloc(FAST_SIZE))) memprob();
      fast_left=FAST_SIZE;
    };
    fast_left-=realsize;
    r=fast_cur;
    fast_cur+=realsize;
    return r;							/* this mem can't be free'd explicitly! */
  };
};

#ifdef __GNUC__
void operator delete(void *p) {};
#else
void operator delete(void *p,size_t size) {};			// gna.
#endif
