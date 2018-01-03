%{
/*
 * File:        scan.l
 * Description: lex specification
 * Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
 */

#include <string.h>
#include "global.h"
#include "parser_internal.h"
#include "y.tab.h"

/* defined in scanhelp.c */
static int get_id(char *s);   
static char *get_qstring(char *qstring, int len);

%}
letter                        [A-Za-z]
digit                         [0-9]
num                           {digit}+
s_num                         [+\-]?{num}
%x comment
%%
"/*"                          { BEGIN(comment); }
<comment>[^*]                 { /* ignore the text of the comment */ }
<comment>"*/"                 { BEGIN(INITIAL); }
<comment>\*                   { /* ignore *'s that aren't part of */ }
[ \n\t]                       { /* ignore spaces, tabs, and newlines */ }
{s_num}                       { sscanf(yytext, "%d", &yylval.ival); return T_INT; }
{s_num}\.{num}                { sscanf(yytext, "%f", &yylval.rval); return T_REAL; }
{s_num}\.{num}[Ee]{s_num}     { sscanf(yytext, "%f", &yylval.rval); return T_REAL; }
\'([^\\\n]|(\\[^\n]))*\'      { yylval.sval = get_qstring(yytext, yyleng); return T_QSTRING; }
\'([^\\\n]|(\\[^\n]))*\n      { printf("newline in string constant\n"); }
{letter}({letter}|{digit}|_)* { return get_id(yylval.sval = yytext); }
"<"                           { return T_LT; }
"<="                          { return T_LE; }
">"                           { return T_GT; }
">="                          { return T_GE; }
"="                           { return T_EQ; }
"<>"                          { return T_NE; }
[*;,.()]                      { return yytext[0]; }
<<EOF>>                       { return T_EOF; }
.                             { printf("illegal character [%c]\n", yytext[0]); }
%%
#include "scanhelp.c"
