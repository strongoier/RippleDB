//
// File:        sm_manager.cc
// Description: SM_Manager class implementation
// Authors:     Yi Xu
//

#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "unistd.h"
#include "global.h"
#include "printer.h"
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
    if (isOpen && (rc = CloseDb())) {
        return rc;
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
    cout << "[OpenDB]" << endl;
    cout << "dbName=" << dbName << endl;
    return OK_RC;
}

RC SM_Manager::CloseDb() {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // close file for relcat
    if ((rc = rmm.CloseFile(relcatFileHandle))) {
        return rc;
    }
    // close file for attrcat
    if ((rc = rmm.CloseFile(attrcatFileHandle))) {
        return rc;
    }
    // success
    chdir("..");
    isOpen = false;
    cout << "[CloseDB]" << endl;
    return OK_RC;
}

RC SM_Manager::CreateTable(const char* relName, int attrCount, AttrInfo* attributes) {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // check whether relation relName already exists
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(relcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relName))) {
        return rc;
    }
    RM_Record relCatRec;
    if ((rc = fileScan.GetNextRec(relCatRec)) != 0 && rc != RM_EOF) {
        return rc;
    }
    if (rc != RM_EOF) {
        return SM_RELEXIST;
    }
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    // update attrcat
    char* recordData = new char[AttrCat::SIZE];
    RID rid;
    int recordSize = 0;
    for (int i = 0; i < attrCount; ++i) {
        AttrCat(relName, attributes[i].attrName, recordSize, attributes[i].attrType, attributes[i].attrLength, -1).WriteRecordData(recordData);
        if ((rc = attrcatFileHandle.InsertRec(recordData, rid))) {
            return rc;
        }
        recordSize += attributes[i].attrLength;
    }
    if ((rc = attrcatFileHandle.ForcePages())) {
        return rc;
    }
    delete[] recordData;
    // update relcat
    recordData = new char[RelCat::SIZE];
    RelCat(relName, recordSize, attrCount, 0).WriteRecordData(recordData);
    if ((rc = relcatFileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    if ((rc = relcatFileHandle.ForcePages())) {
        return rc;
    }
    delete[] recordData;
    // create table file
    if ((rc = rmm.CreateFile(relName, recordSize))) {
        return rc;
    }
    // success
    cout << "[CreateTable]" << endl
         << "relName=" << relName << endl
         << "attrCount=" << attrCount << endl;
    for (int i = 0; i < attrCount; ++i) {
        cout << "attributes[" << i << "].attrName=" << attributes[i].attrName
             << " attrType="
             << (attributes[i].attrType == INT ? "INT" :
                 attributes[i].attrType == FLOAT ? "FLOAT" : "STRING")
             << " attrLength=" << attributes[i].attrLength << endl;
    }
    return OK_RC;
}

RC SM_Manager::DropTable(const char* relName) {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // find relation relName in relcat
    RM_Record relCatRec;
    if ((rc = CheckRelExist(relName, relCatRec))) {
        return rc;
    }
    // remove relation relName in relcat
    RID rid;
    if ((rc = relCatRec.GetRid(rid))) {
        return rc;
    }
    if ((rc = relcatFileHandle.DeleteRec(rid))) {
        return rc;
    }
    // find all attributes of relation relName in attrcat
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relName))) {
        return rc;
    }
    while (true) {
        RM_Record attrCatRec;
        if ((rc = fileScan.GetNextRec(attrCatRec)) != 0 && rc != RM_EOF) {
            return rc;
        }
        if (rc == RM_EOF) {
            break;
        }
        char *attrCatData;
        if ((rc = attrCatRec.GetData(attrCatData))) {
            return rc;
        }
        // remove index for attr
        AttrCat attrCat(attrCatData);
        if (attrCat.indexNo != -1 && (rc = ixm.DestroyIndex(relName, attrCat.indexNo))) {
            return rc;
        }
        // remove attr in attrcat
        if ((rc = attrCatRec.GetRid(rid))) {
            return rc;
        }
        if ((rc = attrcatFileHandle.DeleteRec(rid))) {
            return rc;
        }
        break;
    }
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    if ((rc = attrcatFileHandle.ForcePages())) {
        return rc;
    }
    // destroy table file
    if ((rc = rmm.DestroyFile(relName))) {
        return rc;
    }
    // success
    cout << "[DropTable]" << endl
         << "relName=" << relName << endl;
    return OK_RC;
}

