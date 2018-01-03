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
    RelCat("attrcat", AttrCat::SIZE, 6, 0).WriteRecordData(recordData);
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
    AttrCat("relcat", "relName", 0, STRING, MAXNAME, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("relcat", "tupleLength", MAXNAME, INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("relcat", "attrCount", MAXNAME + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("relcat", "indexCount", MAXNAME + sizeof(int) + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // add attrcat attr records in attrcat
    AttrCat("attrcat", "relName", 0, STRING, MAXNAME, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "attrName", MAXNAME, STRING, MAXNAME, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "offset", MAXNAME + MAXNAME, INT, 4, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "attrType", MAXNAME + MAXNAME + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "attrLength", MAXNAME + MAXNAME + sizeof(int) + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCat("attrcat", "indexNo", MAXNAME + MAXNAME + sizeof(int) + sizeof(int) + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
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
        line[strlen(line) - 1] = 0;
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
    if ((rc = fileScan.OpenScan(relcatFileHandle, INT, sizeof(int), 0, NO_OP, NULL))) {
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
    for (int i = 0; i < fieldCount; ++i) {
        if (fields[i].attr.attrName != NULL) {
            AttrCat(relName, fields[i].attr.attrName, recordSize, fields[i].attr.attrType, fields[i].attr.attrLength, -1).WriteRecordData(recordData);
            if ((rc = attrcatFileHandle.InsertRec(recordData, rid))) {
                return rc;
            }
            recordSize += fields[i].attr.attrLength;
        }
    }
    if ((rc = attrcatFileHandle.ForcePages())) {
        return rc;
    }
    delete[] recordData;
    // update relcat
    recordData = new char[RelCat::SIZE];
    RelCat(relName, recordSize, fieldCount, 0).WriteRecordData(recordData);
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
         << "fieldCount=" << fieldCount << endl;
    for (int i = 0; i < fieldCount; ++i) {
        if (fields[i].attr.attrName != NULL) {
            cout << "attributes[" << i << "].attrName=" << fields[i].attr.attrName
                 << " attrType="
                 << (fields[i].attr.attrType == INT ? "INT" :
                     fields[i].attr.attrType == FLOAT ? "FLOAT" :
                     fields[i].attr.attrType == DATE ? "DATE" : "STRING")
                 << " attrLength=" << fields[i].attr.attrLength << endl;
        }
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

RC SM_Manager::DescTable(const char* relName) {
    RC rc;
    // find all attributes of relation relName in attrcat
    vector<AttrCat> attrs;
    if ((rc = GetAttrs(relName, attrs))) {
        return rc;
    }
    // print all attributes
    for (auto attr : attrs) {
        cout << attr.attrName << " "
             << (attr.attrType == INT ? "INT" :
                attr.attrType == FLOAT ? "FLOAT" :
                attr.attrType == DATE ? "DATE" : "STRING") << " "
             << attr.attrLength << endl;
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
                if (attrid == (int)attrs.size()) {
                    return SM_LOADERROR;
                }
                char value[MAXSTRINGLEN];
                switch (attrs[attrid].attrType) {
                    case INT:
                        if (sscanf(buf + last, "%d", (int*)&value) != 1) {
                            return SM_LOADERROR;
                        }
                        break;
                    case FLOAT:
                        if (sscanf(buf + last, "%f", (float*)&value) != 1) {
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
        if (attrid < (int)attrs.size()) {
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
