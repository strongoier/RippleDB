//
// File:        sm.h
// Description: System Manager component interface
// Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
//

#ifndef SM_H
#define SM_H

// Do not change the following includes
#include <cstdlib>
#include <cstring>
#include <vector>
#include "global.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
using std::vector;

class QL_Manager;

//
// SM_Manager: provides system management
//
class SM_Manager {
    friend QL_Manager;
public:
    friend class QL_Manager;

    SM_Manager(IX_Manager& ixm, RM_Manager& rmm);
    ~SM_Manager();

    // Create database dbName.
    RC CreateDb(const char* dbName);
    // Drop database dbName.
    RC DropDb(const char* dbName);
    // Show databases.
    RC ShowDbs();
    // Open database dbName.
    RC OpenDb(const char* dbName);
    // Close the opened database.
    RC CloseDb();
    // Show tables.
    RC ShowTables();
    // Create relation relName with fieldCount fields.
    RC CreateTable(const char* relName, int fieldCount, Field* fields);
    // Drop relation relName.
    RC DropTable(const char* relName);
    // Desc relation relName.
    RC DescTable(const char* relName);
    // Create index for relName.attrName.
    RC CreateIndex(const char* relName, const char* attrName);    
    // Drop index for relName.attrName.
    RC DropIndex(const char* relName, const char* attrName);
    // Print relation relName contents.
    RC Print(const char* relName);

private:
    // Find relation relName in relcat.
    RC CheckRelExist(const char* relName, RM_Record& relCatRec);
    // Get all attrs relation relName in attrcat.
    RC GetAttrs(const char* relName, std::vector<AttrCat>& attrs);

    IX_Manager& ixm; // internal IX_Manager
    RM_Manager& rmm; // internal RM_Manager
    RM_FileHandle relcatFileHandle; // fileHandle for relcat
    RM_FileHandle attrcatFileHandle; // fileHandle for attrcat
    bool isOpen; // whether a db is open
    char zero[5]; // for scan all
};

//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_DBNOTOPEN       (START_SM_WARN + 0)  // no db is open
#define SM_DBNOTEXIST      (START_SM_WARN + 1)  // db not exist
#define SM_RELNOTFOUND     (START_SM_WARN + 2)  // relation not found
#define SM_ATTRNOTFOUND    (START_SM_WARN + 3)  // attribute not found
#define SM_INDEXEXIST      (START_SM_WARN + 4)  // index already exist
#define SM_INDEXNOTEXIST   (START_SM_WARN + 5)  // index not exist
#define SM_FILENOTFOUND    (START_SM_WARN + 6)  // file not found
#define SM_RELEXIST        (START_SM_WARN + 7)  // relation already exist
#define SM_LOADERROR       (START_SM_WARN + 8)  // error while loading
#define SM_DBISOPEN        (START_SM_WARN + 9)  // a db is open
#define SM_SYSERROR        (START_SM_WARN + 10) // system error
#define SM_INDEXPRIMARYKEY (START_SM_WARN + 11) // index is for primary key
#define SM_ATTRNOTMATCH    (START_SM_WARN + 12) // attr not match

#endif
