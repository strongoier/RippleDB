/*
 * File:        interp.c
 * Description: interpreter
 * Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "global.h"
#include "parser_internal.h"
#include "y.tab.h"

#include "sm.h"
#include "ql.h"

extern SM_Manager *pSmm;
extern QL_Manager *pQlm;

#define E_OK                 0
#define E_TOOMANY            -1
#define E_INVSTRLEN          -2
#define E_DUPLICATEATTR      -3
#define E_TOOLONG            -4
#define E_FOREIGNNOTEXIST    -5
#define E_DUPLICATEFOREIGN   -6
#define E_DUPLICATEPRIMARY   -7
#define E_TOOMANYPRIMARY     -8
#define E_PRIMARYNOTEXIST    -9
#define E_GROUPNOTMATCH      -10

/*
 * file pointer to which error messages are printed
 */
#define ERRFP stderr

/*
 * local functions
 */
static int mk_fields(NODE *list, int max, Field fields[]);
static int parse_format_string(char *format_string, AttrType *type, int *len);
static int mk_rel_attrs(NODE *list, int max, RelAttr relAttrs[]);
static int mk_relations(NODE *list, int max, char *relations[]);
static int mk_setters(NODE *list, int max, RelAttr relAttr[], Value rhsValue[]);
static int mk_conditions(NODE *list, int max, Condition conditions[]);
static int mk_values(NODE *list, int max, Value values[]);
static void mk_value(NODE *node, Value &value);
static void print_error(char *errmsg, RC errval);

/*
 * interp: interprets parse trees
 */
