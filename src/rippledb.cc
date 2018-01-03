//
// File:        rippledb.cc
// Description: RippleDB command
// Authors:     Yi Xu
//

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "global.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "ql.h"

using namespace std;

int main(int argc, char* argv[]) {
    RC rc;

    // initialize RippleDB components
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);
    QL_Manager qlm(smm, ixm, rmm);
    // call the parser
    RippleDBparse(pfm, smm, qlm);
    // close the database
    if ((rc = smm.CloseDb()) != SM_DBNOTOPEN) {
        SM_PrintError(rc);
    }
    cout << "Bye.\n";
    return OK_RC;
}
