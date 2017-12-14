//
// File:        dbcreate.cc
// Description: dbcreate command
// Authors:     Yi Xu
//

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "global.h"

using namespace std;

int main(int argc, char *argv[]) {
    char* dbname;
    char command[255] = "mkdir ";
    RC rc;

    // Look for 2 arguments. The first is always the name of the program
    // that was executed, and the second should be the name of the database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // The database name is the second argument
    dbname = argv[1];

    // Create a subdirectory for the database
    system(strcat(command, dbname));
    if (chdir(dbname) < 0) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    // Create file for relcat
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    if ((rc = rmm.CreateFile("relcat", RelCat::SIZE))) {
        return rc;
    }
    // Open file for relcat
    RM_FileHandle fileHandle;
    if ((rc = rmm.OpenFile("relcat", fileHandle))) {
        return rc;
    }
    // Add relcat record in relcat
    char* recordData = new char[RelCat::SIZE];
    RID rid;
    RelCat("relcat", RelCat::SIZE, 4, 0).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // Add attrcat record in relcat
    RelCat("attrcat", AttrCat::SIZE, 6, 0).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // Close file for relcat
    if ((rc = rmm.CloseFile(fileHandle))) {
        return rc;
    }
    delete[] recordData;

    // Create file for attrcat
    if ((rc = rmm.CreateFile("attrcat", AttrCat::SIZE))) {
        return rc;
    }
    // Open file for attrcat
    if ((rc = rmm.OpenFile("attrcat", fileHandle))) {
        return rc;
    }
    // Add relcat attr records in attrcat
    recordData = new char[AttrCat::SIZE];
    AttrCatRecord("relcat", "relName", 0, STRING, MAXNAME, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("relcat", "tupleLength", MAXNAME, INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("relcat", "attrCount", MAXNAME + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("relcat", "indexCount", MAXNAME + sizeof(int) + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // Add attrcat attr records in attrcat
    AttrCatRecord("attrcat", "relName", 0, STRING, MAXNAME, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("attrcat", "attrName", MAXNAME, STRING, MAXNAME, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("attrcat", "offset", MAXNAME + MAXNAME, INT, 4, -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("attrcat", "attrType", MAXNAME + MAXNAME + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("attrcat", "attrLength", MAXNAME + MAXNAME + sizeof(int) + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    AttrCatRecord("attrcat", "indexNo", MAXNAME + MAXNAME + sizeof(int) + sizeof(int) + sizeof(int), INT, sizeof(int), -1).WriteRecordData(recordData);
    if ((rc = fileHandle.InsertRec(recordData, rid))) {
        return rc;
    }
    // Close file for attrcat
    if ((rc = rmm.CloseFile(fileHandle))) {
        return rc;
    }
    delete[] recordData;

    return 0;
}
