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

//template<class KEY, class VAL>
class RBT {
  public:
	// this RBTree Node has notably no parent pointer, and optionally will fold the red/black bit into the value left pointer.
	struct Node {
	  private:
		void* value;   // User provided, used indirectly via RBT_node_cmp_f.
	#define __HIDDENBIT 1
	#ifndef __HIDDENBIT
		int red; // Color red (1), black (0)
	  public:
		static bool isRed(const Node* self) { return (self->red); }
		static void setRed(Node* self, bool value) {self->red = value;}
		static void* getValue(const Node* self) { return (self->value); }
		static void setValue(Node* self, void* value) { self->value = value; }
	#else
	  public:
		static bool isRed(const Node* self) { return (((size_t)(self)->value) & 1); }
		static void setRed(Node* self, bool value) {
			if(value){self->value=(void*)(((size_t)self->value)|1);}
			else{self->value=(void*)(((size_t)self->value)&~((size_t)1));}
		}
		static void* getValue(const Node* self) {
			return ((void*)(((size_t)self->value) & ~1));
		}
		static void setValue(Node* self, void* value) {
			if(isRed(self)){(self)->value=(void*)(((size_t)(value))|1);}
			else{(self)->value=(void*)(((size_t)(value))&~((size_t)1));}
		}
	#endif
		Node* link[2]; // Link left [0] and right [1]
		static Node* alloc () { return (Node*)malloc (sizeof (Node)); }
		static Node* create (void* value) {
			return Node::init (Node::alloc (), value);
		}
		static Node* init (Node* self, void* value) {
			if (self) {
				setRed(self, 1);
				self->link[0] = self->link[1] = NULL;
				setValue(self, value);//self->value = value;
			}
			return self;
		}
		static void dealloc (Node* self) {
			if (self) {
				free (self);
			}
		}

	  public:
		// OOP convenience
		Node () : value (0) {
			#ifndef __HIDDENBIT
			setRed(0);
			#endif
			link[0] = link[1] = NULL;
		}
		void init (void* value) { init (this); }
		void dealloc () { dealloc (this); }

		bool  isRed () const { return isRed(this); }
		void  setRed (bool red) { setRed(this, red); }
		void* getValue() const { return getValue(this); }
		void  setValue(void* value) { setValue(this, value); }
	};
	struct iter {
		RBT* tree;
		Node* n;                        // Current node
		Node* path[RB_ITER_MAX_HEIGHT]; // Traversal path. kept track of because there's no parent node
		size_t top;                     // Top of stack
		void* info;                     // User provided, not used by rb_iter.

		static void* first (iter* self) {
			return start (self, 0);
		}

		static void* last (iter* self) {
			return start (self, 1);
		}

		static void* next (iter* self) {
			return move (self, 1);
		}

		static void* prev (iter* self) {
			return move (self, 0);
		}

	  private:
		/**
		* Internal function, init traversal object, dir determines whether
		* to begin traversal at the smallest or largest valued node.
		*/
		static void* start (iter* self, int dir) {
			void* result = NULL;
			if (self) {
				self->n = self->tree->root;
				self->top = 0;
				// Save the path for later selfersal
				if (self->n != NULL) {
					while (self->n->link[dir] != NULL) {
						self->path[self->top++] = self->n;
						self->n = self->n->link[dir];
					}
				}
				result = self->n == NULL ? NULL : Node::getValue(self->n);//self->n->value;
			}
			return result;
		}

		// Traverse a red black tree in the user-specified direction (0 asc, 1 desc)
		static void* move (iter* self, int dir) {
			if (self->n->link[dir] != NULL) {
				// Continue down this branch
				self->path[self->top++] = self->n;
				self->n = self->n->link[dir];
				while (self->n->link[!dir] != NULL) {
					self->path[self->top++] = self->n;
					self->n = self->n->link[!dir];
				}
			} else {
				// Move to the next branch
				Node* last = NULL;
				do {
					if (self->top == 0) {
						self->n = NULL;
						break;
					}
					last = self->n;
					self->n = self->path[--self->top];
				} while (last == self->n->link[dir]);
			}
			return self->n == NULL ? NULL : Node::getValue(self->n);//self->n->value;
		}

	  public:
		// OOP convenience
		iter () : tree (NULL), n (NULL), info (NULL) {}
		iter (RBT* tree) : tree (tree), n (NULL), info (NULL) {}
		static iter* alloc () {
			return (iter*)malloc (sizeof (iter));
		}

		static iter* init (iter* self, RBT* tree) {
			if (self) {
				self->tree = tree;
				self->n = NULL;
				self->top = 0;
			}
			return self;
		}

		static iter* create (RBT* tree) {
			return init (alloc (), tree);
		}

