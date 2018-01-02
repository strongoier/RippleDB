//
// File:        ql_manager.cc
// Description: QL_Manager class implementation
// Authors:     Shihong Yan
//

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <sys/times.h>
#include <sys/types.h>
#include <set>
#include <cassert>
#include <unistd.h>
#include "global.h"
#include "printer.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : rmManager(rmm), ixManager(ixm), smManager(smm) {}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager() {}

//
// Handle the select clause
//
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[], int nRelations, const char * const relations[], int nConditions, const Condition conditions[]) {
    RC rc;
    int attrCount;
    // check whether a db is open
    if ((rc = CheckSMManagerIsOpen())) {
        return rc;
    }
    // check and get relation info
    std::map<RelCat, std::vector<AttrCat>> relCats;
    if ((rc = CheckRelCats(nRelations, relations, relCats))) {
        return rc;
    }
    // get attr info
    for (auto &item : relCats) {
        if ((rc = smManager.GetAttrs(item.first.relName, item.second))) {
            return rc;
        }
    }
    // check whether select's attr exists
    std::map<RelCat, std::vector<AttrCat>> attrs;
    if (nSelAttrs == 1 && selAttrs[0].attrName[0] == '*') {
        // select all attrs
        attrs = relCats;
        attrCount = 0;
        for (const auto &item : attrs) {
            attrCount += item.second.size();
        }
    } else {
        attrCount = nSelAttrs;
        for (int i = 0; i < nSelAttrs; ++i) {
            if ((rc = CheckAttrCats(selAttrs[i], relCats, attrs))) {
                return rc;
            }
        }
    }
    cerr << "attr" << endl;
    for (const auto &item : attrs) {
        cerr << "    " << item.first.relName << "    " << item.second.size() << endl;
        for (const auto &attr : item.second) {
            cerr << "        " << attr.attrName << endl;
        }
    }
    // check conditions
    std::map<RelCat, std::vector<FullCondition>> singalRelConds;
    std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>> binaryRelConds;
    for (int i = 0; i < nConditions; ++i) {
        if ((rc = GetFullCondition(conditions[i], relCats, singalRelConds, binaryRelConds))) {
            cerr << "cond error" << endl;
            return rc;
        }
    }
    cerr << "singal" << endl;
    for (const auto &item : singalRelConds) {
        cerr << "    " << item.first.relName << "    " << item.second.size() << endl;
    }
    cerr << "binary" << endl;
    for (const auto &item : binaryRelConds) {
        cerr << "    " << item.first.first.relName << "    " << item.first.second.relName << "    " << item.second.size() << endl;
    }
    // get scan data
    std::map<RelCat, std::vector<char*>> data;
    for (const auto& item : relCats) {
        RM_FileHandle relFileHandle;
        if ((rc = rmManager.OpenFile(item.first.relName, relFileHandle))) {
            return rc;
        }
        if ((rc = GetDataSet(item.first, relFileHandle, singalRelConds[item.first], data[item.first]))) {
            return rc;
        }
        if ((rc = rmManager.CloseFile(relFileHandle))) {
            return rc;
        }
    }
    cerr << "data" << endl;
    for (const auto &item : data) {
        cerr << "    " << item.first.relName << "    " << item.second.size() << endl;
    }
    // join
    std::vector<std::map<RelCat, char*>> joinData{std::map<RelCat, char*>()};
    if ((rc = GetJoinData(data, binaryRelConds, joinData))) {
        return rc;
    }
    cerr << "join" << endl;
    cerr << "    " << joinData.size() << endl;
    // print
    DataAttrInfo* attributes = new DataAttrInfo[attrCount];
    int index = 0;
    int tupleLength = 0;
    for (const auto& item : attrs) {
        for (const auto& attr : item.second) {
            strcpy(attributes[index].relName, attr.relName);
            strcpy(attributes[index].attrName, attr.attrName);
            attributes[index].offset = tupleLength;
            attributes[index].attrType = attr.attrType;
            attributes[index].attrLength = attr.attrLength;
            cerr << attributes[index].attrLength << endl;
            attributes[index].indexNo = attr.indexNo;
            tupleLength += attr.attrLength;
            ++index;
        }
    }
    Printer printer(attributes, attrCount);
    printer.PrintHeader(cout);
    char *tuple = new char[tupleLength];
    for (auto& item : joinData) {
        int pos = 0;
        for (auto& rel : attrs) {
            for (auto& attr : rel.second) {
                memcpy(tuple + pos, item[rel.first] + attr.offset, attr.attrLength);
                pos += attr.attrLength;
       //
    // 将某一原始单表或多表限定条件（select 的 where 字句）集合补充并归类到完整单表与多表限制条件集合
    //     }
        }
        printer.Print(cout, tuple);
    }
    delete[] tuple;
    printer.PrintFooter(cout);
    
    for (auto& item : data) {
        for (auto & i : item.second) {
            delete[] i;
        }
    }

    // print
    cout << "Select\n";
    cout << "   nSelAttrs = " << nSelAttrs << "\n";
    for (int i = 0; i < nSelAttrs; i++)
        cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";
    cout << "   nRelations = " << nRelations << "\n";
    for (int i = 0; i < nRelations; i++)
        cout << "   relations[" << i << "] " << relations[i] << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName, int nValues, const Value values[]) {
    RC rc;
    // check whether a db is open
    if ((rc = CheckSMManagerIsOpen())) {
        return rc;
    }
    // find relation name in relCat
    RelCat relCat;
    if ((rc = CheckRelCat(relName, relCat))) {
        return rc;
    }
    // check nValues == attrCount
    if (nValues != relCat.attrCount) {
        return QL_ATTRSNUMBERWRONG;
    }
    // get attr info in the relation
    vector<AttrCat> attrs;
    if ((rc = smManager.GetAttrs(relName, attrs))) {
        return rc;
    }
    // check the type info
    for (int i = 0; i < nValues; ++i) {
        if (values[i].type != attrs[i].attrType) {
            return QL_ATTRTYPEWRONG;
        }
    }

    // construct the tuple
    char *tuple = new char[relCat.tupleLength];
    for (int i = 0; i < nValues; ++i) {
        Attr::SetAttr(tuple + attrs[i].offset, attrs[i].attrType, values[i].data);
    }

    // insert record into file
    RM_FileHandle relFileHandle;
    if ((rc = rmManager.OpenFile(relName, relFileHandle))) {
        return rc;
    }
    RID rid;
    if ((rc = relFileHandle.InsertRec(tuple, rid))) {
        return rc;
    }
    cerr << "rid: " << rid << endl;
    if ((rc = rmManager.CloseFile(relFileHandle))) {
        return rc;
    }

    // insert index into file
    IX_IndexHandle relIndexHandle;
    for (int i = 0; i < nValues; ++i) {
        // check whether the attr has an index
        if (attrs[i].indexNo != -1) {
            if ((rc = ixManager.OpenIndex(relName, attrs[i].indexNo, relIndexHandle))) {
                return rc;
            }
            if ((rc = relIndexHandle.InsertEntry(values[i].data, rid))) {
                return rc;
            }
            if ((rc = ixManager.CloseIndex(relIndexHandle))) {
                return rc;
            }
        }
    }
    delete[] tuple;

    // print
    cout << "Insert\n";
    cout << "   relName = " << relName << "\n";
    cout << "   nValues = " << nValues << "\n";
    for (int i = 0; i < nValues; i++)
        cout << "   values[" << i << "]:" << values[i] << "\n";
    
    cout << "\n";
    if ((rc = smManager.Print(relName))) {
        return rc;
    }

    return 0;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName, int nConditions, const Condition conditions[]) {
    RC rc;
    // check whether a db is open
    if ((rc = CheckSMManagerIsOpen())) {
        return rc;
    }
    // find relation name in relCat
    RelCat relCat;
    if ((rc = CheckRelCat(relName, relCat))) {
        return rc;
    }
    // get attr info in the relation
    vector<AttrCat> attrs;
    if ((rc = smManager.GetAttrs(relName, attrs))) {
        return rc;
    }
    // check the conditions
    vector<FullCondition> fullConditions;
    if ((rc = GetFullConditions(relName, attrs, nConditions, conditions, fullConditions))) {
        return rc;
    }
    // scan the rid set
    RM_FileHandle rmFileHandle;
    if ((rc = rmManager.OpenFile(relName, rmFileHandle))) {
        return rc;
    }
    vector<RID> rids;
    if ((rc = GetRidSet(relName, rmFileHandle, fullConditions, rids))) {
        return rc;
    }
    // delete indexs
    for (const auto &attr : attrs) {
        if (attr.indexNo != -1) {
            IX_IndexHandle indexHandle;
            if ((rc = ixManager.OpenIndex(relName, attr.indexNo, indexHandle))) {
                return rc;
            }
            for (const auto& rid : rids) {
                RM_Record record;
                if ((rc = rmFileHandle.GetRec(rid, record))) {
                    return rc;
                }
                char *recordData;
                if ((rc = record.GetData(recordData))) {
                    return rc;
                }
                if ((rc = indexHandle.DeleteEntry(recordData + attr.offset, rid))) {
                    return rc;
                }
            }
            if ((rc = ixManager.CloseIndex(indexHandle))) {
                return rc;
            }
        }
    }
    // delete records
    for (const auto &rid : rids) {
        if ((rc = rmFileHandle.DeleteRec(rid))) {
            return rc;
        }
    }
    if ((rc = rmManager.CloseFile(rmFileHandle))) {
        return rc;
    }

    // print
    cout << "Delete\n";
    cout << "   relName = " << relName << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    
    cout << "\n";
    if ((rc = smManager.Print(relName))) {
        return rc;
    }

    return 0;
}

//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName, const RelAttr &updAttr, const int bIsValue, const RelAttr &rhsRelAttr, const Value &rhsValue, int nConditions, const Condition conditions[]) {
    RC rc;
    // check whether a db is open
    if ((rc = CheckSMManagerIsOpen())) {
        return rc;
    }
    // find relation name in relCat
    RelCat relCat;
    if ((rc = CheckRelCat(relName, relCat))) {
        return rc;
    }
    // get attr info in the relation
    vector<AttrCat> attrs;
    if ((rc = smManager.GetAttrs(relName, attrs))) {
        return rc;
    }
    // check updAttr
    auto iter = find_if(attrs.begin(), attrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, updAttr.attrName) == 0; });
    if (iter == attrs.end()) {
        return QL_ATTRNOTFOUND;
    }
    // check rhs type
    auto rIter = attrs.end();
    if (bIsValue) {
        if (iter->attrType != rhsValue.type) {
            return QL_ATTRTYPEWRONG;
        }
    } else {
        rIter = find_if(attrs.begin(), attrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, rhsRelAttr.attrName) == 0; });
        if (iter == attrs.end()) {
            return QL_ATTRNOTFOUND;
        }
        if (iter->attrType != rIter->attrType) {
            return QL_ATTRTYPEWRONG;
        }
    }
    // check the conditions
    vector<FullCondition> fullConditions;
    if ((rc = GetFullConditions(relName, attrs, nConditions, conditions, fullConditions))) {
        return rc;
    }
    // scan the rid set
    RM_FileHandle rmFileHandle;
    if ((rc = rmManager.OpenFile(relName, rmFileHandle))) {
        return rc;
    }
    vector<RID> rids;
    if ((rc = GetRidSet(relName, rmFileHandle, fullConditions, rids))) {
        return rc;
    }
    // update index
    if (iter->indexNo != -1) {
        IX_IndexHandle indexHandle;
        if ((rc = ixManager.OpenIndex(relName, iter->indexNo, indexHandle))) {
            return rc;
        }
        for (const auto& rid : rids) {
            RM_Record record;
            if ((rc = rmFileHandle.GetRec(rid, record))) {
                return rc;
            }
            char *recordData;
            if ((rc = record.GetData(recordData))) {
                return rc;
            }
            if ((rc = indexHandle.DeleteEntry(recordData + iter->offset, rid))) {
                return rc;
            }
            if (bIsValue) {
                if ((rc = indexHandle.InsertEntry(rhsValue.data, rid))) {
                    return rc;
                }
            } else {
                if ((rc = indexHandle.InsertEntry(recordData + rIter->offset, rid))) {
                    return rc;
                }
     //
    // 将某一原始单表或多表限定条件（select 的 where 字句）集合补充并归类到完整单表与多表限制条件集合
    //       }
        }
        if ((rc = ixManager.CloseIndex(indexHandle))) {
            return rc;
        }
    }

    // update record
    for (const auto &rid : rids) {
        RM_Record record;
        if ((rc = rmFileHandle.GetRec(rid, record))) {
            return rc;
        }
        char *recordData;
        if ((rc = record.GetData(recordData))) {
            return rc;
        }
        if (bIsValue) {
            Attr::SetAttr(recordData + iter->offset, iter->attrType, rhsValue.data);
        } else {
            Attr::SetAttr(recordData + iter->offset, iter->attrType, recordData + rIter->offset);
        }
        if ((rc = rmFileHandle.UpdateRec(record))) {
            return rc;
        }
    }
    if ((rc = rmManager.CloseFile(rmFileHandle))) {
        return rc;
    }
    
    // print
    cout << "Update\n";
    cout << "   relName = " << relName << "\n";
    cout << "   updAttr:" << updAttr << "\n";
    if (bIsValue)
        cout << "   rhs is value: " << rhsValue << "\n";
    else
        cout << "   rhs is attribute: " << rhsRelAttr << "\n";

    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    
    cout << "\n";
    if ((rc = smManager.Print(relName))) {
        return rc;
    }

    return 0;
}

