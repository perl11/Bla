// exp parser

extern exp *parse_exp(char take_bar=TRUE);
extern pat *parse_pattern(param *par=NIL);
extern pat *parse_patternid(param *par, char *n);
