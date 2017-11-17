//
// File:        attr.cc
// Description: Attr class implementation
// Authors:     Yi Xu
//

#include "global.h"
#include <cstring>
using namespace std;

bool Attr::CheckAttrLengthValid(AttrType attrType, int attrLength) {
    switch (attrType) {
        case INT:
            if (attrLength != sizeof(int)) {
                return false;
            }
            break;
        case FLOAT:
            if (attrLength != sizeof(float)) {
                return false;
            }
            break;
        case STRING:
            if (attrLength <= 0 || attrLength > MAXSTRINGLEN) {
                return false;
            }
            break;
    }
    return true;
}

void Attr::DeleteValue(AttrType attrType, void* value) {
    switch (attrType) {
        case INT:
            delete (int*)value;
            break;
        case FLOAT:
            delete (float*)value;
            break;
        case STRING:
            delete[] (char*)value;
            break;
    }
}

bool Attr::CompareAttr(AttrType attrType, int attrLength, void* valueA, CompOp compOp, void* valueB) {
    if (valueB == NULL) {
        return true;
    }
    switch (attrType) {
        case INT:
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return *(int*)valueA == *(int*)valueB;
                case NE_OP:
                    return *(int*)valueA != *(int*)valueB;
                case LT_OP:
                    return *(int*)valueA < *(int*)valueB;
                case GT_OP:
                    return *(int*)valueA > *(int*)valueB;
                case LE_OP:
                    return *(int*)valueA <= *(int*)valueB;
                case GE_OP:
                    return *(int*)valueA >= *(int*)valueB;
            }
            break;
        case FLOAT:
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return *(float*)valueA == *(float*)valueB;
                case NE_OP:
                    return *(float*)valueA != *(float*)valueB;
                case LT_OP:
                    return *(float*)valueA < *(float*)valueB;
                case GT_OP:
                    return *(float*)valueA > *(float*)valueB;
                case LE_OP:
                    return *(float*)valueA <= *(float*)valueB;
                case GE_OP:
                    return *(float*)valueA >= *(float*)valueB;
            }
            break;
        case STRING:
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return strncmp((char*)valueA, (char*)valueB, attrLength) == 0;
                case NE_OP:
                    return strncmp((char*)valueA, (char*)valueB, attrLength) != 0;
                case LT_OP:
                    return strncmp((char*)valueA, (char*)valueB, attrLength) < 0;
                case GT_OP:
                    return strncmp((char*)valueA, (char*)valueB, attrLength) > 0;
                case LE_OP:
                    return strncmp((char*)valueA, (char*)valueB, attrLength) <= 0;
                case GE_OP:
                    return strncmp((char*)valueA, (char*)valueB, attrLength) >= 0;
            }
            break;
    }
}

int Attr::lower_bound(AttrType attrType, int attrLength, char* first, int len, char* value) {
    int half, middle, begin = 0;
    while (len > 0) {
        half = len >> 1;
        middle = begin + half;
        if (CompareAttr(attrType, attrLength, first + middle * attrLength, LT_OP, value)) {
            begin = middle + 1;
            len = len - half - 1;
        }  
        else
            len = half;
    }
    return begin;
}

int Attr::upper_bound(AttrType attrType, int attrLength, char* first, int len, char* value) {
    int half, middle, begin = 0;
    while (len > 0) {
        half = len >> 1;
        middle = begin + half;
        if (CompareAttr(attrType, attrLength, value, LT_OP, first + middle * attrLength))
            len = half;
        else {
            begin = middle + 1;
            len = len - half - 1;
        }
    }
    return begin;
}

pair<int, int> Attr::equal_range(AttrType attrType, int attrLength, char* first, int len, char* value) {
    return make_pair(lower_bound(attrType, attrLength, first, len, value), upper_bound(attrType, attrLength, first, len, value));
}

bool Attr::binary_search(AttrType attrType, int attrLength, char* first, int len, char* value) {
    int i = lower_bound(attrType, attrLength, first, len, value);  
    return i != len && !CompareAttr(attrType, attrLength, value, LT_OP, first + i * attrLength);
}
