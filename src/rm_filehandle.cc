//
// File:        rm_filehandle.cc
// Description: RM_FileHandle class implementation
// Authors:     Yi Xu
//

RM_FileHandle::RM_FileHandle() {
    fileName = new char[MAXSTRINGLEN];
    isOpen = false;
}

~RM_FileHandle() {
    delete[] fileName;
}

RC GetRec(const RID &rid, RM_Record &rec) const {

}

RC InsertRec(const char *pData, RID &rid) {

}

RC DeleteRec(const RID &rid) {

}

RC UpdateRec(const RM_Record &rec) {

}

RC ForcePages (PageNum pageNum = ALL_PAGES) {

}