//
// 检查数据库是否被打开
//
RC QL_Manager::CheckSMManagerIsOpen() {
    if (!smManager.isOpen) {
        return QL_DBNOTOPEN;
    }
    return OK_RC;
}

//
// 检查数据表是否存在，如果存在，提取数据表信息
//
RC QL_Manager::CheckRelCat(const char* relName, RelCat& relCat) {
    RC rc;
    RM_Record relCatRec;
    // 检查数据表是否存在
    if ((rc = smManager.CheckRelExist(relName, relCatRec))) {
        return rc;
    }
    // 获取数据表数据
    char* relCatData;
    if ((rc = relCatRec.GetData(relCatData))) {
        return rc;
    }
    // 格式化数据表数据
    relCat = RelCat(relCatData);
    return OK_RC;
}

//
// 检查数据表是否存在，如果存在，提取数据表信息（并不提取属性信息，属性值为空）
//
RC QL_Manager::CheckRelCats(int nRelations, const char* const relations[], std::map<RelCat, std::vector<AttrCat>>& relCats) {
    RC rc;
    // 遍历每个表名称
    for (int i = 0; i < nRelations; ++i) {
        RelCat relCat;
        // 获取数据表信息
        if ((rc = CheckRelCat(relations[i], relCat))) {
            return rc;
        }
        // 如果数据表重复出现，报错
        if (relCats.find(relCat) != relCats.end()) {
            return QL_RELMULTIAPPEAR;
        }
        // 插入到结果集合（属性值均为空）
        relCats.insert(std::make_pair(relCat, std::vector<AttrCat>()));
    }
    return OK_RC;
}

