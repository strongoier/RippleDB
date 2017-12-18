//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H

#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"

class RM_FileIterator {
public:
    RM_FileIterator(RM_FileHandle& rfh, Condition c);
private:
    RM_FileHandle& rmFileHandle;
    RM_FileScan rmFileScan;
    Condition condition;
};

//
// QL_Manager: query language (DML)
//
class QL_Manager {
public:
    QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    ~QL_Manager();                       // Destructor

    RC Select  (int nSelAttrs,           // # attrs in select clause
        const RelAttr selAttrs[],        // attrs in select clause
        int   nRelations,                // # relations in from clause
        const char * const relations[],  // relations in from clause
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Insert  (const char *relName,     // relation to insert into
        int   nValues,                   // # values
        const Value values[]);           // values to insert

    RC Delete  (const char *relName,     // relation to delete from
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Update  (const char *relName,     // relation to update
        const RelAttr &updAttr,          // attribute to update
        const int bIsValue,              // 1 if RHS is a value, 0 if attribute
        const RelAttr &rhsRelAttr,       // attr on RHS to set LHS equal to
        const Value &rhsValue,           // or value to set attr equal to
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

private:
    RM_Manager& rmManager;
    IX_Manager& ixManager;
    SM_Manager& smManager;

    RC GetFullConditions(const char* relName, const std::vector<AttrCat>& attrs, int nConditions, const Condition conditions[], std::vector<FullCondition>& fullConditions);
};

//
// Print-error function
//
void QL_PrintError(RC rc);

#define QL_DBNOTOPEN        (START_QL_WARN + 0)  // no db is open
#define QL_RELNOTFIND       (START_QL_WARN + 1)  // relation not found
#define QL_ATTRSNUMBERWRONG (START_QL_WARN + 2)  // attr number not right
#define QL_ATTRTYPEWRONG    (START_QL_WARN + 3)  // attr type not right
#define QL_ATTRNOTFOUND     (START_QL_WARN + 4)  // attribute not found

#endif
