//
// File:        ql.h
// Description: Query Language component interface
// Authors:     Shihong Yan (Modified from the original file in Stanford CS346 RedBase)
//

#ifndef QL_H
#define QL_H

#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include "global.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"

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
        int   nSetters,                  // number of setters
        const RelAttr updAttr[],         // attribute to update
        const int bIsValue[],            // 1 if RHS is a value, 0 if attribute
        const RelAttr rhsRelAttr[],      // attr on RHS to set LHS equal to
        const Value rhsValue[],          // or value to set attr equal to
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

private:
    RM_Manager& rmManager;
    IX_Manager& ixManager;
    SM_Manager& smManager;

    //
    // 检查数据库是否被打开
    //
    RC CheckSMManagerIsOpen();

    //
    // 检查数据表是否存在，如果存在，提取数据表信息
    //
    RC CheckRelCat(const char* relName, RelCat& relCat);

    //
    // 检查数据表是否存在，如果存在，提取数据表信息（并不提取属性信息，属性值为空）
    //
    RC CheckRelCats(int nRelations, const char* const relations[], std::map<RelCat, std::vector<AttrCat>>& relCats);

    //
    // 在完整的数据表-属性数据结构中查找某一原始属性结构对应的具体信息
    //
    RC CheckAttrCat(const RelAttr& relAttr, const std::map<RelCat, std::vector<AttrCat>>& relCats, RelCat& relCat, AttrCat& attrCat);

    //
    // 在完整的数据表-属性数据结构中查找某一原始属性结构对应的具体信息，并写入查询属性集中
    //
    RC CheckAttrCats(const RelAttr& relAttr, const std::map<RelCat, std::vector<AttrCat>>& relCats, std::map<RelCat, std::vector<AttrCat>>& attrs);

    //
    // 将原始单表限定条件（delete 与 update 的 where 字句）集合补充为完整单表限制条件集合
    //
    RC GetFullConditions(const char* relName, const std::vector<AttrCat>& attrs, int nConditions, const Condition conditions[], std::vector<FullCondition>& fullConditions);

    //
    // 将某一原始单表或多表限定条件（select 的 where 字句）集合补充并归类到完整单表与多表限制条件集合
    //
    RC GetFullCondition(const Condition& condition, const std::map<RelCat, std::vector<AttrCat>>& relCats, std::map<RelCat, std::vector<FullCondition>>& singalRelConds, std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>>& binaryRelConds);

    RC GetRidSet(const char* relName, RM_FileHandle& rmFileHandle, const std::vector<FullCondition>& fullConditions, std::vector<RID>& rids);

    RC GetDataSet(const RelCat& relCat, RM_FileHandle& rmFileHandle, const std::vector<FullCondition>& fullConditions, std::vector<char*>& data);

    RC CheckFullConditions(char* recordData, const std::vector<FullCondition>& fullConditions, bool& result);

    bool CheckFullCondition(char* aData, char* bData, const std::vector<FullCondition>& conditions);

    RC GetJoinData(std::map<RelCat, std::vector<char*>>& data, std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>>& binaryRelConds, std::vector<std::map<RelCat, char*>>& joinData);
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
#define QL_RELMULTIAPPEAR   (START_QL_WARN + 5)  // relation variables to distinguish multiple appearances
#define QL_RELNOTFOUND      (START_QL_WARN + 6)  // relation not found
#define QL_ATTRMULTIAPPEAR  (START_QL_WARN + 7)  // attribute multi appear

#endif