//
// 在完整的数据表-属性数据结构中查找某一原始属性结构对应的具体信息
//
RC QL_Manager::CheckAttrCat(const RelAttr& relAttr, const std::map<RelCat, std::vector<AttrCat>>& relCats, RelCat& relCat, AttrCat& attrCat) {
    if (relAttr.relName != NULL) {
        // 如果属性指定表名
        // 检查该表名是否存在
        auto relIter = find_if(relCats.begin(), relCats.end(), [&](const std::map<RelCat, std::vector<AttrCat>>::value_type& item) { return strcmp(item.first.relName, relAttr.relName) == 0; });
        // 如果不存在，报错
        if (relIter == relCats.end()) {
            return QL_RELNOTFOUND;
        }
        // 获取数据表信息
        relCat = relIter->first;
        // 检查属性名是否存在
        auto attrIter = find_if(relIter->second.begin(), relIter->second.end(), [&](const AttrCat& item) { return strcmp(item.attrName, relAttr.attrName) == 0; });
        // 如果不存在，报错
        if (attrIter == relIter->second.end()) {
            return QL_ATTRNOTFOUND;
        }
        // 获取属性信息
        attrCat = *attrIter;
    } else {
        // 如果属性没有指定表名
        // 遍历数据表查找属性名
        int cnt = 0;
        for (const auto& item : relCats) {
            auto iter = find_if(item.second.begin(), item.second.end(), [&](const AttrCat& i) { return strcmp(i.attrName, relAttr.attrName) == 0; });
            // 没找到，跳过
            if (iter == item.second.end()) {
                continue;
            }
            // 找到了，更新计数器与相关信息
            ++cnt;
            relCat = item.first;
            attrCat = *iter;
            // 发现重复直接退出
            if（cnt == 2) {
                break;
            }
        }
        // 找不到属性名，报错
        if (cnt == 0) {
            return QL_ATTRNOTFOUND;
        }
        // 属性名多次出现，报错
        if (cnt == 2) {
            return QL_ATTRMULTIAPPEAR;
        }
    }
    return OK_RC;
}

