%{
/*
 * File:        parser.y
 * Description: yacc specification
 * Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
 */

#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include "global.h"
#include "parser_internal.h"
#include "pf.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "ql.h"

using namespace std;

#ifndef yyrestart
void yyrestart(FILE*);
#endif

/*
 * string representation of tokens; provided by scanner
 */
extern char *yytext;

/*
 * points to root of parse tree
 */
static NODE *parse_tree;

int bExit;                 // when to return from RBparse

PF_Manager *pPfm;          // PF component manager
SM_Manager *pSmm;          // SM component manager
QL_Manager *pQlm;          // QL component manager

%}

%union {
    int ival;
    CompOp cval;
    float rval;
    char *sval;
    NODE *n;
}

%token     
    RW_DATABASE
    RW_DATABASES
    RW_TABLE
    RW_TABLES
    RW_SHOW
    RW_CREATE
    RW_DROP
    RW_USE
    RW_PRIMARY
    RW_KEY
    RW_NOT
    RW_NULL
    RW_INSERT
    RW_INTO
    RW_VALUES
    RW_DELETE
    RW_FROM
    RW_WHERE
    RW_UPDATE
    RW_SET
    RW_SELECT
    RW_IS
    RW_INT
    RW_VARCHAR
    RW_DESC
    RW_INDEX
    RW_AND
    RW_DATE
    RW_FLOAT
    RW_FOREIGN
    RW_REFERENCES
    RW_EXIT
    RW_PRINT
    RW_LIKE
    RW_SUM
    RW_AVG
    RW_MAX
    RW_MIN
    RW_GROUP
    RW_BY
    T_EQ
    T_LT
    T_LE
    T_GT
    T_GE
    T_NE
    T_EOF
    NOTOKEN

%token <ival>   T_INT

%token <rval>   T_REAL

%token <sval>   T_STRING
                T_QSTRING

%type <cval>    op

%type <n>   program
            command
            createdatabase
            dropdatabase
            showdatabases
            usedatabase
            showtables
            createtable
            droptable
            desctable
            createindex
            dropindex
            print
            exit
            select_func
            select_group
            select
            insert
            delete 
            update
            setter_list
            setter
            field_list
            field
            attrtype
            func
            attr_list
            attr
            select_clause
            relattr_list
            relattr
            relation_list
            relation
            opt_where_clause
            condition_list
            condition
            relattr_or_value
            value_lists
            value_list
            value
%%

start
    : program
    {
        parse_tree = $1;
        YYACCEPT;
    }
    | nothing
    {
        parse_tree = NULL;
        YYACCEPT;
    }
    | error
    {
        reset_scanner();
        parse_tree = NULL;
        YYACCEPT;
    }
    | T_EOF
    {
        parse_tree = NULL;
        bExit = 1;
        YYACCEPT;
    }
    ;

program
    : command ';'
    {
        $$ = list_node($1);
    }
    ;

command
    : createdatabase
    | dropdatabase
    | showdatabases
    | usedatabase
    | showtables
    | createtable
    | droptable
    | desctable
    | createindex
    | dropindex
    | print
    | select
    | insert
    | delete
    | update
    | exit
    ;

createdatabase
    : RW_CREATE RW_DATABASE T_STRING
    {
        $$ = create_database_node($3);
    }
    ;

dropdatabase
    : RW_DROP RW_DATABASE T_STRING
    {
        $$ = drop_database_node($3);
    }
    ;

showdatabases
    : RW_SHOW RW_DATABASES
    {
        $$ = show_databases_node();
    }
    ;

usedatabase
    : RW_USE T_STRING
    {
        $$ = use_database_node($2);
    }
    ;

showtables
    : RW_SHOW RW_TABLES
    {
        $$ = show_tables_node();
    }
    ;

createtable
    : RW_CREATE RW_TABLE T_STRING '(' field_list ')'
    {
        $$ = create_table_node($3, $5);
    }
    ;

droptable
    : RW_DROP RW_TABLE T_STRING
    {
        $$ = drop_table_node($3);
    }
    ;

desctable
    : RW_DESC T_STRING
    {
        $$ = desc_table_node($2);
    }
    ;

createindex
    : RW_CREATE RW_INDEX T_STRING '(' T_STRING ')'
    {
        $$ = create_index_node($3, $5);
    }
    ;

dropindex
    : RW_DROP RW_INDEX T_STRING '(' T_STRING ')'
    {
        $$ = drop_index_node($3, $5);
    }
    ;

print
    : RW_PRINT T_STRING
    {
        $$ = print_node($2);
    }
    ;

exit
    : RW_EXIT
    {
        $$ = NULL;
        bExit = 1;
    }
    ;

select_func
    : RW_SELECT func RW_FROM relation_list opt_where_clause
    {
        $$ = select_func_node($2, $4, $5);
    }
    ;

select_group
    : RW_SELECT relattr ',' func RW_FROM relation_list opt_where_clause RW_GROUP RW_BY relattr
    {
        $$ = select_group_node($2, $4, $6, $7, $10);
    }
    ;

