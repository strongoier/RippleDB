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
    *zero = 1;
    *(int*)(zero + 1) = 0;
}

SM_Manager::~SM_Manager() {
}

RC SM_Manager::CreateDb(const char* dbName) {
    RC rc;
    // check whether a db is open
    if (isOpen) {
        return SM_DBISOPEN;
    }
    char command[255] = "mkdir ";
    // create a subdirectory for the database
    if (system(strcat(command, dbName)) != 0) {
        return SM_SYSERROR;
    }
    // cd into the subdirectory
    if (chdir(dbName) < 0) {
        return SM_SYSERROR;
    }
    // create file for relcat
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    if ((rc = rmm.CreateFile("relcat", RelCat::SIZE))) {
        return rc;
    }
    // open file for relcat
    RM_FileHandle fileHandle;
    if ((rc = rmm.OpenFile("relcat", fileHandle))) {
        return rc;
    }
    // add relcat record in relcat
    char* recordData = new char[RelCat::SIZE];
    RID rid;
    RelCat("relcat", RelCat::SIZE, 4, 0).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // add attrcat record in relcat
    RelCat("attrcat", AttrCat::SIZE, 10, 0).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // close file for relcat
    if ((rc = rmm.CloseFile(fileHandle))) {
        return rc;
    }
    delete[] recordData;
    // create file for attrcat
    if ((rc = rmm.CreateFile("attrcat", AttrCat::SIZE))) {
        return rc;
    }
    // open file for attrcat
    if ((rc = rmm.OpenFile("attrcat", fileHandle))) {
        return rc;
    }
    // add relcat attr records in attrcat
    recordData = new char[AttrCat::SIZE];
    AttrCat("relcat", "relName", RelCat::RELNAME_OFFSET, STRING, MAXNAME, -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("relcat", "tupleLength", RelCat::TUPLELENGTH_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("relcat", "attrCount", RelCat::ATTRCOUNT_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("relcat", "indexCount", RelCat::INDEXCOUNT_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // add attrcat attr records in attrcat
    AttrCat("attrcat", "relName", AttrCat::RELNAME_OFFSET, STRING, MAXNAME, -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "attrName", AttrCat::ATTRNAME_OFFSET, STRING, MAXNAME, -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "offset", AttrCat::OFFSET_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "attrType", AttrCat::ATTRTYPE_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "attrLength", AttrCat::ATTRLENGTH_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "indexNo", AttrCat::INDEXNO_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "isNotNull", AttrCat::ISNOTNULL_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "primaryKey", AttrCat::PRIMARYKEY_OFFSET, INT, sizeof(int), -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "refrel", AttrCat::REFREL_OFFSET, STRING, MAXNAME, -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "refattr", AttrCat::REFATTR_OFFSET, STRING, MAXNAME, -1, 1, 0, "", "").WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // close file for attrcat
    if ((rc = rmm.CloseFile(fileHandle))) {
        return rc;
    }
    delete[] recordData;
    // success
    chdir("..");
    cout << "[CreateDB]" << endl;
    cout << "dbName=" << dbName << endl;
    return OK_RC;
}

RC SM_Manager::DropDb(const char* dbName) {
    RC rc;
    // check whether a db is open
    if (isOpen) {
        return SM_DBISOPEN;
    }
    // remove the subdirectory for the database
    char command[255] = "rm -rf ";
    if (system(strcat(command, dbName)) != 0) {
        return SM_SYSERROR;
    }
    // success
    cout << "[DropDB]" << endl;
    cout << "dbName=" << dbName << endl;
    return OK_RC;
}

RC SM_Manager::ShowDbs() {
    RC rc;
    // check whether a db is open
    if (isOpen) {
        return SM_DBISOPEN;
    }
    // get all dirs
    if (system("ls -1 -d */ > all_dirs.txt") != 0) {
        return SM_SYSERROR;
    }
    char line[MAXSTRINGLEN];
    FILE* fin = fopen("all_dirs.txt", "r");
    while (fgets(line, MAXSTRINGLEN, fin) != NULL) {
        line[strlen(line) - 2] = 0;
        puts(line);
    }
    fclose(fin);
    if (system("rm all_dirs.txt") != 0) {
        return SM_SYSERROR;
    }
    // success
    cout << "[ShowDBs]" << endl;
    return OK_RC;
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

RC SM_Manager::ShowTables() {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // set up relcat file scan
    RM_FileScan fileScan;
    if ((rc = fileScan.OpenScan(relcatFileHandle, INT, sizeof(int), 0, NO_OP, zero))) {
        return rc;
    }
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
        puts(RelCat(recordData).relName);
    }
    // close file scan
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    // success
    cout << "[ShowTables]" << endl;
    return OK_RC;
}

RC SM_Manager::CreateTable(const char* relName, int fieldCount, Field* fields) {
    RC rc;
    // check whether a db is open
    if (!isOpen) {
        return SM_DBNOTOPEN;
    }
    // check whether relation relName already exists
    RM_FileScan fileScan;
    char relNameValue[MAXNAME + 1];
    *relNameValue = 1;
    strcpy(relNameValue + 1, relName);
    if ((rc = fileScan.OpenScan(relcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relNameValue))) {
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
    int indexCount = 0;
    vector<AttrCat> attrs;
    for (int i = 0; i < fieldCount; ++i) {
        if (fields[i].attr.attrName != NULL) { // new attribute
            attrs.push_back(AttrCat(relName, fields[i].attr.attrName, recordSize, fields[i].attr.attrType, fields[i].attr.attrLength, -1, fields[i].isNotNull, 0, "", ""));
            recordSize += 1 + fields[i].attr.attrLength;
        } else if (fields[i].nPrimaryKey > 0) { // primary key
            if (fields[i].nPrimaryKey > 1) {
                // calc total length of primary keys
                int len = 0;
                for (int j = 0; j < fields[i].nPrimaryKey; ++j) {
                    for (auto attr : attrs) {
                        if (!strcmp(attr.attrName, fields[i].primaryKeyList[j])) {
                            len += attr.attrLength;
                        }
                    }
                }
                // create index for primary keys
                ++indexCount;
                if ((rc = ixm.CreateIndex(relName, 0, STRING, len))) {
                    return rc;
                }
            }
            // create index for single primary key
            int count = 0;
            for (int j = 0; j < fields[i].nPrimaryKey; ++j) {
                for (int k = 0; k < attrs.size(); ++k) {
                    if (!strcmp(attrs[k].attrName, fields[i].primaryKeyList[j])) {
                        attrs[k].primaryKey = ++count;
                        attrs[k].indexNo = indexCount;
                        attrs[k].isNotNull = 1;
                        indexCount += 2;
                        if ((rc = ixm.CreateIndex(relName, attrs[k].indexNo, attrs[k].attrType, attrs[k].attrLength))) {
                            return rc;
                        }
                        if ((rc = ixm.CreateIndex(relName, attrs[k].indexNo + 1, attrs[k].attrType, attrs[k].attrLength))) {
                            return rc;
                        }
                    }
                }
            }
        } else { // foreign key
            for (int j = 0; j < attrs.size(); ++j) {
                if (!strcmp(attrs[j].attrName, fields[i].foreignKey)) {
                    strcpy(attrs[j].refrel, fields[i].refRel);
                    strcpy(attrs[j].refattr, fields[i].refAttr);
                    // find relation refrel in relcat
                    RM_Record relCatRec;
                    if ((rc = CheckRelExist(attrs[j].refrel, relCatRec))) {
                        return rc;
                    }
                    // get all attributes of refrel
                    vector<AttrCat> refrelAttrs;
                    if ((rc = GetAttrs(attrs[j].refrel, refrelAttrs))) {
                        return rc;
                    }
                    // search for refattr
                    bool found = false;
                    for (auto attr : refrelAttrs) {
                        if (!strcmp(attr.attrName, attrs[j].refattr)) {
                            found = true;
                            if (attr.attrType != attrs[j].attrType || attr.attrLength != attrs[j].attrLength) {
                                return SM_ATTRNOTMATCH;
                            }
                            break;
                        }
                    }
                    if (!found) {
                        return SM_ATTRNOTFOUND;
                    }
                    break;
                }
            }
        }    
    }
    for (auto attr : attrs) {
        attr.WriteRecordData(recordData);
        if ((rc = attrcatFileHandle.InsertRec(recordData, rid))) {
            return rc;
        }
    }
    if ((rc = attrcatFileHandle.ForcePages())) {
        return rc;
    }
    delete[] recordData;
    // update relcat
    recordData = new char[RelCat::SIZE];
    RelCat(relName, recordSize, attrs.size(), 0).WriteRecordData(recordData);
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
         << "fieldCount=" << fieldCount << endl
         << "attributes=" << endl;
    for (auto attr : attrs) {
        cout << attr.attrName
             << " " << (attr.attrType == INT ? "INT" :
                        attr.attrType == FLOAT ? "FLOAT" :
                        attr.attrType == DATE ? "DATE" : "STRING")
             << " " << attr.attrLength;
        if (attr.isNotNull) {
            cout << " NOT NULL";
        }
        if (attr.primaryKey > 0) {
            cout << " PRIMARY KEY";
        }
        if (attr.refrel[0]) {
            cout << " FOREIGN KEY REFERENCES " << attr.refrel << "(" << attr.refattr << ")";
        }
        cout << endl;
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
    char relNameValue[MAXNAME + 1];
    *relNameValue = 1;
    strcpy(relNameValue + 1, relName);
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relNameValue))) {
        return rc;
    }
    bool multiplePrimaryKey = false;
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
        if (attrCat.primaryKey > 1) {
            multiplePrimaryKey = true;
        }
        if (attrCat.indexNo != -1 && (rc = ixm.DestroyIndex(relName, attrCat.indexNo))) {
            return rc;
        }
        if (attrCat.indexNo != -1 && (rc = ixm.DestroyIndex(relName, attrCat.indexNo + 1))) {
            return rc;
        }
        // remove attr in attrcat
        if ((rc = attrCatRec.GetRid(rid))) {
            return rc;
        }
        if ((rc = attrcatFileHandle.DeleteRec(rid))) {
            return rc;
        }
    }
    if ((rc = fileScan.CloseScan())) {
        return rc;
    }
    if (multiplePrimaryKey && (rc = ixm.DestroyIndex(relName, 0))) {
        return rc;
    }
    if ((rc = relcatFileHandle.ForcePages())) {
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

RC SM_Manager::DescTable(const char* relName) {
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
    // print all attributes
    for (auto attr : attrs) {
        cout << attr.attrName
             << " " << (attr.attrType == INT ? "INT" :
                        attr.attrType == FLOAT ? "FLOAT" :
                        attr.attrType == DATE ? "DATE" : "STRING")
             << " " << attr.attrLength;
        if (attr.isNotNull) {
            cout << " NOT NULL";
        }
        if (attr.primaryKey > 0) {
            cout << " PRIMARY KEY";
        }
        if (attr.refrel[0]) {
            cout << " FOREIGN KEY REFERENCES " << attr.refrel << "(" << attr.refattr << ")";
        }
        cout << endl;
    }
    // success
    cout << "[DescTable]" << endl
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
    char relNameValue[MAXNAME + 1];
    *relNameValue = 1;
    strcpy(relNameValue + 1, relName);
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relNameValue))) {
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
        attrCat.indexNo = relCat.indexCount;
        relCat.indexCount += 2;
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
        if ((rc = ixm.CreateIndex(relName, attrCat.indexNo + 1, attrCat.attrType, attrCat.attrLength))) {
            return rc;
        }
        IX_IndexHandle indexHandleNotNull;
        IX_IndexHandle indexHandleNull;
        if ((rc = ixm.OpenIndex(relName, attrCat.indexNo, indexHandleNotNull))) {
            return rc;
        }
        if ((rc = ixm.OpenIndex(relName, attrCat.indexNo + 1, indexHandleNull))) {
            return rc;
        }
        // insert each record into index
        RM_FileHandle relFileHandle;
        if ((rc = rmm.OpenFile(relName, relFileHandle))) {
            return rc;
        }
        if ((rc = fileScan.OpenScan(relFileHandle, INT, sizeof(int), 0, NO_OP, zero))) {
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
            if (*(recordData + attrCat.offset) == 0) {
                if ((rc = indexHandleNull.InsertEntry(recordData + attrCat.offset, rid))) {
                    return rc;
                }
            } else {
                if ((rc = indexHandleNotNull.InsertEntry(recordData + attrCat.offset, rid))) {
                    return rc;
                }
            }
        }
        // close things
        if ((rc = fileScan.CloseScan())) {
            return rc;
        }
        if ((rc = rmm.CloseFile(relFileHandle))) {
            return rc;
        }
        if ((rc = ixm.CloseIndex(indexHandleNotNull))) {
            return rc;
        }
        if ((rc = ixm.CloseIndex(indexHandleNull))) {
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
    char relNameValue[MAXNAME + 1];
    *relNameValue = 1;
    strcpy(relNameValue + 1, relName);
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relNameValue))) {
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
        // check whether index for primary key
        if (attrCat.primaryKey > 0) {
            return SM_INDEXPRIMARYKEY;
        }
        // found && drop index
        if ((rc = ixm.DestroyIndex(relName, attrCat.indexNo))) {
            return rc;
        }
        if ((rc = ixm.DestroyIndex(relName, attrCat.indexNo + 1))) {
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
    for (unsigned int i = 0; i < attrs.size(); ++i) {
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
    if ((rc = fileScan.OpenScan(relFileHandle, INT, sizeof(int), 0, NO_OP, zero))) {
        return rc;
    }
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
    char relNameValue[MAXNAME + 1];
    *relNameValue = 1;
    strcpy(relNameValue + 1, relName);
    if ((rc = fileScan.OpenScan(relcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relNameValue))) {
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
    char relNameValue[MAXNAME + 1];
    *relNameValue = 1;
    strcpy(relNameValue + 1, relName);
    if ((rc = fileScan.OpenScan(attrcatFileHandle, STRING, MAXNAME, 0, EQ_OP, (void*)relNameValue))) {
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
