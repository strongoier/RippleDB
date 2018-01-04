//
// File:        ql_error.cc
// Description: QL_PrintError function
// Authors:     Shihong Yan
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ql.h"

using namespace std;

//
// Error table
//
static char* QL_WarnMsg[] = {
    (char*)"no db is open",
    (char*)"relation not found",
    (char*)"attr number not right",
    (char*)"attr type not right",
    (char*)"attribute not found",
    (char*)"relation variables to distinguish multiple appearances",
    (char*)"relation not found",
    (char*)"attribute multi appear",
    (char*)"attr is null",
    (char*)"QL_PRIMARYKEYREPEAT",
    (char*)"QL_STRINGLENGTHWRONG",
    (char*)"QL_FOREIGNKEYNOTEXIST",
    (char*)"date format error"
};

//
// QL_PrintError
//
void QL_PrintError(RC rc) {
    // Check the return code is within proper limits
    if (rc >= START_QL_WARN && rc <= END_QL_WARN) {
        cerr << "QL warning: " << QL_WarnMsg[rc - START_QL_WARN] << "\n";
    }
}
