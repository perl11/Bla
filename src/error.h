// error protos

extern void intern(char *msg, char *obj=0, void *e=0);
extern void fatal(char *msg, char *obj=0, void *e=0, int is_intern=0);
extern void error(char *msg, char *obj=0, void *e=0);
extern void error_rec(char *msg, char *obj=0, void *e=0);
extern void warn(char *msg, char *obj=0, void *e=0);
extern void cleanup();

extern int currentline;
extern int errorcount;
extern int warncount;
