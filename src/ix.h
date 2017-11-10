//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "global.h"  // Please don't change these lines
#include "pf.h"

class IX_Manager;
class IX_IndexHandle;
class IX_IndexScan;

//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
    friend class IX_Manager;
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();
private:
    PF_FileHandle _indexFileHandle;
    PF_PageHandle _infoPageHandle;
    PageNum _infoPageNum;
    PF_PageHandle _rootPageHandle;
    PageNum _rootPageNum;
    AttrType _attrType;
    size_t _attrLength;
    size_t _keysNum;
    size_t _dataNum;
    PageNum _dataListHead;
    PageNum _dataListTail;

    // need open info page and need to unpin OK
    RC getRootPageNum(PageNum &rootPageNum) const;
    RC setRootPageNum(const PageNum &rootPageNum);
    RC getAttrType(AttrType &attrType) const;
    RC setAttrType(const AttrType &attrType);
    RC getAttrLength(size_t &attrLength) const;
    RC setAttrLength(const size_t &attrLength);
    RC getKeysNum(size_t &keysNum) const;
    RC setKeysNum(const size_t &keysNum);
    RC getDataNum(size_t &dataNum) const;
    RC setDataNum(const size_t &dataNum);
    RC getDataListHead(PageNum &head) const;
    RC setDataListHead(const PageNum &head);
    RC getDataListTail(PageNum &tail) const;
    RC setDataListTail(const PageNum &tail);

    // need open not leaf page and need to unpin
    RC getChildNum(const PF_PageHandle &notLeafPage, size_t &childNum) const;
    RC setChildNum(const PF_PageHandle &notLeafPage, const size_t &childNum);
    RC getPrevPageNum(const PF_PageHandle &notLeafPage, PageNum &prevPageNum) const;
    RC setPrevPageNum(const PF_PageHandle &notLeafPage, const PageNum &prevPageNum);
    RC getNextPageNum(const PF_PageHandle &notLeafPage, PageNum &nextPageNum) const;
    RC setNextPageNum(const PF_PageHandle &notLeafPage, const PageNum &nextPageNum);
    RC getChildArray(const PF_PageHandle &notLeafPage, char *childArray);
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();
};

//
// IX_Manager: provides IX index file management
//
class IX_Manager {
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);

    //test innerFunctions
    static void test();

private:
    PF_Manager &_PFManager;

    //test if length match type
    static RC isValidAttrLength(AttrType attrType, int attrLength);

    //generate a unique file name
    static RC generateIndexFileName(const char *fileName, int indexNo, char *&file);

    //clac the number of keys(points) in not leaf node
    static size_t calcKeysNum(int attrLength);

    //clac the number of keys(RIDs) in leaf node
    static size_t calcDataNum(int attrLength);
};

//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_NOTVALIDATTRLENGTH (START_IX_WARN + 0) //length not match type
#define IX_EOF (START_IX_WARN + 1) //end of file

/*
Info Page
Size: 4096 - size(PageNum)
rootPageNum: PageNum
attrType: AttrType
attrLength: size_t
keysNum: size_t         number of keys(pointers) in non leaf node
dataNum: size_t         number of data in leaf dataNum
dataListHead: PageNum
dataListTail: PageNum
*/

const size_t ROOTPAGENUMOFFSET = 0;
const size_t ATTRTYPEOFFSET = ROOTPAGENUMOFFSET + sizeof(PageNum);
const size_t ATTRLENGTHOFFSET = ATTRTYPEOFFSET + sizeof(AttrType);
const size_t KEYSNUMOFFSET = ATTRLENGTHOFFSET + sizeof(size_t);
const size_t DATANUMOFFSET = KEYSNUMOFFSET + sizeof(size_t);
const size_t DATALISTHEADOFFSET = DATANUMOFFSET + sizeof(size_t);
const size_t DATALISTTAILOFFSET = DATALISTHEADOFFSET + sizeof(PageNum);

/*
Not Leaf Page
Size: 4096 - size(PageNum)
childNum: size_t
prevPageNum: PageNum
nextPageNum: PageNum
childArray: [attrLength + PageNum, ...]

Leaf Page
Size: 4096 - size(PageNum)
childNum: size_t
prevPageNum: PageNum
nextPageNum: PageNum
childArray: [attrLength + RID, ...]
*/

const size_t CHILDNUMOFFSET = 0;
const size_t PREVPAGENUMOFFSET = CHILDNUMOFFSET + sizeof(size_t);
const size_t NEXTPAGENUMOFFSET = PREVPAGENUMOFFSET + sizeof(PageNum);
const size_t CHILDARRAYOFFSET = NEXTPAGENUMOFFSET + sizeof(PageNum);

#endif
