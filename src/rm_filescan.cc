//
// File:        rm_filescan.cc
// Description: RM_FileScan class implementation
// Authors:     Yi Xu
//

#include "rm.h"
#include <cstring>

RM_FileScan::RM_FileScan() {
    isOpen = RM_SCANSTATUS_CLOSE;
}

RM_FileScan::~RM_FileScan() {
}

RC RM_FileScan::OpenScan(const RM_FileHandle& fileHandle, AttrType attrType, int attrLength,
                         int attrOffset, CompOp compOp, void* value) {
    RC rc;
    // check whether fileScan is already open
    if (isOpen != RM_SCANSTATUS_CLOSE) {
        return RM_FILESCANOPEN;
    }
    // check whether fileHandle is open
    if (!fileHandle.isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // check whether attr is valid
    if (attrOffset < 0 || attrOffset + attrLength > fileHandle.fileHeader.recordSize) {
        return RM_ATTRINVALID;
    }
    // copy the parameters
    this->fileHeader = fileHandle.fileHeader;
    this->pfFileHandle = fileHandle.pfFileHandle;
    this->attrType = attrType;
    this->attrLength = attrLength;
    this->attrOffset = attrOffset;
    this->compOp = compOp;
    this->value = value;
    // get first page
    if ((rc = pfFileHandle.GetFirstPage(pageHandle))) {
        return rc;
    }
    if ((rc = pageHandle.GetPageNum(pageNum))) {
        return rc;
    }
    slotNum = fileHeader.numRecordsPerPage - 1;
    // success
    isOpen = RM_SCANSTATUS_SINGLE;
    isEOF = false;
    return OK_RC;
}

RC RM_FileScan::OpenScan(const RM_FileHandle& fileHandle, const std::vector<FullCondition>& conditions) {
    RC rc;
    // check whether fileScan is already open
    if (isOpen != RM_SCANSTATUS_CLOSE) {
        return RM_FILESCANOPEN;
    }
    // check whether fileHandle is open
    if (!fileHandle.isOpen) {
        return RM_FILEHANDLECLOSED;
    }
    // copy the parameters
    this->fileHeader = fileHandle.fileHeader;
    this->pfFileHandle = fileHandle.pfFileHandle;
    this->conditions = conditions;
    // get first page
    if ((rc = pfFileHandle.GetFirstPage(pageHandle))) {
        return rc;
    }
    if ((rc = pageHandle.GetPageNum(pageNum))) {
        return rc;
    }
    slotNum = fileHeader.numRecordsPerPage - 1;
    // success
    isOpen = RM_SCANSTATUS_MULTIPLE;
    isEOF = false;
    return OK_RC;
}

RC RM_FileScan::GetNextRec(RM_Record& rec) {
    RC rc;
    // check whether fileScan is open
    if (isOpen == RM_SCANSTATUS_CLOSE) {
        return RM_FILESCANCLOSED;
    }
    // check whether isEOF
    if (isEOF) {
        return RM_EOF;
    }
    // find next rec
    bool found = false;
    do {
        // go to next slot
        if (slotNum == fileHeader.numRecordsPerPage - 1) {
            if ((rc = pfFileHandle.UnpinPage(pageNum))) {
                return rc;
            }
            if ((rc = pfFileHandle.GetNextPage(pageNum, pageHandle))) {
                if (rc == PF_EOF) {
                    isEOF = true;
                    return RM_EOF;
                }
                return rc;
            }
            if ((rc = pageHandle.GetData(pData))) {
                return rc;
            }
            if ((rc = pageHandle.GetPageNum(pageNum))) {
                return rc;
            }
            slotNum = 0;
        } else {
            ++slotNum;
        }
        // check whether record exist and satisfies the scan condition
        if (pData[sizeof(PageNum) + slotNum / 8] & (1 << (slotNum & 7))) {
            bool satisfy = true;
            if (isOpen == RM_SCANSTATUS_SINGLE) {
                satisfy = Attr::CompareAttr(attrType, attrLength, pData + sizeof(PageNum) + fileHeader.bitmapSize + slotNum * fileHeader.recordSize + attrOffset, compOp, value);
            } else if (isOpen == RM_SCANSTATUS_MULTIPLE) {
                for (unsigned int i = 0; satisfy && i < conditions.size(); ++i) {
                    satisfy = satisfy && Attr::CompareAttr(conditions[i].lhsAttr.attrType, conditions[i].lhsAttr.attrLength, pData + sizeof(PageNum) + fileHeader.bitmapSize + slotNum * fileHeader.recordSize + conditions[i].lhsAttr.offset, conditions[i].op, conditions[i].bRhsIsAttr ? pData + sizeof(PageNum) + fileHeader.bitmapSize + slotNum * fileHeader.recordSize + conditions[i].rhsAttr.offset : conditions[i].rhsValue.data);
                }
            }
            if (satisfy) {
                rec.rid = RID(pageNum, slotNum);
                if (rec.pData != NULL) {
                    delete[] rec.pData;
                }
                rec.pData = new char[fileHeader.recordSize];
                memcpy(rec.pData, pData + sizeof(PageNum) + fileHeader.bitmapSize + slotNum * fileHeader.recordSize, fileHeader.recordSize);
                found = true;
            }
        }
    } while (!found);
    // success
    return OK_RC;
}

RC RM_FileScan::CloseScan() {
    RC rc;
    // check whether fileScan is open
    if (isOpen == RM_SCANSTATUS_CLOSE) {
        return RM_FILESCANCLOSED;
    }
    // unpin current page
    if (!isEOF && (rc = pfFileHandle.UnpinPage(pageNum))) {
        return rc;
    }
    // success
    isOpen = RM_SCANSTATUS_CLOSE;
    return OK_RC;
}
