//
// File:        rm_record.cc
// Description: RM_Record class implementation
// Authors:     Yi Xu
//

#include "rm.h"

RM_Record::RM_Record() {
    this->pData = NULL;
}

RM_Record::~RM_Record() {
    if (this->pData != NULL) {
        delete[] this->pData;
    }
}

RC RM_Record::GetData(char*& pData) const {
    if (this->pData == NULL) {
        return RM_RECORDUNREAD;
    }
    pData = this->pData;
    return OK_RC;
}

RC RM_Record::GetRid(RID& rid) const {
    if (this->pData == NULL) {
        return RM_RECORDUNREAD;
    }
    rid = this->rid;
    return OK_RC;
}
