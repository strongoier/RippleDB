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

RID* NodeHeader::rid(int index) {
	return (RID*)(values + index * tree->childItemSize);
}

PageNum* NodeHeader::page(int index) {
	return (PageNum*)(values + index * tree->childItemSize);
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

RC NodeHeader::InsertRID(char *pData, const RID &r) {
	RC rc;
	if (nodeType != LeafNode)
		IX_ERROR(IX_INSERTRIDTOINTERNALNODE)
	if (!IsFull()) {
		int pos = UpperBound(pData);
		if (pos == 0 && HaveParentPage()) {
			NodeHeader *parent;
			if (rc = ParentPage(parent))
				IX_PRINTSTACK
			if (rc = parent->UpdateKey(key(0), pData))
				IX_PRINTSTACK
		}
		char *ksrc = key(pos);
		char *kdest = key(pos + 1);
		int kcount = (childNum - pos) * tree->attrLength;
		memmove(kdest, ksrc, kcount);
		char *rsrc = (char*)rid(pos);
		char *rdest = (char*)rid(pos + 1);
		int rcount = (childNum - pos) * tree->childItemSize;
		memmove(rdest, rsrc, rcount);
		memcpy(key(pos), pData, tree->attrLength);
		*rid(pos) = r;
		++childNum;
		return OK_RC;
	} else {
		if (HavePrevPage()) {
			NodeHeader *prev;
			if (rc = PrevPage(prev))
				IX_PRINTSTACK
			if (!prev->IsFull()) {
				// todo multi key
				if (Attr::CompareAttr(tree->attrType, tree->attrLength, pData, LT_OP
					, key(0))) {
					if (rc = prev->InsertRID(pData, r))
						IX_PRINTSTACK
					return OK_RC;
				} else {
					if (rc = prev->InsertRID(key(0), *rid(0)))
						IX_PRINTSTACK
					char *ksrc = key(1);
					char *kdest = key(0);
					int kcount = tree->attrLength * (childNum - 1);
					memmove(kdest, ksrc, kcount);
					char *rsrc = (char*)rid(1);
					char *rdest = (char*)rid(0);
					int rcount = tree->childItemSize * (childNum - 1);
					memmove(rdest, rsrc, rcount);
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
			if (!next->IsFull()) {
				// todo multi key
				if (Attr::CompareAttr(tree->attrType, tree->attrLength, key(childNum - 1), LT_OP, pData)) {
					if (rc = next->InsertRID(pData, r))
						IX_PRINTSTACK
					return OK_RC;
				} else {
					if (rc = next->InsertRID(key(childNum - 1), *rid(childNum - 1)))
						IX_PRINTSTACK
					--childNum;
					if (rc = InsertRID(pData, r))
						IX_PRINTSTACK
					return OK_RC;
				}
			}
		}
		// todo multi key
		// Split
		PageNum newPNum;
		NodeHeader *newPData;
		if (rc = tree->AllocatePage(newPNum, newPData))
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
			parentPData->keys = (char*)parentPData + sizeof(NodeHeader);
			parentPData->values = parentPData->keys + tree->maxChildNum * tree->attrLength;
			*parentPData->page(0) = selfPNum;
			// Update info page.
			tree->rootPNum = parentPNum;
		}

		// Setup the new node.
		newPData->selfPNum = newPNum;
		newPData->nodeType = LeafNode;
		newPData->parentPNum = parentPNum;
		newPData->prevPNum = selfPNum;
		newPData->nextPNum = nextPNum;
		newPData->childNum = 0;
		newPData->keys = (char*)newPData + sizeof(NodeHeader);
		newPData->values = newPData->keys + tree->maxChildNum * tree->attrLength;
		// Update the cur node.
		if (nextPNum == tree->infoPNum) {
			tree->dataTailPNum = newPNum;
		} else {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			next->prevPNum = newPNum;
		}
		nextPNum = newPNum;
		// move the data
		int i = tree->maxChildNum / 2;
		char *ksrc = key(i);
		char *kdest = newPData->key(0);
		int kcount = tree->attrLength * (childNum - i);
		memmove(kdest, ksrc, kcount);
		char *rsrc = (char*)rid(i);
		char *rdest = (char*)newPData->rid(0);
		int rcount = tree->childItemSize * (childNum - i);
		memmove(rdest, rsrc, rcount);
		childNum = i;
		newPData->childNum = tree->maxChildNum - i;
		// Insert parent.
		if (rc = parentPData->InsertPage(newPData->key(0), newPNum))
			IX_PRINTSTACK
		if (Attr::CompareAttr(tree->attrType, tree->attrLength, pData, LT_OP, newPData->key(0))) {
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

	if (!IsFull()) {
		int pos = UpperBound(pData);
		char *ksrc = key(pos);
		char *kdest = key(pos + 1);
		int kcount = (childNum - 1 - pos) * tree->attrLength;
		memmove(kdest, ksrc, kcount);
		char *psrc = (char*)page(pos + 1);
		char *pdest = (char*)page(pos + 2);
		int pcount = (childNum - 1 - pos) * tree->childItemSize;
		memmove(pdest, psrc, pcount);
		memcpy(key(pos), pData, tree->attrLength);
		*page(pos + 1) = newPage;
		++childNum;
		return OK_RC;
	} else {
		if (HavePrevPage()) {
			NodeHeader *prev;
			if (rc = PrevPage(prev))
				IX_PRINTSTACK
			if (!prev->IsFull()) {
				// todo multi key
				char *prevKey;
				if (rc = ParentPrevKey(prevKey))
					IX_PRINTSTACK
				++(prev->childNum);
				memcpy(prev->key(prev->childNum - 2), prevKey, tree->attrLength);
				if (Attr::CompareAttr(tree->attrType, tree->attrLength, pData, LT_OP, key(0))) {
					*(prev->page(prev->childNum - 1)) = newPage;
					memcpy(prevKey, pData, tree->attrLength);
					return OK_RC;
				} else {
					*(prev->page(prev->childNum - 1)) = *page(0);
					memcpy(prevKey, key(0), tree->attrLength);
					char *ksrc = key(1);
					char *kdest = key(0);
					int kcount = tree->attrLength * (childNum - 2);
					memmove(kdest, ksrc, kcount);
					char *psrc = (char*)page(1);
					char *pdest = (char*)page(0);
					int pcount = tree->childItemSize * (childNum - 1);
					memmove(pdest, psrc, pcount);
					if (rc = InsertPage(pData, newPage))
						IX_PRINTSTACK
					return OK_RC;
				}
			}
		}
		if (HaveNextPage()) {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			if (!next->IsFull()) {
				// todo multi key
				char *nextKey;
				if (rc = ParentNextKey(nextKey))
					IX_PRINTSTACK
				char *ksrc = next->key(0);
				char *kdest = next->key(1);
				int kcount = tree->attrLength * (next->childNum - 1);
				memmove(kdest, ksrc, kcount);
				char *psrc = (char*)next->page(0);
				char *pdest = (char*)next->page(1);
				int pcount = tree->childItemSize * (next->childNum);
				memmove(pdest, psrc, pcount);
				++(next->childNum);
				memcpy(next->key(0), nextKey, tree->attrLength);
				if (Attr::CompareAttr(tree->attrType, tree->attrLength, key(childNum - 2), LT_OP, pData)) {
					*(next->page(0)) = newPage;
					memcpy(nextKey, pData, tree->attrLength);
					return OK_RC;
				} else {
					*(next->page(0)) = *page(childNum - 1);
					memcpy(nextKey, key(childNum - 2), tree->attrLength);
					if (rc = InsertPage(pData, newPage))
						IX_PRINTSTACK
					return OK_RC;
				}
			}
		}
		// todo multi key
		// Split
		PageNum newPNum;
		NodeHeader *newPData;
		if (rc = tree->AllocatePage(newPNum, newPData))
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
			parentPData->keys = (char*)parentPData + sizeof(NodeHeader);
			parentPData->values = parentPData->keys + tree->maxChildNum * tree->attrLength;
			*parentPData->page(0) = selfPNum;
			// Update info page.
			tree->rootPNum = parentPNum;
		}

		// Setup the new node.
		newPData->selfPNum = newPNum;
		newPData->nodeType = LeafNode;
		newPData->parentPNum = parentPNum;
		newPData->prevPNum = selfPNum;
		newPData->nextPNum = nextPNum;
		newPData->childNum = 0;
		newPData->keys = (char*)newPData + sizeof(NodeHeader);
		newPData->values = newPData->keys + tree->maxChildNum * tree->attrLength;
		// Update the cur node.
		if (nextPNum == tree->infoPNum) {
			tree->dataTailPNum = newPNum;
		} else {
			NodeHeader *next;
			if (rc = NextPage(next))
				IX_PRINTSTACK
			next->prevPNum = newPNum;
		}
		nextPNum = newPNum;
		// move the data
		int i = tree->maxChildNum / 2;
		char *ksrc = key(i);
		char *kdest = newPData->key(0);
		int kcount = tree->attrLength * (childNum - i);
		memmove(kdest, ksrc, kcount);
		char *rsrc = (char*)rid(i);
		char *rdest = (char*)newPData->rid(0);
		int rcount = tree->childItemSize * (childNum - i);
		memmove(rdest, rsrc, rcount);
		childNum = i;
		newPData->childNum = tree->maxChildNum - i;
		// Insert parent.
		if (rc = parentPData->InsertPage(newPData->key(0), newPNum))
			IX_PRINTSTACK
		if (Attr::CompareAttr(tree->attrType, tree->attrLength, pData, LT_OP, newPData->key(0))) {
			if (rc = InsertRID(pData, r))
				IX_PRINTSTACK
		} else {
			if (newPData->InsertRID(pData, r))
				IX_PRINTSTACK
		}
		return OK_RC;*/
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

RC TreeHeader::GetPageData(PageNum pNum, NodeHeader *&pData) {
    RC rc;
    char *tmp;
    auto iter = pageMap->find(pNum);
    if (iter == pageMap->end()) {
        PF_PageHandle ph;
        if (rc = indexFH->GetThisPage(pNum, ph))
            IX_PRINTSTACK
        if (rc = ph.GetData(tmp))
            IX_PRINTSTACK
        pageMap->insert(make_pair(pNum, tmp));
        pData = (NodeHeader*)tmp;
    } else {
        pData = (NodeHeader*)iter->second;
    }
    pData->tree = this;
    return OK_RC;
}

RC TreeHeader::AllocatePage(PageNum &pageNum, NodeHeader *&pageData) {
	RC rc;

	PF_PageHandle newPH;
	char *tmp = (char*)pageData;
	if (rc = PFHelper::AllocatePage(*indexFH, newPH, pageNum, tmp))
		IX_PRINTSTACK
	pageMap->insert(make_pair(pageNum, tmp));
	pageData = (NodeHeader*)tmp;
	pageData->tree = this;
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

	printf("%p\n", root);

	NodeHeader *leaf;
	if (rc = SearchLeafNode(pData, root, leaf))
		IX_PRINTSTACK
	if (rc = leaf->InsertRID(pData, rid))
		IX_PRINTSTACK

	if (rc = UnpinPages())
		IX_PRINTSTACK
	return OK_RC;
}

RC TreeHeader::Delete(char *pData, const RID& rid) {
	RC rc;

	return OK_RC;
}

RC TreeHeader::SearchLeafNode(char *pData, NodeHeader *cur, NodeHeader *&leaf) {
	RC rc;

	while (cur->nodeType != LeafNode) {
		int index = cur->LowerBound(pData);
		if (rc = cur->ChildPage(index, cur))
			IX_PRINTSTACK
	}

	leaf = cur;

	return OK_RC;
}