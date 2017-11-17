#include "ix.h"
using namespace std;

IX_IndexScan::IX_IndexScan() : tree(nullptr), pData(nullptr), cur(nullptr) {}

IX_IndexScan::~IX_IndexScan() {}

// Open index scan
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value) {
	if (tree != nullptr)
		IX_ERROR(IX_REOPENSCAN)
	tree = indexHandle.treeHeader;
	if (compOp != NO_OP) {
		pData = new char[tree->attrLength];
		memcpy(pData, value, tree->attrLength);
	}
	op = compOp;
	tree->Search(pData, op, cur, index, isLT);
    return OK_RC;
}

// Get the next matching entry return IX_EOF if no more matching entries.
RC IX_IndexScan::GetNextEntry(RID &r) {
	RC rc;

	if (cur == nullptr)
		IX_ERROR(IX_EOF)

	r = *(cur->rid(index));
	
	printf("key: %d\n", *(int*)cur->key(index));

	if (op == NO_OP) {
		++index;
		if (index < cur->childNum)
			return OK_RC;
		if (!cur->HaveNextPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->NextPage(cur))
			IX_PRINTSTACK
		if (cur->IsEmpty())
			cur = nullptr;
		index = 0;
		return OK_RC;
	} else if (op == EQ_OP) {
		++index;
		if (index < cur->childNum) {
			if (!tree->CompareAttr(pData, EQ_OP, cur->key(index))) {
				cur = nullptr;
				return OK_RC;
			}
			return OK_RC;
		}
		if (!cur->HaveNextPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->NextPage(cur))
			IX_PRINTSTACK
		if (cur->IsEmpty()) {
			cur = nullptr; 
			return OK_RC;
		}
		index = 0;
		if (!tree->CompareAttr(pData, EQ_OP, cur->key(index))) {
			cur = nullptr;
			return OK_RC;
		}
		return OK_RC;
	} else if (op == NE_OP) {
		if (isLT) {
			++index;
			if (index < cur->childNum) {
				if (tree->CompareAttr(pData, EQ_OP, cur->key(index))) {
					cur = nullptr;
					return OK_RC;
				}
				return OK_RC;
			}
			if (!cur->HaveNextPage()) {
				cur = nullptr;
				return OK_RC;
			}
			if (rc = cur->NextPage(cur))
				IX_PRINTSTACK
			if (cur->IsEmpty()) {
				cur = nullptr;
				return OK_RC;
			}
			index = 0;
			if (tree->CompareAttr(pData, NE_OP, cur->key(index)))
				return OK_RC;
			else {
				if (rc = tree->GetLastLeafNode(cur))
					IX_PRINTSTACK
				if (cur->IsEmpty()) {
					cur = nullptr;
					return OK_RC;
				}
				isLT = false;
				index = cur->childNum;
			}
		}
		--index;
		if (index >= 0) {
			if (tree->CompareAttr(pData, EQ_OP, cur->key(index))) {
				cur = nullptr;
				return OK_RC;
			}
			return OK_RC;
		}
		if (!cur->HavePrevPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->PrevPage(cur))
			IX_PRINTSTACK
		if (cur->IsEmpty()) {
			cur = nullptr;
			return OK_RC;
		}
		index = cur->childNum - 1;
		if (tree->CompareAttr(pData, EQ_OP, cur->key(index)))
			cur = nullptr;
		return OK_RC;
	} else if (op == LT_OP) {
		++index;
		if (index < cur->childNum) {
			if (!tree->CompareAttr(cur->key(index), LT_OP, pData)) {
				cur = nullptr;
				return OK_RC;
			}
			return OK_RC;
		}
		if (!cur->HaveNextPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->NextPage(cur))
			IX_PRINTSTACK
		if (cur->IsEmpty()) {
			cur = nullptr;
			return OK_RC;
		}
		index = 0;
		if (!tree->CompareAttr(cur->key(index), LT_OP, pData)) {
			cur = nullptr;
			return OK_RC;
		}
		return OK_RC;
	} else if (op == LE_OP) {
		++index;
		if (index < cur->childNum) {
			if (!tree->CompareAttr(cur->key(index), LE_OP, pData)) {
				cur = nullptr;
				return OK_RC;
			}
			return OK_RC;
		}
		if (!cur->HaveNextPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->NextPage(cur))
			IX_PRINTSTACK
		if (cur->IsEmpty()) {
			cur = nullptr;
			return OK_RC;
		}
		index = 0;
		if (!tree->CompareAttr(cur->key(index), LE_OP, pData)) {
			cur = nullptr;
			return OK_RC;
		}
		return OK_RC;
	} else if (op == GT_OP) {
		--index;
		if (index >= 0) {
			if (!tree->CompareAttr(cur->key(index), GT_OP, pData)) {
				cur = nullptr;
				return OK_RC;
			}
			return OK_RC;
		}
		if (!cur->HavePrevPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->PrevPage(cur))
			IX_PRINTSTACK
		if (cur->IsEmpty()) {
			cur = nullptr;
			return OK_RC;
		}
		index = cur->childNum - 1;
		if (!tree->CompareAttr(cur->key(index), GT_OP, pData)) {
			cur = nullptr;
			return OK_RC;
		}
		return OK_RC;
	} else if (op == GE_OP) {
		--index;
		if (index >= 0) {
			if (!tree->CompareAttr(cur->key(index), GE_OP, pData)) {
				cur = nullptr;
				return OK_RC;
			}
			return OK_RC;
		}
		if (!cur->HavePrevPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->PrevPage(cur))
			IX_PRINTSTACK
		if (cur->IsEmpty()) {
			cur = nullptr;
			return OK_RC;
		}
		index = cur->childNum - 1;
		if (!tree->CompareAttr(cur->key(index), GE_OP, pData)) {
			cur = nullptr;
			return OK_RC;
		}
		return OK_RC;
	} else {
		cur = nullptr;
	}
    return OK_RC;
}

// Close index scan
RC IX_IndexScan::CloseScan() {
	if (pData != nullptr) delete pData;
	pData = nullptr;
	cur = nullptr;
	tree->UnpinPages();
	tree = nullptr;
    return OK_RC;
}