RC interp(NODE *n) {
    RC errval = 0; /* returned error value */

    switch (n -> kind) {

        case N_CREATEDATABASE: /* for CreateDb() */
            errval = pSmm->CreateDb(n->u.CREATEDATABASE.dbname);
            break;

        case N_DROPDATABASE: /* for DropDb() */
            errval = pSmm->DropDb(n->u.DROPDATABASE.dbname);
            break;

        case N_SHOWDATABASES: /* for ShowDbs() */
            errval = pSmm->ShowDbs();
            break;

        case N_USEDATABASE: /* for OpenDb() */
            errval = pSmm->OpenDb(n->u.USEDATABASE.dbname);
            break;

        case N_SHOWTABLES: /* for ShowTables() */
            errval = pSmm->ShowTables();
            break;

        case N_CREATETABLE: { /* for CreateTable() */
            int nfields;
            Field fields[MAXATTRS];

            /* Make sure relation name isn't too long */
            if (strlen(n -> u.CREATETABLE.relname) > MAXNAME) {
                print_error((char*)"create", E_TOOLONG);
                break;
            }

            /* Make a list of AttrInfos suitable for sending to Create */
            nfields = mk_fields(n -> u.CREATETABLE.fieldlist, MAXATTRS, fields);
            if (nfields < 0) {
                print_error((char*)"create", nfields);
                break;
            }

            /* Make the call to create */
            errval = pSmm->CreateTable(n->u.CREATETABLE.relname, nfields, fields);
            break;
        }

        case N_DROPTABLE: /* for DropTable() */
            errval = pSmm->DropTable(n->u.DROPTABLE.relname);
            break;

        case N_DESCTABLE: /* for DescTable() */
            errval = pSmm->DescTable(n->u.DESCTABLE.relname);
            break;

        case N_CREATEINDEX: /* for CreateIndex() */
            errval = pSmm->CreateIndex(n->u.CREATEINDEX.relname, n->u.CREATEINDEX.attrname);
            break;

        case N_DROPINDEX: /* for DropIndex() */
            errval = pSmm->DropIndex(n->u.DROPINDEX.relname, n->u.DROPINDEX.attrname);
            break;

        case N_PRINT: /* for Print() */
            errval = pSmm->Print(n->u.PRINT.relname);
            break;

        case N_SELECT_FUNC: { /* for SelectFunc() */

            RelAttr relAttrFunc;
            int nRelations = 0;
            char *relations[MAXATTRS];
            int nConditions = 0;
            Condition conditions[MAXATTRS];

            relAttrFunc.relName = n->u.SELECT_FUNC.func->u.FUNC.relattr->u.RELATTR.relname;
            relAttrFunc.attrName = n->u.SELECT_FUNC.func->u.FUNC.relattr->u.RELATTR.attrname;

            /* Make a list of relation names suitable for sending to Query */
            nRelations = mk_relations(n->u.SELECT_FUNC.rellist, MAXATTRS, relations);
            if (nRelations < 0) {
                print_error((char*)"select_func", nRelations);
                break;
            }

            /* Make a list of Conditions suitable for sending to Query */
            nConditions = mk_conditions(n->u.SELECT_FUNC.conditionlist, MAXATTRS, conditions);
            if (nConditions < 0) {
                print_error((char*)"select_func", nConditions);
                break;
            }

            /* Make the call to Select */
            errval = pQlm->SelectFunc(n->u.SELECT_FUNC.func->u.FUNC.func, relAttrFunc, nRelations, relations, nConditions, conditions);
            break;
        }

        case N_SELECT_GROUP: { /* for SelectGroup() */

            RelAttr relAttrGroup;
            RelAttr relAttrFunc;
            int nRelations = 0;
            char *relations[MAXATTRS];
            int nConditions = 0;
            Condition conditions[MAXATTRS];

            if (strcmp(n->u.SELECT_GROUP.relattr1->u.RELATTR.relname, n->u.SELECT_GROUP.relattr2->u.RELATTR.relname)) {
                print_error((char*)"select_group", E_GROUPNOTMATCH);
                break;
            }
            if (strcmp(n->u.SELECT_GROUP.relattr1->u.RELATTR.attrname, n->u.SELECT_GROUP.relattr2->u.RELATTR.attrname)) {
                print_error((char*)"select_group", E_GROUPNOTMATCH);
                break;
            }

            relAttrGroup.relName = n->u.SELECT_GROUP.relattr1->u.RELATTR.relname;
            relAttrGroup.attrName = n->u.SELECT_GROUP.relattr1->u.RELATTR.attrname;
            relAttrFunc.relName = n->u.SELECT_GROUP.func->u.FUNC.relattr->u.RELATTR.relname;
            relAttrFunc.attrName = n->u.SELECT_GROUP.func->u.FUNC.relattr->u.RELATTR.attrname;

            /* Make a list of relation names suitable for sending to Query */
            nRelations = mk_relations(n->u.SELECT_GROUP.rellist, MAXATTRS, relations);
            if (nRelations < 0) {
                print_error((char*)"select_group", nRelations);
                break;
            }

            /* Make a list of Conditions suitable for sending to Query */
            nConditions = mk_conditions(n->u.SELECT_GROUP.conditionlist, MAXATTRS, conditions);
            if (nConditions < 0) {
                print_error((char*)"select_group", nConditions);
                break;
            }

            /* Make the call to Select */
            errval = pQlm->SelectGroup(n->u.SELECT_GROUP.func->u.FUNC.func, relAttrFunc, relAttrGroup, nRelations, relations, nConditions, conditions);
            break;
        }

        case N_SELECT: { /* for Select() */
            int nSelAttrs = 0;
            RelAttr relAttrs[MAXATTRS];
            int nRelations = 0;
            char *relations[MAXATTRS];
            int nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a list of RelAttrs suitable for sending to Query */
            nSelAttrs = mk_rel_attrs(n->u.SELECT.relattrlist, MAXATTRS, relAttrs);
            if (nSelAttrs < 0) {
                print_error((char*)"select", nSelAttrs);
                break;
            }

            /* Make a list of relation names suitable for sending to Query */
            nRelations = mk_relations(n->u.SELECT.rellist, MAXATTRS, relations);
            if (nRelations < 0) {
                print_error((char*)"select", nRelations);
                break;
            }

            /* Make a list of Conditions suitable for sending to Query */
            nConditions = mk_conditions(n->u.SELECT.conditionlist, MAXATTRS, conditions);
            if (nConditions < 0) {
                print_error((char*)"select", nConditions);
                break;
            }

            /* Make the call to Select */
            errval = pQlm->Select(nSelAttrs, relAttrs, nRelations, relations, nConditions, conditions);
            break;
        }

        case N_INSERT: { /* for Insert() */
            NODE *cur = n->u.INSERT.valuelists;
            for (; cur != NULL; cur = cur->u.LIST.next) {
                int nValues = 0;
                Value values[MAXATTRS];

                /* Make a list of Values suitable for sending to Insert */
                nValues = mk_values(cur->u.LIST.curr, MAXATTRS, values);
                if (nValues < 0) {
                    print_error((char*)"insert", nValues);
                    break;
                }

                /* Make the call to insert */
                if ((errval = pQlm->Insert(n->u.INSERT.relname, nValues, values))) break;
            }
            break;
        }

        case N_DELETE: { /* for Delete() */
            int nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a list of Conditions suitable for sending to delete */
            nConditions = mk_conditions(n->u.DELETE.conditionlist, MAXATTRS, conditions);
            if (nConditions < 0) {
                print_error((char*)"delete", nConditions);
                break;
            }

            /* Make the call to delete */
            errval = pQlm->Delete(n->u.DELETE.relname, nConditions, conditions);
            break;
        }

        case N_UPDATE: { /* for Update() */
            int nSetters = 0;
            RelAttr relAttr[MAXATTRS];
            Value rhsValue[MAXATTRS];
            int nConditions = 0;
            Condition conditions[MAXATTRS];

            /* Make a list of setters suitable for sending to Update */
            nSetters = mk_setters(n->u.UPDATE.setterlist, MAXATTRS, relAttr, rhsValue);
            if (nSetters < 0) {
                print_error((char*)"update", nSetters);
                break;
            }

            /* Make a list of Conditions suitable for sending to Update */
            nConditions = mk_conditions(n->u.UPDATE.conditionlist, MAXATTRS, conditions);
            if (nConditions < 0) {
                print_error((char*)"update", nConditions);
                break;
            }

            /* Make the call to update */
            errval = pQlm->Update(n->u.UPDATE.relname, nSetters, relAttr, rhsValue, nConditions, conditions);
            break;
        }

        default: // should never get here
            break;
    }

    return errval;
}

