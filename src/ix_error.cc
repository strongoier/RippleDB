#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ix.h"

using namespace std;

static char *IX_WarnMsg[] = {
  (char*)"attr type donot matchs attr length",
  (char*)"insert rid into full leaf node",
  (char*)"insert rid into internal node",
  (char*)"update key in leaf node",
  (char*)"insert pagenum into leaf node",
  (char*)"get parent next ot prev key in leaf node",
  (char*)"the search ends",
  (char*)"the indexscan has already opened",
  (char*)"the indexhandle has not been opened",
  (char*)"the indexhandle has already been opened",
  (char*)"delete rid from internal node",
  (char*)"delete rid not exist",
  (char*)"delete page from leaf node"
};

static char *IX_ErrorMsg[] = {};

void IX_PrintError(RC rc)
{
  if (rc >= START_IX_WARN && rc <= END_IX_WARN)
    cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << "\n";
  else if (-rc >= -START_IX_ERR && -rc < -END_IX_ERR)
    cerr << "IX error: " << IX_ErrorMsg[-rc + START_IX_ERR] << "\n";
  else if (rc == 0)
    cerr << "IX_PrintError called with return code of 0\n";
  else
    cerr << "IX error: " << rc << " is out of bounds\n";
}
