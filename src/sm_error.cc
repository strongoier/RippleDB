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
    (char*)"no db is open",
    (char*)"db not exist",
    (char*)"relation not found",
    (char*)"attribute not found",
    (char*)"index already exist",
    (char*)"index not exist",
    (char*)"file not found",
    (char*)"relation already exist",
    (char*)"error while loading",
    (char*)"a db is open",
    (char*)"system error"
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
