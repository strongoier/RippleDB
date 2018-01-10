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
#include <limits>
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

RC QL_Manager::SelectFunc(FuncType func, const RelAttr relAttrFunc, int nRelations, const char * const relations[], int nConditions, Condition conditions[]) {
    RC rc;
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
    if ((rc = CheckAttrCats(relAttrFunc, relCats, attrs))) {
        return rc;
    }
    if (attrs.begin()->second.begin()->attrType != INT && attrs.begin()->second.begin()->attrType != FLOAT) {
        return QL_ATTRTYPEWRONG;
    }
    // check conditions
    std::map<RelCat, std::vector<FullCondition>> singalRelConds;
    std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>> binaryRelConds;
    for (int i = 0; i < nConditions; ++i) {
        if ((rc = GetFullCondition(conditions[i], relCats, singalRelConds, binaryRelConds))) {
            return rc;
        }
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
    // join
    std::vector<std::map<RelCat, char*>> joinData{std::map<RelCat, char*>()};
    if ((rc = GetJoinData(data, binaryRelConds, joinData))) {
        return rc;
    }
    // print
    DataAttrInfo* attributes = new DataAttrInfo[1];
    int index = 0;
    int tupleLength = 0;
    for (const auto& item : attrs) {
        for (const auto& attr : item.second) {
            strcpy(attributes[index].relName, attr.relName);
            strcpy(attributes[index].attrName, attr.attrName);
            attributes[index].offset = tupleLength;
            attributes[index].attrType = attr.attrType;
            attributes[index].attrLength = attr.attrLength;
            attributes[index].indexNo = attr.indexNo;
            tupleLength += attr.attrLength + 1;
            ++index;
        }
    }
    Printer printer(attributes, 1);
    printer.PrintHeader(cout);
    char *tuple = new char[tupleLength];
    *tuple = 1;
    // 聚集
    RelCat funcRel = attrs.begin()->first;
    AttrCat funcAttr = *(attrs.begin()->second.begin());
    int nullCount = 0;
    if (attributes[0].attrType == FLOAT) {
        switch (func) {
            case SUM: {
                float result = 0;
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    float tmp = *(float*)(item[funcRel] + funcAttr.offset + 1);
                    result += tmp;
                }
                memcpy(tuple + 1, &result, 4);
                break;
            }
            case AVG: {
                float result = 0;
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    float tmp = *(float*)(item[funcRel] + funcAttr.offset + 1);
                    result += tmp;
                }
                result /= joinData.size();
                memcpy(tuple + 1, &result, 4);
                break;
            }
            case MAX: {
                float result = (numeric_limits<float>::min)();
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    float tmp = *(float*)(item[funcRel] + funcAttr.offset + 1);
                    if (tmp > result)
                        result = tmp;
                }
                memcpy(tuple + 1, &result, 4);
                break;
            }
            case MIN: {
                float result = (numeric_limits<float>::max)();
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    float tmp = *(float*)(item[funcRel] + funcAttr.offset + 1);
                    if (tmp < result)
                        result = tmp;
                }
                memcpy(tuple + 1, &result, 4);
                break;
            }
        }
    } else if (attributes[0].attrType == INT) {
        switch (func) {
            case SUM: {
                int result = 0;
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    int tmp = *(int*)(item[funcRel] + funcAttr.offset + 1);
                    result += tmp;
                }
                memcpy(tuple + 1, &result, 4);
                break;
            }
            case AVG: {
                int result = 0;
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    int tmp = *(int*)(item[funcRel] + funcAttr.offset + 1);
                    result += tmp;
                }
                result /= joinData.size();
                memcpy(tuple + 1, &result, 4);
                break;
            }
            case MAX: {
                int result = (numeric_limits<float>::min)();
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    int tmp = *(int*)(item[funcRel] + funcAttr.offset + 1);
                    if (tmp > result)
                        result = tmp;
                }
                memcpy(tuple + 1, &result, 4);
                break;
            }
            case MIN: {
                int result = (numeric_limits<float>::max)();
                for (auto& item : joinData) {
                    if (*(char*)(item[funcRel] + funcAttr.offset) == 0) {
                        ++nullCount;
                        continue;
                    }
                    int tmp = *(int*)(item[funcRel] + funcAttr.offset + 1);
                    if (tmp < result)
                        result = tmp;
                }
                memcpy(tuple + 1, &result, 4);
                break;
            }
        }
    }
    if (nullCount != joinData.size())
        printer.Print(cout, tuple);
    delete[] tuple;
    printer.PrintFooter(cout);
    
    for (auto& item : data) {
        for (auto & i : item.second) {
            delete[] i;
        }
    }

    // print
    /*cout << "Select\n";
    cout << "   SelAttr = " << relAttrFunc << "\n";
    cout << "   nRelations = " << nRelations << "\n";
    for (int i = 0; i < nRelations; i++)
        cout << "   relations[" << i << "] " << relations[i] << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";*/

    return 0;
}