//
// 在完整的数据表-属性数据结构中查找某一原始属性结构对应的具体信息，并写入查询属性集中
//
RC QL_Manager::CheckAttrCats(const RelAttr& relAttr, const std::map<RelCat, std::vector<AttrCat>>& relCats, std::map<RelCat, std::vector<AttrCat>>& attrs) {
    RC rc;
    RelCat key;
    AttrCat value;
    // 获取属性值的具体信息
    if ((rc = CheckAttrCat(relAttr, relCats, key, value))) {
        return rc;
    }
    // 插入查询属性集中
    auto relIterator = attrs.find(key);
    if (relIterator == attrs.end()) {
        // 新的数据表，直接插入键值对
        std::vector<AttrCat> tmp = { value };
        attrs.insert(std::make_pair(key, tmp));
    } else {
        // 已有数据表键值对
        auto attrIterator = find_if(relIterator->second.begin(), relIterator->second.end(), [&](const AttrCat& item) { return strcmp(item.attrName, relAttr.attrName) == 0; });
        // 重复出现，报错
        if (attrIterator != relIterator->second.end())
            {
            return QL_ATTRMULTIAPPEAR;
        }
        // 插入
        relIterator->second.push_back(value);
    }
    return OK_RC;
}

//
// 将原始单表限定条件（delete 与 update 的 where 字句）集合补充为完整单表限制条件集合
//
RC QL_Manager::GetFullConditions(const char* relName, const vector<AttrCat>& attrs, int nConditions, const Condition conditions[], vector<FullCondition>& fullConditions) {
    // 遍历每一个原始条件
    for (int i = 0; i < nConditions; ++i) {
        // 在完整属性集合中查找条件中左属性名是否存在
        auto iter = find_if(attrs.begin(), attrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, conditions[i].lhsAttr.attrName) == 0; });
        // 不存在，报错
        if (iter == attrs.end()) {
            return QL_ATTRNOTFOUND;
        }
        // 获得完整限制条件结构
        FullCondition fc;
        fc.lhsAttr = *iter;
        fc.op = conditions[i].op;
        fc.bRhsIsAttr = conditions[i].bRhsIsAttr;
        if (fc.bRhsIsAttr) {
            // 条件右侧为属性，查找是否存在
            auto rIter = find_if(attrs.begin(), attrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, conditions[i].rhsAttr.attrName) == 0; });
            // 不存在，报错
            if (rIter == attrs.end()) {
                return QL_ATTRNOTFOUND;
            }
            // 类型不一致，报错
            if (iter->attrType != rIter->attrType) {
                return QL_ATTRTYPEWRONG;
            }
            fc.rhsAttr = *rIter;
        } else {
            // 条件右侧为值
            // 类型不一致，报错
            if (iter->attrType != conditions[i].rhsValue.type) {
                return QL_ATTRTYPEWRONG;
            }
            fc.rhsValue = conditions[i].rhsValue;
        }
        fullConditions.push_back(fc);
    }
    return OK_RC;
}