select
    : RW_SELECT select_clause RW_FROM relation_list opt_where_clause
    {
        $$ = select_node($2, $4, $5);
    }
    ;

insert
    : RW_INSERT RW_INTO T_STRING RW_VALUES value_lists
    {
        $$ = insert_node($3, $5);
    }
    ;

delete
    : RW_DELETE RW_FROM T_STRING opt_where_clause
    {
        $$ = delete_node($3, $4);
    }
    ;

update
    : RW_UPDATE T_STRING RW_SET setter_list opt_where_clause
    {
        $$ = update_node($2, $4, $5);
    }
    ;

setter_list
    : setter ',' setter_list
    {
        $$ = prepend($1, $3);
    }
    | setter
    {
        $$ = list_node($1);
    }
    ;

setter
    : T_STRING T_EQ value
    {
        $$ = setter_node($1, $3);
    }
    ;

field_list
    : field ',' field_list
    {
        $$ = prepend($1, $3);
    }
    | field
    {
        $$ = list_node($1);
    }
    ;

field
    : attrtype
    {
        $$ = field_node($1, false, NULL, NULL, NULL, NULL);
    }
    | attrtype RW_NOT RW_NULL
    {
        $$ = field_node($1, true, NULL, NULL, NULL, NULL);
    }
    | RW_PRIMARY RW_KEY '(' attr_list ')'
    {
        $$ = field_node(NULL, false, $4, NULL, NULL, NULL);
    }
    | RW_FOREIGN RW_KEY '(' T_STRING ')' RW_REFERENCES T_STRING '(' T_STRING ')'
    {
        $$ = field_node(NULL, false, NULL, $4, $7, $9);
    }
    ;

attrtype
    : T_STRING RW_INT '(' T_INT ')'
    {
        $$ = attrtype_node($1, INT, $4);
    }
    | T_STRING RW_VARCHAR '(' T_INT ')'
    {
        $$ = attrtype_node($1, STRING, $4);
    }
    | T_STRING RW_DATE
    {
        $$ = attrtype_node($1, DATE, 0);
    }
    | T_STRING RW_FLOAT
    {
        $$ = attrtype_node($1, FLOAT, 0);
    }
    ;

func
    : RW_SUM '(' relattr ')'
    {
        $$ = func_node(SUM, $3);
    }
    | RW_AVG '(' relattr ')'
    {
        $$ = func_node(AVG, $3);
    }
    | RW_MAX '(' relattr ')'
    {
        $$ = func_node(MAX, $3);
    }
    | RW_MIN '(' relattr ')'
    {
        $$ = func_node(MIN, $3);
    }
    ;

attr_list
    : attr ',' attr_list
    {
        $$ = prepend($1, $3);
    }
    | attr
    {
        $$ = list_node($1);
    }
    ;

attr
    : T_STRING
    {
        $$ = attr_node($1);
    }
    ;

select_clause
    : relattr_list
    | '*'
    {
        $$ = list_node(relattr_node(NULL, (char*)"*"));
    }
    ; 

relattr_list
    : relattr ',' relattr_list
    {
        $$ = prepend($1, $3);
    }
    | relattr
    {
        $$ = list_node($1);
    }
    ;

relattr
    : T_STRING '.' T_STRING
    {
        $$ = relattr_node($1, $3);
    }
    | T_STRING
    {
        $$ = relattr_node(NULL, $1);
    }
    ;

relation_list
    : relation ',' relation_list
    {
        $$ = prepend($1, $3);
    }
    | relation
    {
        $$ = list_node($1);
    }
    ;

relation
    : T_STRING
    {
        $$ = relation_node($1);
    }
    ;

opt_where_clause
    : RW_WHERE condition_list
    {
        $$ = $2;
    }
    | nothing
    {
        $$ = NULL;
    }
    ;

condition_list
    : condition RW_AND condition_list
    {
        $$ = prepend($1, $3);
    }
    | condition
    {
        $$ = list_node($1);
    }
    ;

condition
    : relattr op relattr_or_value
    {
        $$ = condition_node($1, $2, $3);
    }
    | relattr RW_IS RW_NULL
    {
        $$ = condition_node($1, EQ_OP, relattr_or_value_node(NULL, NULL));
    }
    | relattr RW_IS RW_NOT RW_NULL
    {
        $$ = condition_node($1, NE_OP, relattr_or_value_node(NULL, NULL));
    }
    | relattr RW_LIKE relattr_or_value
    {
        $$ = condition_node($1, LIKE_OP, $3);
    }
    ;

relattr_or_value
    : relattr
    {
        $$ = relattr_or_value_node($1, NULL);
    }
    | value
    {
        $$ = relattr_or_value_node(NULL, $1);
    }
    ;

value_lists
    : '(' value_list ')' ',' value_lists
    {
        $$ = prepend($2, $5);
    }
    | '(' value_list ')'
    {
        $$ = list_node($2);
    }
    ;

