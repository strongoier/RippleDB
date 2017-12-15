#include "ix.h"
#include <limits>
using namespace std;

/**
 * PageNum selfPNum
 * NodeType nodeType
 * PageNum parentPNum
 * PageNum prevPNum
 * PageNum nextPNum
 * int childNum
 * char* keys
 * char* values
 * TreeHeader* tree
 */

bool NodeHeader::IsEmpty() {
	return childNum == 0;
}

bool NodeHeader::IsFull() {
	return childNum == tree->maxChildNum;
}

bool NodeHeader::HavePrevPage() {
	return prevPNum != -1 && prevPNum != tree->infoPNum;
}

bool NodeHeader::HaveNextPage() {
	return nextPNum != -1 && nextPNum != tree->infoPNum;
}

bool NodeHeader::HaveParentPage() {
	return parentPNum != -1;
}

char* NodeHeader::key(int index) {
	return keys + index * tree->attrLengthWithRid;
}

char *NodeHeader::lastKey() {
	return endKey() - tree->attrLengthWithRid;
}

char* NodeHeader::endKey() {
	if (nodeType == InternalNode)
		return keys + (childNum - 1) * tree->attrLengthWithRid;
	else
		return keys + childNum * tree->attrLengthWithRid;
}

RID* NodeHeader::rid(int index) {
	return (RID*)(key(index) + tree->attrLength);
}

RID* NodeHeader::lastRid() {
	return (RID*)(lastKey() + tree->attrLength);
}

RID* NodeHeader::endRid() {
	return (RID*)(endKey() + tree->attrLength);
}

PageNum* NodeHeader::page(int index) {
	return (PageNum*)(values + index * tree->childItemSize);
}

PageNum* NodeHeader::lastPage() {
	return (PageNum*)((char*)endPage() - tree->childItemSize);
}

PageNum* NodeHeader::endPage() {
	return (PageNum*)(values + childNum * tree->childItemSize);
}

