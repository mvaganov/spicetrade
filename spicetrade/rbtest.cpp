// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #if defined(_WIN32) && defined(_MSC_VER)
// #pragma warning (disable : 4996)
// #endif

// enum { RED, BLACK };

// typedef int COLOR;
// typedef int valueType;
// struct Node {
// 	valueType value;
// 	COLOR color;
// 	struct Node *right, *left, *parent;
// };
// typedef Node** tree;

// Node* initilize(Node*, valueType);
// Node* grandparent(Node*);
// Node* uncle(Node*);
// Node* sibling(Node*);

// Node* findNode(tree, valueType);

// void insertNode(tree, valueType);
// void insertUtil(Node*);

// void removeNode(tree, valueType);
// void removeUtil(Node*);

// void rotateRight(Node*);
// void rotateLeft(Node*);

// Node* findNode(tree, valueType);
// void replaceNode(tree, Node*, Node*);

// void printNode(Node* n);
// void inorderIterator(Node* n, void(*func)(Node*));
// void inorderPrint(tree t);

// void destroy(Node*);

// Node* initilize(Node* p, valueType v)
// {
// 	Node* tree = (Node*)malloc(sizeof(struct Node));
// 	tree->left = tree->right = NULL;
// 	tree->parent = p;
// 	tree->value = v;
// 	tree->color = RED;
// 	return tree;
// }
// Node* grandparent(Node* n)
// {
// 	if (n == NULL || n->parent == NULL)
// 		return NULL;
// 	return n->parent->parent;
// }
// Node* uncle(Node* n)
// {
// 	Node* g = grandparent(n);
// 	if (n == NULL || g == NULL)
// 		return NULL;
// 	if (n->parent == g->left)
// 		return g->right;
// 	else
// 		return g->left;
// }
// Node* sibling(Node* n)
// {
// 	if (n == n->parent->left)
// 		return n->parent->right;
// 	else
// 		return n->parent->left;
// }
// int colorOf(Node* n)
// {
// 	return n == NULL ? BLACK : n->color;
// }
// void insertNode(tree t, valueType v)
// {
// 	int pl = 0;
// 	Node* ptr, *btr = NULL, *newNode;

// 	for (ptr = *t; ptr != NULL;
// 	btr = ptr, ptr = ((pl = (ptr->value > v)) ? ptr->left : ptr->right));

// 		newNode = initilize(btr, v);

// 	if (btr != NULL)
// 		(pl) ? (btr->left = newNode) : (btr->right = newNode);

// 	insertUtil(newNode);

// 	ptr = newNode;
// 	for (ptr = newNode; ptr != NULL; btr = ptr, ptr = ptr->parent);
// 	*t = btr;
// }
// void insertUtil(Node* n)
// {
// 	Node* u = uncle(n), *g = grandparent(n), *p = n->parent;
// 	if (p == NULL)
// 		n->color = BLACK;
// 	else if (p->color == BLACK)
// 		return;
// 	else if (u != NULL && u->color == RED)
// 	{
// 		p->color = BLACK;
// 		u->color = BLACK;
// 		g->color = RED;

// 		insertUtil(g);
// 	}
// 	else
// 	{
// 		if (n == p->right && p == g->left)
// 		{
// 			rotateLeft(p);
// 			n = n->left;
// 		}
// 		else if (n == p->left && p == g->right)
// 		{
// 			rotateRight(p);
// 			n = n->right;
// 		}

// 		g = grandparent(n);
// 		p = n->parent;

// 		p->color = BLACK;
// 		g->color = RED;

// 		if (n == p->left)
// 			rotateRight(g);
// 		else
// 			rotateLeft(g);
// 	}
// }
// void replaceNode(tree t, Node* o, Node* n)
// {
// 	if (o->parent == NULL)
// 		*t = n;
// 	else
// 	{
// 		if (o == o->parent->left)
// 			o->parent->left = n;
// 		else
// 			o->parent->right = n;
// 	}

// 	if (n != NULL)
// 		n->parent = o->parent;
// }
// void removeNode(tree t, valueType v)
// {
// 	Node* n = findNode(t, v), *c;

// 	if (n == NULL)
// 		return;
// 	if (n->left != NULL && n->right != NULL)
// 	{
// 		Node* pred = n->left;
// 		while (pred->right != NULL)
// 			pred = pred->right;
// 		n->value = pred->value;
// 		n = pred;
// 	}

// 	c = n->right == NULL ? n->left : n->right;
// 	if (n->color == BLACK)
// 	{
// 		n->color = colorOf(c);
// 		removeUtil(n);
// 	}

// 	replaceNode(t, n, c);
// 	free(n);
// }
// void removeUtil(Node* n)
// {
// 	if (n->parent == NULL)
// 		return;

// 	Node* s = sibling(n);
// 	if (colorOf(s) == RED)
// 	{
// 		n->parent->color = RED;
// 		s->color = BLACK;
// 		if (n == n->parent->left)
// 			rotateLeft(n->parent);
// 		else
// 			rotateRight(n->parent);
// 	}

// 	s = sibling(n);
// 	if (colorOf(n->parent) == BLACK && colorOf(s) == BLACK &&
// 		colorOf(s->left) == BLACK && colorOf(s->right) == BLACK)
// 	{
// 		s->color = RED;
// 		removeUtil(n->parent);
// 	}
// 	else if (colorOf(n->parent) == RED && colorOf(s) == BLACK &&
// 		colorOf(s->left) == BLACK && colorOf(s->right) == BLACK)
// 	{
// 		s->color = RED;
// 		n->parent->color = BLACK;
// 	}
// 	else
// 	{
// 		if (n == n->parent->left && colorOf(s) == BLACK &&
// 			colorOf(s->left) == RED && colorOf(s->right) == BLACK)
// 		{
// 			s->color = RED;
// 			s->left->color = BLACK;
// 			rotateRight(s);
// 		}
// 		else if (n == n->parent->right && colorOf(s) == BLACK &&
// 			colorOf(s->right) == RED && colorOf(s->left) == BLACK)
// 		{
// 			s->color = RED;
// 			s->right->color = BLACK;
// 			rotateLeft(s);
// 		}

