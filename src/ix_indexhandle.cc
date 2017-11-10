#include "ix.h"

// Insert a new index entry
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
    RC rc;
    
}

// Delete a new index entry
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {

}

// Force index files to disk
RC IX_IndexHandle::ForcePages() {
    RC rc;
    if (rc = _indexFileHandle.ForcePages())
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getRootPageNum(PageNum &rootPageNum) const {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    rootPageNum = *(PageNum*)(pData + ROOTPAGENUMOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setRootPageNum(const PageNum &rootPageNum) {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    *(PageNum*)(pData + ROOTPAGENUMOFFSET) = rootPageNum;
    if (rc = _indexFileHandle.MarkDirty(_infoPageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getAttrType(AttrType &attrType) const {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    attrType = *(AttrType*)(pData + ATTRTYPEOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setAttrType(const AttrType &attrType) {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    *(AttrType*)(pData + ATTRTYPEOFFSET) = attrType;
    if (rc = _indexFileHandle.MarkDirty(_infoPageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getAttrLength(size_t &attrLength) const {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    attrLength = *(size_t*)(pData + ATTRLENGTHOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setAttrLength(const size_t &attrLength) {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    *(size_t*)(pData + ATTRLENGTHOFFSET) = attrLength;
    if (rc = _indexFileHandle.MarkDirty(_infoPageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getKeysNum(size_t &keysNum) const {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    keysNum = *(size_t*)(pData + KEYSNUMOFFSET);
    return OK_RCd;
}

RC IX_IndexHandle::setKeysNum(const size_t &keysNum) {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    *(size_t*)(pData + KEYSNUMOFFSET) = keysNum;
    if (rc = _indexFileHandle.MarkDirty(_infoPageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getDataNum(size_t &dataNum) const {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    dataNum = *(size_t*)(pData + DATANUMOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setDataNum(const size_t &dataNum) {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    *(size_t*)(pData + DATANUMOFFSET) = dataNum;
    if (rc = _indexFileHandle.MarkDirty(_infoPageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getDataListHead(PageNum &head) const {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    head = *(PageNum*)(pData + DATALISTHEADOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setDataListHead(const PageNum &head) {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    *(PageNum*)(pData + DATALISTHEADOFFSET) = head;
    if (rc = _indexFileHandle.MarkDirty(_infoPageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getDataListTail(PageNum &tail) const {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    tail = *(PageNum*)(pData + DATALISTTAILOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setDataListTail(const PageNum &tail) {
    RC rc;
    char *pData;
    if (rc = _infoPageHandle.GetData(pData))
        return rc;
    *(PageNum*)(pData + DATALISTTAILOFFSET) = tail;
    if (rc = _indexFileHandle.MarkDirty(_infoPageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getChildNum(const PF_PageHandle &notLeafPage, size_t &childNum) const {
    RC rc;
    char *pData;
    if (rc = notLeafPage.GetData(pData))
        return rc;
    childNum = *(size_t*)(pData + CHILDNUMOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setChildNum(const PF_PageHandle &notLeafPage, const size_t &childNum) {
    RC rc;
    char *pData;
    if (rc = notLeafPage.GetData(pData))
        return rc;
    *(size_t*)(pData + CHILDNUMOFFSET) = childNum;
    PageNum pageNum;
    if (rc = notLeafPage.GetPageNum(pageNum))
        return rc;
    if (rc = _indexFileHandle.MarkDirty(pageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getPrevPageNum(const PF_PageHandle &notLeafPage, PageNum &prevPageNum) const {
    RC rc;
    char *pData;
    if (rc = notLeafPage.GetData(pData))
        return rc;
    prevPageNum = *(PageNum*)(pData + PREVPAGENUMOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setPrevPageNum(const PF_PageHandle &notLeafPage, const PageNum &prevPageNum) {
    RC rc;
    char *pData;
    if (rc = notLeafPage.GetData(pData))
        return rc;
    *(PageNum*)(pData + PREVPAGENUMOFFSET) = prevPageNum;
    PageNum pageNum;
    if (rc = notLeafPage.GetPageNum(pageNum))
        return rc;
    if (rc = _indexFileHandle.MarkDirty(pageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getNextPageNum(const PF_PageHandle &notLeafPage, PageNum &nextPageNum) const {
    RC rc;
    char *pData;
    if (rc = notLeafPage.GetData(pData))
        return rc;
    nextPageNum = *(PageNum*)(pData + NEXTPAGENUMOFFSET);
    return OK_RC;
}

RC IX_IndexHandle::setNextPageNum(const PF_PageHandle &notLeafPage, const PageNum &nextPageNum) {
    RC rc;
    char *pData;
    if (rc = notLeafPage.GetData(pData))
        return rc;
    *(PageNum*)(pData + NEXTPAGENUMOFFSET) = nextPageNum;
    PageNum pageNum;
    if (rc = notLeafPage.GetPageNum(pageNum))
        return rc;
    if (rc = _indexFileHandle.MarkDirty(pageNum))
        return rc;
    return OK_RC;
}

RC IX_IndexHandle::getChildArray(const PF_PageHandle &notLeafPage, char *childArray) {
    RC rc;
    char *pData;
    if (rc = notLeafPage.GetData(pData))
        return rc;
    childArray = (char*)(pData + CHILDARRAYOFFSET);
    return OK_RC;
}