/*
 * File:        parser_internal.h
 * Description: internal declarations for parser
 * Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
 */

#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "parser.h"

/*
 * use double for real
 */
typedef float real;

/*
 * the prompt
 */
#define PROMPT	"\nRIPPLEDB >> "

/*
 * REL_ATTR: describes a qualified attribute (relName.attrName)
 */
typedef struct {
    char *relName; /* relation name */
    char *attrName; /* attribute name */
} REL_ATTR;

/*
 * ATTR_VAL: <attribute, value> pair
 */
typedef struct {
    char *attrName; /* attribute name */
    AttrType valType; /* type of value */
    int valLength; /* length if type = STRING */
    void *value; /* value for attribute */
} ATTR_VAL;

/*
 * all the available kinds of nodes
 */
typedef enum {
    N_CREATEDATABASE,
    N_DROPDATABASE,
    N_SHOWDATABASES,
    N_USEDATABASE,
    N_SHOWTABLES,
    N_CREATETABLE,
    N_DROPTABLE,
    N_DESCTABLE,
    N_CREATEINDEX,
    N_DROPINDEX,
    N_PRINT,
    N_SELECT,
    N_INSERT,
    N_DELETE,
    N_UPDATE,
    N_RELATTR,
    N_CONDITION,
    N_RELATTR_OR_VALUE,
    N_VALUE,
    N_FIELD,
    N_SETTER,
    N_ATTRTYPE,
    N_ATTR,
    N_RELATION,
    N_LIST
} NODEKIND;

/*
 * structure of parse tree nodes
 */
typedef struct node{
    NODEKIND kind;
    union {
        /* SM component nodes */
        /* create database node */
        struct {
            char *dbname;
        } CREATEDATABASE;
        /* drop database node */
        struct {
            char *dbname;
        } DROPDATABASE;
        /* show databases node */
        struct {
        } SHOWDATABASES;
        /* use database node */
        struct {
            char *dbname;
        } USEDATABASE;
        /* show tables node */
        struct {
        } SHOWTABLES;
        /* create table node */
        struct {
            char *relname;
            struct node *fieldlist;
        } CREATETABLE;
        /* drop table node */
        struct {
            char *relname;
        } DROPTABLE;
        /* desc table node */
        struct {
            char *relname;
        } DESCTABLE;
        /* create index node */
        struct {
            char *relname;
            char *attrname;
        } CREATEINDEX;
        /* drop index node */
        struct {
            char *relname;
            char *attrname;
        } DROPINDEX;
        /* print node */
        struct {
            char *relname;
        } PRINT;
        /* QL component nodes */
        /* select node */
        struct {
            struct node *relattrlist;
            struct node *rellist;
            struct node *conditionlist;
        } SELECT;
        /* insert node */
        struct {
            char *relname;
            struct node *valuelists;
        } INSERT;
        /* delete node */
        struct {
            char *relname;
            struct node *conditionlist;
        } DELETE;
        /* update node */
        struct {
            char *relname;
            struct node *setterlist;
            struct node *conditionlist;
        } UPDATE;
        /* command support nodes */
        /* relation attribute node */
        struct {
            char *relname;
            char *attrname;
        } RELATTR;
        /* condition node */
        struct {
            struct node *lhsRelattr;
            CompOp op;
            struct node *rhsRelattr;
            struct node *rhsValue;
        } CONDITION;
        /* relation-attribute or value */
        struct {
            struct node *relattr;
            struct node *value;
        } RELATTR_OR_VALUE;
        /* <value, type> pair */
        struct {
            AttrType type;
            int ival;
            real rval;
            char *sval;
        } VALUE;
        /* field node */
        struct {
            struct node *attrType;
            bool isNotNull;
            struct node *primaryKeyList;
            char *foreignKey;
            char *refRel;
            char *refAttr;
        } FIELD;
        /* setter node */
        struct {
            char *attrname;
            struct node *value;
        } SETTER;
        /* <attribute, type> pair */
        struct {
            char *attrname;
            AttrType type;
            int length;
        } ATTRTYPE;
        /* attribute node */
        struct {
            char *attrname;
        } ATTR;
        /* relation node */
        struct {
            char *relname;
        } RELATION;
        /* list node */
        struct {
            struct node *curr;
            struct node *next;
        } LIST;
    } u;
} NODE;

/*
 * function prototypes
 */
NODE *newnode(NODEKIND kind);
NODE *create_database_node(char *dbname);
NODE *drop_database_node(char *dbname);
NODE *show_databases_node();
NODE *use_database_node(char *dbname);
NODE *show_tables_node();
NODE *create_table_node(char *relname, NODE *fieldlist);
NODE *drop_table_node(char *relname);
NODE *desc_table_node(char *relname);
NODE *create_index_node(char *relname, char *attrname);
NODE *drop_index_node(char *relname, char *attrname);
NODE *print_node(char *relname);
NODE *select_node(NODE *relattrlist, NODE *rellist, NODE *conditionlist);
NODE *insert_node(char *relname, NODE *valuelists);
NODE *delete_node(char *relname, NODE *conditionlist);
NODE *update_node(char *relname, NODE *setterlist, NODE *conditionlist);
NODE *relattr_node(char *relname, char *attrname);
NODE *condition_node(NODE *lhsRelattr, CompOp op, NODE *rhsRelattrOrValue);
NODE *relattr_or_value_node(NODE *relattr, NODE *value);
NODE *value_node(AttrType type, void *value);
NODE *field_node(NODE *attrType, bool isNotNull, NODE *primaryKeyList, char *foreignKey, char *refRel, char *refAttr);
NODE *setter_node(char *attrname, NODE *value);
NODE *attrtype_node(char *attrname, AttrType type, int length);
NODE *attr_node(char *attrname);
NODE *relation_node(char *relname);
NODE *list_node(NODE *n);
NODE *prepend(NODE *n, NODE *list);

void reset_scanner(void);
void reset_charptr(void);
void new_query(void);
RC   interp(NODE *n);
int  yylex(void);
int  yyparse(void);

#endif
