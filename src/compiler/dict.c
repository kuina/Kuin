#include "dict.h"

#include "mem.h"

static SDict* AddRecursion(SDict* node, const Char* key, const void* value);
static SDictI* AddIRecursion(SDictI* node, U64 key, const void* value);

SDict* DictAdd(SDict* root, const Char* key, const void* value)
{
	SDict* node = AddRecursion(root, key, value);
	node->Red = False; // Set the root to black.
	return node;
}

const void* DictSearch(const SDict* root, const Char* key)
{
	const SDict* node = root;
	while (node != NULL)
	{
		int cmp = wcscmp(key, node->Key);
		if (cmp == 0)
			return node->Value;
		if (cmp < 0)
			node = node->Left;
		else
			node = node->Right;
	}
	return NULL;
}

void DictForEach(SDict* root, const void*(*func)(const Char* key, const void* value, void* param), void* param)
{
	if (root == NULL)
		return;
	DictForEach(root->Left, func, param);
	root->Value = func(root->Key, root->Value, param);
	DictForEach(root->Right, func, param);
}

SDictI* DictIAdd(SDictI* root, U64 key, const void* value)
{
	SDictI* node = AddIRecursion(root, key, value);
	node->Red = False; // Set the root to black.
	return node;
}

const void* DictISearch(const SDictI* root, U64 key)
{
	const SDictI* node = root;
	while (node != NULL)
	{
		S64 cmp = (S64)key - (S64)node->Key;
		if (cmp == 0)
			return node->Value;
		if (cmp < 0)
			node = node->Left;
		else
			node = node->Right;
	}
	return NULL;
}

void DictIForEach(SDictI* root, const void*(*func)(U64 key, const void* value, void* param), void* param)
{
	if (root == NULL)
		return;
	DictIForEach(root->Left, func, param);
	root->Value = func(root->Key, root->Value, param);
	DictIForEach(root->Right, func, param);
}

static SDict* AddRecursion(SDict* node, const Char* key, const void* value)
{
	// Add a node if the key does not exist.
	if (node == NULL)
	{
		SDict* n = (SDict*)Alloc(sizeof(SDict));
		n->Key = key;
		n->Value = value;
		n->Red = True; // Set new nodes to red.
		n->Left = NULL;
		n->Right = NULL;
		return n;
	}

	{
		int cmp = wcscmp(key, node->Key);
		// Overwrite the value if the key already exists.
		if (cmp == 0)
		{
			node->Value = value;
			return node;
		}
		if (cmp < 0)
			node->Left = AddRecursion(node->Left, key, value);
		else
			node->Right = AddRecursion(node->Right, key, value);
	}

	// Rotate to the left when the right node is red.
	if (node->Right != NULL && node->Right->Red)
	{
		SDict* r = node->Right;
		node->Right = r->Left;
		r->Left = node;
		r->Red = node->Red;
		node->Red = True;
		node = r;
	}

	// Rotate to the right when the left node is red.
	if (node->Left != NULL && node->Left->Red && node->Left->Left != NULL && node->Left->Left->Red)
	{
		SDict* l = node->Left;
		node->Left = l->Right;
		l->Right = node;
		l->Red = node->Red;
		node->Red = True;
		node = l;

		// Since both child nodes are red here, devide it so that the tree does not become a quad tree.
		node->Red = True;
		node->Left->Red = False;
		node->Right->Red = False;
	}

	return node;
}

static SDictI* AddIRecursion(SDictI* node, U64 key, const void* value)
{
	if (node == NULL)
	{
		SDictI* n = (SDictI*)Alloc(sizeof(SDictI));
		n->Key = key;
		n->Value = value;
		n->Red = True;
		n->Left = NULL;
		n->Right = NULL;
		return n;
	}

	{
		S64 cmp = (S64)key - (S64)node->Key;
		if (cmp == 0)
		{
			node->Value = value;
			return node;
		}
		if (cmp < 0)
			node->Left = AddIRecursion(node->Left, key, value);
		else
			node->Right = AddIRecursion(node->Right, key, value);
	}

	if (node->Right != NULL && node->Right->Red)
	{
		SDictI* r = node->Right;
		node->Right = r->Left;
		r->Left = node;
		r->Red = node->Red;
		node->Red = True;
		node = r;
	}

	if (node->Left != NULL && node->Left->Red && node->Left->Left != NULL && node->Left->Left->Red)
	{
		SDictI* l = node->Left;
		node->Left = l->Right;
		l->Right = node;
		l->Red = node->Red;
		node->Red = True;
		node = l;

		node->Red = True;
		node->Left->Red = False;
		node->Right->Red = False;
	}

	return node;
}
