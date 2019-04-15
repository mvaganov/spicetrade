//
// Based on Julienne Walker's <http://eternallyconfuzzled.com/> RBTree
// implementation.
//
// Modified by Mirek Rusin <http://github.com/mirek/RBTree>.
// Modified by Michael Vaganov
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
//

#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifndef RB_ITER_MAX_HEIGHT
#	define RB_ITER_MAX_HEIGHT 64 // Tallest allowable tree to iterate
#endif

// template<typename K, typename V>
class RBT {
  public:
	struct node {
	  private:
		int red; // Color red (1), black (0)
	  public:
		node *link[2]; // Link left [0] and right [1]
		void *value;   // User provided, used indirectly via RBT_node_cmp_f.
		static node *alloc () { return (RBT::node *)malloc (sizeof (RBT::node)); }
		static node *create (void *value);
		static node *init (node *self, void *value) {
			if (self) {
				self->setRed (1);
				self->link[0] = self->link[1] = NULL;
				self->value = value;
			}
			return self;
		}
		static void dealloc (node *self);

		// OOP convenience
		node () : red (0), value (0) { link[0] = link[1] = NULL; }
		void init (void *value) { init (this); }
		void dealloc () { dealloc (this); }

		void setRed (bool red) { this->red = red; }
		bool isRed () const { return this->red; }
	};
	struct iter {
		RBT *tree;
		node *n;                        // Current node
		node *path[RB_ITER_MAX_HEIGHT]; // Traversal path
		size_t top;                     // Top of stack
		void *info;                     // User provided, not used by rb_iter.

		static iter *alloc ();
		static iter *init (iter *self, RBT *tree);
		static iter *create (RBT *tree);
		static void dealloc (iter *self);
		static void *first (iter *self);
		static void *last (iter *self);
		static void *next (iter *self);
		static void *prev (iter *self);

	  private:
		static void *start (iter *self, int dir);
		static void *move (iter *self, int dir);

	  public:
		// OOP convenience
		iter () : tree (NULL), n (NULL), info (NULL) {}
		iter (RBT *tree) : tree (tree), n (NULL), info (NULL) {}
		void init (RBT *tree) { init (this, tree); }
		void dealloc () { dealloc (this); }
		void *first () { return first (this); }
		void *last () { return last (this); }
		void *next () { return next (this); }
		void *prev () { return prev (this); }
	};

	typedef int (*node_cmp_f) (RBT *self, node *a, node *b);
	typedef void (*node_f) (RBT *self, node *node);
	node *root;
	node_cmp_f cmp;
	size_t _size;
	void *info; // User provided, not used by RBT.

	static int node_cmp_ptr_cb (RBT *self, node *a, node *b);
	static void node_dealloc_cb (RBT *self, node *node);

	static RBT *alloc ();
	static RBT *create (RBT::node_cmp_f cmp);
	static RBT *init (RBT *self, node_cmp_f cmp);
	static void dealloc (RBT *self, node_f node_cb);
	static void *find (RBT *self, void *value);
	static int insert (RBT *self, void *value);
	static int remove (RBT *self, void *value);
	static size_t size (RBT *self);

	static int insert_node (RBT *self, node *node);
	static int remove_with_cb (RBT *self, void *value, node_f node_cb);

  private:
	static int test (RBT *self, node *root);

  public:
	// OOP convenience
	void *find (void *value) { return find (this, value); }
	int insert (void *value) { return insert (this, value); }
	int remove (void *value) { return remove (this, value); }
	size_t size () { return size (this); }
	iter *createIter () { return iter::create (this); }
};