/*
 * mk_fields: converts a list of fields to an array of Field's so
 * it can be sent to Create.
 *
 * Returns:
 *    length of the list on success (>= 0)
 *    error code otherwise
 */
static int mk_fields(NODE *list, int max, Field fields[]) {
    int i;
    int j;
    int k;
    int len;
    int nameCount = 0;
    char *names[MAXATTRS];
    bool hasPrimaryKey = false;
    int foreignCount = 0;
    char *foreigns[MAXATTRS];
    NODE *field;
    NODE *attr;
    NODE *primaryKey;
    RC errval;
    /* for each element of the list... */
    for (i = 0; list != NULL; ++i, list = list -> u.LIST.next) {
        /* if the list is too long, then error */
        if (i == max) return E_TOOMANY;
        field = list -> u.LIST.curr;
        /* Make sure the attribute name isn't too long */
        fields[i].attr.attrName = NULL;
        fields[i].isNotNull = attr -> u.FIELD.isNotNull;
        fields[i].nPrimaryKey = 0;
        fields[i].foreignKey = NULL;
        if (field -> u.FIELD.attrType != NULL) {
            attr = field -> u.FIELD.attrType;
            if (strlen(attr -> u.ATTRTYPE.attrname) > MAXNAME)
                return E_TOOLONG;
            for (j = 0; j < nameCount; ++j)
                if (!strcmp(names[j], attr -> u.ATTRTYPE.attrname))
                    return E_DUPLICATEATTR;
            switch (attr -> u.ATTRTYPE.type) {
                case INT:
                    len = sizeof(int);
                    break;
                case FLOAT:
                    len = sizeof(float);
                    break;
                case DATE:
                    len = sizeof(int);
                    break;
                case STRING:
                    len = attr -> u.ATTRTYPE.length;
                    if (len < 1 || len > MAXSTRINGLEN) return E_INVSTRLEN;
                    break;
            }
            names[nameCount++] = attr -> u.ATTRTYPE.attrname;
            fields[i].attr.attrName = attr -> u.ATTRTYPE.attrname;
            fields[i].attr.attrType = attr -> u.ATTRTYPE.type;
            fields[i].attr.attrLength = len;
        } else if (field -> u.FIELD.primaryKeyList != NULL) {
            if (hasPrimaryKey) return E_TOOMANYPRIMARY;
            hasPrimaryKey = true;
            primaryKey = field -> u.FIELD.primaryKeyList;
            for (; primaryKey != NULL; primaryKey = primaryKey -> u.LIST.next) {
                bool found = false;
                for (j = 0; !found && j < nameCount; ++j)
                    if (!strcmp(names[j], primaryKey -> u.LIST.curr -> u.ATTR.attrname))
                        found = true;
                if (!found) return E_PRIMARYNOTEXIST;
                fields[i].primaryKeyList[fields[i].nPrimaryKey++] = primaryKey -> u.LIST.curr -> u.ATTR.attrname;
            }
            for (j = 0; j < fields[i].nPrimaryKey; ++j)
                for (k = 0; k < j; ++k)
                    if (!strcmp(fields[i].primaryKeyList[j], fields[i].primaryKeyList[k]))
                        return E_DUPLICATEPRIMARY;
        } else {
            if (strlen(attr -> u.FIELD.foreignKey) > MAXNAME)
                return E_TOOLONG;
            bool found = false;
            for (j = 0; !found && j < nameCount; ++j)
                if (!strcmp(names[j], field -> u.FIELD.foreignKey))
                    found = true;
            if (!found) return E_FOREIGNNOTEXIST;
            for (j = 0; j < foreignCount; ++j)
                if (!strcmp(foreigns[j], field -> u.FIELD.foreignKey))
                    return E_DUPLICATEFOREIGN;
            foreigns[foreignCount++] = field -> u.FIELD.foreignKey;
            fields[i].foreignKey = field -> u.FIELD.foreignKey;
            fields[i].refRel = field -> u.FIELD.refRel;
            fields[i].refAttr = field -> u.FIELD.refAttr;
        }
    }
    return i;
}