RC SM_Manager::CreateIndex(const char* relName, const char* attrName) {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // find relation relName in relcat
    RM_Record relCatRec;
    if ((rc = CheckRelExist(relName, relCatRec))) {
        return rc;
    }
    char *relCatData;
    if ((rc = relCatRec.GetData(relCatData))) {
        return rc;
    }
    RelCat relCat(relCatData);
    // find (relName, attrName) in attrcat
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relName))) {
        return rc;
    }
    while (true) {
        RM_Record attrCatRec;
        if ((rc = fileScan.GetNextRec(attrCatRec)) != 0 && rc != RM_EOF) {
            return rc;
        }
        if (rc == RM_EOF) {
            return SM_ATTRNOTFOUND;
        }
        char *attrCatData;
        if ((rc = attrCatRec.GetData(attrCatData))) {
            return rc;
        }
        AttrCat attrCat(attrCatData);
        if (strcmp(attrCat.attrName, attrName)) {
            continue;
        }
        // check whether index already exist
        if (attrCat.indexNo != -1) {
            return SM_INDEXEXIST;
        }
        // found && update info
        if ((rc = fileScan.CloseScan())) {
            return rc;
        }
        attrCat.indexNo = relCat.indexCount++;
        attrCat.WriteRecordData(attrCatData);
        if ((rc = attrcatFileHandle.UpdateRec(attrCatRec))) {
            return rc;
        }
        if ((rc = attrcatFileHandle.ForcePages())) {
            return rc;
        }
        relCat.WriteRecordData(relCatData);
        if ((rc = relcatFileHandle.UpdateRec(relCatRec))) {
            return rc;
        }
        if ((rc = relcatFileHandle.ForcePages())) {
            return rc;
        }
        // create index
        if ((rc = ixm.CreateIndex(relName, attrCat.indexNo, attrCat.attrType, attrCat.attrLength))) {
            return rc;
        }
        IX_IndexHandle indexHandle;
        if ((rc = ixm.OpenIndex(relName, attrCat.indexNo, indexHandle))) {
            return rc;
        }
        // insert each record into index
        RM_FileHandle relFileHandle;
        if ((rc = rmm.OpenFile(relName, relFileHandle))) {
            return rc;
        }
        if ((rc = fileScan.OpenScan(relFileHandle, INT, sizeof(int), 0, NO_OP, NULL))) {
            return rc;
        }
        while (true) {
            RM_Record record;
            if ((rc = fileScan.GetNextRec(record)) != 0 && rc != RM_EOF) {
                return rc;
            }
            if (rc == RM_EOF) {
                break;
            }
            char* recordData;
            if ((rc = record.GetData(recordData))) {
                return rc;
            }
            RID rid;
            if ((rc = record.GetRid(rid))) {
                return rc;
            }
            if ((rc = indexHandle.InsertEntry(recordData + attrCat.offset, rid))) {
                return rc;
            }
        }
        // close things
        if ((rc = fileScan.CloseScan())) {
            return rc;
        }
        if ((rc = rmm.CloseFile(relFileHandle))) {
            return rc;
        }
        if ((rc = ixm.CloseIndex(indexHandle))) {
            return rc;
        }
        break;
    }
    // success
    cout << "[CreateIndex]" << endl
         << "relName=" << relName << endl
         << "attrName=" << attrName << endl;
    return OK_RC;
}

RC SM_Manager::DropIndex(const char* relName, const char* attrName) {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // find relation relName in relcat
    RM_Record relCatRec;
    if ((rc = CheckRelExist(relName, relCatRec))) {
        return rc;
    }
    // find (relName, attrName) in attrcat
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relName))) {
        return rc;
    }
    while (true) {
        RM_Record attrCatRec;
        if ((rc = fileScan.GetNextRec(attrCatRec)) != 0 && rc != RM_EOF) {
            return rc;
        }
        if (rc == RM_EOF) {
            return SM_ATTRNOTFOUND;
        }
        char *attrCatData;
        if ((rc = attrCatRec.GetData(attrCatData))) {
            return rc;
        }
        AttrCat attrCat(attrCatData);
        if (strcmp(attrCat.attrName, attrName)) {
            continue;
        }
        // check whether index already exist
        if (attrCat.indexNo == -1) {
            return SM_INDEXNOTEXIST;
        }
        // found && drop index
        if ((rc = ixm.DestroyIndex(relName, attrCat.indexNo))) {
            return rc;
        }
        // update info
        attrCat.indexNo = -1;
        attrCat.WriteRecordData(attrCatData);
        if ((rc = attrcatFileHandle.UpdateRec(attrCatRec))) {
            return rc;
        }
        if ((rc = attrcatFileHandle.ForcePages())) {
            return rc;
        }
        break;
    }
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    // success
    cout << "[DropIndex]" << endl
         << "relName=" << relName << endl
         << "attrName=" << attrName << endl;
    return OK_RC;
}

