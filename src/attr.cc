//
// File:        attr.cc
// Description: Attr class implementation
// Authors:     Yi Xu
//

#include "global.h"

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
