//
// File:        attr.cc
// Description: Attr class implementation
// Authors:     Yi Xu, Shihong Yan
//

#include "global.h"
#include <cstring>
using namespace std;

bool Attr::CompareAttrWithRID(AttrType attrType, int attrLength, void* valueA, CompOp compOp, void* valueB) {
    valueA = (char*)valueA + 1;
    valueB = (char*)valueB + 1;
    const RID& ridA = *(RID*)((char*)valueA + attrLength);
    const RID& ridB = *(RID*)((char*)valueB + attrLength);
    switch (attrType) {
        case INT: {
            const int& intA = *(int*)valueA;
            const int& intB = *(int*)valueB;
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return intA == intB && ridA == ridB;
                case NE_OP:
                    return !(intA == intB && ridA == ridB);
                case LT_OP:
                    return intA < intB || (intA == intB && ridA < ridB);
                case GT_OP:
                    return intA > intB || (intA == intB && ridA > ridB);
                case LE_OP:
                    return !(intA > intB || (intA == intB && ridA > ridB));
                case GE_OP:
                    return !(intA < intB || (intA == intB && ridA < ridB));
            }
            break;
        }
        case FLOAT: {
            const float& floatA = *(float*)valueA;
            const float& floatB = *(float*)valueB;
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return floatA == floatB && ridA == ridB;
                case NE_OP:
                    return !(floatA == floatB && ridA == ridB);
                case LT_OP:
                    return floatA < floatB || (floatA == floatB && ridA < ridB);
                case GT_OP:
                    return floatA > floatB || (floatA == floatB && ridA > ridB);
                case LE_OP:
                    return !(floatA > floatB || (floatA == floatB && ridA > ridB));
                case GE_OP:
                    return !(floatA < floatB || (floatA == floatB && ridA < ridB));
            }
            break;
        }
        case DATE:
        case PRIMARYKEY:
        case STRING: {
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return strcmp((char *) valueA, (char *) valueB) == 0 && ridA == ridB;
                case NE_OP:
                    return !(strcmp((char *) valueA, (char *) valueB) == 0 && ridA == ridB);
                case LT_OP:
                    return strcmp((char *) valueA, (char *) valueB) < 0 || (strcmp((char *) valueA, (char *) valueB) == 0 && ridA < ridB);
                case GT_OP:
                    return strcmp((char *) valueA, (char *) valueB) > 0 || (strcmp((char *) valueA, (char *) valueB) == 0 && ridA > ridB);
                case LE_OP:
                    return !(strcmp((char *) valueA, (char *) valueB) > 0 || (strcmp((char *) valueA, (char *) valueB) == 0 && ridA > ridB));
                case GE_OP:
                    return !(strcmp((char *) valueA, (char *) valueB) < 0 || (strcmp((char *) valueA, (char *) valueB) == 0 && ridA < ridB));
            }
            break;
        }
    }
    return true;
}

/*void Attr::SetAttr(char* destination, AttrType attrType, void* value) {
    *destination = *(char*)value;
    switch (attrType) {
        case DATE:
        case INT:
            *(int*)(destination + 1) = *(int*)(value + 1);
            break;
        case FLOAT:
            *(float*)(destination + 1) = *(float*)(value + 1);
            break;
        case PRIMARYKEY:
        case STRING:
            strcpy(destination + 1, (const char*)(value + 1));
            break;
    }
}*/

bool Attr::CompareAttr(AttrType attrType, int attrLength, void* valueA, CompOp compOp, void* valueB) {
    valueA = (char*)valueA + 1;
    valueB = (char*)valueB + 1;
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
        case DATE:
        case PRIMARYKEY:
        case STRING:
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return strcmp((char*)valueA, (char*)valueB) == 0;
                case NE_OP:
                    return strcmp((char*)valueA, (char*)valueB) != 0;
                case LT_OP:
                    return strcmp((char*)valueA, (char*)valueB) < 0;
                case GT_OP:
                    return strcmp((char*)valueA, (char*)valueB) > 0;
                case LE_OP:
                    return strcmp((char*)valueA, (char*)valueB) <= 0;
                case GE_OP:
                    return strcmp((char*)valueA, (char*)valueB) >= 0;
                case LIKE_OP:
                    return like_match((char*)valueB - 1, (char*)valueA - 1);
            }
            break;
    }
    return true;
}