value_list
    : value ',' value_list
    {
        $$ = prepend($1, $3);
    }
    | value
    {
        $$ = list_node($1);
    }
    ;

value
    : T_QSTRING
    {
        $$ = value_node(STRING, (void *) $1);
    }
    | T_INT
    {
        $$ = value_node(INT, (void *)& $1);
    }
    | T_REAL
    {
        $$ = value_node(FLOAT, (void *)& $1);
    }
    | RW_NULL
    {
        $$ = value_node(STRING, NULL);
    }
    ;

op
    : T_LT
    {
        $$ = LT_OP;
    }
    | T_LE
    {
        $$ = LE_OP;
    }
    | T_GT
    {
        $$ = GT_OP;
    }
    | T_GE
    {
        $$ = GE_OP;
    }
    | T_EQ
    {
        $$ = EQ_OP;
    }
    | T_NE
    {
        $$ = NE_OP;
    }
    ;

nothing
    : /* epsilon */
    ;

%%

//
// PrintError
//
// Desc: Print an error message by calling the proper component-specific
//       print-error function
//
void PrintError(RC rc) {
    if (abs(rc) <= END_PF_WARN)
        PF_PrintError(rc);
    else if (abs(rc) <= END_RM_WARN)
        RM_PrintError(rc);
    else if (abs(rc) <= END_IX_WARN)
        IX_PrintError(rc);
    else if (abs(rc) <= END_SM_WARN)
        SM_PrintError(rc);
    else if (abs(rc) <= END_QL_WARN)
        QL_PrintError(rc);
    else
        cerr << "Error code out of range: " << rc << "\n";
}

//
// RippleDBparse
//
// Desc: Parse RippleDB commands
//
void RippleDBparse(PF_Manager &pfm, SM_Manager &smm, QL_Manager &qlm) {
    RC rc;

    // Set up global variables to their defaults
    pPfm  = &pfm;
    pSmm  = &smm;
    pQlm  = &qlm;
    bExit = 0;

    /* Do forever */
    while (!bExit) {
        /* Reset parser and scanner for a new query */
        new_query();

        /* Print a prompt */
        cout << PROMPT;

        /* Get the prompt to actually show up on the screen */
        cout.flush(); 

        /* If a query was successfully read, interpret it */
        if (yyparse() == 0 && parse_tree != NULL) {
            NODE *n = parse_tree;
            for (; n != NULL; n = n->u.LIST.next) {
                if (n->u.LIST.curr != NULL && (rc = interp(n->u.LIST.curr))) {
                    PrintError(rc);
                    if (rc < 0)
                        bExit = TRUE;
                }
            }
        }
    }
}

//
// Functions for printing the various structures to an output stream
//
ostream &operator<<(ostream &s, const AttrInfo &ai) {
    return s << " attrName=" << ai.attrName << " attrType="
             << (ai.attrType == INT ? "INT" :
                 ai.attrType == FLOAT ? "FLOAT" : "STRING")
             << " attrLength=" << ai.attrLength;
}

ostream &operator<<(ostream &s, const RelAttr &qa) {
    return s << (qa.relName ? qa.relName : "NULL") << "." << qa.attrName;
}

ostream &operator<<(ostream &s, const Condition &c) {
    s << "\n      lhsAttr:" << c.lhsAttr << "\n" << "      op=" << c.op << "\n";
    if (c.bRhsIsAttr)
        s << "      bRhsIsAttr=TRUE \n      rhsAttr:" << c.rhsAttr;
    else
        s << "      bRshIsAttr=FALSE\n      rhsValue:" << c.rhsValue;
    return s;
}

ostream &operator<<(ostream &s, const Value &v) {
    s << "AttrType: " << v.type;
    switch (v.type) {
        case INT:
            s << " *(int *)data=" << *(int *)v.data;
            break;
        case FLOAT:
            s << " *(float *)data=" << *(float *)v.data;
            break;
        case STRING:
            s << " (char *)data=" << (char *)v.data;
            break;
    }
    return s;
}

ostream &operator<<(ostream &s, const CompOp &op) {
    switch (op) {
        case EQ_OP:
            s << " =";
            break;
        case NE_OP:
            s << " <>";
            break;
        case LT_OP:
            s << " <";
            break;
        case LE_OP:
            s << " <=";
            break;
        case GT_OP:
            s << " >";
            break;
        case GE_OP:
            s << " >=";
            break;
        case NO_OP:
            s << " NO_OP";
            break;
    }
    return s;
}

ostream &operator<<(ostream &s, const AttrType &at) {
    switch (at) {
        case INT:
            s << "INT";
            break;
        case FLOAT:
            s << "FLOAT";
            break;
        case STRING:
            s << "STRING";
            break;
    }
    return s;
}

/*
 * Required by yacc
 */
void yyerror(char const *s) {
    puts(s);
}

#if 0
/*
 * Sometimes required
 */
int yywrap(void) {
    return 1;
}
#endif
