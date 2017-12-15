#include "ix.h"
using namespace std;

IX_IndexHandle::IX_IndexHandle() { isOpen = false; }

IX_IndexHandle::~IX_IndexHandle() {}

// Insert a new index entry
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
    RC rc;

    if (!isOpen)
        IX_ERROR(IX_INDEXHANDLECLOSED)

    char tmp[300];
    memmove(tmp, pData, treeHeader->attrLength);
    memmove(tmp + treeHeader->attrLength, &rid, sizeof(RID));
    
    if (rc = treeHeader->Insert((char*)tmp))
        IX_PRINTSTACK;

    return OK_RC;
}

// Delete a new index entry
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
    RC rc;

    if (!isOpen)
        IX_ERROR(IX_INDEXHANDLECLOSED)

    char tmp[300];
    memmove(tmp, pData, treeHeader->attrLength);
    memmove(tmp + treeHeader->attrLength, &rid, sizeof(RID));

    if (rc = treeHeader->Delete((char*)tmp))
        IX_PRINTSTACK

    return OK_RC;
}

// Force index files to disk
RC IX_IndexHandle::ForcePages() {
    RC rc;

    if (!isOpen)
        IX_ERROR(IX_INDEXHANDLECLOSED)

    if (rc = indexFH.ForcePages())
        IX_ERROR(rc)
    return OK_RC;
}

RC IX_IndexHandle::DisplayAllTree() {
    treeHeader->DisplayAllTree();
}

// Create a new Index.
RC IX_IndexHandle::CreateIndex(AttrType attrType, int attrLength) {
    RC rc;

    // Allocate new info page and new root page.
    PF_PageHandle infoPH;
    PageNum infoPNum;
    char *infoPData;
    PF_PageHandle rootPH;
    PageNum rootPNum;
    char *rootPData;
    if (rc = IX_AllocatePage(indexFH, infoPH, infoPNum, infoPData))
        IX_ERROR(rc)
    if (rc = IX_AllocatePage(indexFH, rootPH, rootPNum, rootPData))
        IX_ERROR(rc)

    // Setup info page and root page.
    treeHeader = (TreeHeader*)infoPData;
    NodeHeader *rootHeader = (NodeHeader*)rootPData;
    treeHeader->infoPNum = infoPNum;
    treeHeader->rootPNum = rootPNum;
    treeHeader->attrType = attrType;
    treeHeader->attrLength = attrLength;
    treeHeader->attrLengthWithRid = attrLength + sizeof(RID);
    treeHeader->childItemSize = sizeof(PageNum);
    treeHeader->maxChildNum = (PF_PAGE_SIZE - sizeof(NodeHeader)) / (treeHeader->attrLengthWithRid + treeHeader->childItemSize);
    treeHeader->dataHeadPNum = treeHeader->rootPNum;
    treeHeader->dataTailPNum = treeHeader->rootPNum;
    rootHeader->selfPNum = rootPNum;
    rootHeader->nodeType = LeafNode;
    rootHeader->parentPNum = -1;
    rootHeader->prevPNum = treeHeader->infoPNum;
    rootHeader->nextPNum = treeHeader->infoPNum;
    rootHeader->childNum = 0;
    if (rc = indexFH.MarkDirty(infoPNum))
        IX_ERROR(rc)
    if (rc = indexFH.MarkDirty(rootPNum))
        IX_ERROR(rc)
    if (rc = indexFH.UnpinPage(infoPNum))
        IX_ERROR(rc)
    if (rc = indexFH.UnpinPage(rootPNum))
        IX_ERROR(rc)

    return OK_RC;
}

RC IX_IndexHandle::OpenIndex() {
    RC rc;
    PF_PageHandle infoPH;
    char *infoPData;
    if (rc = indexFH.GetFirstPage(infoPH))
        IX_ERROR(rc)
    if (rc = infoPH.GetData(infoPData))
        IX_ERROR(rc)
    treeHeader = (TreeHeader*)infoPData;
    treeHeader->pageMap = &(this->pageMap);
    treeHeader->indexFH = &(this->indexFH);
    isOpen = true;
    return OK_RC;
}

RC IX_IndexHandle::CloseIndex() {
    RC rc;
    if (rc = indexFH.MarkDirty(treeHeader->infoPNum))
        IX_ERROR(rc)
    if (rc = indexFH.UnpinPage(treeHeader->infoPNum))
        IX_ERROR(rc)
    isOpen = false;
    return OK_RC;
}
