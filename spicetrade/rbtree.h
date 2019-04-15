#pragma once
#include "mem.h"
#include "list.h"
#include <stdio.h>

template<typename KTYPE, typename VTYPE>
class RBTree {
public:
	#define LEAF NULL
	#define assert(condition)	if(!(condition)){ printf("failed condition at %d\n", __LINE__); int i=0;i=1/i;}
	static const char BLACK = 0;
	static const char RED = 1;
	struct node {
		KTYPE key;
		VTYPE value;
		char color;
		node *left, *right, *parent;
		node():left(LEAF),right(LEAF),parent(NULL),color(BLACK) {}
		node(KTYPE key, VTYPE value):key(key),value(value),color(BLACK),left(LEAF),right(LEAF),parent(NULL){}
	};
	RBTree():root(0),count(0),nodes(8) {}
	PageList<node> nodes; // create nodes in pages, to reduce per-node overhead
	VList<node*> deleted;
	
private:
	node* CreateNode(const KTYPE key, const VTYPE & value) {
		if(deleted.Count() > 0) {
			node* n = deleted.PopLast();
			n->key = key;
			n->value = value;
			return n;
		}
		return nodes.GetAdd(node(key,value));//NEWMEM(node(key,value));
	}
public:
	void Add(const KTYPE key, const VTYPE & value) {
		node * n = CreateNode(key,value);
		if(root == NULL) {
			root = n;
		} else {
			root = insert(root, n);
		}
		count++;
	}
	void Remove(const KTYPE key) {
		node * found = find(root, key);
		if(found != NULL) {
			printf("found one to delete\n");
			found = delete_one_child(found);
			printf("deleted\n");
			deleted.Add(found);
			printf("marked\n");
		}
	}
	void Set(const KTYPE key, const VTYPE & value) {
		node * found = find(root, key);
		if(found != NULL) {
			found->value = value;
		} else {
			Add(key, value);
		}
	}
	/** this works if KTYPE is a pointer */
	VTYPE Get(const KTYPE & key) const {
		node * found = find(root, key);
		return (found != NULL) ? found->value : NULL;
	}
	/** this works if KTYPE is not a pointer */
	VTYPE* GetPtr(const KTYPE & key) const {
		node * found = find(root, key);
		return (found != NULL) ? &(found->value) : NULL;
	}
	struct KVP {
		KTYPE key; VTYPE value;
		KVP(){}
		KVP(KTYPE key, VTYPE value):key(key),value(value){}
	};
	void ToArray(KVP * list) {
		int index = 0;
		depthFirstMarkInOrder(root, list, &index);
	}
	int Count() const { return count; }
	~RBTree() {
		// if(root != NULL) { release(root); DELMEM(root); root = NULL; }
		nodes.Release();
		count = 0;
	}
private:
	node * root;
	int count;
	void depthFirstMarkInOrder(struct node* n, KVP * order, int * pindex) {
		if(n->left != LEAF) {
			depthFirstMarkInOrder(n->left, order, pindex);
		}
		order[*pindex] = KVP(n->key, n->value);
		(*pindex)++;
		if(n->right != LEAF) {
			depthFirstMarkInOrder(n->right, order, pindex);
		}
	}
	struct node* find(struct node* n, const KTYPE key) const {
		node* cursor;
		for (cursor = n;
			cursor != LEAF && cursor->key != key; 
			cursor = (cursor->key > key ? cursor->left : cursor->right));
		return cursor;
	}
	// void release(struct node* n) {
	// 	if(n->left != LEAF) {
	// 		release(n->left);
	// 		DELMEM(n->left);
	// 		n->left = LEAF;
	// 	}
	// 	if(n->right != LEAF) {
	// 		release(n->right);
	// 		DELMEM(n->right);
	// 		n->right = LEAF;
	// 	}
	// }

