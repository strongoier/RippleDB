//
// File:        ix_indexscan.cc
// Description: IX_IndexScan class implementation
// Authors:     Shihong Yan
//

#include "ix.h"
using namespace std;

IX_IndexScan::IX_IndexScan() : tree(nullptr), pData(nullptr), cur(nullptr) {}

IX_IndexScan::~IX_IndexScan() {}

// Open index scan
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value) {
	RC rc;

	if (tree != nullptr)
		IX_ERROR(IX_REOPENSCAN)

	tree = indexHandle.treeHeader;
	if (compOp != NO_OP) {
		pData = new char[tree->attrLength];
		memcpy(pData, value, tree->attrLength);
	}
	op = compOp;

	if (rc = tree->Search(pData, op, cur, index))
		IX_PRINTSTACK

	if (cur != nullptr) {
		memcpy(buffer, cur, PF_PAGE_SIZE);
		cur = (NodeHeader*)buffer;
	}

	if (rc = tree->UnpinPages())
		IX_PRINTSTACK
	/*printf("maxChildNum: %d\n", tree->maxChildNum);
	printf("curPage: %d\n", cur->selfPNum);
	printf("curKey: %d\n", *(int*)cur->key(index));*/
	/*NodeHeader* parent;
	cur->ParentPage(parent);
	while (true) {
		int t = parent->selfPNum;
		printf("selfPNum: %d ----", parent->selfPNum);
		for (int i = 0; i < parent->childNum; ++i) {
			int tKey = *(int*)parent->key(i);
			RID tRID = *parent->rid(i);
			int tPageNum;
			int tSlotNum;
			tRID.GetPageNum(tPageNum);
			tRID.GetSlotNum(tSlotNum);
			printf("%d: tKey = %d tRID = (%d, %d)\n", i, tKey, tPageNum, tSlotNum);
		}
		printf("\n");
		if (parent->HaveNextPage()) {
			parent->NextPage(parent);
		} else {
			break;
		}
		tree->indexFH->UnpinPage(t);
	}*/
	/*printf("\n\n\n\n\nLeaf:\n");
	while (true) {
		int t = cur->selfPNum;
		printf("selfPNum: %d ----", cur->selfPNum);
		for (int i = 0; i < cur->childNum; ++i) {
			int tKey = *(int*)cur->key(i);
			RID tRID = *cur->rid(i);
			int tPageNum;
			int tSlotNum;
			tRID.GetPageNum(tPageNum);
			tRID.GetSlotNum(tSlotNum);
			printf("%d: tKey = %d tRID = (%d, %d)\n", i, tKey, tPageNum, tSlotNum);
		}
		printf("\n");
		if (cur->HaveNextPage()) {
			cur->NextPage(cur);
		} else {
			break;
		}
		tree->indexFH->UnpinPage(t);
	}
	tree->GetPageData(tree->rootPNum, cur);
	printf("selfPNum: %d ----", cur->selfPNum);
	for (int i = 0; i < cur->childNum; ++i) {
		int tKey = *(int*)cur->key(i);
		RID tRID = *cur->rid(i);
		int tPageNum;
		int tSlotNum;
		tRID.GetPageNum(tPageNum);
		tRID.GetSlotNum(tSlotNum);
		printf("%d: tKey = %d tRID = (%d, %d)\n", i, tKey, tPageNum, tSlotNum);
	}
	printf("\n");*/
  return OK_RC;
}

// Get the next matching entry return IX_EOF if no more matching entries.
RC IX_IndexScan::GetNextEntry(RID &r) {
	RC rc;

	if (cur == nullptr)
		IX_ERROR(IX_EOF)

	r = *(cur->rid(index));
	// printf("cur: %p\n", cur);
	// printf("key: %d\n", *(int*)cur->key(index));

	if (rc = tree->GetNextEntry(pData, op, cur, index))
		IX_PRINTSTACK
	
	if (cur != nullptr) {
		memcpy(buffer, cur, PF_PAGE_SIZE);
		cur = (NodeHeader*)buffer;
	}
	
	if (rc = tree->UnpinPages())
		IX_PRINTSTACK
	
	return OK_RC;
}

// Close index scan
RC IX_IndexScan::CloseScan() {
	if (pData != nullptr)
		delete pData;
	pData = nullptr;
	cur = nullptr;
	tree->UnpinPages();
	tree = nullptr;
  return OK_RC;
}