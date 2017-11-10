//
// File:        rid.cc
// Description: RID class implementation
// Authors:     Yi Xu
//

#include "global.h"

RID::RID() {
    // use default values
    this->pageNum = NULL_PAGE_NUM;
    this->slotNum = NULL_SLOT_NUM;
}

RID::RID(PageNum pageNum, SlotNum slotNum) {
    this->pageNum = pageNum;
    this->slotNum = slotNum;
}

RID::~RID() {
}

RC RID::GetPageNum(PageNum &pageNum) const {
    pageNum = this->pageNum;
    return OK_RC;
}

RC RID::GetSlotNum(SlotNum &slotNum) const {
    slotNum = this->slotNum;
    return OK_RC;
}
