//
// File:        redbase.cc
// Description: redbase command
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

    // Look for 2 arguments. The first is always the name of the program
    // that was executed, and the second should be the name of the database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // initialize RedBase components
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);
    QL_Manager qlm(smm, ixm, rmm);
    // open the database
    if ((rc = smm.OpenDb(argv[1]))) {
        SM_PrintError(rc);
        return rc;
    }
    // call the parser
    RBparse(pfm, smm, qlm);
    // close the database
    if ((rc = smm.CloseDb())) {
        SM_PrintError(rc);
        return rc;
    }

    cout << "Bye.\n";
    return OK_RC;
}
