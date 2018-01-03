/*
 * File:        nodes.c
 * Description: functions to allocate an initialize parse-tree nodes
 * Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "global.h"
#include "parser_internal.h"
#include "y.tab.h"

/*
 * total number of nodes available for a given parse-tree
 */
#define MAXNODE	5000000

static NODE nodepool[MAXNODE];
static int nodeptr = 0;

/*
 * reset_parser: resets the scanner and parser when a syntax error occurs.
 *
 * No return value
 */
void reset_parser(void) {
    reset_scanner();
    nodeptr = 0;
}

static void (*cleanup_func)() = NULL;

/*
 * new_query: prepares for a new query by releasing all resources used by previous queries.
 *
 * No return value.
 */
void new_query(void) {
    nodeptr = 0;
    reset_charptr();
    if (cleanup_func != NULL) (*cleanup_func)();
}

void register_cleanup_function(void (*func)()) {
    cleanup_func = func;
}

/*
 * newnode: allocates a new node of the specified kind and returns a pointer
 * to it on success. Returns NULL on error.
 */
NODE *newnode(NODEKIND kind) {
    NODE *n;
    /* if we've used up all of the nodes then error */
    if (nodeptr == MAXNODE) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    /* get the next node */
    n = nodepool + nodeptr;
    ++nodeptr;
    /* initialize the `kind' field */
    n -> kind = kind;
    return n;
}

/*
 * xx_node: allocates, initializes, and returns a pointer to a new
 * xx node having the indicated values.
 */
NODE *create_database_node(char *dbname) {
    NODE *n = newnode(N_CREATEDATABASE);
    n -> u.CREATEDATABASE.dbname = dbname;
    return n;
}

NODE *drop_database_node(char *dbname) {
    NODE *n = newnode(N_DROPDATABASE);
    n -> u.DROPDATABASE.dbname = dbname;
    return n;
}

NODE *show_databases_node() {
    NODE *n = newnode(N_SHOWDATABASES);
    return n;
}

NODE *use_database_node(char *dbname) {
    NODE *n = newnode(N_USEDATABASE);
    n -> u.USEDATABASE.dbname = dbname;
    return n;
}

NODE *show_tables_node() {
    NODE *n = newnode(N_SHOWTABLES);
    return n;
}

NODE *create_table_node(char *relname, NODE *fieldlist) {
    NODE *n = newnode(N_CREATETABLE);
    n -> u.CREATETABLE.relname = relname;
    n -> u.CREATETABLE.fieldlist = fieldlist;
    return n;
}

NODE *drop_table_node(char *relname) {
    NODE *n = newnode(N_DROPTABLE);
    n -> u.DROPTABLE.relname = relname;
    return n;
}

NODE *desc_table_node(char *relname) {
    NODE *n = newnode(N_DESCTABLE);
    n -> u.DESCTABLE.relname = relname;
    return n;
}

NODE *create_index_node(char *relname, char *attrname) {
    NODE *n = newnode(N_CREATEINDEX);
    n -> u.CREATEINDEX.relname = relname;
    n -> u.CREATEINDEX.attrname = attrname;
    return n;
}

NODE *drop_index_node(char *relname, char *attrname) {
    NODE *n = newnode(N_DROPINDEX);
    n -> u.DROPINDEX.relname = relname;
    n -> u.DROPINDEX.attrname = attrname;
    return n;
}

NODE *print_node(char *relname) {
    NODE *n = newnode(N_PRINT);
    n -> u.PRINT.relname = relname;
    return n;
}

NODE *select_func_node(NODE *func, NODE *rellist, NODE *conditionlist) {
    NODE *n = newnode(N_SELECT_FUNC);
    n->u.SELECT_FUNC.func = func;
    n->u.SELECT_FUNC.rellist = rellist;
    n->u.SELECT_FUNC.conditionlist = conditionlist;
    return n;
}

NODE *select_group_node(NODE *relattr1, NODE *func, NODE *rellist, NODE *conditionlist, NODE *relattr2) {
    NODE *n = newnode(N_SELECT_GROUP);
    n->u.SELECT_GROUP.relattr1 = relattr1;
    n->u.SELECT_GROUP.func = func;
    n->u.SELECT_GROUP.rellist = rellist;
    n->u.SELECT_GROUP.conditionlist = conditionlist;
    n->u.SELECT_GROUP.relattr2 = relattr2;
    return n;
}