// 		s->color = colorOf(n->parent);
// 		n->parent->color = BLACK;
// 		if (n == n->parent->left)
// 		{
// 			s->right->color = BLACK;
// 			rotateLeft(n->parent);
// 		}
// 		else
// 		{
// 			s->left->color = BLACK;
// 			rotateRight(n->parent);
// 		}
// 	}
// }

// void rotateRight(Node* tree)
// {
// 	Node* c = tree->left;
// 	Node* p = tree->parent;

// 	if (c->right != NULL)
// 		c->right->parent = tree;

// 	tree->left = c->right;
// 	tree->parent = c;
// 	c->right = tree;
// 	c->parent = p;

// 	if (p != NULL)
// 	{
// 		if (p->right == tree)
// 			p->right = c;
// 		else
// 			p->left = c;
// 	}
// 	printf("rotation %d, right\n", tree->value);
// }
// void rotateLeft(Node* tree)
// {
// 	Node* c = tree->right;
// 	Node* p = tree->parent;

// 	if (c->left != NULL)
// 		c->left->parent = tree;

// 	tree->right = c->left;
// 	tree->parent = c;
// 	c->left = tree;
// 	c->parent = p;

// 	if (p != NULL)
// 	{
// 		if (p->left == tree)
// 			p->left = c;
// 		else
// 			p->right = c;
// 	}
// 	printf("rotation %d, left\n", tree->value);
// }

// Node* findNode(tree t, valueType v)
// {
// 	Node* p;
// 	for (p = *t; p != NULL && p->value != v; p = (p->value > v ? p->left : p->right));
// 	return p;
// }
// void printNode(Node* n)
// {
// 	printf("%d(%s) ", n->value, (n->color == BLACK ? "b" : "r"));
// }
// void inorderIterator(Node* n, void(*func)(Node*))
// {
// 	if (n == NULL)
// 		return;
// 	inorderIterator(n->left, func);
// 	func(n);
// 	inorderIterator(n->right, func);
// }
// void inorderPrint(tree t)
// {
// 	inorderIterator(*t, printNode);
// 	printf("\n");
// }
// void destroy(Node* tree)
// {
// 	if (tree == NULL)
// 		return;
// 	destroy(tree->left);
// 	destroy(tree->right);
// 	free(tree);
// }
#include "list.h"
#include "rb_tree.h"
#include <string.h>
#include <string>
#include <vector>
#include <stdio.h>

struct KVP {
	std::string key;
	int value;
	KVP(std::string key, int value):key(key),value(value){}
	KVP(){}
};

int RBNodeCompare_KVP(struct rb_tree *self, struct rb_node *a, struct rb_node *b) {
	KVP* ka = (KVP*)a->value;
	KVP* kb = (KVP*)b->value;
	if(ka->key < kb->key) return -1;
	if(ka->key > kb->key) return 1;
	return 0;
}
void RBNodeDelete_KVP(struct rb_tree *self, struct rb_node *node){
	printf("need to delete %s\n", ((KVP*)node->value)->key.c_str());
}
int main(int argc, char * argv[])
{
	FILE * fp;
	fp = fopen("input.txt", "r");
	const int BUFF_SIZE = 1024;
	char buff[BUFF_SIZE];
	char* result;
	const char* whitespace = " \t\n\r\b";
	struct rb_tree *tree = rb_tree_create(RBNodeCompare_KVP);
	do{
		result = fgets(buff, BUFF_SIZE, fp);
		int start = 0, end = 0;
		int index = 0;
		while(buff[index] && buff[index] != '\n') {
			char find = buff[index];
			const char* foundWhitespace = strchr(whitespace, find);
			if(foundWhitespace) {
				end = index;
				if(start != end){
					std::string s(buff+start, end-start);
					KVP search(s, 0);
					struct KVP *f = (KVP*)rb_tree_find(tree, &search);
					if(f != NULL) {
						f->value++;
					} else {
						KVP * kvp = NEWMEM(KVP(s, 1));
						rb_tree_insert(tree, kvp);
					}
					// putchar('\"');
					// for(int i=start; i<end; ++i){
					// 	putchar(buff[i]);
					// }
					// putchar('\"');
				}
				start = end+1;
			}
			index++;
		}
	}while(result);
	fclose(fp);

	VList<std::string> toDelete;
	//std::vector<std::string> toDelete;
	struct rb_iter *iter = rb_iter_create();
	if (iter) {
		for (struct KVP *v = (KVP*)rb_iter_last(iter, tree); v; v = (KVP*)rb_iter_prev(iter)) {
			printf("%s : %d\n", v->key.c_str(), v->value);
			if(v->value == 1) {
				//toDelete.push_back(v->key);
				toDelete.Add(v->key);
			}
		}
		rb_iter_dealloc(iter);
	}
	for(int i = 0; i < toDelete.Count(); ++i) {
		KVP kvp(toDelete[i], 0);
		rb_tree_remove_with_cb(tree, &kvp, RBNodeDelete_KVP);
	}
	iter = rb_iter_create();
	if (iter) {
		for (struct KVP *v = (KVP*)rb_iter_last(iter, tree); v; v = (KVP*)rb_iter_prev(iter)) {
			printf("%s : %d\n", v->key.c_str(), v->value);
		}
		rb_iter_dealloc(iter);
	}
	rb_tree_dealloc(tree, NULL);
	return 0;
}