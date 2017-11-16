#include "ix.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
using namespace std;

IX_Manager::IX_Manager(PF_Manager &pfm) : PFMgr(pfm) {}

IX_Manager::~IX_Manager() {}

// Create a new Index
RC IX_Manager::CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength) {
    RC rc;

    if (!Attr::CheckAttrLengthValid(attrType, attrLength))
        IX_ERROR(IX_LENGTHNOTVALID)

    char *file = generateIndexFileName(fileName, indexNo);
    if (rc = PFMgr.CreateFile(file))
        IX_ERROR(rc)

    IX_IndexHandle indexHandle;
    if (rc = PFMgr.OpenFile(file, indexHandle.indexFH))
        IX_ERROR(rc)
    if (rc = indexHandle.CreateIndex(attrType, attrLength))
        IX_PRINTSTACK
    if (rc = PFMgr.CloseFile(indexHandle.indexFH))
        IX_ERROR(rc)

    return OK_RC;
}

// Destroy and Index
RC IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    RC rc;

    char *file = generateIndexFileName(fileName, indexNo);
    if (rc = PFMgr.DestroyFile(file))
        IX_ERROR(rc)

    return OK_RC;
}

// Open an Index
RC IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
    RC rc;

    char *file = generateIndexFileName(fileName, indexNo);
    if (rc = PFMgr.OpenFile(file, indexHandle.indexFH))
        IX_ERROR(rc)
    if (rc = indexHandle.OpenIndex())
        IX_PRINTSTACK

    return OK_RC;
}

// Close an Index
RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    RC rc;
    if (rc = indexHandle.CloseIndex())
        IX_PRINTSTACK
    if (rc = PFMgr.CloseFile(indexHandle.indexFH))
        IX_ERROR(rc)
    return OK_RC;
}

//generate a unique file name
char* IX_Manager::generateIndexFileName(const char *fileName, int indexNo) {
    size_t fileNameLen = strlen(fileName);
    char *file = new char[fileNameLen + 15];
    strcpy(file, fileName);
    sprintf(file + fileNameLen, "%d", indexNo);
    return file;
}