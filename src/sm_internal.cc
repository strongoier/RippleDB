//
// File:        sm_internal.cc
// Description: SM internal classes implementation
// Authors:     Yi Xu
//

#include "sm.h"

RelCat::RelCat(const char* recordData) {
    strcpy(relName, recordData + RELNAME_OFFSET + 1);
    tupleLength = *(int*)(recordData + TUPLELENGTH_OFFSET + 1);
    attrCount = *(int*)(recordData + ATTRCOUNT_OFFSET + 1);
    indexCount = *(int*)(recordData + INDEXCOUNT_OFFSET + 1);
}

RelCat::RelCat(const char* relName, int tupleLength, int attrCount, int indexCount)
    : tupleLength(tupleLength), attrCount(attrCount), indexCount(indexCount) {
    strcpy(this->relName, relName);
}

void RelCat::WriteRecordData(char* recordData) {
    memset(recordData, 1, SIZE);
    memcpy(recordData + RELNAME_OFFSET + 1, relName, MAXNAME);
    memcpy(recordData + TUPLELENGTH_OFFSET + 1, &tupleLength, sizeof(int));
    memcpy(recordData + ATTRCOUNT_OFFSET + 1, &attrCount, sizeof(int));
    memcpy(recordData + INDEXCOUNT_OFFSET + 1, &indexCount, sizeof(int));
}

bool operator==(const RelCat& a, const RelCat& b) {
    return strcmp(a.relName, b.relName) == 0;
}

bool operator<(const RelCat& a, const RelCat& b) {
    return strcmp(a.relName, b.relName) < 0;
}

AttrCat::AttrCat(const char* recordData) {
    strcpy(relName, recordData + RELNAME_OFFSET + 1);
    strcpy(attrName, recordData + ATTRNAME_OFFSET + 1);
    offset = *(int*)(recordData + OFFSET_OFFSET + 1);
    attrType = (AttrType)*(int*)(recordData + ATTRTYPE_OFFSET + 1);
    attrLength = *(int*)(recordData + ATTRLENGTH_OFFSET + 1);
    indexNo = *(int*)(recordData + INDEXNO_OFFSET + 1);
    isNotNull = *(int*)(recordData + ISNOTNULL_OFFSET + 1);
    primaryKey = *(int*)(recordData + PRIMARYKEY_OFFSET + 1);
    strcpy(refrel, recordData + REFREL_OFFSET + 1);
    strcpy(refattr, recordData + REFATTR_OFFSET + 1);
}

AttrCat::AttrCat(const char* relName, const char* attrName, int offset, AttrType attrType, int attrLength, int indexNo, int isNotNull, int primaryKey, const char* refrel, const char* refattr)
    : offset(offset), attrType(attrType), attrLength(attrLength), indexNo(indexNo), isNotNull(isNotNull), primaryKey(primaryKey) {
    strcpy(this->relName, relName);
    strcpy(this->attrName, attrName);
    strcpy(this->refrel, refrel);
    strcpy(this->refattr, refattr);
}

void AttrCat::WriteRecordData(char* recordData) {
    memset(recordData, 1, SIZE);
    memcpy(recordData + RELNAME_OFFSET + 1, relName, MAXNAME);
    memcpy(recordData + ATTRNAME_OFFSET + 1, attrName, MAXNAME);
    memcpy(recordData + OFFSET_OFFSET + 1, &offset, sizeof(int));
    memcpy(recordData + ATTRTYPE_OFFSET + 1, &attrType, sizeof(int));
    memcpy(recordData + ATTRLENGTH_OFFSET + 1, &attrLength, sizeof(int));
    memcpy(recordData + INDEXNO_OFFSET + 1, &indexNo, sizeof(int));
    memcpy(recordData + ISNOTNULL_OFFSET + 1, &isNotNull, sizeof(int));
    memcpy(recordData + PRIMARYKEY_OFFSET + 1, &primaryKey, sizeof(int));
    memcpy(recordData + REFREL_OFFSET + 1, refrel, MAXNAME);
    memcpy(recordData + REFATTR_OFFSET + 1, refattr, MAXNAME);
}

bool operator==(const AttrCat& a, const AttrCat& b) {
    return strcmp(a.relName, b.relName) == 0 && strcmp(a.attrName, b.attrName) == 0;
}
