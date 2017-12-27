//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Authors:     Yi Xu
//

#include "rm.h"
#include <cstring>

RM_FileHandle::RM_FileHandle() {
    isOpen = false;
}

RM_FileHandle::~RM_FileHandle() {
}

RC RM_FileHandle::GetRec(const RID& rid, RM_Record& rec) const {
    RC rc;
    // check whether fileHandle is open
    if (!isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // check whether record with given rid is exist and get its info if exist
    PageNum pageNum;
    SlotNum slotNum;
    char* pData;
    if ((rc = CheckRecExist(rid, pageNum, slotNum, pData))) {
        return rc;
    }
    // copy record data to rec
    rec.rid = rid;
    if (rec.pData != NULL) {
        delete[] rec.pData;
    }
    rec.pData = new char[fileHeader.recordSize];
    memcpy(rec.pData, pData + sizeof(PageNum) + fileHeader.bitmapSize + slotNum * fileHeader.recordSize, fileHeader.recordSize);
    // unpin page
    if ((rc = pfFileHandle.UnpinPage(pageNum))) {
        return rc;
    }
    // success
    return OK_RC;
}

RC RM_FileHandle::InsertRec(const char *pRecData, RID &rid) {
    RC rc;
    // check whether fileHandle is open
    if (!isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // get data pointer of first free page
    char* pData;
    if (fileHeader.firstFreePageNum == RM_PAGE_LIST_END) {
        // allocate a new page
        PF_PageHandle pageHandle;
        if ((rc = pfFileHandle.AllocatePage(pageHandle))) {
            return rc;
        }
        // get pageNum of the page
        if ((rc = pageHandle.GetPageNum(fileHeader.firstFreePageNum))) {
            return rc;
        }
        // get data pointer of the page
        if ((rc = pageHandle.GetData(pData))) {
           return rc;
        }
        // initialize the page
        *(PageNum*)pData = RM_PAGE_LIST_END;
        for (int i = 0; i < fileHeader.bitmapSize; ++i) {
            pData[sizeof(PageNum) + i] = 0xff;
        }
        for (int i = 0; i < fileHeader.numRecordsPerPage; ++i) {
            pData[sizeof(PageNum) + i / 8] ^= 1 << (i & 7);
        }
        isHeaderModified = true;
    } else {
        // get pageHandle of first free page
        PF_PageHandle pageHandle;
        if ((rc = pfFileHandle.GetThisPage(fileHeader.firstFreePageNum, pageHandle))) {
            return rc;
        }
        // get data pointer of the page
        if ((rc = pageHandle.GetData(pData))) {
           return rc;
        }
    }
    // find first available slot and insert the record
    PageNum pageNum = fileHeader.firstFreePageNum;
    int i = 0;
    for (; i < fileHeader.bitmapSize; ++i) {
        if ((unsigned char)pData[sizeof(PageNum) + i] != 0xff) {
            break;
        }
    }
    for (int j = 0; j < 8; ++j) {
        if (!(pData[sizeof(PageNum) + i] & (1 << j))) {
            pData[sizeof(PageNum) + i] ^= 1 << j;
            memcpy(pData + sizeof(PageNum) + fileHeader.bitmapSize + (i * 8 + j) * fileHeader.recordSize, pRecData, fileHeader.recordSize);
            rid.pageNum = pageNum;
            rid.slotNum = i * 8 + j;
            break;
        }
    }
    // if page is full, update firstFreePageNum
    bool isFull = true;
    for (; isFull && i < fileHeader.bitmapSize; ++i) {
        if ((unsigned char)pData[sizeof(PageNum) + i] != 0xff) {
            isFull = false;
        }
    }
    if (isFull) {
        fileHeader.firstFreePageNum = *(PageNum*)pData;
        *(PageNum*)pData = RM_PAGE_FULL;
        isHeaderModified = true;
    }
    // mark page dirty
    if ((rc = pfFileHandle.MarkDirty(pageNum))) {
        return rc;
    }
    // unpin page
    if ((rc = pfFileHandle.UnpinPage(pageNum))) {
        return rc;
    }
    // success
    return OK_RC;
}

RC RM_FileHandle::DeleteRec(const RID &rid) {
    RC rc;
    // check whether fileHandle is open
    if (!isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // check whether record with given rid is exist and get its info if exist
    PageNum pageNum;
    SlotNum slotNum;
    char* pData;
    if ((rc = CheckRecExist(rid, pageNum, slotNum, pData))) {
        return rc;
    }
    // if page is full, it will become another free page
    if (*(PageNum*)pData == RM_PAGE_FULL) {
        *(PageNum*)pData = fileHeader.firstFreePageNum;
        fileHeader.firstFreePageNum = pageNum;
        isHeaderModified = true;
    }
    // mark slot as available
    pData[sizeof(PageNum) + slotNum / 8] ^= 1 << (slotNum & 7);
    // mark page dirty
    if ((rc = pfFileHandle.MarkDirty(pageNum))) {
        return rc;
    }
    // unpin page
    if ((rc = pfFileHandle.UnpinPage(pageNum))) {
        return rc;
    }
    // success
    return OK_RC;
}

RC RM_FileHandle::UpdateRec(const RM_Record& rec) {
    RC rc;
    // check whether fileHandle is open
    if (!isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // check whether rec has been read and get its rid if success
    RID rid;
    if ((rc = rec.GetRid(rid))) {
        return rc;
    }
    // check whether record with given rid is exist and get its info if exist
    PageNum pageNum;
    SlotNum slotNum;
    char* pData;
    if ((rc = CheckRecExist(rid, pageNum, slotNum, pData))) {
        return rc;
    }
    // update rec
    memcpy(pData + sizeof(PageNum) + fileHeader.bitmapSize + slotNum * fileHeader.recordSize, rec.pData, fileHeader.recordSize);
    // mark page dirty
    if ((rc = pfFileHandle.MarkDirty(pageNum))) {
        return rc;
    }
    // unpin page
    if ((rc = pfFileHandle.UnpinPage(pageNum))) {
        return rc;
    }
    // success
    return OK_RC;
}

RC RM_FileHandle::ForcePages(PageNum pageNum) {
    RC rc;
    // check whether fileHandle is open
    if (!isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // do ForcePages
    if ((rc = pfFileHandle.ForcePages(pageNum))) {
        return rc;
    }
    // success
    return OK_RC;
}

RC RM_FileHandle::CheckRecExist(const RID& rid, PageNum& pageNum, SlotNum& slotNum, char*& pData) const {
    RC rc;
    // check whether rid is legal
    if ((rc = rid.GetPageNum(pageNum))) {
        return rc;
    }
    if ((rc = rid.GetSlotNum(slotNum))) {
        return rc;
    }
    // get pageHandle
    PF_PageHandle pageHandle;
    if ((rc = pfFileHandle.GetThisPage(pageNum, pageHandle))) {
        return rc;
    }
    // get data pointer of the page
    if ((rc = pageHandle.GetData(pData))) {
        return rc;
    }
    // check whether record with given rid is exist
    if (slotNum < 0 || slotNum >= fileHeader.numRecordsPerPage) {
        return RM_RECORDNOTEXIST;
    }
    if (!(pData[sizeof(PageNum) + slotNum / 8] & (1 << (slotNum & 7)))) {
        return RM_RECORDNOTEXIST;
    }
    // success
    return OK_RC;
}