//
// 将某一原始单表或多表限定条件（select 的 where 字句）集合补充并归类到完整单表与多表限制条件集合
//
RC QL_Manager::GetFullCondition(const Condition& condition, const std::map<RelCat, std::vector<AttrCat>>& relCats, std::map<RelCat, std::vector<FullCondition>>& singalRelConds, std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>>& binaryRelConds) {
    RC rc;
    FullCondition fullCondition;
    // 检查左侧属性
    RelCat relCat;
    AttrCat attrCat;
    // 获取左侧属性的详细信息
    if ((rc = CheckAttrCat(condition.lhsAttr, relCats, relCat, attrCat))) {
        return rc;
    }
    // 检查右侧属性或值
    if (condition.bRhsIsAttr == 0) {
        // 如果右侧为字面值
        // 类型不一致，报错
        if (attrCat.attrType != condition.rhsValue.type) {
            return QL_ATTRTYPEWRONG;
        }
        // 补充为完整结构体
        fullCondition.lhsAttr = attrCat;
        fullCondition.op = condition.op;
        fullCondition.bRhsIsAttr = condition.bRhsIsAttr;
        fullCondition.rhsValue = condition.rhsValue;
        // 插入到完整单表限制条件集合中（右侧为值一定为单表条件）
        auto iter = singalRelConds.find(relCat);
        if (iter == singalRelConds.end()) {
            std::vector<FullCondition> tmp = {fullCondition};
            singalRelConds.insert(std::make_pair(relCat, tmp));
        } else {
            iter->second.push_back(fullCondition);
        }
    } else {
        // 如果右侧为属性
        // 检查右侧属性是否存在
        RelCat rhsRelCat;
        AttrCat rhsAttrCat;
        if ((rc = CheckAttrCat(condition.rhsAttr, relCats, rhsRelCat, rhsAttrCat))) {
            return rc;
        }
        // 如果类型不一致，报错
        if (attrCat.attrType != rhsAttrCat.attrType) {
            return QL_ATTRTYPEWRONG;
        }
        // ok
        fullCondition.lhsAttr = relCat < rhsRelCat ? attrCat : rhsAttrCat;
        fullCondition.op = condition.op;
        fullCondition.bRhsIsAttr = condition.bRhsIsAttr;
        fullCondition.rhsAttr = relCat < rhsRelCat ? rhsAttrCat : attrCat;
        if (relCat == rhsRelCat) {
            auto iter = singalRelConds.find(relCat);
            if (iter == singalRelConds.end()) {
                std::vector<FullCondition> tmp = {fullCondition};
                singalRelConds.insert(std::make_pair(relCat, tmp));
            } else {
                iter->second.push_back(fullCondition);
            }
        } else {
            if (!(relCat < rhsRelCat || relCat == rhsRelCat)) {
                switch (fullCondition.op) {
                    case LT_OP:
                        fullCondition.op = GE_OP;
                        break;
                    case LE_OP:
                        fullCondition.op = GT_OP;
                        break;
                    case GT_OP:
                        fullCondition.op = LE_OP;
                        break;
                    case GE_OP:
                        fullCondition.op = LT_OP;
                        break;
                    default:
                        break;
                }
            }
            std::pair<RelCat, RelCat> key = relCat < rhsRelCat ? std::make_pair(relCat, rhsRelCat) : std::make_pair(rhsRelCat, relCat);
            auto iter = binaryRelConds.find(key);
            if (iter == binaryRelConds.end()) {
                std::vector<FullCondition> tmp = {fullCondition};
                binaryRelConds.insert(std::make_pair(key, tmp));
            } else {
                iter->second.push_back(fullCondition);
            }
        }
    }
    return OK_RC;
}

