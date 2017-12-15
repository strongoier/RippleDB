//
// File:        global.h
// Description: global declarations
// Authors:     Yi Xu (Modified from the original file in Stanford CS346 RedBase)
//

#ifndef GLOBAL_H
#define GLOBAL_H

#include <cstring>
#include <utility>
using std::pair;

//
// Globally-useful defines
//
#define MAXNAME       24                // maximum length of a relation
                                        // or attribute name   + 1 ????
#define MAXSTRINGLEN  255               // maximum length of a
                                        // string-type attribute + 1 ????
#define MAXATTRS      40                // maximum number of attributes
                                        // in a relation

#define YY_SKIP_YYWRAP 1
#define yywrap() 1
void yyerror(const char *);

//
// Return codes
//
typedef int RC;

#define OK_RC         0    // OK_RC return code is guaranteed to always be 0

#define START_PF_ERR  (-1)
#define END_PF_ERR    (-100)
#define START_RM_ERR  (-101)
#define END_RM_ERR    (-200)
#define START_IX_ERR  (-201)
#define END_IX_ERR    (-300)
#define START_SM_ERR  (-301)
#define END_SM_ERR    (-400)
#define START_QL_ERR  (-401)
#define END_QL_ERR    (-500)

#define START_PF_WARN  1
#define END_PF_WARN    100
#define START_RM_WARN  101
#define END_RM_WARN    200
#define START_IX_WARN  201
#define END_IX_WARN    300
#define START_SM_WARN  301
#define END_SM_WARN    400
#define START_QL_WARN  401
#define END_QL_WARN    500

// ALL_PAGES is defined and used by the ForcePages method defined in RM
// and PF layers
const int ALL_PAGES = -1;

//
// TRUE, FALSE and BOOLEAN
//
#ifndef BOOLEAN
typedef char Boolean;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

/********** Attribute **********/

//
// Attribute types
//
enum AttrType {
    INT,
    FLOAT,
    STRING
};

//
// Comparison operators
//
enum CompOp {
    NO_OP, // no comparison
    EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP // binary atomic operators
};

//
// Attr: Class for operating attributes
//
class Attr {
public:
    static bool CheckAttrLengthValid(AttrType attrType, int attrLength);
    static void DeleteValue(AttrType attrType, void* value);
    static void SetAttr(char* destination, AttrType attrType, void* value);
    static bool CompareAttr(AttrType attrType, int attrLength, void* valueA, CompOp compOp, void* valueB);
    static bool CompareAttrWithRID(AttrType attrType, int attrLength, void* valueA, CompOp compOp, void* valueB);
    static int lower_bound(AttrType attrType, int attrLength, char* first, int len, char* value);
    static int lower_boundWithRID(AttrType attrType, int attrLength, char* first, int len, char* value);
    static int upper_bound(AttrType attrType, int attrLength, char* first, int len, char* value);
    static int upper_boundWithRID(AttrType attrType, int attrLength, char* first, int len, char* value);
    static pair<int, int> equal_range(AttrType attrType, int attrLength, char* first, int len, char* value);
    static pair<int, int> equal_rangeWithRID(AttrType attrType, int attrLength, char* first, int len, char* value);
    static bool binary_search(AttrType attrType, int attrLength, char* first, int len, char* value);
    static bool binary_searchWithRID(AttrType attrType, int attrLength, char* first, int len, char* value);
};

/********** RID **********/

//
// PageNum: uniquely identifies a page in a file
//
typedef int PageNum;

//
// SlotNum: uniquely identifies a record in a page
//
typedef int SlotNum;

//
// RID: Record ID
//
class RID {
    friend bool operator<(const RID& a, const RID& b);
    friend bool operator<=(const RID& a, const RID& b);
    friend bool operator==(const RID& a, const RID& b);
    friend bool operator!=(const RID& a, const RID& b);
    friend bool operator>(const RID& a, const RID& b);
    friend bool operator>=(const RID& a, const RID& b);
public:
    static const PageNum NULL_PAGE_NUM = -1; // default value when not set
    static const SlotNum NULL_SLOT_NUM = -1; // default value when not set

    RID();
    RID(PageNum pageNum, SlotNum slotNum);
    ~RID();

    RC GetPageNum(PageNum &pageNum) const;
    RC GetSlotNum(SlotNum &slotNum) const;

private:
    PageNum pageNum; // unique page number in a file
    SlotNum slotNum; // unique slot number in a page
};
#endif