		static void dealloc (iter* self) {
			if (self) {
				free (self);
			}
		}

		void init (RBT* tree) { init (this, tree); }
		void dealloc () { dealloc (this); }
		void* first () { return first (this); }
		void* last () { return last (this); }
		void* next () { return next (this); }
		void* prev () { return prev (this); }
	};

	typedef int (*node_cmp_f) (RBT* self, Node* a, Node* b);
	typedef void (*node_f) (RBT* self, Node* node);
	Node* root;
	node_cmp_f cmp;
	size_t _size;
	void* info; // User provided, not used by RBT.

	static int node_cmp_ptr_cb (RBT* self, Node* a, Node* b) {
		void* va = a->getValue(), * vb = b->getValue();
		return (va > vb) - (va < vb);
	}

	static void node_dealloc_cb (RBT* self, Node* node) {
		if (self && node) {
			Node::dealloc (node);
		}
	}

	static RBT* alloc () { return (RBT*)malloc (sizeof (RBT)); }

	static RBT* init (RBT* self, node_cmp_f node_cmp_cb) {
		if (self) {
			self->root = NULL;
			self->_size = 0;
			self->cmp = node_cmp_cb ? node_cmp_cb : node_cmp_ptr_cb;
		}
		return self;
	}

	static RBT* create (node_cmp_f node_cb) {
		return init (alloc (), node_cb);
	}

	static void dealloc (RBT* self, node_f node_cb) {
		if (self) {
			if (node_cb) {
				Node* that = self->root;
				Node* save = NULL;
				// Rotate away the left links so that we can treat this like the destruction of a linked list
				while (that) {
					if (that->link[0] == NULL) { // No left links, just kill that node and move on
						save = that->link[1];
						node_cb (self, that);
						that = NULL;
					} else { // Rotate away the left link and check again
						save = that->link[0];
						that->link[0] = save->link[1];
						save->link[1] = that;
					}
					that = save;
				}
			}
			free (self);
		}
	}

	static void* find (RBT* self, void* value) {
		void* result = NULL;
		if (self) {
			Node what;
			Node::setValue(&what, value);//what.value = value;
			Node* it = self->root;
			int cmp = 0;
			while (it) {
				if ((cmp = self->cmp (self, it, &what))) {
					// If tree supports duplicates, they should be chained to the right subtree for this to work
					it = it->link[cmp < 0];
				} else {
					break;
				}
			}
			result = it ? Node::getValue(it) //it->value
				: NULL;
		}
		return result;
	}

	// Creates (malloc'ates)
	static int insert (RBT* self, void* value) {
		return insert_node (self, Node::create (value));
	}

	/** Returns 1 on success, 0 otherwise. */
	static int insert_node (RBT* self, Node* what) {
		int result = 0;
		if (self && what) {
			if (self->root == NULL) {
				self->root = what;
				result = 1;
			} else {
				Node head;   // False tree root
				Node *g, *t; // Grandparent & parent
				Node *p, *q; // Iterator & parent
				int dir = 0, last = 0;
				// Set up our helpers
				t = &head;
				g = p = NULL;
				q = t->link[1] = self->root;
				// Search down the tree for a place to insert
				while (1) {
					if (q == NULL) {
						// Insert node at the first null link.
						p->link[dir] = q = what;
					} else if (rb_node_is_red (q->link[0]) && rb_node_is_red (q->link[1])) {
						// Simple red violation: color flip
						q->setRed (1);
						q->link[0]->setRed (0);
						q->link[1]->setRed (0);
					}
					if (rb_node_is_red (q) && rb_node_is_red (p)) {
						// Hard red violation: rotations necessary
						int dir2 = t->link[1] == g;
						if (q == p->link[last]) {
							t->link[dir2] = rb_node_rotate (g, !last);
						} else {
							t->link[dir2] = rb_node_rotate2 (g, !last);
						}
					}
					// Stop working if we inserted a node. This check also disallows duplicates in the tree
					if (self->cmp (self, q, what) == 0) {
						break;
					}
					last = dir;
					dir = self->cmp (self, q, what) < 0;
					// Move the helpers down
					if (g != NULL) {
						t = g;
					}
					g = p, p = q;
					q = q->link[dir];
				}
				// Update the root (it may be different)
				self->root = (head.link[1]);
			}
			// Make the root black for simplified logic
			self->root->setRed (0);
			++self->_size;
		}
		return 1;
	}
	static int remove (RBT* self, void* value) {
		int result = 0;
		if (self) {
			result = remove_with_cb (self, value, node_dealloc_cb);
		}
		return result;
	}

	static size_t size (RBT* self) {
		size_t result = 0;
		if (self) {
			result = self->_size;
		}
		return result;
	}