int NodeHeader::UpperBound(char *pData) {
	if (nodeType == LeafNode)
		return Attr::upper_bound(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::upper_bound(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

int NodeHeader::UpperBoundWithRID(char *pData) {
	if (nodeType == LeafNode)
		return Attr::upper_boundWithRID(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::upper_boundWithRID(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

int NodeHeader::LowerBound(char* pData) {
	if (nodeType == LeafNode)
		return Attr::lower_bound(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::lower_bound(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

int NodeHeader::LowerBoundWithRID(char *pData) {
	if (nodeType == LeafNode)
		return Attr::lower_boundWithRID(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::lower_boundWithRID(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

pair<int, int> NodeHeader::EqualRange(char *pData) {
	if (nodeType == LeafNode)
		return Attr::equal_range(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::equal_range(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

pair<int, int> NodeHeader::EqualRangeWithRID(char *pData) {
	if (nodeType == LeafNode)
		return Attr::equal_rangeWithRID(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::equal_rangeWithRID(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

bool NodeHeader::BinarySearch(char *pData) {
	if (nodeType == LeafNode)
		return Attr::binary_search(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::binary_search(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

bool NodeHeader::BinarySearchWithRID(char *pData) {
	if (nodeType == LeafNode)
		return Attr::binary_searchWithRID(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::binary_searchWithRID(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

RC NodeHeader::DeleteRID(char *pData) {
	RC rc;
	if (nodeType != LeafNode)
		IX_ERROR(IX_DELETERIDFROMINTERNALNODE)
	int pos = LowerBoundWithRID(pData);
	if (pos == childNum || !tree->CompareAttr(pData, EQ_OP, key(pos))) {
		//if (pos == childNum) printf("error!"); else printf("error2!!%d", selfPNum);
		IX_ERROR(IX_DELETERIDNOTEXIST)
	}
	if (rc = MarkDirty())
		IX_PRINTSTACK
	MoveKey(pos + 1, key(pos));
	MoveValue(pos + 1, (char*)page(pos));
	if (--childNum == 0 && HaveParentPage()) {
		//printf("Deleted leaf page: %d\n", selfPNum);
		if (HavePrevPage()) {
			NodeHeader *prev;
			if (rc = PrevPage(prev))
				IX_PRINTSTACK
			prev->nextPNum = nextPNum;
			if (rc = prev->MarkDirty())
				IX_PRINTSTACK
		} else {
			tree->dataHeadPNum = nextPNum;
		}
		if (HaveNextPage()) {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			next->prevPNum = prevPNum;
			if (rc = next->MarkDirty())
				IX_PRINTSTACK
		} else {
			tree->dataTailPNum = prevPNum;
		}
		NodeHeader *parent;
		if (rc = ParentPage(parent))
			IX_PRINTSTACK
		if (rc = parent->DeletePage(pData))
			IX_PRINTSTACK
	}
	return OK_RC;
}

RC NodeHeader::DeletePage(char *pData) {
	RC rc;
	if (nodeType != InternalNode)
		IX_ERROR(IX_DELETEPAGEFROMLEAFNODE)
	if (rc = MarkDirty())
		IX_PRINTSTACK
	int pos = UpperBoundWithRID(pData);
	/*printf("Todelete internal page: %d pos = %d\n", selfPNum, pos);
	printf("from");
	for (int i = 0; i < childNum - 1; ++i) {
		printf("key: %d", *(int*)key(i));
	}
	for (int i = 0; i < childNum; ++i) {
		printf("page: %d", *(PageNum*)page(i));
	}*/
	if (pos > 0) {
		MoveKey(pos, key(pos - 1));
	} else if (pos + 1 <= childNum - 1) { // pos == 0 also needs moving!!!!
		MoveKey(pos + 1, key(pos));
	}
	MoveValue(pos + 1, (char*)page(pos));
	/*printf("\nto");
	for (int i = 0; i < childNum - 2; ++i) {
		printf("key: %d", *(int*)key(i));
	}
	for (int i = 0; i < childNum - 1; ++i) {
		printf("page: %d", *(PageNum*)page(i));
	}
	printf("\n");*/
	if (--childNum == 0) {
		//printf("Deleted internal page: %d\n", selfPNum);
		if (!HaveParentPage()) {
			nodeType = LeafNode;
			tree->dataHeadPNum = tree->dataTailPNum = selfPNum;
		} else {
			if (HavePrevPage()) {
				NodeHeader *prev;
				if (rc = PrevPage(prev))
					IX_PRINTSTACK
				prev->nextPNum = nextPNum;
				if (rc = prev->MarkDirty())
					IX_PRINTSTACK
			}
			if (HaveNextPage()) {
				NodeHeader *next;
				if (rc = NextPage(next))
					IX_PRINTSTACK
				next->prevPNum = prevPNum;
				if (rc = next->MarkDirty())
					IX_PRINTSTACK
			}
			NodeHeader *parent;
			if (rc = ParentPage(parent))
				IX_PRINTSTACK
			if (rc = parent->DeletePage(pData))
				IX_PRINTSTACK
		}
	}
	return OK_RC;
}

RC NodeHeader::InsertRID(char *pData) {
	//PageNum num;
    //r.GetPageNum(num);
    //printf("%d\n", num);
	RC rc;
	if (nodeType != LeafNode)
		IX_ERROR(IX_INSERTRIDTOINTERNALNODE)
	if (rc = MarkDirty())
		IX_PRINTSTACK
	if (!IsFull()) {
		int pos = UpperBoundWithRID(pData);
		//if (pos == 0 && HaveParentPage()) {
		//	NodeHeader *parent;
		//	if (rc = ParentPage(parent))
		//		IX_PRINTSTACK
			//if (rc = parent->UpdateKey(key(0), pData))
			//	IX_PRINTSTACK
		//}
		MoveKey(pos, key(pos + 1));
		MoveValue(pos, (char*)page(pos + 1));
		memcpy(key(pos), pData, tree->attrLengthWithRid);
		++childNum;
		/*printf("selfPNum: %d ----", selfPNum);
		for (int i = 0; i < childNum; ++i) {
			int tKey = *(int*)key(i);
			RID tRID = *rid(i);
			int tPageNum;
			int tSlotNum;
			tRID.GetPageNum(tPageNum);
			tRID.GetSlotNum(tSlotNum);
			printf("%d: tKey = %d tRID = (%d, %d) ", i, tKey, tPageNum, tSlotNum);
		}
		printf("\n");*/
		return OK_RC;
	} else {
		/*if (HavePrevPage()) {
			NodeHeader *prev;
			if (rc = PrevPage(prev))
				IX_PRINTSTACK
			if (prev->parentPNum == parentPNum && !prev->IsFull()) {
				if (rc = prev->MarkDirty())
					IX_PRINTSTACK
				if (tree->CompareAttr(pData, LT_OP, key(0))) {
					if (rc = prev->InsertRID(pData, r))
						IX_PRINTSTACK
					return OK_RC;
				} else {
					if (rc = prev->InsertRID(key(0), *rid(0)))
						IX_PRINTSTACK
					MoveKey(1, key(0));
					MoveValue(1, (char*)rid(0));
					--childNum;
					if (rc = InsertRID(pData, r))
						IX_PRINTSTACK
					return OK_RC;
				}
			}
		}
		if (HaveNextPage()) {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			if (parentPNum == next->parentPNum && !next->IsFull()) {
				if (rc = next->MarkDirty())
					IX_PRINTSTACK
				if (tree->CompareAttr(lastKey(), LT_OP, pData)) {
					if (rc = next->InsertRID(pData, r))
						IX_PRINTSTACK
					return OK_RC;
				} else {
					if (rc = next->InsertRID(lastKey(), *lastRid()))
						IX_PRINTSTACK
					--childNum;
					if (rc = InsertRID(pData, r))
						IX_PRINTSTACK
					return OK_RC;
				}
			}
		}*/
		// Split
		PageNum newPNum;
		NodeHeader *newPData;
		if (rc = tree->AllocatePage(newPNum, newPData))
			IX_PRINTSTACK
		if (rc = newPData->MarkDirty())
			IX_PRINTSTACK
		NodeHeader *parentPData;
		if (HaveParentPage()) {
			if (rc = ParentPage(parentPData))
				IX_PRINTSTACK
		} else {
			if (rc = tree->AllocatePage(parentPNum, parentPData))
				IX_PRINTSTACK
			// Setup the new root node.
			parentPData->selfPNum = parentPNum;
			parentPData->nodeType = InternalNode;
			parentPData->parentPNum = -1;
			parentPData->prevPNum = -1;
			parentPData->nextPNum = -1;
			parentPData->childNum = 1;
			*parentPData->page(0) = selfPNum;
			// Update info page.
			tree->rootPNum = parentPNum;
		}
		if (rc = parentPData->MarkDirty())
			IX_PRINTSTACK
		// Setup the new node.
		newPData->selfPNum = newPNum;
		newPData->nodeType = LeafNode;
		newPData->parentPNum = parentPNum;
		newPData->prevPNum = selfPNum;
		newPData->nextPNum = nextPNum;
		newPData->childNum = 0;
		// Update the cur node.
		if (nextPNum == tree->infoPNum) {
			tree->dataTailPNum = newPNum;
		} else {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			next->prevPNum = newPNum;
			if (rc = next->MarkDirty())
				IX_PRINTSTACK
		}
		nextPNum = newPNum;
		// move the data
		int i = tree->maxChildNum / 2;
		MoveKey(i, newPData->key(0));
		MoveValue(i, (char*)newPData->page(0));
		newPData->childNum = childNum - i;
		childNum = i;
		// Insert parent.
		if (rc = parentPData->InsertPage(newPData->key(0), newPNum))
			IX_PRINTSTACK
		if (tree->CompareAttrWithRID(pData, LT_OP, newPData->key(0))) {
			if (rc = InsertRID(pData))
				IX_PRINTSTACK
		} else {
			if (newPData->InsertRID(pData))
				IX_PRINTSTACK
		}
		return OK_RC;
	}

	return OK_RC;
}

RC NodeHeader::InsertPage(char *pData, PageNum newPage) {
	RC rc;

	if (nodeType != InternalNode)
		IX_ERROR(IX_INSERTPAGETOLEAFNODE)
	if (rc = MarkDirty())
		IX_PRINTSTACK
	if (!IsFull()) {
		int pos = UpperBoundWithRID(pData);
		MoveKey(pos, key(pos + 1));
		MoveValue(pos + 1, (char*)page(pos + 2));
		memcpy(key(pos), pData, tree->attrLengthWithRid);
		*page(pos + 1) = newPage;
		++childNum;
		return OK_RC;
	} else {
		/*if (HavePrevPage()) {
			NodeHeader *prev;
			if (rc = PrevPage(prev))
				IX_PRINTSTACK
			if (!prev->IsFull()) {
				if (rc = prev->MarkDirty())
					IX_PRINTSTACK
				char *prevKey;
				if (rc = ParentPrevKey(prevKey))
					IX_PRINTSTACK
				if (prev != nullptr) {
					++(prev->childNum);
					memcpy(prev->lastKey(), prevKey, tree->attrLength);
					if (tree->CompareAttr(pData, LT_OP, key(0))) {
						*(prev->lastPage()) = *page(0);
						memcpy(prevKey, pData, tree->attrLength);
						*page(0) = newPage;
						return OK_RC;
					} else {
						*(prev->lastPage()) = *page(0);
						memcpy(prevKey, key(0), tree->attrLength);
						MoveKey(1, key(0));
						MoveValue(1, (char*)page(0));
						--childNum;
						if (rc = InsertPage(pData, newPage))
							IX_PRINTSTACK
						return OK_RC;
					}
				}
			}
		}
		if (HaveNextPage()) {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			if (!next->IsFull()) {
				if (rc = next->MarkDirty())
					IX_PRINTSTACK
				char *nextKey;
				if (rc = ParentNextKey(nextKey))
					IX_PRINTSTACK
				if (nextKey != nullptr) {
					next->MoveKey(0, key(1));
					next->MoveValue(0, (char*)page(1));
					++(next->childNum);
					memcpy(next->key(0), nextKey, tree->attrLength);
					if (tree->CompareAttr(lastKey(), LT_OP, pData)) {
						*(next->page(0)) = newPage;
						memcpy(nextKey, pData, tree->attrLength);
						return OK_RC;
					} else {
						*(next->page(0)) = *lastPage();
						memcpy(nextKey, lastKey(), tree->attrLength);
						--childNum;
						if (rc = InsertPage(pData, newPage))
							IX_PRINTSTACK
						return OK_RC;
					}
				}
			}
		}*/
		// Split
		PageNum newPNum;
		NodeHeader *newPData;
		if (rc = tree->AllocatePage(newPNum, newPData))
			IX_PRINTSTACK
		if (rc = newPData->MarkDirty())
			IX_PRINTSTACK
		NodeHeader *parentPData;
		if (HaveParentPage()) {
			if (rc = ParentPage(parentPData))
				IX_PRINTSTACK
		} else {
			if (rc = tree->AllocatePage(parentPNum, parentPData))
				IX_PRINTSTACK
			// Setup the new root node.
			parentPData->selfPNum = parentPNum;
			parentPData->nodeType = InternalNode;
			parentPData->parentPNum = -1;
			parentPData->prevPNum = -1;
			parentPData->nextPNum = -1;
			parentPData->childNum = 1;
			*parentPData->page(0) = selfPNum;
			// Update info page.
			tree->rootPNum = parentPNum;
		}
		if (rc = parentPData->MarkDirty())
			IX_PRINTSTACK
		// Setup the new node.
		newPData->selfPNum = newPNum;
		newPData->nodeType = InternalNode;
		newPData->parentPNum = parentPNum;
		newPData->prevPNum = selfPNum;
		newPData->nextPNum = nextPNum;
		newPData->childNum = 0;
		// Update the cur node.
		if (nextPNum != -1) {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			if (rc = next->MarkDirty())
				IX_PRINTSTACK
			next->prevPNum = newPNum;
		}
		nextPNum = newPNum;
		// move the data
		int i = tree->maxChildNum / 2;
		MoveKey(i, newPData->key(0));
		MoveValue(i, (char*)newPData->page(0));
		newPData->childNum = childNum - i;
		childNum = i;
		// Insert parent.
		if (rc = parentPData->InsertPage(key(i - 1), newPNum))
			IX_PRINTSTACK
		if (tree->CompareAttrWithRID(pData, LT_OP, key(i - 1))) {
			if (rc = InsertPage(pData, newPage))
				IX_PRINTSTACK
		} else {
			if (rc = newPData->InsertPage(pData, newPage))
				IX_PRINTSTACK
		}
		return OK_RC;
	}

	return OK_RC;
}

RC NodeHeader::ChildPage(int index, NodeHeader *&child) {
	RC rc;

	if (tree->GetPageData(*page(index), child))
		IX_PRINTSTACK

	return OK_RC;
}

RC NodeHeader::PrevPage(NodeHeader *&prev) {
	RC rc;

	if (tree->GetPageData(prevPNum, prev))
		IX_PRINTSTACK

	return OK_RC;
}

RC NodeHeader::NextPage(NodeHeader *&next) {
	RC rc;

	if (tree->GetPageData(nextPNum, next))
		IX_PRINTSTACK

	return OK_RC;
}

// the parent page number must be right !!!
RC NodeHeader::ParentPage(NodeHeader *&parent) {
	RC rc;

	if (tree->GetPageData(parentPNum, parent))
		IX_PRINTSTACK

	return OK_RC;
}

RC NodeHeader::ParentPrevKey(char *&k) {
	RC rc;

	if (nodeType != InternalNode)
		IX_ERROR(IX_GETPARENTKEYINLEAFNODE)

	NodeHeader *parent;
	if (rc = ParentPage(parent))
		IX_PRINTSTACK
	int index = parent->UpperBoundWithRID(key(0));
	if (index == 0)
		k = nullptr;
	else
		k = parent->key(index - 1);

	return OK_RC;
}

RC NodeHeader::ParentNextKey(char *&k) {
	RC rc;

	if (nodeType != InternalNode)
		IX_ERROR(IX_GETPARENTKEYINLEAFNODE)

	NodeHeader *parent;
	if (rc = ParentPage(parent))
		IX_PRINTSTACK
	int index = parent->UpperBoundWithRID(key(0));
	if (index == parent->childNum - 1)
		k = nullptr;
	else
		k = parent->key(index);

	return OK_RC;
}

RC NodeHeader::UpdateKey(char *oldKey, char *newKey) {
	RC rc;

	if (nodeType != InternalNode)
		IX_ERROR(IX_UPDATEKEYINLEAFNODE)

	int pos = LowerBoundWithRID(oldKey);
	memcpy(key(pos), newKey, tree->attrLengthWithRid);

	return OK_RC;
}

void NodeHeader::MoveKey(int begin, char* dest) {
	char *src = key(begin);
	int count = endKey() - src;
	memmove(dest, src, count);
}

void NodeHeader::MoveValue(int begin, char *dest) {
	char *src = (char*)page(begin);
	int count = (char*)endPage() - src;
	memmove(dest, src, count);
}

RC NodeHeader::MarkDirty() {
	RC rc;
	if (rc = tree->indexFH->MarkDirty(selfPNum))
		IX_ERROR(rc)
	return OK_RC;
}

bool TreeHeader::CompareAttr(void* valueA, CompOp compOp, void* valueB) {
	return Attr::CompareAttr(attrType, attrLength, valueA, compOp, valueB);
}

bool TreeHeader::CompareAttrWithRID(void *valueA, CompOp compOp, void *valueB) {
	return Attr::CompareAttrWithRID(attrType, attrLength, valueA, compOp, valueB);
}

RC TreeHeader::GetPageData(PageNum pNum, NodeHeader *&pData) {
    RC rc;
    char *tmp;
    auto iter = pageMap->find(pNum);
    if (iter == pageMap->end()) {
        PF_PageHandle ph;
        if (rc = indexFH->GetThisPage(pNum, ph))
            IX_ERROR(rc)
        if (rc = ph.GetData(tmp))
            IX_PRINTSTACK
        pageMap->insert(make_pair(pNum, tmp));
        pData = (NodeHeader*)tmp;
    } else {
        pData = (NodeHeader*)iter->second;
    }
    pData->tree = this;
    pData->keys = ((char*)pData + sizeof(NodeHeader));
    pData->values = (pData->keys + attrLengthWithRid * maxChildNum);
    return OK_RC;
}

RC TreeHeader::AllocatePage(PageNum &pageNum, NodeHeader *&pageData) {
	RC rc;

	PF_PageHandle newPH;
	char *tmp = (char*)pageData;
	if (rc = IX_AllocatePage(*indexFH, newPH, pageNum, tmp))
		IX_PRINTSTACK
	pageMap->insert(make_pair(pageNum, tmp));
	pageData = (NodeHeader*)tmp;
	pageData->tree = this;
	pageData->keys = (tmp + sizeof(NodeHeader));
    pageData->values = (pageData->keys + attrLengthWithRid * maxChildNum);
	return OK_RC;
}

RC TreeHeader::UnpinPages() {
    RC rc;
    for (auto i : *pageMap)
        if (rc = indexFH->UnpinPage(i.first))
            IX_PRINTSTACK
    pageMap->clear();
    return OK_RC;
}

RC TreeHeader::Insert(char *pData) {
	RC rc;

	NodeHeader *root;
	if (rc = GetPageData(rootPNum, root))
		IX_PRINTSTACK

	NodeHeader *leaf;
	if (rc = SearchLeafNodeWithRID(pData, root, leaf))
		IX_PRINTSTACK
	if (rc = leaf->InsertRID(pData))
		IX_PRINTSTACK

	if (rc = UnpinPages())
		IX_PRINTSTACK
	return OK_RC;
}

RC TreeHeader::Search(char *pData, CompOp compOp, NodeHeader *&leaf, int &index) {
	RC rc;

	if (compOp == NO_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		index = 0;
		if (!IsValidScanResult(pData, compOp, leaf, index))
			leaf = nullptr;
		return OK_RC;
	}
	
	if (compOp == EQ_OP) {
		NodeHeader *root;
		if (rc = GetPageData(rootPNum, root))
			IX_PRINTSTACK
		appendMinRID(pData);
		if (rc = SearchLeafNode(pData, root, leaf))
			IX_PRINTSTACK
        appendMinRID(pData);
		index = leaf->LowerBoundWithRID(pData);
		if (!IsValidScanResult(pData, compOp, leaf, index))
			leaf = nullptr;
		return OK_RC;
	}
	
	if (compOp == NE_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		index = 0;
		if (IsValidScanResult(pData, compOp, leaf, index))
			return OK_RC;

		NodeHeader *root;
		if (rc = GetPageData(rootPNum, root))
			IX_PRINTSTACK
		appendMinRID(pData);
		if (rc = SearchLeafNode(pData, root, leaf))
			IX_PRINTSTACK
		appendMaxRID(pData);
		index = leaf->UpperBoundWithRID(pData);
		if (IsValidScanResult(pData, compOp, leaf, index))
			return OK_RC;
		
		if (index != leaf->childNum || !leaf->HaveNextPage()) {
			leaf = nullptr;
			return OK_RC;
		}

		if (rc = leaf->NextPage(leaf))
			IX_PRINTSTACK
		index = 0;
		if (!IsValidScanResult(pData, compOp, leaf, index))
			leaf = nullptr;
		return OK_RC;
	}
	
	if (compOp == LT_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		index = 0;
		if (!IsValidScanResult(pData, compOp, leaf, index))
			leaf = nullptr;
		return OK_RC;
	}
	
	if (compOp == LE_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		index = 0;
		if (!IsValidScanResult(pData, compOp, leaf, index))
			leaf = nullptr;
		return OK_RC;
	}
	
	if (compOp == GT_OP) {
		NodeHeader *root;
		if (rc = GetPageData(rootPNum, root))
			IX_PRINTSTACK
		appendMinRID(pData);
		if (rc = SearchLeafNode(pData, root, leaf))
			IX_PRINTSTACK
		appendMaxRID(pData);
		index = leaf->UpperBoundWithRID(pData);
		if (IsValidScanResult(pData, compOp, leaf, index))
			return OK_RC;
		
		if (index != leaf->childNum || !leaf->HaveNextPage()) {
			leaf = nullptr;
			return OK_RC;
		}

		if (rc = leaf->NextPage(leaf))
			IX_PRINTSTACK
		index = 0;
		if (!IsValidScanResult(pData, compOp, leaf, index))
			leaf = nullptr;
		return OK_RC;
	}
	
	if (compOp == GE_OP) {
		NodeHeader *root;
		if (rc = GetPageData(rootPNum, root))
			IX_PRINTSTACK
		appendMinRID(pData);
		if (rc = SearchLeafNode(pData, root, leaf))
			IX_PRINTSTACK
		appendMinRID(pData);
		index = leaf->LowerBoundWithRID(pData);
		if (!IsValidScanResult(pData, compOp, leaf, index))
			leaf = nullptr;
		return OK_RC;
	}

	return OK_RC;
}

RC TreeHeader::Delete(char *pData) {
	RC rc;

	NodeHeader *root;
	if (rc = GetPageData(rootPNum, root))
		IX_PRINTSTACK

	NodeHeader *leaf;
	if (rc = SearchLeafNodeWithRID(pData, root, leaf))
		IX_PRINTSTACK
	if (rc = leaf->DeleteRID(pData))
		IX_PRINTSTACK

	if (rc = UnpinPages())
		IX_PRINTSTACK
	return OK_RC;
}

RC TreeHeader::GetNextEntry(char *pData, CompOp compOp, NodeHeader *&cur, int &index, bool &newPage) {
	RC rc;

	bool gt = CompareAttr(cur->key(index), GT_OP, pData);
	newPage = false;

	++index;
	if (IsValidScanResult(pData, compOp, cur, index))
		return OK_RC;
	if (compOp != NE_OP || gt) {
		if (index != cur->childNum || !cur->HaveNextPage()) {
			cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->NextPage(cur))
			IX_PRINTSTACK
		newPage = true;
		index = 0;
		if (!IsValidScanResult(pData, compOp, cur, index))
			cur = nullptr;
		return OK_RC;
	} else {
		if (index != cur->childNum || !cur->HaveNextPage()) {
			NodeHeader *root;
			if (rc = GetPageData(rootPNum, root))
				IX_PRINTSTACK
			appendMinRID(pData);
			if (rc = SearchLeafNode(pData, root, cur))
				IX_PRINTSTACK
			appendMaxRID(pData);
			newPage = true;
			index = cur->UpperBoundWithRID(pData);
			if (IsValidScanResult(pData, compOp, cur, index))
				return OK_RC;
			if (index != cur->childNum || !cur->HaveNextPage()) {
				cur = nullptr;
				return OK_RC;
			}
			if (rc = cur->NextPage(cur))
				IX_PRINTSTACK
			newPage = true;
			index = 0;
			if (!IsValidScanResult(pData, compOp, cur, index))
				cur = nullptr;
			return OK_RC;
		}
		if (rc = cur->NextPage(cur))
			IX_PRINTSTACK
		newPage = true;
		index = 0;
		if (!IsValidScanResult(pData, compOp, cur, index))
			cur = nullptr;
		return OK_RC;
	}
	return OK_RC;
}

RC TreeHeader::SearchLeafNodeWithRID(char *pData, NodeHeader *cur, NodeHeader *&leaf) {
	RC rc;
	//printf(" selfPNum = [");
	while (cur->nodeType != LeafNode) {
		int index = cur->UpperBoundWithRID(pData);
		int lastPNum = cur->selfPNum;
		if (rc = cur->ChildPage(index, cur))
			IX_PRINTSTACK
		cur->parentPNum = lastPNum; // important !!!
		if (rc = cur->MarkDirty())
			IX_PRINTSTACK
		//printf(" ,%d) ", cur->selfPNum);
	}
	//printf("]\n");

	leaf = cur;

	return OK_RC;
}

RC TreeHeader::SearchLeafNode(char *pData, NodeHeader *cur, NodeHeader *&leaf) {
    RC rc;
    //printf("pagenum: %d\n", cur->selfPNum);
    while (cur->nodeType != LeafNode) {
        NodeHeader *p = cur;
        int index = p->UpperBoundWithRID(pData);
        int lastPNum = p->selfPNum;
        if (rc = p->ChildPage(index, cur))
            IX_PRINTSTACK
        //printf("pagenum: %d\n", cur->selfPNum);
        /*if (cur->HaveNextPage()) {
            if (CompareAttrWithRID(cur->lastKey(), LT_OP, pData)) {
                if (rc = cur->NextPage(cur))
                    IX_PRINTSTACK
                printf("+1 %%%% pagenum: %d\n", cur->selfPNum);
            }
        }*/
        cur->parentPNum = lastPNum; // important !!!
        if (rc = cur->MarkDirty())
            IX_PRINTSTACK
    }
    while (cur->HaveNextPage()) {
        if (CompareAttrWithRID(cur->lastKey(), LT_OP, pData)) {
            if (rc = cur->NextPage(cur))
                IX_PRINTSTACK
            //printf("+1 %%%% pagenum: %d\n", cur->selfPNum);
        } else {
            break;
        }
    }
    leaf = cur;
    return OK_RC;
}

RC TreeHeader::GetFirstLeafNode(NodeHeader *&leaf) {
	RC rc;

	if (rc = GetPageData(dataHeadPNum, leaf))
		IX_PRINTSTACK

	return OK_RC;
}

RC TreeHeader::GetLastLeafNode(NodeHeader *&leaf) {
	RC rc;

	if (rc = GetPageData(dataTailPNum, leaf))
		IX_PRINTSTACK

	return OK_RC;
}

bool TreeHeader::IsValidScanResult(char *pData, CompOp compOp, NodeHeader *cur, int index) {
	if (cur == nullptr)
		return false;
	if (index < 0 || index >= cur->childNum)
		return false;
	return CompareAttr(cur->key(index), compOp, pData);
}

void TreeHeader::appendMaxRID(char *pData) {
	static RID maxRID(numeric_limits<int>::max(), numeric_limits<int>::max());
	memmove(pData + attrLength, &maxRID, sizeof(RID));
}

void TreeHeader::appendMinRID(char *pData) {
	static RID minRID(-1, -1);
	memmove(pData + attrLength, &minRID, sizeof(RID));
}

RC TreeHeader::DisplayAllTree() {
	RC rc;
	int depth = 0;
    char leftBuffer[PF_PAGE_SIZE];
    char curBuffer[PF_PAGE_SIZE];
	NodeHeader *left = nullptr;
	if ((rc = GetPageData(rootPNum, left)))
		IX_PRINTSTACK
    memmove(leftBuffer, left, PF_PAGE_SIZE);
    UnpinPages();
	while (true) {
		printf("\n depth  %d  \n\n", depth);
		NodeHeader* cur = (NodeHeader*)leftBuffer;
		while (true) {
			printf("\nnew page  %d\n\nprev page  %d  next  page  %d\n\n", cur->selfPNum, cur->prevPNum, cur->nextPNum);
            int maxC = cur->nodeType == LeafNode ? cur->childNum : cur->childNum - 1;
			for (int i = 0; i < maxC; ++i) {
                if (cur->nodeType == InternalNode)
				    printf("page: %d\n", *(cur->page(i)));
				PageNum pageNum;
				SlotNum slotNum;
				if ((rc = cur->rid(i)->GetPageNum(pageNum)) || (rc = cur->rid(i)->GetSlotNum(slotNum))) IX_PRINTSTACK
				printf("key: %d, rid: ( %d, %d )\n", *(int*)(cur->key(i)), pageNum, slotNum);
			}
            if (cur->nodeType == InternalNode)
			    printf("page: %d\n", *(cur->page(cur->childNum - 1)));
			if (!cur->HaveNextPage())
				break;
			if ((rc = cur->NextPage(cur)))
				IX_PRINTSTACK
            memmove(curBuffer, cur, PF_PAGE_SIZE);
            UnpinPages();
		}
        left = (NodeHeader*)leftBuffer;
		if (left->nodeType == LeafNode)
			break;
		if ((rc = left->ChildPage(0, left)))
			IX_PRINTSTACK
		memmove(leftBuffer, left, PF_PAGE_SIZE);
	}
    UnpinPages();
	return OK_RC;
}