RC QL_Manager::SelectGroup(FuncType func, const RelAttr relAttrFunc, const RelAttr relAttrGroup, int nRelations, const char * const relations[], int nConditions, Condition conditions[]) {
    RC rc;
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
    if ((rc = CheckAttrCats(relAttrFunc, relCats, attrs))) {
        return rc;
    }
    std::map<RelCat, std::vector<AttrCat>> groupAttrs;
    if ((rc = CheckAttrCats(relAttrGroup, relCats, groupAttrs))) {
        return rc;
    }
    if (attrs.begin()->second.begin()->attrType != INT && attrs.begin()->second.begin()->attrType != FLOAT) {
        return QL_ATTRTYPEWRONG;
    }
    // check conditions
    std::map<RelCat, std::vector<FullCondition>> singalRelConds;
    std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>> binaryRelConds;
    for (int i = 0; i < nConditions; ++i) {
        if ((rc = GetFullCondition(conditions[i], relCats, singalRelConds, binaryRelConds))) {
            return rc;
        }
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
    // join
    std::vector<std::map<RelCat, char*>> joinData{std::map<RelCat, char*>()};
    if ((rc = GetJoinData(data, binaryRelConds, joinData))) {
        return rc;
    }
    // print
    DataAttrInfo* attributes = new DataAttrInfo[2];
    int index = 0;
    int tupleLength = 0;
    for (const auto& item : groupAttrs) {
        for (const auto& attr : item.second) {
            strcpy(attributes[index].relName, attr.relName);
            strcpy(attributes[index].attrName, attr.attrName);
            attributes[index].offset = tupleLength;
            attributes[index].attrType = attr.attrType;
            attributes[index].attrLength = attr.attrLength;
            attributes[index].indexNo = attr.indexNo;
            tupleLength += attr.attrLength + 1;
            ++index;
        }
    }
    for (const auto& item : attrs) {
        for (const auto& attr : item.second) {
            strcpy(attributes[index].relName, attr.relName);
            strcpy(attributes[index].attrName, attr.attrName);
            attributes[index].offset = tupleLength;
            attributes[index].attrType = attr.attrType;
            attributes[index].attrLength = attr.attrLength;
            attributes[index].indexNo = attr.indexNo;
            tupleLength += attr.attrLength + 1;
            ++index;
        }
    }
    Printer printer(attributes, 2);
    printer.PrintHeader(cout);
    char *tuple = new char[tupleLength];
    // 聚集分组
    RelCat funcRel = attrs.begin()->first;
    AttrCat funcAttr = *(attrs.begin()->second.begin());
    RelCat groupRel = groupAttrs.begin()->first;
    AttrCat groupAttr = *(groupAttrs.begin()->second.begin());
    switch (groupAttr.attrType) {
        case INT: {
            switch (funcAttr.attrType) {
                case INT: {
                    map<int, int> group;
                    switch (func) {
                        case SUM: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcInt;
                                } else {
                                    group[groupInt] += funcInt;
                                }
                            }
                            break;
                        }
                        case AVG: {
                            map<int, int> count;
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcInt;
                                    count[groupInt] = 1;
                                } else {
                                    group[groupInt] += funcInt;
                                    count[groupInt] += 1;
                                }
                            }
                            for (auto& item : group) {
                                item.second /= count[item.first];
                            }
                            break;
                        }
                        case MIN: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcInt;
                                } else {
                                    if (funcInt < group[groupInt])
                                        group[groupInt] = funcInt;
                                }
                            }
                            break;
                        }
                        case MAX: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcInt;
                                } else {
                                    if (funcInt > group[groupInt])
                                        group[groupInt] = funcInt;
                                }
                            }
                            break;
                        }
                    }
                    for (const auto & item : group) {
                        memset(tuple, 0, tupleLength);
                        *(char*)(tuple + attributes[0].offset) = 1;
                        *(int*)(tuple + attributes[0].offset + 1) = item.first;
                        *(char*)(tuple + attributes[1].offset) = 1;
                        *(int*)(tuple + attributes[1].offset + 1) = item.second;
                        printer.Print(cout, tuple);
                    }
                    break;
                }
                case FLOAT: {
                    map<int, float> group;
                    switch (func) {
                        case SUM: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcFloat;
                                } else {
                                    group[groupInt] += funcFloat;
                                }
                            }
                            break;
                        }
                        case AVG: {
                            map<int, int> count;
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcFloat;
                                    count[groupInt] = 1;
                                } else {
                                    group[groupInt] += funcFloat;
                                    count[groupInt] += 1;
                                }
                            }
                            for (auto& item : group) {
                                item.second /= count[item.first];
                            }
                            break;
                        }
                        case MIN: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcFloat;
                                } else {
                                    if (funcFloat < group[groupInt])
                                        group[groupInt] = funcFloat;
                                }
                            }
                            break;
                        }
                        case MAX: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                int groupInt = *(int*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupInt);
                                if (iter == group.end()) {
                                    group[groupInt] = funcFloat;
                                } else {
                                    if (funcFloat > group[groupInt])
                                        group[groupInt] = funcFloat;
                                }
                            }
                            break;
                        }
                    }
                    for (const auto & item : group) {
                        memset(tuple, 0, tupleLength);
                        *(char*)(tuple + attributes[0].offset) = 1;
                        *(int*)(tuple + attributes[0].offset + 1) = item.first;
                        *(char*)(tuple + attributes[1].offset) = 1;
                        *(float*)(tuple + attributes[1].offset + 1) = item.second;
                        printer.Print(cout, tuple);
                    }
                    break;
                }
            }
            break;
        }
        case FLOAT: {
            switch (funcAttr.attrType) {
                case INT: {
                    map<float, int> group;
                    switch (func) {
                        case SUM: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcInt;
                                } else {
                                    group[groupFloat] += funcInt;
                                }
                            }
                            break;
                        }
                        case AVG: {
                            map<int, int> count;
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcInt;
                                    count[groupFloat] = 1;
                                } else {
                                    group[groupFloat] += funcInt;
                                    count[groupFloat] += 1;
                                }
                            }
                            for (auto& item : group) {
                                item.second /= count[item.first];
                            }
                            break;
                        }
                        case MIN: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcInt;
                                } else {
                                    if (funcInt < group[groupFloat])
                                        group[groupFloat] = funcInt;
                                }
                            }
                            break;
                        }
                        case MAX: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcInt;
                                } else {
                                    if (funcInt > group[groupFloat])
                                        group[groupFloat] = funcInt;
                                }
                            }
                            break;
                        }
                    }
                    for (const auto & item : group) {
                        memset(tuple, 0, tupleLength);
                        *(char*)(tuple + attributes[0].offset) = 1;
                        *(float*)(tuple + attributes[0].offset + 1) = item.first;
                        *(char*)(tuple + attributes[1].offset) = 1;
                        *(int*)(tuple + attributes[1].offset + 1) = item.second;
                        printer.Print(cout, tuple);
                    }
                    break;
                }
                case FLOAT: {
                    map<float, float> group;
                    switch (func) {
                        case SUM: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcFloat;
                                } else {
                                    group[groupFloat] += funcFloat;
                                }
                            }
                            break;
                        }
                        case AVG: {
                            map<float, int> count;
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcFloat;
                                    count[groupFloat] = 1;
                                } else {
                                    group[groupFloat] += funcFloat;
                                    count[groupFloat] += 1;
                                }
                            }
                            for (auto& item : group) {
                                item.second /= count[item.first];
                            }
                            break;
                        }
                        case MIN: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcFloat;
                                } else {
                                    if (funcFloat < group[groupFloat])
                                        group[groupFloat] = funcFloat;
                                }
                            }
                            break;
                        }
                        case MAX: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                float groupFloat = *(float*)(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupFloat);
                                if (iter == group.end()) {
                                    group[groupFloat] = funcFloat;
                                } else {
                                    if (funcFloat > group[groupFloat])
                                        group[groupFloat] = funcFloat;
                                }
                            }
                            break;
                        }
                    }
                    for (const auto & item : group) {
                        memset(tuple, 0, tupleLength);
                        *(char*)(tuple + attributes[0].offset) = 1;
                        *(float*)(tuple + attributes[0].offset + 1) = item.first;
                        *(char*)(tuple + attributes[1].offset) = 1;
                        *(float*)(tuple + attributes[1].offset + 1) = item.second;
                        printer.Print(cout, tuple);
                    }
                    break;
                }
            }
            break;
        }
        case STRING:
        case DATE: {
            switch (funcAttr.attrType) {
                case INT: {
                    map<string, int> group;
                    switch (func) {
                        case SUM: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcInt;
                                } else {
                                    group[groupString] += funcInt;
                                }
                            }
                            break;
                        }
                        case AVG: {
                            map<string, int> count;
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcInt;
                                    count[groupString] = 1;
                                } else {
                                    group[groupString] += funcInt;
                                    count[groupString] += 1;
                                }
                            }
                            for (auto& item : group) {
                                item.second /= count[item.first];
                            }
                            break;
                        }
                        case MIN: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcInt;
                                } else {
                                    if (funcInt < group[groupString])
                                        group[groupString] = funcInt;
                                }
                            }
                            break;
                        }
                        case MAX: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                int funcInt = *(int*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcInt;
                                } else {
                                    if (funcInt > group[groupString])
                                        group[groupString] = funcInt;
                                }
                            }
                            break;
                        }
                    }
                    for (const auto & item : group) {
                        memset(tuple, 0, tupleLength);
                        *(char*)(tuple + attributes[0].offset) = 1;
                        strcpy(tuple + attributes[0].offset + 1, item.first.c_str());
                        *(char*)(tuple + attributes[1].offset) = 1;
                        *(int*)(tuple + attributes[1].offset + 1) = item.second;
                        printer.Print(cout, tuple);
                    }
                    break;
                }
                case FLOAT: {
                    map<string, float> group;
                    switch (func) {
                        case SUM: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcFloat;
                                } else {
                                    group[groupString] += funcFloat;
                                }
                            }
                            break;
                        }
                        case AVG: {
                            map<string, int> count;
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcFloat;
                                    count[groupString] = 1;
                                } else {
                                    group[groupString] += funcFloat;
                                    count[groupString] += 1;
                                }
                            }
                            for (auto& item : group) {
                                item.second /= count[item.first];
                            }
                            break;
                        }
                        case MIN: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcFloat;
                                } else {
                                    if (funcFloat < group[groupString])
                                        group[groupString] = funcFloat;
                                }
                            }
                            break;
                        }
                        case MAX: {
                            for (auto & item : joinData) {
                                if (*(char*)(item[groupRel] + groupAttr.offset) == 0)
                                    continue;
                                if (*(char*)(item[funcRel] + funcAttr.offset) == 0)
                                    continue;
                                string groupString(item[groupRel] + groupAttr.offset + 1);
                                float funcFloat = *(float*)(item[funcRel] + funcAttr.offset + 1);
                                auto iter = group.find(groupString);
                                if (iter == group.end()) {
                                    group[groupString] = funcFloat;
                                } else {
                                    if (funcFloat > group[groupString])
                                        group[groupString] = funcFloat;
                                }
                            }
                            break;
                        }
                    }
                    for (const auto & item : group) {
                        memset(tuple, 0, tupleLength);
                        *(char*)(tuple + attributes[0].offset) = 1;
                        strcpy(tuple + attributes[0].offset + 1, item.first.c_str());
                        *(char*)(tuple + attributes[1].offset) = 1;
                        *(float*)(tuple + attributes[1].offset + 1) = item.second;
                        printer.Print(cout, tuple);
                    }
                    break;
                }
            }
            break;
        }
    }
    delete[] tuple;
    printer.PrintFooter(cout);
    
    for (auto& item : data) {
        for (auto & i : item.second) {
            delete[] i;
        }
    }

    // print
    /*cout << "Select\n";
    cout << "   SelAttr = " << relAttrFunc << "\n";
    cout << "   nRelations = " << nRelations << "\n";
    for (int i = 0; i < nRelations; i++)
        cout << "   relations[" << i << "] " << relations[i] << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";*/

    return 0;
}