	// code is consistent with wikipedia: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
	struct node* parent(struct node* n) const {
		return n->parent; // NULL for root node
	}
	struct node* grandparent(struct node* n) const {
		struct node* p = parent(n);
		if (p == NULL) {
			return NULL; // No parent means no grandparent
		}
		return parent(p); // NULL if parent is root
	}
	struct node* sibling(struct node* n) const {
		struct node* p = parent(n);
		if (p == NULL) {
			return NULL; // No parent means no sibling
		}
		if (n == p->left) { // TODO ternary operator
			return p->right;
		} else {
			return p->left;
		}
	}
	struct node* uncle(struct node* n) const {
		struct node* p = parent(n);
		struct node* g = grandparent(n);
		if (g == NULL) {
			return NULL; // No grandparent means no uncle
		}
		return sibling(p);
	}
	char colorOf(struct node* n) {
		return (n != NULL) ? n->color : BLACK;
	}
	void rotate_left(struct node* n) {
		struct node* nnew = n->right;
		struct node* p = parent(n);
		assert(nnew != LEAF); // since the leaves of a red-black tree are empty, they cannot become internal nodes
		n->right = nnew->left; 
		nnew->left = n;
		n->parent = nnew;
		// handle other child/parent pointers
		if (n->right != NULL) {
			n->right->parent = n;
		}
		if (p != NULL) { // initially n could be the root
			if (n == p->left) {
				p->left = nnew;
			} else if (n == p->right) { // if (...) is excessive
				p->right = nnew;
			}
		}
		nnew->parent = p;
	}
	void rotate_right(struct node* n) {
		struct node* nnew = n->left;
		struct node* p = parent(n);
		assert(nnew != LEAF); // since the leaves of a red-black tree are empty, they cannot become internal nodes
		n->left = nnew->right;
		nnew->right = n;
		n->parent = nnew;
		// handle other child/parent pointers
		if (n->left != NULL) {
			n->left->parent = n;
		}
		if (p != NULL) { // initially n could be the root
			if (n == p->left) {
				p->left = nnew;
			} else if (n == p->right) { // if (...) is excessive
				p->right = nnew;
			}
		}
		nnew->parent = p;
	}

	void insert_basic(struct node* root, struct node* n) {
		if(n->key <= root->key) {
			if(root->left == LEAF){
				root->left = n;
				n->parent = root;
			} else {
				insert_basic(root->left, n);
			}
		} else {
			if(root->right == LEAF){
				root->right = n;
				n->parent = root;
			} else {
				insert_basic(root->right, n);
			}
		}
	}

	struct node *insert(struct node* root, struct node* n) {
		insert_recurse(root, n);// insert new node into tree
		insert_repair_tree(n); // repair violated red-black properties
		root = n; // find root, which may have shifted during balancing
		while (parent(root) != NULL) {
			root = parent(root);
		}
		return root;
	}
	void insert_recurse(struct node* root, struct node* n) {
		// recursively descend the tree until a leaf is found
		if (root != NULL && n->key < root->key) {
			if (root->left != LEAF) {
				insert_recurse(root->left, n);
				return;
			} else {
				root->left = n;
			}
		} else if (root != NULL) {
			if (root->right != LEAF){
				insert_recurse(root->right, n);
				return;
			} else {
				root->right = n;
			}
		}
		// insert new node n
		n->parent = root;
		n->left = LEAF;
		n->right = LEAF;
		n->color = RED;
	}
	void insert_repair_tree(struct node* n) {
		if (parent(n) == NULL) {
			insert_case1(n);
		} else if (parent(n)->color == BLACK) {
			insert_case2(n);
		} else if (uncle(n) != NULL && uncle(n)->color == RED) {
			insert_case3(n);
		} else {
			insert_case4(n);
		}
	}
	void insert_case1(struct node* n) {
		if (parent(n) == NULL) { n->color = BLACK; }
	}
	void insert_case2(struct node* n) {
		return; /* Do nothing since tree is still valid */
	}
	void insert_case3(struct node* n) {
		parent(n)->color = BLACK;
		uncle(n)->color = BLACK;
		grandparent(n)->color = RED;
		insert_repair_tree(grandparent(n));
	}
	void insert_case4(struct node* n) {
		struct node* p = parent(n);
		struct node* g = grandparent(n);
		if (n == p->right && p == g->left) {
			rotate_left(p);
			n = n->left;
		} else if (n == p->left && p == g->right) {
			rotate_right(p);
			n = n->right; 
		}
		insert_case4step2(n);
	}
	void insert_case4step2(struct node* n) {
		struct node* p = parent(n);
		struct node* g = grandparent(n);
		if (n == p->left) {
			rotate_right(g);
		} else {
			rotate_left(g);
		}
		p->color = BLACK;
		g->color = RED;
	}
	void replace_node(struct node* n, struct node* child) {
		printf("~A %d %d\n", child, n);
		child->parent = n->parent;
		printf("~B\n");
		if (n == n->parent->left) {
			printf("~C\n");
			n->parent->left = child;
			printf("~D\n");
		} else {
			printf("~E\n");
			n->parent->right = child;
			printf("~F\n");
		}
	}
	bool is_leaf(struct node* n) {
		return (n == LEAF); return true;
		//return n->left == LEAF && n->right == LEAF;
	}