RC QL_Manager::GetRidSet(const char* relName, RM_FileHandle& rmFileHandle, const std::vector<FullCondition>& fullConditions, std::vector<RID>& rids) {
    RC rc;
    // the condition with index
    int index = -1;
    for (unsigned int i = 0; i < fullConditions.size(); ++i) {
        if (fullConditions[i].lhsAttr.indexNo >= 0 && fullConditions[i].bRhsIsAttr == 0) {
            index = i;
            break;
        }
    }
    // scan the rids
    if (index == -1) {
        // use only rm scan
        RM_FileScan rmFileScan;
        if ((rc = rmFileScan.OpenScan(rmFileHandle, fullConditions))) {
            return rc;
        }
        RM_Record record;
        RID rid;
        while (true) {
            if ((rc = rmFileScan.GetNextRec(record)) && rc != RM_EOF) {
                return rc;
            }
            if (rc == RM_EOF) {
                break;
            }
            if ((rc = record.GetRid(rid))) {
                return rc;
            }
            rids.push_back(rid);
        }
        if ((rc = rmFileScan.CloseScan())) {
            return rc;
        }
    } else {
        // use index scan and rm scan
        // open index scan
        IX_IndexHandle indexHandle;
        if ((rc = ixManager.OpenIndex(relName, fullConditions[index].lhsAttr.indexNo, indexHandle))) {
            return rc;
        }
        IX_IndexScan indexScan;
        if ((rc = indexScan.OpenScan(indexHandle, fullConditions[index].op, fullConditions[index].rhsValue.data))) {
            return rc;
        }
        // scan the rid
        while (true) {
            RID rid;
            if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
                return rc;
            }
            if (rc == IX_EOF) {
                break;
            }
            RM_Record record;
            if ((rc = rmFileHandle.GetRec(rid, record))) {
                return rc;
            }
            char *recordData;
            if ((rc = record.GetData(recordData))) {
                return rc;
            }
            bool result;
            if ((rc = CheckFullConditions(recordData, fullConditions, result))) {
                return rc;
            }
            if (result) {
                rids.push_back(rid);
            }
        }
        // close scan
        if ((rc = indexScan.CloseScan())) {
            return rc;
        }
        if ((rc = ixManager.CloseIndex(indexHandle))) {
            return rc;
        }
    }
    return OK_RC;
}

