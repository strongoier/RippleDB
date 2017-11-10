#include "ix.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

IX_Manager::IX_Manager(PF_Manager &pfm) : _PFManager(pfm) {}

IX_Manager::~IX_Manager() {}

// Create a new Index
RC IX_Manager::CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength) {
    RC rc;
    if (rc = isValidAttrLength(attrType, attrLength))
        return rc;
    char *file;
    if (rc = generateIndexFileName(fileName, indexNo, file))
        return rc;
    if (rc = _PFManager.CreateFile(file))
        return rc;
    IX_IndexHandle indexHandle;
    // need to close OK
    if (rc = _PFManager.OpenFile(file, indexHandle._indexFileHandle))
        return rc;
    // need to unpin OK
    if (rc = indexHandle._indexFileHandle.AllocatePage(indexHandle._infoPageHandle))
        return rc;
    PF_PageHandle rootPage;
    // need to unpin OK
    if (rc = indexHandle._indexFileHandle.AllocatePage(rootPage))
        return rc;
    if (rc = indexHandle.setRootPageNum(rootPage.GetPageNum))
        return rc;
    if (rc = indexHandle.setAttrType(attrType))
        return rc;
    if (rc = indexHandle.setAttrLength(attrLength))
        return rc;
    if (rc = indexHandle.setKeysNum(calcKeysNum(attrLength)))
        return rc;
    if (rc = indexHandle.setDataNum(calcDataNum(attrLength)))
        return rc;
    PageNum infoPageNum;
    if (rc = indexHandle._infoPageHandle.GetPageNum(infoPageNum))
        return rc;
    if (rc = indexHandle.setDataListHead(infoPageNum))
        return rc;
    if (rc = indexHandle.setDataListTail(infoPageNum))
        return rc;
    PageNum rootPageNum;
    if (rc = rootPage.GetPageNum(rootPageNum))
        return rc;
    if (rc = indexHandle.setChildNum(rootPage, 0))
        return rc;
    if (rc = indexHandle.setPrevPageNum(rootPage, rootPageNum))
        return rc;
    if (rc = indexHandle.setNextPageNum(rootPage, rootPageNum))
        return rc;
    if (rc = indexHandle._indexFileHandle.UnpinPage(rootPageNum))
        return rc;
    if (rc = indexHandle._indexFileHandle.UnpinPage(infoPageNum))
        return rc;
    if (rc = _PFManager.CloseFile(indexHandle._indexFileHandle));
        return rc;
    return OK_RC;
}

// Destroy and Index
RC IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    RC rc;
    char *file;
    if (rc = generateIndexFileName(fileName, indexNo, file))
        return rc;
    if (rc = _PFManager.DestroyFile(file))
        return rc;
    return OK_RC;
}

// Open an Index
RC IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
    RC rc;
    char *file;
    if (rc = generateIndexFileName(fileName, indexNo, file))
        return rc;
    // need to close in CloseIndex function OK
    if (rc = _PFManager.OpenFile(file, indexHandle._indexFileHandle))
        return rc;
    // need to unpin in CloseIndex function OK
    if (rc = indexHandle._indexFileHandle.GetFirstPage(indexHandle._infoPageHandle))
        return rc;
    return OK_RC;
}

// Close an Index
RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    RC rc;
    PageNum infoPageNum;
    if (rc = indexHandle._infoPageHandle.GetPageNum(infoPageNum))
        return rc;
    if (rc = indexHandle._indexFileHandle.UnpinPage(infoPageNum))
        return rc;
    if (rc = _PFManager.CloseFile(indexHandle._indexFileHandle))
        return rc;
    return OK_RC;
}


//test innerFunctions
void IX_Manager::test() {
    RC rc;

    printf("IX_Manager TEST isValidAttrLength:\n");
    int randomLen;
    rc = isValidAttrLength(INT, 4);
    if (rc) printf("Fail, INT 4\n");
    else printf("Success\n");
    randomLen = rand() % 512;
    rc = isValidAttrLength(INT, randomLen);
    if (rc && randomLen == 4 || !rc && randomLen != 4) printf("Fail, INT %d\n", randomLen);
    else printf("Success\n");
    rc = isValidAttrLength(FLOAT, 4);
    if (rc) printf("Fail, FLOAT 4\n");
    randomLen = rand() % 512;
    rc = isValidAttrLength(FLOAT, randomLen);
    if (rc && randomLen == 4 || !rc && randomLen != 4) printf("Fail, FLOAT %d\n", randomLen);
    else printf("Success\n");
    rc = isValidAttrLength(STRING, 30);
    if (rc) printf("Fail, STRING 30\n");
    else printf("Success\n");
    randomLen = rand() % 512;
    rc = isValidAttrLength(STRING, randomLen);
    if (rc && randomLen >= 1 && randomLen <= MAXSTRINGLEN || !rc && (randomLen < 1 || randomLen > MAXSTRINGLEN)) printf("Fail, STRING %d\n", randomLen);
    else printf("Success\n");
    printf("TEST END\n");

    printf("IX_Manager TEST generateIndexFileName\n");
    const char *fileName = "yansh";
    int indexNo = rand();
    char *file;
    rc = generateIndexFileName(fileName, indexNo, file);
    printf("%s %d\n", file, indexNo);
    printf("TEST END\n");
}

//test if length match type
RC IX_Manager::isValidAttrLength(AttrType attrType, int attrLength) {
    if (attrType == INT && attrLength == 4)
        return OK_RC;
    if (attrType == FLOAT && attrLength == 4)
        return OK_RC;
    if (attrType == STRING && attrLength >= 1 && attrLength <= MAXSTRINGLEN)
        return OK_RC;
    return IX_NOTVALIDATTRLENGTH;
}

//generate a unique file name
RC IX_Manager::generateIndexFileName(const char *fileName, int indexNo, char *&file) {
    size_t fileNameLen = strlen(fileName);
    file = new char[fileNameLen + 15];
    strcpy(file, fileName);
    sprintf(file + fileNameLen, "%d", indexNo);
    return OK_RC;
}

//clac the number of keys(points) in not leaf node
size_t IX_Manager::calcKeysNum(int attrLength) {
    return (PF_PAGE_SIZE - CHILDARRAYOFFSET) / (attrLength + sizeof(PageNum));
}

//clac the number of keys(RIDs) in leaf node
size_t IX_Manager::calcDataNum(int attrLength) {
    return (PF_PAGE_SIZE - CHILDARRAYOFFSET) / (attrLength + sizeof(RID));
}