NODE *select_node(NODE *relattrlist, NODE *rellist, NODE *conditionlist) {
    NODE *n = newnode(N_SELECT);
    n->u.SELECT.relattrlist = relattrlist;
    n->u.SELECT.rellist = rellist;
    n->u.SELECT.conditionlist = conditionlist;
    return n;
}

NODE *insert_node(char *relname, NODE *valuelists) {
    NODE *n = newnode(N_INSERT);
    n->u.INSERT.relname = relname;
    n->u.INSERT.valuelists = valuelists;
    return n;
}

NODE *delete_node(char *relname, NODE *conditionlist) {
    NODE *n = newnode(N_DELETE);
    n->u.DELETE.relname = relname;
    n->u.DELETE.conditionlist = conditionlist;
    return n;
}

NODE *update_node(char *relname, NODE *setterlist, NODE *conditionlist) {
    NODE *n = newnode(N_UPDATE);
    n->u.UPDATE.relname = relname;
    n->u.UPDATE.setterlist = setterlist;
    n->u.UPDATE.conditionlist = conditionlist;
    return n;
}

NODE *func_node(FuncType func, NODE *relattr) {
    NODE *n = newnode(N_FUNC);
    n -> u.FUNC.func = func;
    n -> u.FUNC.relattr = relattr;
    return n;
}

NODE *relattr_node(char *relname, char *attrname) {
    NODE *n = newnode(N_RELATTR);
    n -> u.RELATTR.relname = relname;
    n -> u.RELATTR.attrname = attrname;
    return n;
}

NODE *condition_node(NODE *lhsRelattr, CompOp op, NODE *rhsRelattrOrValue) {
    NODE *n = newnode(N_CONDITION);
    n->u.CONDITION.lhsRelattr = lhsRelattr;
    n->u.CONDITION.op = op;
    n->u.CONDITION.rhsRelattr = rhsRelattrOrValue->u.RELATTR_OR_VALUE.relattr;
    n->u.CONDITION.rhsValue = rhsRelattrOrValue->u.RELATTR_OR_VALUE.value;
    return n;
}

NODE *relattr_or_value_node(NODE *relattr, NODE *value) {
    NODE *n = newnode(N_RELATTR_OR_VALUE);
    n->u.RELATTR_OR_VALUE.relattr = relattr;
    n->u.RELATTR_OR_VALUE.value = value;
    return n;
}

NODE *value_node(AttrType type, void *value) {
    NODE *n = newnode(N_VALUE);
    n->u.VALUE.type = type;
    switch (type) {
        case INT:
            n->u.VALUE.ival = *(int *)value;
            break;
        case FLOAT:
            n->u.VALUE.rval = *(float *)value;
            break;
        case STRING:
            n->u.VALUE.sval = (char *)value;
            break;
    }
    return n;
}

NODE *field_node(NODE *attrType, bool isNotNull, NODE *primaryKeyList, char *foreignKey, char *refRel, char *refAttr) {
    NODE *n = newnode(N_FIELD);
    n -> u.FIELD.attrType = attrType;
    n -> u.FIELD.isNotNull = isNotNull;
    n -> u.FIELD.primaryKeyList = primaryKeyList;
    n -> u.FIELD.foreignKey = foreignKey;
    n -> u.FIELD.refRel = refRel;
    n -> u.FIELD.refAttr = refAttr;
    return n;
}

NODE *setter_node(char *attrname, NODE *value) {
    NODE *n = newnode(N_SETTER);
    n -> u.SETTER.attrname = attrname;
    n -> u.SETTER.value = value;
    return n;
}

NODE *attrtype_node(char *attrname, AttrType type, int length) {
    NODE *n = newnode(N_ATTRTYPE);
    n -> u.ATTRTYPE.attrname = attrname;
    n -> u.ATTRTYPE.type = type;
    n -> u.ATTRTYPE.length = length;
    return n;
}

NODE *attr_node(char *attrname) {
    NODE *n = newnode(N_ATTR);
    n->u.ATTR.attrname = attrname;
    return n;
}

NODE *relation_node(char *relname) {
    NODE *n = newnode(N_RELATION);
    n->u.RELATION.relname = relname;
    return n;
}

NODE *list_node(NODE *n) {
    NODE *list = newnode(N_LIST);
    list -> u.LIST.curr = n;
    list -> u.LIST.next = NULL;
    return list;
}

/*
 * prepends node n onto the front of list.
 *
 * Returns the resulting list.
 */
NODE *prepend(NODE *n, NODE *list) {
    NODE *newlist = newnode(N_LIST);
    newlist -> u.LIST.curr = n;
    newlist -> u.LIST.next = list;
    return newlist;
}
