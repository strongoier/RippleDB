#include "../src/ix.h"
#include <cstdio>

int main() {
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RC rc;
    if (rc = ixm.CreateIndex("yansh", 12, INT, 4))
    	PF_PrintError(rc);
    IX_IndexHandle indexFH;
    if (rc = ixm.OpenIndex("yansh", 12, indexFH))
    	PF_PrintError(rc);
    int a = 4;
    RID r;
    if (rc = indexFH.InsertEntry(&a, r))
        IX_PRINTSTACK
    if (rc = ixm.CloseIndex(indexFH))
    	PF_PrintError(rc);
    if (rc = ixm.DestroyIndex("yansh", 12))
    	PF_PrintError(rc);
    return 0;
}