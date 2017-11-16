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
#include <map>
using std::map;

class IX_Manager;
class IX_IndexHandle;
class IX_IndexScan;
struct TreeHeader;
struct NodeHeader;

enum NodeType {
    EmptyNode,
    LeafNode,
    InternalNode,
    OverflowNode
};

struct TreeHeader {
    PageNum infoPNum;
    PageNum rootPNum;
    AttrType attrType;
    int attrLength;
    int childItemSize;
    int maxChildNum;
    bool duplicateValuesAllowed;
    PageNum dataHeadPNum;
    PageNum dataTailPNum;
    PF_FileHandle* indexFH;
    map<PageNum, char*> *pageMap;

    RC GetPageData(PageNum pNum, NodeHeader *&pData);
    RC AllocatePage(PageNum &pageNum, NodeHeader *&pageData);
    RC UnpinPages();

    RC Insert(char *pData, const RID& rid);
    RC Delete(char *pData, const RID& rid);

    RC SearchLeafNode(char *pData, NodeHeader *cur, NodeHeader *&leaf);
};

struct NodeHeader {
    PageNum selfPNum;
    NodeType nodeType;
    PageNum parentPNum;
    PageNum prevPNum;
    PageNum nextPNum;
    int childNum;
    char* keys;
    char* values;
    TreeHeader *tree;

    bool IsEmpty();
    bool IsFull();
    bool HavePrevPage();
    bool HaveNextPage();
    bool HaveParentPage();

    char *key(int index);
    RID *rid(int index);
    PageNum *page(int index);

    int UpperBound(char *pData);
    int LowerBound(char *pData);
    pair<int, int> EqualRange(char *pData);
    bool BinarySearch(char *pData);

    RC InsertRID(char *pData, const RID &rid);
    RC InsertPage(char *pData, PageNum newPage);

    RC ChildPage(int index, NodeHeader *&child);
    RC PrevPage(NodeHeader *&prev);
    RC NextPage(NodeHeader *&next);
    RC ParentPage(NodeHeader *&parent);
    RC ParentPrevKey(char *&key);
    RC ParentNextKey(char *&key);

    RC UpdateKey(char *oldKey, char *newKey);
};

namespace PFHelper {
    RC AllocatePage(PF_FileHandle &fh, PF_PageHandle &ph, PageNum &pNum, char *&pData);
}

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
    PF_FileHandle indexFH;
    TreeHeader *treeHeader;
    map<PageNum, char*> pageMap;

    RC CreateIndex(AttrType attrType, int attrLength);

    RC OpenIndex();

    RC CloseIndex();
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value);

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

private:
    PF_Manager &PFMgr;

    //generate a unique file name
    static char* generateIndexFileName(const char *fileName, int indexNo);
};

//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_LENGTHNOTVALID (START_IX_WARN + 0)
#define IX_INSERTRIDINFULLNODE (START_IX_WARN + 1)
#define IX_INSERTRIDTOINTERNALNODE (START_IX_WARN + 2)
#define IX_UPDATEKEYINLEAFNODE (START_IX_WARN + 3)
#define IX_INSERTPAGETOLEAFNODE (START_IX_WARN + 4)
#define IX_GETPARENTKEYINLEAFNODE (START_IX_WARN + 5)

#define IX_ERROR(rc) { if (rc > 0 && rc < 100 || rc < 0 && rc >- -100) PF_PrintError(rc); else if (rc > 100 && rc < 200 || rc < -100 && rc > -200) IX_PrintError(rc); printf("CALLSTACK:\nFILE: %s, FUNC: %s, LINE: %d\n", __FILE__, __func__, __LINE__); return rc; }

#define IX_PRINTSTACK { printf("FILE: %s, FUNC: %s, LINE: %d\n", __FILE__, __func__, __LINE__); return rc; }

#endif