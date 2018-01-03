//
// File:        parser.h
// Description: Parser Component Interface
// Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
//

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include "global.h"
#include "pf.h"

//
// Structure declarations and output functions
//
struct AttrInfo {
    char     *attrName;        /* attribute name       */
    AttrType attrType;         /* type of attribute    */
    int      attrLength;       /* length of attribute  */
};

struct Field {
    AttrInfo attr;                      /* attr info                */
    bool     isNotNull;                 /* whether attr allows null */
    int      nPrimaryKey;               /* number of primary keys   */
    char     *primaryKeyList[MAXATTRS]; /* list of primary keys     */
    char     *foreignKey;               /* foreign key              */
    char     *refRel;                   /* reference relation       */
    char     *refAttr;                  /* reference attribute      */
};

struct RelAttr {
    char     *relName;    /* relation name (may be NULL) */
    char     *attrName;   /* attribute name              */
    /* print function */
    friend std::ostream &operator<<(std::ostream &s, const RelAttr &ra);
};

struct Value {
    AttrType type;         /* type of value              */
    void     *data;        /* value                      */
    /* print function */
    friend std::ostream &operator<<(std::ostream &s, const Value &v);
};

struct Condition {
    RelAttr  lhsAttr;    /* left-hand side attribute             */
    CompOp   op;         /* comparison operator                  */
    int      bRhsIsAttr; /* TRUE if the rhs is an attribute,     */
                         /* in which case rhsAttr below is valid;*/
                         /* otherwise, rhsValue below is valid.  */
    RelAttr  rhsAttr;    /* right-hand side attribute            */
    Value    rhsValue;   /* right-hand side value                */
	/* print function */
    friend std::ostream &operator<<(std::ostream &s, const Condition &c);
};

std::ostream &operator<<(std::ostream &s, const CompOp &op);
std::ostream &operator<<(std::ostream &s, const AttrType &at);

//
// Parse function
//
class QL_Manager;
class SM_Manager;

void RippleDBparse(PF_Manager &pfm, SM_Manager &smm, QL_Manager &qlm);

//
// Error printing function; calls component-specific functions
//
void PrintError(RC rc);

#endif
