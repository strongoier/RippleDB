//
// File:        rm.h
// Description: Record Manager component interface
// Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
//

#ifndef RM_H
#define RM_H

// Do not change the following includes
#include "global.h"
#include "pf.h"

//
// RM_FileHeader: Header structure for files
//
#define RM_PAGE_LIST_END -1
#define RM_PAGE_FULL     -2

struct RM_FileHeader {
    int recordSize;           // record size
    int numRecordsPerPage;    // number of records per page
    PageNum firstFreePageNum; // firstFreePageNum can be any of these values:
                              //  - the number of the first free page
                              //  - RM_PAGE_LIST_END if there is no free page
    int bitmapSize;           // sizeof(bitmap)

    RM_FileHeader() {
    }
    RM_FileHeader(int recordSize, int numRecordsPerPage, PageNum firstFreePageNum = RM_PAGE_LIST_END) :
        recordSize(recordSize), numRecordsPerPage(numRecordsPerPage), firstFreePageNum(firstFreePageNum), bitmapSize((numRecordsPerPage - 1 / 8) + 1) {
    }
};

//
// RM_Record: RM Record interface
//
class RM_Record {
public:
    friend class RM_FileHandle;
    friend class RM_FileScan;

    RM_Record();
    ~RM_Record();

    // Return the data corresponding to the record.
    RC GetData(char*& pData) const;
    // Return the RID associated with the record.
    RC GetRid(RID& rid) const;

private:
    // Disable copy constructor and overloaded =.
    RM_Record(const RM_Record&);
    RM_Record& operator =(const RM_Record&);

    char* pData; // record contents pointer
    RID rid; // RID associated with the record
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
public:
    friend class RM_Manager;
    friend class RM_FileScan;

    RM_FileHandle();
    ~RM_FileHandle();

    // Return the record with given RID.
    RC GetRec(const RID& rid, RM_Record& rec) const;
    // Insert a new record and return its RID.
    RC InsertRec(const char* pData, RID& rid);
    // Delete the record with given RID.
    RC DeleteRec(const RID& rid);
    // Update the record with given RID.
    RC UpdateRec(const RM_Record& rec);
    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk. Default value forces all pages.
    RC ForcePages(PageNum pageNum = ALL_PAGES);

private:
    // Disable copy constructor and overloaded =.
    RM_FileHandle(const RM_FileHandle&);
    RM_FileHandle& operator =(const RM_FileHandle&);

    // Check whether record with given rid is exist and get its info if exist.
    RC CheckRecExist(const RID& rid, PageNum& pageNum, SlotNum& slotNum, char*& pData) const;

    RM_FileHeader fileHeader; // header of the file
    PF_FileHandle pfFileHandle; // internal PF_FileHandle
    bool isOpen; // whether this fileHandle is open for a file (which means valid)
    bool isHeaderModified; // whether header of the file is modified
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan {
public:
    RM_FileScan();
    ~RM_FileScan();

    // Initialize a file scan.
    RC OpenScan(const RM_FileHandle& fileHandle, AttrType attrType, int attrLength, int attrOffset, CompOp compOp, void* value);
    // Get next matching record.
    RC GetNextRec(RM_Record& rec);
    // Close the scan.
    RC CloseScan();

private:
    // Disable copy constructor and overloaded =.
    RM_FileScan(const RM_FileScan&);
    RM_FileScan& operator =(const RM_FileScan&);

    RM_FileHeader fileHeader; // header of the file
    PF_FileHandle pfFileHandle; // internal PF_FileHandle
    AttrType attrType; // type of the attribute being compared
    int attrLength; // length of the attribute being compared
    int attrOffset; // position in a record of the attribute being compared
    CompOp compOp; // comparing operator
    void* value; // value being compared
    PF_PageHandle pageHandle; // current pageHandle
    char *pData; // current page data pointer
    PageNum pageNum; // current pageNum;
    SlotNum slotNum; // current slotNum
    bool isOpen; // whether this fileScan is open
    bool isEOF; // whether there are no records left satisfying the scan condition
};

//
// RM_Manager: provides RM file management
//
class RM_Manager {
public:
    RM_Manager(PF_Manager& pfm);
    ~RM_Manager();

    // Create a file with given fileName and recordSize.
    RC CreateFile(const char* fileName, int recordSize);
    // Destroy the file with given fileName.
    RC DestroyFile(const char* fileName);
    // Open the file with given fileName and return its fileHandle.
    RC OpenFile(const char* fileName, RM_FileHandle& fileHandle);
    // Close the file with given fileHandle.
    RC CloseFile(RM_FileHandle& fileHandle);

private:
    // Disable copy constructor and overloaded =.
    RM_Manager(const RM_Manager&);
    RM_Manager& operator =(const RM_Manager&);

    PF_Manager* pPFMgr; // internal PF_Manager pointer
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_RECORDSIZETOOSMALL (START_RM_WARN + 0)  // record size is too small to handle
#define RM_RECORDSIZETOOLARGE (START_RM_WARN + 1)  // record size is too large to handle
#define RM_FILEHANDLECLOSED   (START_RM_WARN + 2)  // file handle is not open for a file
#define RM_RECORDUNREAD       (START_RM_WARN + 3)  // record has not been read
#define RM_RECORDNOTEXIST     (START_RM_WARN + 4)  // there is no record with given RID
#define RM_FILESCANOPEN       (START_RM_WARN + 5)  // file scan is already open
#define RM_FILESCANCLOSED     (START_RM_WARN + 6)  // file scan is not open
#define RM_ATTRINVALID        (START_RM_WARN + 7)  // attr is invalid (offset or length)
#define RM_EOF                (START_RM_WARN + 8)  // there are no records left satisfying the scan condition

#endif
