//
// File:        sm_internal.cc
// Description: SM internal classes implementation
// Authors:     Yi Xu
//

#include "sm.h"

RelCat::RelCat(const char* recordData) {
    strcpy(relName, recordData);
    tupleLength = *(int*)(recordData + MAXNAME);
    attrCount = *(int*)(recordData + MAXNAME + sizeof(int));
    indexCount = *(int*)(recordData + MAXNAME + sizeof(int) + sizeof(int));
}

RelCat::RelCat(const char* relName, int tupleLength, int attrCount, int indexCount)
    : tupleLength(tupleLength), attrCount(attrCount), indexCount(indexCount) {
    strcpy(this->relName, relName);
}

void RelCat::WriteRecordData(char* recordData) {
    Attr::SetAttr(recordData, STRING, &relName);
    Attr::SetAttr(recordData + MAXNAME, INT, &tupleLength);
    Attr::SetAttr(recordData + MAXNAME + sizeof(int), INT, &attrCount);
    Attr::SetAttr(recordData + MAXNAME + sizeof(int) + sizeof(int), INT, &indexCount);
}

AttrCat::AttrCat(const char* recordData) {
    strcpy(relName, recordData);
    strcpy(attrName, recordData + MAXNAME);
    offset = *(int*)(recordData + MAXNAME + MAXNAME);
    attrType = (AttrType)*(int*)(recordData + MAXNAME + MAXNAME + sizeof(int));
    attrLength = *(int*)(recordData + MAXNAME + MAXNAME + sizeof(int) + sizeof(int));
    indexNo = *(int*)(recordData + MAXNAME + MAXNAME + sizeof(int) + sizeof(int) + sizeof(int));
}

AttrCat::AttrCat(const char* relName, const char* attrName, int offset, AttrType attrType, int attrLength, int indexNo)
    : offset(offset), attrType(attrType), attrLength(attrLength), indexNo(indexNo) {
    strcpy(this->relName, relName);
    strcpy(this->attrName, attrName);
}

void AttrCat::WriteRecordData(char* recordData) {
    Attr::SetAttr(recordData, STRING, relName);
    Attr::SetAttr(recordData + MAXNAME, STRING, &attrName);
    Attr::SetAttr(recordData + MAXNAME + MAXNAME, INT, &offset);
    Attr::SetAttr(recordData + MAXNAME + MAXNAME + sizeof(int), INT, &attrType);
    Attr::SetAttr(recordData + MAXNAME + MAXNAME + sizeof(int) + sizeof(int), INT, &attrLength);
    Attr::SetAttr(recordData + MAXNAME + MAXNAME + sizeof(int) + sizeof(int) + sizeof(int), INT, &indexNo);
}
