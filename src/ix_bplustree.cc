#include "ix.h"
using namespace std;

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
	return keys + index * tree->attrLength;
}

char *NodeHeader::lastKey() {
	return endKey() - tree->attrLength;
}

char* NodeHeader::endKey() {
	if (nodeType == InternalNode)
		return keys + (childNum - 1) * tree->attrLength;
	else
		return keys + childNum * tree->attrLength;
}

RID* NodeHeader::rid(int index) {
	return (RID*)(values + index * tree->childItemSize);
}

RID* NodeHeader::lastRid() {
	return (RID*)((char*)endRid() - tree->childItemSize);
}

RID* NodeHeader::endRid() {
	return (RID*)(values + childNum * tree->childItemSize);
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

int NodeHeader::LowerBound(char* pData) {
	if (nodeType == LeafNode)
		return Attr::lower_bound(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::lower_bound(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

pair<int, int> NodeHeader::EqualRange(char *pData) {
	if (nodeType == LeafNode)
		return Attr::equal_range(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::equal_range(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

bool NodeHeader::BinarySearch(char *pData) {
	if (nodeType == LeafNode)
		return Attr::binary_search(tree->attrType, tree->attrLength, keys, childNum, pData);
	else
		return Attr::binary_search(tree->attrType, tree->attrLength, keys, childNum - 1, pData);
}

RC NodeHeader::DeleteRID(char *pData, const RID &r) {
	RC rc;
	if (nodeType != LeafNode)
		IX_ERROR(IX_DELETERIDFROMINTERNALNODE)
	int pos = LowerBound(pData);
	if (pos == childNum || !tree->CompareAttr(pData, EQ_OP, key(pos))) {
		//if (pos == childNum) printf("error!"); else printf("error2!!%d", selfPNum);
		IX_ERROR(IX_DELETERIDNOTEXIST)
	}
	if (rc = MarkDirty())
		IX_PRINTSTACK
	MoveKey(pos + 1, key(pos));
	MoveValue(pos + 1, (char*)rid(pos));
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
	int pos = UpperBound(pData);
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

RC NodeHeader::InsertRID(char *pData, const RID &r) {
	//PageNum num;
    //r.GetPageNum(num);
    //printf("%d\n", num);
	RC rc;
	if (nodeType != LeafNode)
		IX_ERROR(IX_INSERTRIDTOINTERNALNODE)
	if (rc = MarkDirty())
		IX_PRINTSTACK
	if (!IsFull()) {
		int pos = UpperBound(pData);
		//if (pos == 0 && HaveParentPage()) {
		//	NodeHeader *parent;
		//	if (rc = ParentPage(parent))
		//		IX_PRINTSTACK
			//if (rc = parent->UpdateKey(key(0), pData))
			//	IX_PRINTSTACK
		//}
		MoveKey(pos, key(pos + 1));
		MoveValue(pos, (char*)rid(pos + 1));
		memcpy(key(pos), pData, tree->attrLength);
		*rid(pos) = r;
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
		MoveValue(i, (char*)newPData->rid(0));
		newPData->childNum = childNum - i;
		childNum = i;
		// Insert parent.
		if (rc = parentPData->InsertPage(newPData->key(0), newPNum))
			IX_PRINTSTACK
		if (tree->CompareAttr(pData, LT_OP, newPData->key(0))) {
			if (rc = InsertRID(pData, r))
				IX_PRINTSTACK
		} else {
			if (newPData->InsertRID(pData, r))
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
		int pos = UpperBound(pData);
		MoveKey(pos, key(pos + 1));
		MoveValue(pos + 1, (char*)page(pos + 2));
		memcpy(key(pos), pData, tree->attrLength);
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
		if (tree->CompareAttr(pData, LT_OP, key(i - 1))) {
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
	int index = parent->UpperBound(key(0));
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
	int index = parent->UpperBound(key(0));
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

	int pos = LowerBound(oldKey);
	memcpy(key(pos), newKey, tree->attrLength);

	return OK_RC;
}

void NodeHeader::MoveKey(int begin, char* dest) {
	char *src = key(begin);
	int count = endKey() - src;
	memmove(dest, src, count);
}

void NodeHeader::MoveValue(int begin, char *dest) {
	char *src = (char*)rid(begin);
	int count = (char*)endRid() - src;
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
    pData->values = (pData->keys + attrLength * maxChildNum);
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
    pageData->values = (pageData->keys + attrLength * maxChildNum);
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

RC TreeHeader::Insert(char *pData, const RID& rid) {
	RC rc;

	NodeHeader *root;
	if (rc = GetPageData(rootPNum, root))
		IX_PRINTSTACK

	NodeHeader *leaf;
	if (rc = SearchLeafNode(pData, root, leaf))
		IX_PRINTSTACK
	if (rc = leaf->InsertRID(pData, rid))
		IX_PRINTSTACK

	if (rc = UnpinPages())
		IX_PRINTSTACK
	return OK_RC;
}

RC TreeHeader::Search(char *pData, CompOp compOp, NodeHeader *&leaf, int &index, bool &isLT) {
	RC rc;

	if (compOp == NO_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		index = 0;
		if (leaf->IsEmpty())
			leaf = nullptr;
		return OK_RC;
	} else if (compOp == EQ_OP) {
		NodeHeader *root;
		if (rc = GetPageData(rootPNum, root))
			IX_PRINTSTACK
		if (rc = SearchLeafNode(pData, root, leaf))
			IX_PRINTSTACK
		if (leaf->IsEmpty()) {
			leaf = nullptr;
			return OK_RC;
		}
		index = leaf->LowerBound(pData);
		if (CompareAttr(pData, NE_OP, leaf->key(index)))
			leaf = nullptr;
		return OK_RC;
	} else if (compOp == NE_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		if (leaf->IsEmpty()) {
			leaf = nullptr;
			return OK_RC;
		}
		index = 0;
		if (CompareAttr(leaf->key(index), LT_OP, pData)) {
			isLT = true;
			return OK_RC;
		}
		if (rc = GetLastLeafNode(leaf))
			IX_PRINTSTACK
		if (leaf->IsEmpty()) {
			leaf = nullptr;
			return OK_RC;
		}
		index = leaf->childNum - 1;
		if (CompareAttr(pData, LT_OP, leaf->key(index))) {
			isLT = false;
			return OK_RC;
		}
		leaf = nullptr;
		return OK_RC;
	} else if (compOp == LT_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		if (leaf->IsEmpty()) {
			leaf == nullptr;
			return OK_RC;
		}
		index = 0;
		if (CompareAttr(pData, LE_OP, leaf->key(index)))
			leaf = nullptr;
		return OK_RC;
	} else if (compOp == LE_OP) {
		if (rc = GetFirstLeafNode(leaf))
			IX_PRINTSTACK
		if (leaf->IsEmpty()) {
			leaf = nullptr;
			return OK_RC;
		}
		index = 0;
		if (CompareAttr(pData, LT_OP, leaf->key(index)))
			leaf = nullptr;
		return OK_RC;
	} else if (compOp == GT_OP) {
		if (rc = GetLastLeafNode(leaf))
			IX_PRINTSTACK;
		if (leaf->IsEmpty()) {
			leaf = nullptr;
			return OK_RC;
		}
		index = leaf->childNum - 1;
		if (CompareAttr(leaf->key(index), LE_OP, pData))
			leaf = nullptr;
		return OK_RC;
	} else if (compOp == GE_OP) {
		if (rc = GetLastLeafNode(leaf))
			IX_PRINTSTACK
		if (leaf->IsEmpty()) {
			leaf = nullptr;
			return OK_RC;
		}
		index = leaf->childNum - 1;
		if (CompareAttr(leaf->key(index), LT_OP, pData))
			leaf = nullptr;
		return OK_RC;
	} else {
		leaf = nullptr;
	}

	return OK_RC;
}

RC TreeHeader::Delete(char *pData, const RID& rid) {
	RC rc;

	NodeHeader *root;
	if (rc = GetPageData(rootPNum, root))
		IX_PRINTSTACK

	NodeHeader *leaf;
	if (rc = SearchLeafNode(pData, root, leaf))
		IX_PRINTSTACK
	if (rc = leaf->DeleteRID(pData, rid))
		IX_PRINTSTACK

	if (rc = UnpinPages())
		IX_PRINTSTACK
	return OK_RC;
}

RC TreeHeader::SearchLeafNode(char *pData, NodeHeader *cur, NodeHeader *&leaf) {
	RC rc;
	//printf(" selfPNum = [");
	while (cur->nodeType != LeafNode) {
		int index = cur->UpperBound(pData);
		/*if (*(int*)pData == 10) {
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
			printf("index = %d\n", index);
		}*/
		//printf("(%d", *(int*)cur->key(index));
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