RC QL_Manager::GetDataSet(const RelCat& relCat, RM_FileHandle& rmFileHandle, const std::vector<FullCondition>& fullConditions, std::vector<char*>& data) {
    RC rc;
    // the condition with index
    int index = -1;
    for (unsigned int i = 0; i < fullConditions.size(); ++i) {
        if (fullConditions[i].lhsAttr.indexNo >= 0 && fullConditions[i].bRhsIsAttr == 0) {
            index = i;
            break;
        }
    }
    // scan the rids
    if (index == -1) {
        // use only rm scan
        RM_FileScan rmFileScan;
        if ((rc = rmFileScan.OpenScan(rmFileHandle, fullConditions))) {
            return rc;
        }
        RM_Record record;
        RID rid;
        while (true) {
            if ((rc = rmFileScan.GetNextRec(record)) && rc != RM_EOF) {
                return rc;
            }
            if (rc == RM_EOF) {
                break;
            }
            char *tmp;
            if ((rc = record.GetData(tmp))) {
                return rc;
            }
            char *d = new char[relCat.tupleLength];
            memcpy(d, tmp, relCat.tupleLength);
            data.push_back(d);
        }
        if ((rc = rmFileScan.CloseScan())) {
            return rc;
        }
    } else {
        // use index scan and rm scan
        // open index scan
        IX_IndexHandle indexHandle;
        if ((rc = ixManager.OpenIndex(relCat.relName, fullConditions[index].lhsAttr.indexNo, indexHandle))) {
            return rc;
        }
        IX_IndexScan indexScan;
        if ((rc = indexScan.OpenScan(indexHandle, fullConditions[index].op, fullConditions[index].rhsValue.data))) {
            return rc;
        }
        // scan the rid
        while (true) {
            RID rid;
            if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
                return rc;
            }
            if (rc == IX_EOF) {
                break;
            }
            RM_Record record;
            if ((rc = rmFileHandle.GetRec(rid, record))) {
                return rc;
            }
            char *recordData;
            if ((rc = record.GetData(recordData))) {
                return rc;
            }
            bool result;
            if ((rc = CheckFullConditions(recordData, fullConditions, result))) {
                return rc;
            }
            if (result) {
                char *d = new char[relCat.tupleLength];
                memcpy(d, recordData, relCat.tupleLength);
                data.push_back(d);
            }
        }
        // close scan
        if ((rc = indexScan.CloseScan())) {
            return rc;
        }
        if ((rc = ixManager.CloseIndex(indexHandle))) {
            return rc;
        }
    }
    return OK_RC;
}

