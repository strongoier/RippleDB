#include <cstdio>
#include <iostream>
#include <algorithm>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include "global.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;

/*RM_FileIterator::RM_FileIterator(RM_FileHandle& rfh, Condition c) : rmFileHandle(rfh), condition(c) {
    rmFileScan.OpenScan(rmFileHandle, condition.lhsAttr)
}*/

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : smManager(smm), ixManager(ixm), rmManager(rmm) {}

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

    // check whether a db is open
    if (!smManager.isOpen) {
        return QL_DBNOTOPEN;
    }

    // find relation name in relCat
    vector<RelCat> relations;
    for (int i = 0; i < nRelations; ++i) {
        RM_Record relCatRec;
        if ((rc = smManager.CheckRelExist(relName, relCatRec))) {
            if (rc == SM_RELNOTFOUND) {
                return QL_RELNOTFIND;
            } else {
                return rc;
            }
        }
        char* relCatData;
        if ((rc = relCatRec.GetData(relCatData))) {
            return rc;
        }
        relations.push_back(RelCat(relCatData));
    }

    // get attr info in the relation
    vector<AttrCat> attrs;
    if (nSelAttrs == 1 && selAttrs[0].attrName[0] == '*') {
        for (int i = 0; i < nRelations; ++i) {
            if ((rc = smManager.GetAttrs(relations[i], attrs))) {
                return rc;
            }
        }
    } else {
        for (int i = 0; i < nSelAttrs; ++i) {
            if (selAttrs[i].relName == NULL) {
                RM_FileScan fileScan;
                if ((rc = fileScan.OpenScan(smManager.attrcatFileHandle, STRING, MAXNAME, MAXNAME, EQ_OP, selAttrs[i].attrName))) {
                    return rc;
                }
                RM_Record attrCatRec;
                if ((rc = fileScan.GetNextRec(attrCatRec)) != 0 && rc != RM_EOF) {
                    return rc;
                }
                if (rc == RM_EOF) {
                    return QL_ATTRNOTFOUND;
                }
                char *attrCatRecData;
                if ((rc = attrCatRec.GetData(attrCatRecData))) {
                    return rc;
                }
                AttrCat attrCat(attrCatRecData);
                if ((rc = fileScan.GetNextRec(attrCatRec)) && rc != EOF) {
                    return QL_ATTRTYPEWRONG;
                }
                attrs.push_back(attrCat);
                if ((rc = fileScan.CloseScan())) {
                    return rc;
                }
            } else {
                RM_FileScan fileScan;
                if ((rc = fileScan.OpenScan(smManager.attrcatFileHandle, STRING, MAXNAME, MAXNAME, EQ_OP, selAttrs[i].attrName))) {
                    return rc;
                }
                while (true) {
                    RM_Record attrCatRec;
                    if ((rc = fileScan.GetNextRec(attrCatRec)) != 0 && rc != RM_EOF) {
                        return rc;
                    }
                    if (rc == RM_EOF) {
                        return QL_ATTRNOTFOUND;
                    }
                    char *attrCatRecData;
                    if ((rc = attrCatRec.GetData(attrCatRecData))) {
                        return rc;
                    }
                    AttrCat attrCat(attrCatRecData);
                    if (strcmp(selAttrs[i].relName, attrCat.relName) == 0) {
                        attrs.push_back(attrCat);
                        break;
                    }
                }
                if ((rc = fileScan.CloseScan())) {
                    return rc;
                }
            }
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
    if (!smManager.isOpen) {
        return QL_DBNOTOPEN;
    }

    // find relation name in relCat
    RM_Record relCatRec;
    if ((rc = smManager.CheckRelExist(relName, relCatRec))) {
        if (rc == SM_RELNOTFOUND) {
            return QL_RELNOTFIND;
        } else {
            return rc;
        }
    }
    char* relCatData;
    if ((rc = relCatRec.GetData(relCatData))) {
        return rc;
    }
    RelCat relCat(relCatData);

    // check nValues == attrCount
    if (nValues != relCat.attrCount) {
        return QL_ATTRSNUMBERWRONG;
    }

    // get attr info in the relation and check the type info
    vector<AttrCat> attrs;
    if ((rc = smManager.GetAttrs(relName, attrs))) {
        return rc;
    }
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

    return 0;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName, int nConditions, const Condition conditions[]) {
    RC rc;

    // check whether a db is open
    if (!smManager.isOpen) {
        return QL_DBNOTOPEN;
    }

    // find relation name in relCat
    RM_Record relCatRec;
    if ((rc = smManager.CheckRelExist(relName, relCatRec))) {
        if (rc == SM_RELNOTFOUND) {
            return QL_RELNOTFIND;
        } else {
            return rc;
        }
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
    RM_FileScan rmFileScan;
    if ((rc = rmFileScan.OpenScan(rmFileHandle, fullConditions))) {
        return rc;
    }
    vector<RID> rids;
    while (true) {
        RM_Record record;
        rc = rmFileScan.GetNextRec(record);
        if (rc != 0 && rc != RM_EOF) {
            return rc;
        }
        if (rc == RM_EOF) {
            break;
        }
        RID rid;
        if ((rc = record.GetRid(rid))) {
            return rc;
        }
        rids.push_back(rid);
    }
    if ((rc = rmFileScan.CloseScan())) {
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

    return 0;
}

//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName, const RelAttr &updAttr, const int bIsValue, const RelAttr &rhsRelAttr, const Value &rhsValue, int nConditions, const Condition conditions[]) {
    RC rc;

    // check whether a db is open
    if (!smManager.isOpen) {
        return QL_DBNOTOPEN;
    }

    // find relation name in relCat
    RM_Record relCatRec;
    if ((rc = smManager.CheckRelExist(relName, relCatRec))) {
        if (rc == SM_RELNOTFOUND) {
            return QL_RELNOTFIND;
        } else {
            return rc;
        }
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
    RM_FileScan rmFileScan;
    if ((rc = rmFileScan.OpenScan(rmFileHandle, fullConditions))) {
        return rc;
    }
    vector<RID> rids;
    while (true) {
        RM_Record record;
        rc = rmFileScan.GetNextRec(record);
        if (rc != 0 && rc != RM_EOF) {
            return rc;
        }
        if (rc == RM_EOF) {
            break;
        }
        RID rid;
        if ((rc = record.GetRid(rid))) {
            return rc;
        }
        rids.push_back(rid);
    }
    if ((rc = rmFileScan.CloseScan())) {
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
            }
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

    return 0;
}

RC QL_Manager::GetFullConditions(const char* relName, const vector<AttrCat>& attrs, int nConditions, const Condition conditions[], vector<FullCondition>& fullConditions) {
    for (int i = 0; i < nConditions; ++i) {
        auto iter = find_if(attrs.begin(), attrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, conditions[i].lhsAttr.attrName) == 0; });
        if (iter == attrs.end()) {
            return QL_ATTRNOTFOUND;
        }
        FullCondition fc;
        fc.lhsAttr = *iter;
        fc.op = conditions[i].op;
        fc.bRhsIsAttr = conditions[i].bRhsIsAttr;
        if (fc.bRhsIsAttr) {
            auto rIter = find_if(attrs.begin(), attrs.end(), [&](const AttrCat& item) { return strcmp(item.attrName, conditions[i].rhsAttr.attrName) == 0; });
            if (rIter == attrs.end()) {
                return QL_ATTRNOTFOUND;
            }
            fc.rhsAttr = *rIter;
        } else {
            fc.rhsValue = conditions[i].rhsValue;
        }
        fullConditions.push_back(fc);
    }
}

void QL_PrintError(RC rc) {
    cout << "QL_PrintError\n   rc=" << rc << "\n";
}
