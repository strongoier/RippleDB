//
// File:        rm_manager.cc
// Description: RM_Manager class implementation
// Authors:     Yi Xu
//

#include "rm.h"

RM_Manager::RM_Manager(PF_Manager &pfm) {
    pPFMgr = &pfm;
}

RM_Manager::~RM_Manager() {
}

RC RM_Manager::CreateFile(const char* fileName, int recordSize) {
    RC rc;
    // check recordSize by calculating numRecordsPerPage
    if (recordSize <= 0) {
        return RM_RECORDSIZETOOSMALL;
    }
    if (recordSize >= PF_PAGE_SIZE) {
        return RM_RECORDSIZETOOLARGE;
    }
    int numRecordsPerPage = 0;
    while (sizeof(PageNum) + (numRecordsPerPage - 1 / 8) + 1 + recordSize * numRecordsPerPage < PF_PAGE_SIZE) {
        ++numRecordsPerPage;
    }
    if (--numRecordsPerPage <= 0) {
        return RM_RECORDSIZETOOLARGE;
    }
    // create file
    if ((rc = pPFMgr->CreateFile(fileName))) {
        return rc;
    }
    // open file
    PF_FileHandle pfFileHandle;
    if ((rc = pPFMgr->OpenFile(fileName, pfFileHandle))) {
        return rc;
    }
    // create header page
    PF_PageHandle pageHandle;
    if ((rc = pfFileHandle.AllocatePage(pageHandle))) {
        return rc;
    }
    // get header page data pointer
    char *pData;
    if ((rc = pageHandle.GetData(pData))) {
        return rc;
    }
    // write header page data
    RM_FileHeader fileHeader(recordSize, numRecordsPerPage);
    *(RM_FileHeader*)pData = fileHeader;
    // get header page num
    PageNum pageNum;
    if ((rc = pageHandle.GetPageNum(pageNum))) {
        return rc;
    }
    // mark header page dirty
    if ((rc = pfFileHandle.MarkDirty(pageNum))) {
        return rc;
    }
    // unpin header page
    if ((rc = pfFileHandle.UnpinPage(pageNum))) {
        return rc;
    }
    // close file
    if ((rc = pPFMgr->CloseFile(pfFileHandle))) {
        return rc;
    }
    // success
    return OK_RC;
}

RC RM_Manager::DestroyFile(const char* fileName) {
    RC rc;
    if ((rc = pPFMgr->DestroyFile(fileName))) {
        return rc;
    }
    return OK_RC;
}

RC RM_Manager::OpenFile(const char* fileName, RM_FileHandle& fileHandle) {
    RC rc;
    // open file
    PF_FileHandle pfFileHandle;
    if ((rc = pPFMgr->OpenFile(fileName, pfFileHandle))) {
        return rc;
    }
    // get header page
    PF_PageHandle pageHandle;
    if ((rc = pfFileHandle.GetFirstPage(pageHandle))) {
        return rc;
    }
    // get header page data pointer
    char* pData;
    if ((rc = pageHandle.GetData(pData))) {
        return rc;
    }
    // generate fileHandle from header page data
    fileHandle.fileHeader = *(RM_FileHeader*)pData;
    fileHandle.pfFileHandle = pfFileHandle;
    fileHandle.isHeaderModified = false;
    fileHandle.isOpen = true;
    // get header page num
    PageNum pageNum;
    if ((rc = pageHandle.GetPageNum(pageNum))) {
        return rc;
    }
    // unpin header page
    if ((rc = pfFileHandle.UnpinPage(pageNum))) {
        return rc;
    }
    // success
    return OK_RC;
}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle) {
    RC rc;
    // check whether fileHandle is open
    if (!fileHandle.isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // check whether fileHeader is modified
    if (fileHandle.isHeaderModified) {
        // get header page
        PF_PageHandle pageHandle;
        if ((rc = fileHandle.pfFileHandle.GetFirstPage(pageHandle))) {
            return rc;
        }
        // get header page data pointer
        char* pData;
        if ((rc = pageHandle.GetData(pData))) {
            return rc;
        }
        // write header page data
        *(RM_FileHeader*)pData = fileHandle.fileHeader;
        // get header page num
        PageNum pageNum;
        if ((rc = pageHandle.GetPageNum(pageNum))) {
            return rc;
        }
        // mark header page dirty
        if ((rc = fileHandle.pfFileHandle.MarkDirty(pageNum))) {
            return rc;
        }
        // unpin header page
        if ((rc = fileHandle.pfFileHandle.UnpinPage(pageNum))) {
            return rc;
        }
    }
    // close file
    if ((rc = pPFMgr->CloseFile(fileHandle.pfFileHandle))) {
        return rc;
    }
    fileHandle.isOpen = false;
    // success
    return OK_RC;
}
