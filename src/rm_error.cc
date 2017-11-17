//
// File:        rm_error.cc
// Description: RM_PrintError function
// Authors:     Yi Xu
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm.h"

using namespace std;

//
// Error table
//
static char* RM_WarnMsg[] = {
    (char*)"record size is too small to handle",
    (char*)"record size is too large to handle",
    (char*)"file handle is not open for a file",
    (char*)"record has not been read",
    (char*)"there is no record with given RID",
    (char*)"file scan is already open",
    (char*)"file scan is not open",
    (char*)"attr is invalid (offset or length)",
    (char*)"there are no records left satisfying the scan condition"
};

//
// RM_PrintError
//
void RM_PrintError(RC rc) {
    // Check the return code is within proper limits
    if (rc >= START_RM_WARN && rc <= END_RM_WARN) {
        cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
    }
}