RC QL_Manager::CheckFullConditions(char* recordData, const std::vector<FullCondition>& fullConditions, bool& result) {
    result = true;
    for (unsigned int i = 0; result && i < fullConditions.size(); ++i) {
        result = result && Attr::CompareAttr(fullConditions[i].lhsAttr.attrType, fullConditions[i].lhsAttr.attrLength, recordData + fullConditions[i].lhsAttr.offset, fullConditions[i].op, fullConditions[i].bRhsIsAttr ? recordData + fullConditions[i].rhsAttr.offset : fullConditions[i].rhsValue.data);
    }
    return OK_RC;
}

bool QL_Manager::CheckFullCondition(char* aData, char* bData, const std::vector<FullCondition>& conditions) {
    bool ret = true;
    for (unsigned int i = 0; ret && i < conditions.size(); ++i) {
        ret = ret && Attr::CompareAttr(conditions[i].lhsAttr.attrType, conditions[i].lhsAttr.attrLength, aData + conditions[i].lhsAttr.offset, conditions[i].op, bData + conditions[i].rhsAttr.offset);
    }
    return ret;
}

RC QL_Manager::GetJoinData(std::map<RelCat, std::vector<char*>>& data, std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>>& binaryRelConds, std::vector<std::map<RelCat, char*>>& joinData) {
    std::set<RelCat> rels;
    for (const auto& conditions : binaryRelConds) {
        const RelCat& aRelCat = conditions.first.first;
        const RelCat& bRelCat = conditions.first.second;
        auto aIter = rels.find(aRelCat);
        auto bIter = rels.find(bRelCat);
        if (aIter == rels.end() && bIter == rels.end()) {
            std::vector<std::map<RelCat, char*>> tmp;
            for (auto& aData : data[aRelCat]) {
                for (auto& bData : data[bRelCat]) {
                    if (CheckFullCondition(aData, bData, conditions.second)) {
                        tmp.push_back(std::map<RelCat, char*>{{aRelCat, aData}, {bRelCat, bData}});
                    }
                }
            }
            std::vector<std::map<RelCat, char*>> tmpJoin;
            for (const auto& a : joinData) {
                for (const auto& b : tmp) {
                    std::map<RelCat, char*> join = a;
                    for (const auto& i : b) {
                        join.insert(i);
                    }
                    tmpJoin.emplace_back(join);
                }
            }
            joinData = tmpJoin;
            rels.insert(aRelCat);
            rels.insert(bRelCat);
        } else if (aIter == rels.end() && bIter != rels.end()) {
            std::vector<std::map<RelCat, char*>> tmpJoin;
            for (auto& b : joinData) {
                for (auto& a : data[aRelCat]) {
                    if (CheckFullCondition(b[bRelCat], a, conditions.second)) {
                        std::map<RelCat, char*> join = b;
                        join.insert(std::make_pair(aRelCat, a));
                        tmpJoin.emplace_back(join);
                    }
                }
            }
            joinData = tmpJoin;
            rels.insert(aRelCat);
        } else if (aIter != rels.end() && bIter == rels.end()) {
            std::vector<std::map<RelCat, char*>> tmpJoin;
            for (auto& a : joinData) {
                for (auto& b : data[bRelCat]) {
                    if (CheckFullCondition(a[aRelCat], b, conditions.second)) {
                        std::map<RelCat, char*> join = a;
                        join.insert(std::make_pair(bRelCat, b));
                        tmpJoin.emplace_back(join);
                    }
                }
            }
            joinData = tmpJoin;
            rels.insert(bRelCat);
        }
    }
    for (const auto& d : data) {
        const RelCat& relCat = d.first;
        auto iter = rels.find(relCat);
        if (iter == rels.end()) {
            std::vector<std::map<RelCat, char*>> tmpJoin;
            for (const auto& a : joinData) {
                for (const auto& b : d.second) {
                    std::map<RelCat, char*> join = a;
                    join.insert(std::make_pair(relCat, b));
                    tmpJoin.emplace_back(join);
                }
            }
            joinData = tmpJoin;
            rels.insert(relCat);
        }
    }
    return OK_RC;
}

void QL_PrintError(RC rc) {
    cout << "QL_PrintError\n   rc=" << rc << "\n";
}