/*
 * mk_rel_attrs: converts a list of relation-attributes (<relation,
 * attribute> pairs) into an array of RelAttrs
 *
 * Returns:
 *    the length of the list on success (>= 0)
 *    error code otherwise
 */
static int mk_rel_attrs(NODE *list, int max, RelAttr relAttrs[]) {
    int i;
    NODE *current;
    /* For each element of the list... */
    for (i = 0; list != NULL; ++i, list = list -> u.LIST.next) {
        /* If the list is too long then error */
        if (i == max) return E_TOOMANY;
        current = list -> u.LIST.curr;
        relAttrs[i].relName = current->u.RELATTR.relname;
        relAttrs[i].attrName = current->u.RELATTR.attrname;
    }
    return i;
}

/*
 * mk_relations: converts a list of relations into an array of relations
 *
 * Returns:
 *    the length of the list on success (>= 0)
 *    error code otherwise
 */
static int mk_relations(NODE *list, int max, char *relations[]) {
    int i;
    NODE *current;
    /* for each element of the list... */
    for (i = 0; list != NULL; ++i, list = list -> u.LIST.next) {
        /* If the list is too long then error */
        if (i == max) return E_TOOMANY;
        current = list -> u.LIST.curr;
        relations[i] = current->u.RELATION.relname;
    }
    return i;
}

/*
 * mk_setters: converts a list of setters into arrays
 *
 * Returns:
 *    the length of the list on success (>= 0)
 *    error code otherwise
 */
static int mk_setters(NODE *list, int max, RelAttr relAttr[], Value rhsValue[]) {
    int i;
    NODE *current;
    /* for each element of the list... */
    for (i = 0; list != NULL; ++i, list = list -> u.LIST.next) {
        /* If the list is too long then error */
        if (i == max) return E_TOOMANY;
        current = list -> u.LIST.curr;
        relAttr[i].relName = NULL;
        relAttr[i].attrName = current->u.SETTER.attrname;
        mk_value(current->u.SETTER.value, rhsValue[i]);
    }
    return i;
}

/*
 * mk_conditions: converts a list of conditions into an array of conditions
 *
 * Returns:
 *    the length of the list on success (>= 0)
 *    error code otherwise
 */