	node* removeNode(node* t, KTYPE v) {
		node n = find(t, v), c;
		if (n == NULL) {
			return NULL;
		}
		if (n->left != NULL && n->right != NULL) {
			node* pred = n->left;
			while (pred->right != NULL)
				pred = pred->right;
			n->value = pred->value;
			n = pred;
		}
		c = n->right == NULL ? n->left : n->right;
		if (n->color == BLACK)
		{
			n->color = colorOf(c);
			removeUtil(n);
		}
		replaceNode(t, n, c);
		return n;
		//free(n);
	}
	void removeUtil(node n)
	{
		if (n->parent == NULL)
			return;

		node s = sibling(n);
		if (colorOf(s) == RED)
		{
			n->parent->color = RED;
			s->color = BLACK;
			if (n == n->parent->left)
				rotateLeft(n->parent);
			else
				rotateRight(n->parent);
		}

		s = sibling(n);
		if (colorOf(n->parent) == BLACK && colorOf(s) == BLACK &&
			colorOf(s->left) == BLACK && colorOf(s->right) == BLACK)
		{
			s->color = RED;
			removeUtil(n->parent);
		}
		else if (colorOf(n->parent) == RED && colorOf(s) == BLACK &&
			colorOf(s->left) == BLACK && colorOf(s->right) == BLACK)
		{
			s->color = RED;
			n->parent->color = BLACK;
		}
		else
		{
			if (n == n->parent->left && colorOf(s) == BLACK &&
				colorOf(s->left) == RED && colorOf(s->right) == BLACK)
			{
				s->color = RED;
				s->left->color = BLACK;
				rotateRight(s);
			}
			else if (n == n->parent->right && colorOf(s) == BLACK &&
				colorOf(s->right) == RED && colorOf(s->left) == BLACK)
			{
				s->color = RED;
				s->right->color = BLACK;
				rotateLeft(s);
			}

			s->color = colorOf(n->parent);
			n->parent->color = BLACK;
			if (n == n->parent->left)
			{
				s->right->color = BLACK;
				rotateLeft(n->parent);
			}
			else
			{
				s->left->color = BLACK;
				rotateRight(n->parent);
			}
		}
	}
	void replaceNode(node* t, node o, node n)
	{
		if (o->parent == NULL)
			*t = n;
		else
		{
			if (o == o->parent->left)
				o->parent->left = n;
			else
				o->parent->right = n;
		}

		if (n != NULL)
			n->parent = o->parent;
	}

	struct node* delete_one_child(struct node* n) {
		printf("A\n");
		// Precondition: n has at most one non-leaf child
		struct node* child = is_leaf(n->right) ? n->left : n->right;
		printf("B\n");
		if(child == LEAF) {
			struct node fakeChild;
			fakeChild.parent = n;
			child = &fakeChild;
			replace_node(n, child);
		} else {
			replace_node(n, child);
		}
		printf("C\n");
		if (n->color == BLACK) {
			if (child->color == RED) {
				child->color = BLACK;
			} else {
				delete_case1(child);
			}
		}
		//free(n);
	}
	void delete_case1(struct node* n) {
		if (n->parent != NULL) {
			delete_case2(n);
		}
	}
	void delete_case2(struct node* n) {
		struct node* s = sibling(n);
		if (s->color == RED) {
			n->parent->color = RED;
			s->color = BLACK;
			if (n == n->parent->left) {
				rotate_left(n->parent);
			} else {
				rotate_right(n->parent);
			}
		}
		delete_case3(n);
	}
	void delete_case3(struct node* n) {
		struct node* s = sibling(n);
		if ((n->parent->color == BLACK) &&
			(s->color == BLACK) &&
			(s->left->color == BLACK) &&
			(s->right->color == BLACK)) {
			s->color = RED;
			delete_case1(n->parent);
		} else {
			delete_case4(n);
		}
	}
	void delete_case4(struct node* n) {
		struct node* s = sibling(n);
		if ((n->parent->color == RED) &&
			(s->color == BLACK) &&
			(s->left->color == BLACK) &&
			(s->right->color == BLACK)) {
			s->color = RED;
			n->parent->color = BLACK;
		} else {
			delete_case5(n);
		}
	}
	void delete_case5(struct node* n) {
		struct node* s = sibling(n);
		if  (s->color == BLACK) { /* this if statement is trivial,
			due to case 2 (even though case 2 changed the sibling to a sibling's child,
			the sibling's child can't be red, since no red parent can have a red child). */
			/* the following statements just force the red to be on the left of the left of the parent,
			or right of the right, so case six will rotate correctly. */
			if ((n == n->parent->left) &&
				(s->right->color == BLACK) &&
				(s->left->color == RED)) { /* this last test is trivial too due to cases 2-4. */
				s->color = RED;
				s->left->color = BLACK;
				rotate_right(s);
			} else if ((n == n->parent->right) &&
						(s->left->color == BLACK) &&
						(s->right->color == RED)) {/* this last test is trivial too due to cases 2-4. */
				s->color = RED;
				s->right->color = BLACK;
				rotate_left(s);
			}
		}
		delete_case6(n);
	}
	void delete_case6(struct node* n) {
		struct node* s = sibling(n);
		s->color = n->parent->color;
		n->parent->color = BLACK;
		if (n == n->parent->left) {
			s->right->color = BLACK;
			rotate_left(n->parent);
		} else {
			s->left->color = BLACK;
			rotate_right(n->parent);
		}
	}
};
#undef LEAF
#undef assert