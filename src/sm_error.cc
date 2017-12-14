//
// File:        sm_error.cc
// Description: SM_PrintError function
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
static char* SM_WarnMsg[] = {
    (char*)"a db is already open",
    (char*)"no db is open"
};

//
// SM_PrintError
//
void SM_PrintError(RC rc) {
    // Check the return code is within proper limits
    if (rc >= START_SM_WARN && rc <= END_SM_WARN) {
        cerr << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";
    }
}