static int mk_conditions(NODE *list, int max, Condition conditions[]) {
    int i;
    NODE *current;
    /* for each element of the list... */
    for (i = 0; list != NULL; ++i, list = list -> u.LIST.next) {
        /* If the list is too long then error */
        if (i == max) return E_TOOMANY;
        current = list -> u.LIST.curr;
        conditions[i].lhsAttr.relName = current->u.CONDITION.lhsRelattr->u.RELATTR.relname;
        conditions[i].lhsAttr.attrName = current->u.CONDITION.lhsRelattr->u.RELATTR.attrname;
        conditions[i].op = current->u.CONDITION.op;
        if (current->u.CONDITION.rhsRelattr) {
            conditions[i].bRhsIsAttr = TRUE;
            conditions[i].rhsAttr.relName = current->u.CONDITION.rhsRelattr->u.RELATTR.relname;
            conditions[i].rhsAttr.attrName = current->u.CONDITION.rhsRelattr->u.RELATTR.attrname;
        } else {
            conditions[i].bRhsIsAttr = FALSE;
            mk_value(current->u.CONDITION.rhsValue, conditions[i].rhsValue);
        }
    }
    return i;
}

/*
 * mk_values: converts a list of values into an array of values
 *
 * Returns:
 *    the length of the list on success (>= 0)
 *    error code otherwise
 */
static int mk_values(NODE *list, int max, Value values[]) {
    int i;
    /* for each element of the list... */
    for (i = 0; list != NULL; ++i, list = list -> u.LIST.next) {
        /* If the list is too long then error */
        if (i == max) return E_TOOMANY;
        mk_value(list->u.LIST.curr, values[i]);
    }
    return i;
}

/*
 * mk_value: converts a single value node into a Value
 */
static void mk_value(NODE *node, Value &value) {
    value.type = node->u.VALUE.type;
    switch (value.type) {
        case INT:
            value.data = new char[1 + sizeof(int)];
            *(char*)value.data = 1;
            *(int*)((char*)value.data + 1) = node->u.VALUE.ival;
            break;
        case FLOAT:
            value.data = new char[1 + sizeof(float)];
            *(char*)value.data = 1;
            *(float*)((char*)value.data + 1) = node->u.VALUE.rval;
            break;
        case STRING:
            if (node->u.VALUE.sval == NULL) {
                value.data = new char[2];
                *(char*)value.data = 0;
                *((char*)value.data + 1) = 0;
            } else {
                value.data = new char[2 + strlen(node->u.VALUE.sval)];
                *(char*)value.data = 1;
                strcpy((char*)value.data + 1, node->u.VALUE.sval);
            }
            break;
    }
}

/*
 * print_error: prints an error message corresponding to errval
 */
static void print_error(char *errmsg, RC errval) {
    if (errmsg != NULL)
        fprintf(ERRFP, "%s: ", errmsg);
    switch (errval) {
        case E_OK:
            fprintf(ERRFP, "no error\n");
            break;
        case E_TOOMANY:
            fprintf(ERRFP, "too many elements\n");
            break;
        case E_INVSTRLEN:
            fprintf(ERRFP, "invalid length for string attribute\n");
            break;
        case E_DUPLICATEATTR:
            fprintf(ERRFP, "duplicated attribute name\n");
            break;
        case E_TOOLONG:
            fprintf(ERRFP, "relation name or attribute name too long\n");
            break;
        case E_FOREIGNNOTEXIST:
            fprintf(ERRFP, "foreign key does not exist\n");
            break;
        case E_DUPLICATEFOREIGN:
            fprintf(ERRFP, "duplicated foreign key name\n");
            break;
        case E_DUPLICATEPRIMARY:
            fprintf(ERRFP, "duplicated primary key name\n");
            break;
        case E_TOOMANYPRIMARY:
            fprintf(ERRFP, "too many primary keys\n");
            break;
        case E_PRIMARYNOTEXIST:
            fprintf(ERRFP, "primary key does not exist\n");
            break;
        case E_GROUPNOTMATCH:
            fprintf(ERRFP, "group not match\n");
            break;
        default:
            fprintf(ERRFP, "unrecognized errval: %d\n", errval);
    }
}
