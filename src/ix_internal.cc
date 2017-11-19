#include "ix.h"

RC IX_AllocatePage(PF_FileHandle &fh, PF_PageHandle &ph, PageNum &pNum, char *&pData) {
    RC rc;
    if (rc = fh.AllocatePage(ph))
        IX_ERROR(rc)
    if (rc = ph.GetPageNum(pNum))
        IX_ERROR(rc)
    if (rc = ph.GetData(pData))
        IX_ERROR(rc)
    return OK_RC;
}