RC SM_Manager::Load(const char* relName, const char* fileName) {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // check whether file exists
    FILE* fin = fopen(fileName, "r");
    if (fin == NULL) {
        return SM_FILENOTFOUND;
    }
    // find relation relName in relcat
    RM_Record relCatRec;
    if ((rc = CheckRelExist(relName, relCatRec))) {
        return rc;
    }
    // find all attributes of relation relName in attrcat
    vector<AttrCat> attrs;
    if ((rc = GetAttrs(relName, attrs))) {
        return rc;
    }
    // read data from file && insert record into relation
    RM_FileHandle relFileHandle;
    if ((rc = rmm.OpenFile(relName, relFileHandle))) {
        return rc;
    }
    char buf[MAXATTRS * (MAXSTRINGLEN + 1)];
    char recordData[MAXATTRS * MAXSTRINGLEN];
    while (fgets(buf, MAXATTRS * (MAXSTRINGLEN + 1), fin) != NULL) {
        int attrid = 0;
        for (int i = 0, last = 0; ; ++i) {
            if (buf[i] == ',' || buf[i] == 0) {
                if (attrid == attrs.size()) {
                    return SM_LOADERROR;
                }
                char value[MAXSTRINGLEN];
                switch (attrs[attrid].attrType) {
                    case INT:
                        if (sscanf(buf + last, "%d", &value) != 1) {
                            return SM_LOADERROR;
                        }
                        break;
                    case FLOAT:
                        if (sscanf(buf + last, "%f", &value) != 1) {
                            return SM_LOADERROR;
                        }
                        break;
                    case STRING:
                        if (i - last >= attrs[attrid].attrLength) {
                            return SM_LOADERROR;
                        }
                        strncpy(value, buf + last, i - last);
                        value[i - last] = 0;
                        break;
                }
                Attr::SetAttr(recordData + attrs[attrid].offset, attrs[attrid].attrType, &value);
                last = i + 1;
                ++attrid;
                if (buf[i] == 0) {
                    break;
                }
            }
        }
        if (attrid < attrs.size()) {
            return SM_LOADERROR;
        }
        // a record ok
        RID rid;
        if ((rc = relFileHandle.InsertRec(recordData, rid))) {
            return rc;
        }
    }
    if ((rc = rmm.CloseFile(relFileHandle))) {
        return rc;
    }
    fclose(fin);
    // success
    cout << "[Load]" << endl
         << "relName=" << relName << endl
         << "fileName=" << fileName << endl;
    return OK_RC;
}

RC SM_Manager::Print(const char* relName) {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // find relation relName in relcat
    RM_Record relCatRec;
    if ((rc = CheckRelExist(relName, relCatRec))) {
        return rc;
    }
    // find all attributes of relation relName in attrcat
    vector<AttrCat> attrs;
    if ((rc = GetAttrs(relName, attrs))) {
        return rc;
    }
    // fill in the attributes structure and print the header information
    DataAttrInfo* attributes = new DataAttrInfo[attrs.size()];
    for (int i = 0; i < attrs.size(); ++i) {
        strcpy(attributes[i].relName, attrs[i].relName);
        strcpy(attributes[i].attrName, attrs[i].attrName);
        attributes[i].offset = attrs[i].offset;
        attributes[i].attrType = attrs[i].attrType;
        attributes[i].attrLength = attrs[i].attrLength;
        attributes[i].indexNo = attrs[i].indexNo;
    }
    Printer printer(attributes, attrs.size());
    printer.PrintHeader(cout);
    // open the file and set up the file scan
    RM_FileHandle relFileHandle;
    if ((rc = rmm.OpenFile(relName, relFileHandle))) {
        return rc;
    }
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(relFileHandle, INT, sizeof(int), 0, NO_OP, NULL))) 
       return rc;
    // print each record
    while (true) {
        RM_Record record;
        if ((rc = fileScan.GetNextRec(record)) != 0 && rc != RM_EOF) {
            return rc;
        }
        if (rc == RM_EOF) {
            break;
        }
        char* recordData;
        if ((rc = record.GetData(recordData))) {
            return rc;
        }
        printer.Print(cout, recordData);
    }
    // print the footer information and close things
    printer.PrintFooter(cout);
    delete[] attributes;
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    if ((rc = rmm.CloseFile(relFileHandle))) {
        return rc;
    }
    // success
    cout << "[Print]" << endl
         << "relName=" << relName << endl;
    return OK_RC;
}

RC SM_Manager::CheckRelExist(const char* relName, RM_Record& relCatRec) {
    RC rc;
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(relcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relName))) {
        return rc;
    }
    if ((rc = fileScan.GetNextRec(relCatRec)) != 0 && rc != RM_EOF) {
        return rc;
    }
    if (rc == RM_EOF) {
        return SM_RELNOTFOUND;
    }
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    return OK_RC;
}

RC SM_Manager::GetAttrs(const char* relName, vector<AttrCat>& attrs) {
    RC rc;
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relName))) {
        return rc;
    }
    while (true) {
        RM_Record attrCatRec;
        if ((rc = fileScan.GetNextRec(attrCatRec)) != 0 && rc != RM_EOF) {
            return rc;
        }
        if (rc == RM_EOF) {
            break;
        }
        char *attrCatData;
        if ((rc = attrCatRec.GetData(attrCatData))) {
            return rc;
        }
        attrs.push_back(AttrCat(attrCatData));
    }
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    sort(attrs.begin(), attrs.end(), [](AttrCat a, AttrCat b) -> bool { return a.offset < b.offset; });
    return OK_RC;
}