//
// Handle the select clause
//
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[], int nRelations, const char * const relations[], int nConditions, Condition conditions[]) {
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
    // check conditions
    std::map<RelCat, std::vector<FullCondition>> singalRelConds;
    std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>> binaryRelConds;
    for (int i = 0; i < nConditions; ++i) {
        if ((rc = GetFullCondition(conditions[i], relCats, singalRelConds, binaryRelConds))) {
            return rc;
        }
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
    // join
    std::vector<std::map<RelCat, char*>> joinData{std::map<RelCat, char*>()};
    if ((rc = GetJoinData(data, binaryRelConds, joinData))) {
        return rc;
    }
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
            attributes[index].indexNo = attr.indexNo;
            tupleLength += attr.attrLength + 1;
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
                memcpy(tuple + pos, item[rel.first] + attr.offset, attr.attrLength + 1);
                pos += attr.attrLength + 1;
            }
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
    /*cout << "Select\n";
    cout << "   nSelAttrs = " << nSelAttrs << "\n";
    for (int i = 0; i < nSelAttrs; i++)
        cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";
    cout << "   nRelations = " << nRelations << "\n";
    for (int i = 0; i < nRelations; i++)
        cout << "   relations[" << i << "] " << relations[i] << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";*/

    return 0;
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName, int nValues, Value values[]) {
    RC rc;
    // 检查数据库是否打开
    if ((rc = CheckSMManagerIsOpen())) {
        return rc;
    }
    // 检查数据表是否存在
    RelCat relCat;
    if ((rc = CheckRelCat(relName, relCat))) {
        return rc;
    }
    // 检查属性个数是否一致
    if (nValues != relCat.attrCount) {
        return QL_ATTRSNUMBERWRONG;
    }
    // 获取属性详细信息
    vector<AttrCat> attrs;
    if ((rc = smManager.GetAttrs(relName, attrs))) {
        return rc;
    }
    // 判断主键个数
    int primaryKeyCount = 0;
    int primaryKeyTupleLength = 0;
    for (const auto& attr: attrs) {
        if (attr.primaryKey > 0) {
            primaryKeyTupleLength += attr.attrLength;
            if (attr.primaryKey > primaryKeyCount)
                primaryKeyCount = attr.primaryKey;
        }
    }
    // 检查属性信息
    for (int i = 0; i < nValues; ++i) {
        // 判断非空属性是否一致
        if (attrs[i].isNotNull && *(char*)(values[i].data) == 0) {
            return QL_ATTRISNULL;
        }
        // 如果值为空，将值的类型转化为相应类型
        if (*(char*)(values[i].data) == 0) {
            values[i].type = attrs[i].attrType;
        }
        // 判断int给了float
        if (attrs[i].attrType == FLOAT && values[i].type == INT) {
            values[i].type = FLOAT;
            *(float*)((char*)values[i].data + 1) = *(int*)((char*)values[i].data + 1);
        }
        // 判断日期类型是否合法
        if (attrs[i].attrType == DATE && values[i].type == STRING) {
            values[i].type = DATE;
            if (strlen((char*)values[i].data + 1) != 10) {
                return QL_DATEFORMATERROR;
            }
            for (int j = 1; j <= 10; ++j) {
                char c = *((char*)values[i].data + j);
                if (j == 5 || j == 8) {
                    if (c != '-') return QL_DATEFORMATERROR;
                } else {
                    if (c < '0' || c > '9') return QL_DATEFORMATERROR;
                }
            }
            int yyyy, mm, dd;
            sscanf((char*)values[i].data + 1, "%d-%d-%d", &yyyy, &mm, &dd);
            if (dd < 1 || mm < 1 || mm > 12) return QL_DATEFORMATERROR;
            int end2 = yyyy % 4 == 0 && yyyy % 100 != 0 || yyyy % 400 == 0 ? 29 : 28;            
            if (mm == 2 && dd > end2) return QL_DATEFORMATERROR;
            if ((mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) && dd > 31) return QL_DATEFORMATERROR;
            if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && dd > 30) return QL_DATEFORMATERROR;
        }
        // 判断类型是否一致
        if (values[i].type != attrs[i].attrType) {
            return QL_ATTRTYPEWRONG;
        }
        // 判断字符串类型长度是否合法
        if (attrs[i].attrType == STRING && strlen((char*)(values[i].data) + 1) >= attrs[i].attrLength) {
            return QL_STRINGLENGTHWRONG;
        }
        // 判断单主键是否重复
        if (attrs[i].primaryKey > 0 && primaryKeyCount == 1) {
            IX_IndexHandle indexHandle;
            if ((rc = ixManager.OpenIndex(relName, attrs[i].indexNo, indexHandle))) {
                return rc;
            }
            IX_IndexScan indexScan;
            if ((rc = indexScan.OpenScan(indexHandle, EQ_OP, values[i].data))) {
                return rc;
            }
            RID rid;
            // 如果重复，报错
            if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
                return rc;
            }
            if (rc != IX_EOF) {
                return QL_PRIMARYKEYREPEAT;
            }
            if ((rc = indexScan.CloseScan())) {
                return rc;
            }
            if ((rc = ixManager.CloseIndex(indexHandle))) {
                return rc;
            }
        }
        // 判断外键合法性
        if (attrs[i].refrel[0] != 0) {
            // 获取外键属性详细信息
            vector<AttrCat> refAttrs;
            if ((rc = smManager.GetAttrs(attrs[i].refrel, refAttrs))) {
                return rc;
            }
            auto iter = find_if(refAttrs.begin(), refAttrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, attrs[i].refattr) == 0; });
            IX_IndexHandle indexHandle;
            if ((rc = ixManager.OpenIndex(attrs[i].refrel, iter->indexNo, indexHandle))) {
                return rc;
            }
            IX_IndexScan indexScan;
            if ((rc = indexScan.OpenScan(indexHandle, EQ_OP, values[i].data))) {
                return rc;
            }
            RID rid;
            // 如果不存在，报错
            if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
                return rc;
            }
            if (rc == IX_EOF) {
                return QL_FOREIGNKEYNOTEXIST;
            }
            if ((rc = indexScan.CloseScan())) {
                return rc;
            }
            if ((rc = ixManager.CloseIndex(indexHandle))) {
                return rc;
            }
        }
    }
    // 判断多重主键是否重复
    if (primaryKeyCount > 1) {
        // 清零 ！！！
        char *key = new char[primaryKeyTupleLength];
        memset(key, 0, primaryKeyTupleLength);
        // 构造
        int offset = 0;
        for (int i = 1; i <= primaryKeyCount; ++i) {
            for (int j = 0; j < nValues; ++j) {
                if (attrs[j].primaryKey == i) {
                    memcpy(key + offset, values[j].data, attrs[j].attrLength + 1);
                    offset += attrs[j].attrLength + 1;
                    break;
                }
            }
        }
        IX_IndexHandle indexHandle;
        if ((rc = ixManager.OpenIndex(relName, 0, indexHandle))) {
            return rc;
        }
        IX_IndexScan indexScan;
        if ((rc = indexScan.OpenScan(indexHandle, EQ_OP, key))) {
            return rc;
        }
        RID rid;
        // 如果重复，报错
        if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
            return QL_PRIMARYKEYREPEAT;
        }
        if ((rc = indexScan.CloseScan())) {
            return rc;
        }
        // 插入
        if ((rc = indexHandle.InsertEntry(key, RID(0, 0)))) {
            return rc;
        }
        if ((rc = ixManager.CloseIndex(indexHandle))) {
            return rc;
        }
        delete[] key;
    }
    // 构造记录数据
    char *tuple = new char[relCat.tupleLength];
    for (int i = 0; i < nValues; ++i) {
        memcpy(tuple + attrs[i].offset, values[i].data, attrs[i].attrLength + 1);
    }
    // 插入到记录文件
    RM_FileHandle relFileHandle;
    if ((rc = rmManager.OpenFile(relName, relFileHandle))) {
        return rc;
    }
    RID rid;
    if ((rc = relFileHandle.InsertRec(tuple, rid))) {
        return rc;
    }
    if ((rc = rmManager.CloseFile(relFileHandle))) {
        return rc;
            return 2;
    }
    // 插入到索引文件
    IX_IndexHandle relIndexHandle;
    for (int i = 0; i < nValues; ++i) {
        // 判断该属性是否有索引
        if (attrs[i].indexNo != -1) {
            // 按照是否为空插入不同的索引中
            int indexNo = *(char*)(values[i].data) == 0 ? attrs[i].indexNo + 1 : attrs[i].indexNo;
            if ((rc = ixManager.OpenIndex(relName, indexNo, relIndexHandle))) {
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
    /*cout << "Insert\n";
    cout << "   relName = " << relName << "\n";
    cout << "   nValues = " << nValues << "\n";
    for (int i = 0; i < nValues; i++)
        cout << "   values[" << i << "]:" << values[i] << "\n";
    
    cout << "\n";*/
    /*if ((rc = smManager.Print(relName))) {
        return rc;
    }*/

    return 0;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName, int nConditions, Condition conditions[]) {
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
    // 判断主键个数
    int primaryKeyCount = 0;
    int primaryKeyTupleLength = 0;
    for (const auto& attr: attrs) {
        if (attr.primaryKey > 0) {
            primaryKeyTupleLength += attr.attrLength;
            if (attr.primaryKey > primaryKeyCount)
                primaryKeyCount = attr.primaryKey;
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
    // delete indexs
    for (const auto &attr : attrs) {
        if (attr.indexNo != -1) {
            IX_IndexHandle indexHandle;
            if ((rc = ixManager.OpenIndex(relName, attr.indexNo, indexHandle))) {
                return rc;
            }
            IX_IndexHandle nullHandle;
            if ((rc = ixManager.OpenIndex(relName, attr.indexNo + 1, nullHandle))) {
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
                if (*(char*)(recordData + attr.offset) == 0) {
                    if ((rc = nullHandle.DeleteEntry(recordData + attr.offset, rid))) {
                        return rc;
                    }
                } else {
                    if ((rc = indexHandle.DeleteEntry(recordData + attr.offset, rid))) {
                        return rc;
                    }
                }
            }
            if ((rc = ixManager.CloseIndex(indexHandle))) {
                return rc;
            }
            if ((rc = ixManager.CloseIndex(nullHandle))) {
                return rc;
            }
        }
    }
    // 如果有多重主键的话，删除多重主键
    if (primaryKeyCount > 1) {
        IX_IndexHandle primaryHandle;
        if ((rc = ixManager.OpenIndex(relName, 0, primaryHandle))) {
            return rc;
        }
        char *key = new char[primaryKeyTupleLength];
        for (const auto& rid : rids) {
            RM_Record record;
            if ((rc = rmFileHandle.GetRec(rid, record))) {
                return rc;
            }
            char *recordData;
            if ((rc = record.GetData(recordData))) {
                return rc;
            }
            // !!!
            memset(key, 0, primaryKeyTupleLength);
            // 构造
            int offset = 0;
            for (int i = 1; i <= primaryKeyCount; ++i) {
                for (int j = 0; j < attrs.size(); ++j) {
                    if (attrs[j].primaryKey == i) {
                        memcpy(key + offset, recordData + attrs[j].offset, attrs[j].attrLength + 1);
                        offset += attrs[j].attrLength + 1;
                        break;
                    }
                }
            }
            if ((rc = primaryHandle.DeleteEntry(key, RID(0, 0)))) {
                return rc;
            }
        }
        delete[] key;
        if ((rc = ixManager.CloseIndex(primaryHandle))) {
            return rc;
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
    /*cout << "Delete\n";
    cout << "   relName = " << relName << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    
    cout << "\n";*/
    //if ((rc = smManager.Print(relName))) {
    //    return rc;
    //}

    return 0;
}

//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName, int nSetters, const RelAttr updAttrs[], Value rhsValues[], int nConditions, Condition conditions[]) {
    RC rc;
    // 判断数据库是否被打开
    if ((rc = CheckSMManagerIsOpen())) {
        return rc;
    }
    // 获取数据表信息
    RelCat relCat;
    if ((rc = CheckRelCat(relName, relCat))) {
        return rc;
    }
    // 获取数据表中的属性值信息
    vector<AttrCat> attrs;
    if ((rc = smManager.GetAttrs(relName, attrs))) {
        return rc;
    }
    // 判断主键个数
    int primaryKeyCount = 0;
    int primaryKeyTupleLength = 0;
        for (const auto& attr: attrs) {
            if (attr.primaryKey > 0) {
                primaryKeyTupleLength += attr.attrLength;
                if (attr.primaryKey > primaryKeyCount)
                    primaryKeyCount = attr.primaryKey;
            }
        }
    // 检查被更新属性是否合法，同时判断是否影响主键
    int primaryKeyModifyCount = 0;
    vector<AttrCat>::iterator *iters = new vector<AttrCat>::iterator[nSetters];
    for (int i = 0; i < nSetters; ++i) {
        iters[i] = find_if(attrs.begin(), attrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, updAttrs[i].attrName) == 0; });
        // 找不到，报错
        if (iters[i] == attrs.end()) {
            return QL_ATTRNOTFOUND;
        }
        // 判断非空属性是否一致
        if (iters[i]->isNotNull && *(char*)(rhsValues[i].data) == 0) {
            return QL_ATTRISNULL;
        }
        // 如果值为空，将值的类型转化为相应类型
        if (*(char*)(rhsValues[i].data) == 0) {
            rhsValues[i].type = iters[i]->attrType;
        }
        // 判断int给了float
        if (iters[i]->attrType == FLOAT && rhsValues[i].type == INT) {
            rhsValues[i].type = FLOAT;
            *(float*)((char*)rhsValues[i].data + 1) = *(int*)((char*)rhsValues[i].data + 1);
        }
        // 判断日期类型是否合法
        if (iters[i]->attrType == DATE && rhsValues[i].type == STRING) {
            rhsValues[i].type = DATE;
            if (strlen((char*)rhsValues[i].data + 1) != 10) {
                return QL_DATEFORMATERROR;
            }
            for (int j = 1; j <= 10; ++j) {
                char c = *((char*)rhsValues[i].data + j);
                if (j == 5 || j == 8) {
                    if (c != '-') return QL_DATEFORMATERROR;
                } else {
                    if (c < '0' || c > '9') return QL_DATEFORMATERROR;
                }
            }
            int yyyy, mm, dd;
            sscanf((char*)rhsValues[i].data + 1, "%d-%d-%d", &yyyy, &mm, &dd);
            if (dd < 1 || mm < 1 || mm > 12) return QL_DATEFORMATERROR;
            int end2 = yyyy % 4 == 0 && yyyy % 100 != 0 || yyyy % 400 == 0 ? 29 : 28;            
            if (mm == 2 && dd > end2) return QL_DATEFORMATERROR;
            if ((mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) && dd > 31) return QL_DATEFORMATERROR;
            if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && dd > 30) return QL_DATEFORMATERROR;
        }
        // 类型不一致，报错
        if (iters[i]->attrType != rhsValues[i].type) {
            cout << i << " " << iters[i]->attrType << " " << rhsValues[i].type << endl;
            return QL_ATTRTYPEWRONG;
        }
        // 判断字符串类型长度是否合法
        if (iters[i]->attrType == STRING && strlen((char*)(rhsValues[i].data) + 1) >= iters[i]->attrLength) {
            return QL_STRINGLENGTHWRONG;
        }
        // 判断单主键是否重复 更新时判断
        // 判断外键合法性
        if (iters[i]->refrel[0] != 0) {
            // 获取外键属性详细信息
            vector<AttrCat> refAttrs;
            if ((rc = smManager.GetAttrs(iters[i]->refrel, refAttrs))) {
                return rc;
            }
            auto iter = find_if(refAttrs.begin(), refAttrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, iters[i]->refattr) == 0; });
            IX_IndexHandle indexHandle;
            if ((rc = ixManager.OpenIndex(iters[i]->refrel, iter->indexNo, indexHandle))) {
                return rc;
            }
            IX_IndexScan indexScan;
            if ((rc = indexScan.OpenScan(indexHandle, EQ_OP, rhsValues[i].data))) {
                return rc;
            }
            RID rid;
            // 如果不存在，报错
            if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
                return rc;
            }
            if (rc == IX_EOF) {
                return QL_FOREIGNKEYNOTEXIST;
            }
            if ((rc = indexScan.CloseScan())) {
                return rc;
            }
            if ((rc = ixManager.CloseIndex(indexHandle))) {
                return rc;
            }
        }
        if (iters[i]->primaryKey > 0) {
            ++primaryKeyModifyCount;
        }
    }
    // 检查限制条件并补充信息
    vector<FullCondition> fullConditions;
    if ((rc = GetFullConditions(relName, attrs, nConditions, conditions, fullConditions))) {
        return rc;
    }
    // 获取被更新的记录集合
    RM_FileHandle rmFileHandle;
    if ((rc = rmManager.OpenFile(relName, rmFileHandle))) {
        return rc;
    }
    vector<RID> rids;
    if ((rc = GetRidSet(relName, rmFileHandle, fullConditions, rids))) {
        return rc;
    }
    if (rids.size() > 1 && primaryKeyModifyCount > 0) {
        return QL_PRIMARYKEYREPEAT;
    }
    // 更新索引
    for (int i = 0; i < nSetters; ++i) {
        if (iters[i]->indexNo != -1) {
            IX_IndexHandle indexHandle;
            if ((rc = ixManager.OpenIndex(relName, iters[i]->indexNo, indexHandle))) {
                return rc;
            }
            IX_IndexHandle nullHandle;
            if ((rc = ixManager.OpenIndex(relName, iters[i]->indexNo + 1, nullHandle))) {
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
                if (Attr::CompareAttr(iters[i]->attrType, iters[i]->attrLength, recordData + iters[i]->offset, EQ_OP, rhsValues[i].data)) {
                    continue;
                }
                if (iters[i]->primaryKey > 0) {
                    IX_IndexScan indexScan;
                    if ((rc = indexScan.OpenScan(indexHandle, EQ_OP, rhsValues[i].data))) {
                        return rc;
                    }
                    RID r;
                    if ((rc = indexScan.GetNextEntry(r)) && rc != IX_EOF) {
                        return rc;
                    }
                    if (rc != IX_EOF) {
                        return QL_PRIMARYKEYREPEAT;
                    }
                    if ((rc = indexScan.CloseScan())) {
                        return rc;
                    }
                }
                if (*(char*)(recordData + iters[i]->offset) == 0) {
                    if ((rc = nullHandle.DeleteEntry(recordData + iters[i]->offset, rid))) {
                        return rc;
                    }
                } else {
                    if ((rc = indexHandle.DeleteEntry(recordData + iters[i]->offset, rid))) {
                        return rc;
                    }
                }
                if (*(char*)(rhsValues[i].data) == 0) {
                    if ((rc = nullHandle.InsertEntry(rhsValues[i].data, rid))) {
                        return rc;
                    }
                } else {
                    if ((rc = indexHandle.InsertEntry(rhsValues[i].data, rid))) {
                        return rc;
                    }
                }
            }
            if ((rc = ixManager.CloseIndex(indexHandle))) {
                return rc;
            }
        }
    }
    // 如果有多重主键并被影响的话，更新多重主键
    char *key;
    IX_IndexHandle primaryHandle;
    if (primaryKeyCount > 1 && primaryKeyModifyCount > 0) {
        if ((rc = ixManager.OpenIndex(relName, 0, primaryHandle))) {
            return rc;
        }
        key = new char[primaryKeyTupleLength];
    }
    // 更新记录文件
    for (const auto &rid : rids) {
        RM_Record record;
        if ((rc = rmFileHandle.GetRec(rid, record))) {
            return rc;
        }
        char *recordData;
        if ((rc = record.GetData(recordData))) {
            return rc;
        }
        // 删除原有多重主键
        if (primaryKeyCount > 1 && primaryKeyModifyCount > 0) {
            // !!!
            memset(key, 0, primaryKeyTupleLength);
            // 构造
            int offset = 0;
            for (int i = 1; i <= primaryKeyCount; ++i) {
                for (int j = 0; j < attrs.size(); ++j) {
                    if (attrs[j].primaryKey == i) {
                        memcpy(key + offset, recordData + attrs[j].offset, attrs[j].attrLength + 1);
                        offset += attrs[j].attrLength + 1;
                        break;
                    }
                }
            }
            if ((rc = primaryHandle.DeleteEntry(key, RID(0, 0)))) {
                return rc;
            }
        }
        for (int i = 0; i < nSetters; ++i) {
            memcpy(recordData + iters[i]->offset, rhsValues[i].data, iters[i]->attrLength + 1);
        }
        // 插入多重主键
        if (primaryKeyCount > 1 && primaryKeyModifyCount > 0) {
            // !!!
            memset(key, 0, primaryKeyTupleLength);
            // 构造
            int offset = 0;
            for (int i = 1; i <= primaryKeyCount; ++i) {
                for (int j = 0; j < attrs.size(); ++j) {
                    if (attrs[j].primaryKey == i) {
                        memcpy(key + offset, recordData + attrs[j].offset, attrs[j].attrLength + 1);
                        offset += attrs[j].attrLength + 1;
                        break;
                    }
                }
            }
            if ((rc = primaryHandle.InsertEntry(key, RID(0, 0)))) {
                return rc;
            }
        }
        if ((rc = rmFileHandle.UpdateRec(record))) {
            return rc;
        }
    }
    if (primaryKeyCount > 1 && primaryKeyModifyCount > 0) {
        delete[] key;
        if ((rc = ixManager.CloseIndex(primaryHandle))) {
            return rc;
        }
    }
    if ((rc = rmManager.CloseFile(rmFileHandle))) {
        return rc;
    }
    delete[] iters;
    
    // print
    /*cout << "Update\n";
    cout << "   relName = " << relName << "\n";
    cout << "   nSetters = " << nSetters << "\n";
    for (int i = 0; i < nSetters; ++i) {
        cout << "   updAttrs[i]:" << updAttrs[i] << "\n";
        cout << "   rhs is value: " << rhsValues[i] << "\n";
    }
    cout << "   nCondtions = " << nConditions << "\n";
    for (int i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    
    cout << "\n";*/
    //if ((rc = smManager.Print(relName))) {
    //    return rc;
    //}

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
            if (cnt == 2) {
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
RC QL_Manager::GetFullConditions(const char* relName, const vector<AttrCat>& attrs, int nConditions, Condition conditions[], vector<FullCondition>& fullConditions) {
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
            // 如果值为空，将值的类型转化为相应类型
            if (*(char*)(conditions[i].rhsValue.data) == 0) {
                conditions[i].rhsValue.type = iter->attrType;
            }
            // 判断int给了float
            if (iter->attrType == FLOAT && conditions[i].rhsValue.type == INT) {
                conditions[i].rhsValue.type = FLOAT;
                *(float*)((char*)conditions[i].rhsValue.data + 1) = *(int*)((char*)conditions[i].rhsValue.data + 1);
            }
            // 判断日期类型是否合法
            if (iter->attrType == DATE && conditions[i].rhsValue.type == STRING) {
                conditions[i].rhsValue.type = DATE;
                if (strlen((char*)conditions[i].rhsValue.data + 1) != 10) {
                    return QL_DATEFORMATERROR;
                }
                for (int j = 1; j <= 10; ++j) {
                    char c = *((char*)conditions[i].rhsValue.data + j);
                    if (j == 5 || j == 8) {
                        if (c != '-') return QL_DATEFORMATERROR;
                    } else {
                        if (c < '0' || c > '9') return QL_DATEFORMATERROR;
                    }
                }
                int yyyy, mm, dd;
                sscanf((char*)conditions[i].rhsValue.data + 1, "%d-%d-%d", &yyyy, &mm, &dd);
                if (dd < 1 || mm < 1 || mm > 12) return QL_DATEFORMATERROR;
                int end2 = yyyy % 4 == 0 && yyyy % 100 != 0 || yyyy % 400 == 0 ? 29 : 28;            
                if (mm == 2 && dd > end2) return QL_DATEFORMATERROR;
                if ((mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) && dd > 31) return QL_DATEFORMATERROR;
                if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && dd > 30) return QL_DATEFORMATERROR;
            }
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
RC QL_Manager::GetFullCondition(Condition& condition, const std::map<RelCat, std::vector<AttrCat>>& relCats, std::map<RelCat, std::vector<FullCondition>>& singalRelConds, std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>>& binaryRelConds) {
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
        // 判断非空属性是否一致
        if (attrCat.isNotNull && *(char*)(condition.rhsValue.data) == 0) {
            return QL_ATTRISNULL;
        }
        // 如果值为空，将值的类型转化为相应类型
        if (*(char*)(condition.rhsValue.data) == 0) {
            condition.rhsValue.type = attrCat.attrType;
        }
        // 判断int给了float
        if (attrCat.attrType == FLOAT && condition.rhsValue.type == INT) {
            condition.rhsValue.type = FLOAT;
            *(float*)((char*)condition.rhsValue.data + 1) = *(int*)((char*)condition.rhsValue.data + 1);
        }
        // 判断日期类型是否合法
        if (attrCat.attrType == DATE && condition.rhsValue.type == STRING) {
            condition.rhsValue.type = DATE;
            if (strlen((char*)condition.rhsValue.data + 1) != 10) {
                return QL_DATEFORMATERROR;
            }
            for (int j = 1; j <= 10; ++j) {
                char c = *((char*)condition.rhsValue.data + j);
                if (j == 5 || j == 8) {
                    if (c != '-') return QL_DATEFORMATERROR;
                } else {
                    if (c < '0' || c > '9') return QL_DATEFORMATERROR;
                }
            }
            int yyyy, mm, dd;
            sscanf((char*)condition.rhsValue.data + 1, "%d-%d-%d", &yyyy, &mm, &dd);
            if (dd < 1 || mm < 1 || mm > 12) return QL_DATEFORMATERROR;
            int end2 = yyyy % 4 == 0 && yyyy % 100 != 0 || yyyy % 400 == 0 ? 29 : 28;            
            if (mm == 2 && dd > end2) return QL_DATEFORMATERROR;
            if ((mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) && dd > 31) return QL_DATEFORMATERROR;
            if ((mm == 4 || mm == 6 || mm == 9 || mm == 11) && dd > 30) return QL_DATEFORMATERROR;
        }
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
        // 生成完整限制条件（此时可能会切换左右属性顺序，保证左侧属性字典序小于等于右侧）
        fullCondition.lhsAttr = relCat < rhsRelCat ? attrCat : rhsAttrCat;
        fullCondition.op = condition.op;
        fullCondition.bRhsIsAttr = condition.bRhsIsAttr;
        fullCondition.rhsAttr = relCat < rhsRelCat ? rhsAttrCat : attrCat;
        if (relCat == rhsRelCat) {
            // 如果为单表限制条件，插入单表限制条件集合
            auto iter = singalRelConds.find(relCat);
            if (iter == singalRelConds.end()) {
                std::vector<FullCondition> tmp = {fullCondition};
                singalRelConds.insert(std::make_pair(relCat, tmp));
            } else {
                iter->second.push_back(fullCondition);
            }
        } else {
            // 如果为多表限制条件，修改比较操作符然后，插入多表限制条件集合
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

//
// 将比较操作符转换为优先级
//
int ToLevel(CompOp op) {
    switch (op) {
        case EQ_OP:
            return 0;
        case LT_OP:
        case GT_OP:
        case LE_OP:
        case GE_OP:
            return 2;
        case LIKE_OP:
        case NE_OP:
        case NO_OP:
            return 2;
        default:
            return 2;
    }
}

//
// 利用单表限制集合在某个数据表中提取满足条件的 RID 集合（尽可能使用索引加速）
//
RC QL_Manager::GetRidSet(const char* relName, RM_FileHandle& rmFileHandle, const std::vector<FullCondition>& fullConditions, std::vector<RID>& rids) {
    RC rc;
    // 查找出带有索引的属性值
    int index = -1;
    int indexLevel = 2;
    for (unsigned int i = 0; i < fullConditions.size(); ++i) {
        if (fullConditions[i].lhsAttr.indexNo >= 0 && fullConditions[i].bRhsIsAttr == 0 && ToLevel(fullConditions[i].op) < indexLevel) {
            index = i;
            indexLevel = ToLevel(fullConditions[i].op);
        }
    }
    if (index == -1) {
        // 如果没有属性带有索引，使用记录文件暴力扫描
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
        // 发现属性带有索引
        // 先使用索引缩小查找范围，然后确定最终集合
        int indexNo = *(char*)fullConditions[index].rhsValue.data == 0 ? fullConditions[index].lhsAttr.indexNo + 1 : fullConditions[index].lhsAttr.indexNo;
        IX_IndexHandle indexHandle;
        if ((rc = ixManager.OpenIndex(relName, indexNo, indexHandle))) {
            return rc;
        }
        IX_IndexScan indexScan;
        if ((rc = indexScan.OpenScan(indexHandle, fullConditions[index].op, fullConditions[index].rhsValue.data))) {
            return rc;
        }
        // 索引扫描
        while (true) {
            RID rid;
            if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
                return rc;
            }
            if (rc == IX_EOF) {
                break;
            }
            // 判断该条记录是否满足条件
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
        if ((rc = indexScan.CloseScan())) {
            return rc;
        }
        if ((rc = ixManager.CloseIndex(indexHandle))) {
            return rc;
        }
    }
    return OK_RC;
}

//
// 利用单表限制条件集合在某个数据表中提取满足条件的记录（尽可能使用索引，分配的空间需要在外部释放）
//
RC QL_Manager::GetDataSet(const RelCat& relCat, RM_FileHandle& rmFileHandle, const std::vector<FullCondition>& fullConditions, std::vector<char*>& data) {
    RC rc;
    // 查找出带有索引的属性值
    int index = -1;
    int indexLevel = 2;
    for (unsigned int i = 0; i < fullConditions.size(); ++i) {
        if (fullConditions[i].lhsAttr.indexNo >= 0 && fullConditions[i].bRhsIsAttr == 0 && ToLevel(fullConditions[i].op) < indexLevel) {
            index = i;
            indexLevel = ToLevel(fullConditions[i].op);
        }
    }
    if (index == -1) {
        // 如果没有属性带有索引，使用记录文件暴力扫描
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
        // 发现属性带有索引
        // 先使用索引缩小查找范围，然后确定最终集合
        int indexNo = *(char*)fullConditions[index].rhsValue.data == 0 ? fullConditions[index].lhsAttr.indexNo + 1 : fullConditions[index].lhsAttr.indexNo;
        IX_IndexHandle indexHandle;
        if ((rc = ixManager.OpenIndex(relCat.relName, indexNo, indexHandle))) {
            return rc;
        }
        IX_IndexScan indexScan;
        if ((rc = indexScan.OpenScan(indexHandle, fullConditions[index].op, fullConditions[index].rhsValue.data))) {
            return rc;
        }
        // 索引扫描
        while (true) {
            RID rid;
            if ((rc = indexScan.GetNextEntry(rid)) && rc != IX_EOF) {
                return rc;
            }
            if (rc == IX_EOF) {
                break;
            }
            // 判断该条记录是否满足条件
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
        if ((rc = indexScan.CloseScan())) {
            return rc;
        }
        if ((rc = ixManager.CloseIndex(indexHandle))) {
            return rc;
        }
    }
    return OK_RC;
}

//
// 检查某条记录是否满足给定单表限制条件集合
//
RC QL_Manager::CheckFullConditions(char* recordData, const std::vector<FullCondition>& fullConditions, bool& result) {
    result = true;
    for (unsigned int i = 0; result && i < fullConditions.size(); ++i) {
        if (*(char*)fullConditions[i].rhsValue.data == 0) {
            if (fullConditions[i].op == EQ_OP) {
                result = result && *(recordData + fullConditions[i].lhsAttr.offset) == 0;
            } else if (fullConditions[i].op == NE_OP) {
                result = result && *(recordData + fullConditions[i].lhsAttr.offset) == 1;
            }
        } else if (*(recordData + fullConditions[i].lhsAttr.offset) == 0) {
            result = false;
        } else {
            result = result && Attr::CompareAttr(fullConditions[i].lhsAttr.attrType, fullConditions[i].lhsAttr.attrLength, recordData + fullConditions[i].lhsAttr.offset, fullConditions[i].op, fullConditions[i].bRhsIsAttr ? recordData + fullConditions[i].rhsAttr.offset : fullConditions[i].rhsValue.data);
        }
    }
    return OK_RC;
}

//
// 检查两条记录是否满足给定多表限制条件集合
//
bool QL_Manager::CheckFullCondition(char* aData, char* bData, const std::vector<FullCondition>& conditions) {
    bool ret = true;
    for (unsigned int i = 0; ret && i < conditions.size(); ++i) {
        if (*(aData + conditions[i].lhsAttr.offset) == 0 || *(bData + conditions[i].rhsAttr.offset) == 0) {
            ret = false;
        } else {
            ret = ret && Attr::CompareAttr(conditions[i].lhsAttr.attrType, conditions[i].lhsAttr.attrLength, aData + conditions[i].lhsAttr.offset, conditions[i].op, bData + conditions[i].rhsAttr.offset);
        }
    }
    return ret;
}

//
// 将多个单表记录集合按照多表限制条件集合连接成结果数据集
//
RC QL_Manager::GetJoinData(std::map<RelCat, std::vector<char*>>& data, std::map<std::pair<RelCat, RelCat>, std::vector<FullCondition>>& binaryRelConds, std::vector<std::map<RelCat, char*>>& joinData) {
    // 记录已处理的数据表
    std::set<RelCat> rels;
    // 遍历多表限制条件集合
    for (const auto& conditions : binaryRelConds) {
        const RelCat& aRelCat = conditions.first.first;
        const RelCat& bRelCat = conditions.first.second;
        // 判断数据表是否被处理过
        auto aIter = rels.find(aRelCat);
        auto bIter = rels.find(bRelCat);
        if (aIter == rels.end() && bIter == rels.end()) {
            // 如果两个数据表均未被处理过
            std::vector<std::map<RelCat, char*>> tmp;
            // 连接两个数据表作为临时结果
            for (auto& aData : data[aRelCat]) {
                for (auto& bData : data[bRelCat]) {
                    if (CheckFullCondition(aData, bData, conditions.second)) {
                        tmp.push_back(std::map<RelCat, char*>{{aRelCat, aData}, {bRelCat, bData}});
                    }
                }
            }
            // 连接临时结果与已有连接结果
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
            // 更新已有连接结果
            joinData = tmpJoin;
            // 标注已被处理
            rels.insert(aRelCat);
            rels.insert(bRelCat);
        } else if (aIter == rels.end() && bIter != rels.end()) {
            // 有一个数据表被处理过
            // 直接在已有连接结果与另一数据表集合间连接
            std::vector<std::map<RelCat, char*>> tmpJoin;
            for (auto& b : joinData) {
                for (auto& a : data[aRelCat]) {
                    if (CheckFullCondition(a, b[bRelCat], conditions.second)) {
                        std::map<RelCat, char*> join = b;
                        join.insert(std::make_pair(aRelCat, a));
                        tmpJoin.emplace_back(join);
                    }
                }
            }
            // 更新已有连接结果
            joinData = tmpJoin;
            // 标注已被处理
            rels.insert(aRelCat);
        } else if (aIter != rels.end() && bIter == rels.end()) {
            // 同上
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
    // 遍历数据集，将多表条件中没有涉及到的数据表连接至已有结果中。
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