	/**
	* Returns 1 if the value was removed, 0 otherwise. Optional node callback
	* can be provided to dealloc node and/or user data. Use RBT_node_dealloc
	* default callback to deallocate node created by RBT_insert(...).
	*/
	static int remove_with_cb (RBT* self, void* value, node_f node_cb) {
		if (self->root != NULL) {
			Node head; // False tree root
			Node what; // Value wrapper node
			Node::setValue(&what,value);//what.value = value;
			Node *q, *p, *g; // Helpers
			Node* f = NULL;  // Found item
			int dir = 1;
			// Set up our helpers
			q = &head;
			g = p = NULL;
			q->link[1] = self->root;
			// Search and push a red node down to fix red violations as we go
			while (q->link[dir] != NULL) {
				int last = dir;
				// Move the helpers down
				g = p, p = q;
				q = q->link[dir];
				dir = self->cmp (self, q, &what) < 0;
				// Save the node with matching value and keep going; we'll do removal tasks at the end
				if (self->cmp (self, q, &what) == 0) {
					f = q;
				}
				// Push the red node down with rotations and color flips
				if (!rb_node_is_red (q) && !rb_node_is_red (q->link[dir])) {
					if (rb_node_is_red (q->link[!dir])) {
						p = p->link[last] = rb_node_rotate (q, dir);
					} else if (!rb_node_is_red (q->link[!dir])) {
						Node* s = p->link[!last];
						if (s) {
							if (!rb_node_is_red (s->link[!last]) && !rb_node_is_red (s->link[last])) {
								// Color flip
								p->setRed (0);
								s->setRed (1);
								q->setRed (1);
							} else {
								int dir2 = g->link[1] == p;
								if (rb_node_is_red (s->link[last])) {
									g->link[dir2] = rb_node_rotate2 (p, last);
								} else if (rb_node_is_red (s->link[!last])) {
									g->link[dir2] = rb_node_rotate (p, last);
								}
								// Ensure correct coloring
								q->setRed (1);
								g->link[dir2]->setRed (1);
								g->link[dir2]->link[0]->setRed (0);
								g->link[dir2]->link[1]->setRed (0);
							}
						}
					}
				}
			}
			// Replace and remove the saved node
			if (f) {
				void* tmp = Node::getValue(f);//f->value;
				Node::setValue(f, Node::getValue(q));//f->value = q->value;
				Node::setValue(q, tmp);//q->value = tmp;
				p->link[p->link[1] == q] = q->link[q->link[0] == NULL];
				if (node_cb) {
					node_cb (self, q);
				}
				q = NULL;
			}
			// Update the root (it may be different)
			self->root = head.link[1];
			// Make the root black for simplified logic
			if (self->root != NULL) {
				self->root->setRed (1);
			}
			--self->_size;
		}
		return 1;
	}

  private:
	static Node* rb_node_rotate (Node* self, int dir) {
		Node* result = NULL;
		if (self) {
			result = self->link[!dir];
			self->link[!dir] = result->link[dir];
			result->link[dir] = self;
			self->setRed (1);
			result->setRed (0);
		}
		return result;
	}

	static Node* rb_node_rotate2 (Node* self, int dir) {
		Node* result = NULL;
		if (self) {
			self->link[!dir] = rb_node_rotate (self->link[!dir], !dir);
			result = rb_node_rotate (self, dir);
		}
		return result;
	}
	/** won't just use ->isRed() because NULL nodes must always count as black. */
	static int rb_node_is_red (const Node* self) {
		return self ? self->isRed () : 0;
	}
	static int test (RBT* self, Node* root) {
		int lh, rh;
		if (root == NULL)
			return 1;
		else {
			Node* ln = root->link[0];
			Node* rn = root->link[1];
			// Consecutive red links
			if (root->isRed ()) {
				if (ln->isRed () || rn->isRed ()) {
					printf ("Red violation");
					return 0;
				}
			}
			lh = test (self, ln);
			rh = test (self, rn);
			// Invalid binary search tree
			if ((ln != NULL && self->cmp (self, ln, root) >= 0) || (rn != NULL && self->cmp (self, rn, root) <= 0)) {
				puts ("Binary tree violation");
				return 0;
			}
			// Black height mismatch
			if (lh != 0 && rh != 0 && lh != rh) {
				puts ("Black violation");
				return 0;
			}
			// Only count black links
			if (lh != 0 && rh != 0)
				return rb_node_is_red (root) ? lh : lh + 1;
			else
				return 0;
		}
	}

  public:
	// OOP convenience
	void* find (void* value) { return find (this, value); }
	int insert (void* value) { return insert (this, value); }
	int remove (void* value) { return remove (this, value); }
	size_t size () { return size (this); }
	iter* createIter () { return iter::create (this); }
};