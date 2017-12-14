//
// File:        sm_manager.cc
// Description: SM_Manager class implementation
// Authors:     Yi Xu
//

#include <cstdio>
#include <iostream>
#include "global.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) : ixm(ixm), rmm(rmm), isOpen(false) {
}

SM_Manager::~SM_Manager() {
}

RC SM_Manager::OpenDb(const char* dbName) {
    RC rc;
    // check whether a db is open
    if (isOpen) {
        return SM_DBOPEN;
    }
    // check whether dbName exist
    if (chdir(dbName) < 0) {
        return SM_DBNOTEXIST;
    }
    // open file for relcat
    if ((rc = rmm.OpenFile("relcat", relcatFileHandle))) {
        return rc;
    }
    // open file for attrcat
    if ((rc = rmm.OpenFile("attrcat", attrcatFileHandle))) {
        return rc;
    }
    // success
    isOpen = true;
    return OK_RC;
}

RC SM_Manager::CloseDb() {
    RC rc;
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // close file for relcat
    if ((rc = rmm.OpenFile("relcat", relcatFileHandle))) {
        return rc;
    }
    // close file for attrcat
    if ((rc = rmm.OpenFile("attrcat", attrcatFileHandle))) {
        return rc;
    }
    // success
    isOpen = false;
    return OK_RC;
}

RC SM_Manager::CreateTable(const char* relName, int attrCount, AttrInfo* attributes) {
    RC rc;
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    if ((rc = rmm.CreateFile(relName, attributes->recordSize))) {
        return rc;
    }
    return OK_RC;
}

RC SM_Manager::DropTable(const char* relName) {
    RC rc;
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    if ((rc = rmm.DestroyFile(relName))) {
        return rc;
    }
    return OK_RC;
}

RC SM_Manager::CreateIndex(const char* relName, const char* attrName) {
    RC rc;
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    RM_FileScan fileScan;
    fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP);
    int indexNo;
    AttrType attrType;
    int attrLength;
    fileScan.GetNextRec().Get(indexNo, attrType, attrLength);
    ixm.CreateIndex(relName, indexNo, attrType, attrLength);
    // success
    return OK_RC;
}

RC SM_Manager::DropIndex(const char* relName, const char* attrName) {
    RC rc;
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP);
    int indexNo;
    AttrType attrType;
    int attrLength;
    fileScan.GetNextRec().Get(indexNo, attrType, attrLength);
    fileScan.GetNextRec().GetIndexNo(indexNo);
    ixm.DestroyIndex(relName, indexNo);
    // success
    return OK_RC;
}