bool Attr::like_match(char* originalPattern, char* text) {
    bool possible[MAXSTRINGLEN][MAXSTRINGLEN];
    int lenOriginalPattern = strlen(originalPattern + 1);
    int lenText = strlen(text + 1);
    char pattern[MAXSTRINGLEN];
    memset(pattern, 0, sizeof pattern);
    int lenPattern = 0;
    for (int i = 1; i <= lenOriginalPattern; ++i) {
        if (originalPattern[i] == '%') {
            pattern[++lenPattern] = 1;
        } else if (originalPattern[i] == '_') {
            pattern[++lenPattern] = 2;
        } else if (originalPattern[i] == '\\') {
            if (originalPattern[i + 1]) {
                pattern[++lenPattern] = originalPattern[i + 1];
                ++i;
            }
        } else {
            pattern[++lenPattern] = originalPattern[i];
        }
    }
    memset(possible, false, sizeof possible);
    possible[0][0] = true;
    for (int i = 1; i <= lenPattern; ++i) {
        if (pattern[i] == 1) {
            possible[i][0] = possible[i - 1][0];
        }
        for (int j = 1; j <= lenText; ++j) {
            if (pattern[i] == 1) {
                possible[i][j] = possible[i - 1][j] || possible[i - 1][j - 1] || possible[i][j - 1];
            } else if (pattern[i] == 2 || pattern[i] == text[j]) {
                possible[i][j] = possible[i - 1][j - 1];
            }
        }
    }
    return possible[lenPattern][lenText];
}

int Attr::lower_bound(AttrType attrType, int attrLength, char* first, int len, char* value) {
    int half, middle, begin = 0;
    int attrLengthWithRID = attrLength + sizeof(RID) + 1;
    while (len > 0) {
        half = len >> 1;
        middle = begin + half;
        if (CompareAttr(attrType, attrLength, first + middle * attrLengthWithRID, LT_OP, value)) {
            begin = middle + 1;
            len = len - half - 1;
        }  
        else
            len = half;
    }
    return begin;
}

int Attr::lower_boundWithRID(AttrType attrType, int attrLength, char *first, int len, char *value) {
    int half, middle, begin = 0;
    int attrLengthWithRID = attrLength + sizeof(RID) + 1;
    while (len > 0) {
        half = len >> 1;
        middle = begin + half;
        if (CompareAttrWithRID(attrType, attrLength, first + middle * attrLengthWithRID, LT_OP, value)) {
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
    int attrLengthWithRID = attrLength + sizeof(RID) + 1;
    while (len > 0) {
        half = len >> 1;
        middle = begin + half;
        if (CompareAttr(attrType, attrLength, value, LT_OP, first + middle * attrLengthWithRID))
            len = half;
        else {
            begin = middle + 1;
            len = len - half - 1;
        }
    }
    return begin;
}

int Attr::upper_boundWithRID(AttrType attrType, int attrLength, char* first, int len, char* value) {
    int half, middle, begin = 0;
    int attrLengthWithRID = attrLength + sizeof(RID) + 1;
    while (len > 0) {
        half = len >> 1;
        middle = begin + half;
        if (CompareAttrWithRID(attrType, attrLength, value, LT_OP, first + middle * attrLengthWithRID))
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

pair<int, int> Attr::equal_rangeWithRID(AttrType attrType, int attrLength, char* first, int len, char* value) {
    return make_pair(lower_boundWithRID(attrType, attrLength, first, len, value), upper_boundWithRID(attrType, attrLength, first, len, value));
}

bool Attr::binary_search(AttrType attrType, int attrLength, char* first, int len, char* value) {
    int i = lower_bound(attrType, attrLength, first, len, value);
    return i != len && !CompareAttr(attrType, attrLength, value, LT_OP, first + i * attrLength);
}

bool Attr::binary_searchWithRID(AttrType attrType, int attrLength, char* first, int len, char* value) {
    int i = lower_boundWithRID(attrType, attrLength, first, len, value);
    return i != len && !CompareAttrWithRID(attrType, attrLength, value, LT_OP, first + i * attrLength);
}
