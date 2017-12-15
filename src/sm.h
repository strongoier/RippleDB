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

//
// RelCat: Relation Catalog
//
struct RelCat {
    static const int SIZE = MAXNAME + sizeof(int) + sizeof(int) + sizeof(int);

    char relName[MAXNAME]; // relation name
    int tupleLength; // tuple length in bytes
    int attrCount; // number of attributes
    int indexCount; // number of indexed attributes

    // Construct from char* data.
    RelCat(const char* recordData);
    // Construct from values.
    RelCat(const char* relName, int tupleLength, int attrCount, int indexCount);

    // Convert to char* data.
    void WriteRecordData(char* recordData);
};

//
// AttrCat: Attribute Catalog
//
struct AttrCat {
    static const int SIZE = MAXNAME + MAXNAME + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int);

    char relName[MAXNAME]; // this attribute's relation
    char attrName[MAXNAME]; // attribute name
    int offset; // offset in bytes from beginning of tuple
    AttrType attrType; // attribute type
    int attrLength; // attribute length
    int indexNo; // index number, or -1 if not indexed

    // Construct from char* data.
    AttrCat(const char* recordData);
    // Construct from values.
    AttrCat(const char* relName, const char* attrName, int offset, AttrType attrType, int attrLength, int indexNo);

    // Convert to char* data.
    void WriteRecordData(char* recordData);
};

//
// SM_Manager: provides system management
//
class SM_Manager {
public:
    friend class QL_Manager;

    SM_Manager(IX_Manager& ixm, RM_Manager& rmm);
    ~SM_Manager();

    // Open database dbName.
    RC OpenDb(const char* dbName);
    // Close the opened database.
    RC CloseDb();
    // Create relation relName with attrCount attributes.
    RC CreateTable(const char* relName, int attrCount, AttrInfo* attributes);
    // Create index for relName.attrName.
    RC CreateIndex(const char* relName, const char* attrName);
    // Drop relation relName.
    RC DropTable(const char* relName);
    // Drop index for relName.attrName.
    RC DropIndex(const char* relName, const char* attrName);
    // Load relation relName from file fileName.
    RC Load(const char* relName, const char* fileName);
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
};

//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_DBNOTOPEN     (START_SM_WARN + 0)  // no db is open
#define SM_DBNOTEXIST    (START_SM_WARN + 1)  // db not exist
#define SM_RELNOTFOUND   (START_SM_WARN + 2)  // relation not found
#define SM_ATTRNOTFOUND  (START_SM_WARN + 3)  // attribute not found
#define SM_INDEXEXIST    (START_SM_WARN + 4)  // index already exist
#define SM_INDEXNOTEXIST (START_SM_WARN + 5)  // index not exist
#define SM_FILENOTFOUND  (START_SM_WARN + 6)  // file not found
#define SM_RELEXIST      (START_SM_WARN + 7)  // relation already exist
#define SM_LOADERROR     (START_SM_WARN + 8)  // error while loading

#endif
