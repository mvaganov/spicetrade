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
#include "platform_conio.h"
#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifndef RB_ITER_MAX_HEIGHT
#	define RB_ITER_MAX_HEIGHT 64 // Tallest allowable tree to iterate
#endif

#define __VALUE_SELF	0
#define __VALUE_PTR		1
#define __VALUE	__VALUE_PTR

//template<class KEY, class VAL>
class RBT {
  public:
	// this RBTree Node has notably no parent pointer, and optionally will fold the red/black bit into the value left pointer.
	struct Node {
	  //private:
		Node* _link[2]; // Link left [0] and right [1]

#if __VALUE == __VALUE_PTR
		void* value;    // User provided, used indirectly via RBT_node_cmp_f.
#endif

// give the red/black mark a whole variable. this increases the size of the node by one alignment-width (sizeof(size_t))
#define __RB_VAR 0
// hide the red/black mark in the first link. this slows down all link traversal operations, and requires tree nodes to be aligned to even addresses.
#define __RB_LINK 1
// hide the red/black mark in the value. this requires that the value be a pointer :(
#define __RB_VALUE 2
#define __BIT_IN __RB_VAR

#if __BIT_IN == __RB_LINK
	#define FLAG_LINK	1
	  public:
		Node* link (const int whichLink) { return (Node*)((size_t)(_link[whichLink]) & (~((size_t)1))); }
		static void setlink(Node* &ptrToSet, Node* valueToGiveIt) {
			ptrToSet = (Node*)((size_t)valueToGiveIt | (((size_t)ptrToSet) & 1));
		}
		void setlink (const int whichLink, Node* n) {
			if (whichLink == FLAG_LINK) {
				//_link[FLAG_LINK] = (Node*)((size_t)n | (((size_t)_link[FLAG_LINK]) & 1));
				setlink(_link[FLAG_LINK], n);
			} else {
				_link[whichLink] = n;
			}
		}
		static bool isRed (const Node* self) { return (((size_t)self->_link[FLAG_LINK]) & 1); }
		static void setRed (Node* self, bool value) {
			self->_link[FLAG_LINK] = (Node*)(((size_t)self->_link[FLAG_LINK]) | isRed (self));
		}
		#if __VALUE == __VALUE_PTR
		static void* getValue (const Node* self) { return (self->value); }
		static void setValue (Node* self, void* value) { self->value = value; }
		#elif __VALUE == __VALUE_SELF
		static void* getValue (const Node* self) { return (void*)self; }
		static void setValue (Node* self, void* value) { }
		#endif
#elif __BIT_IN == __RB_VALUE
	  public:
		Node* link (const int right) { return _link[right]; }
		static void setlink(Node* &ptrToSet, Node* valueToGiveIt) { ptrToSet = valueToGiveIt; }
		void setlink (const int right, Node* n) { _link[right] = n; }
		static bool isRed (const Node* self) { return (((size_t) (self)->value) & 1); }
		static void setRed (Node* self, bool value) {
			if (value) {
				self->value = (void*)(((size_t)self->value) | 1);
			} else {
				self->value = (void*)(((size_t)self->value) & ~((size_t)1));
			}
		}
		static void* getValue (const Node* self) {
			return ((void*)(((size_t)self->value) & ~1));
		}
		static void setValue (Node* self, void* value) {
			if (isRed (self)) {
				(self)->value = (void*)(((size_t) (value)) | 1);
			} else {
				(self)->value = (void*)(((size_t) (value)) & ~((size_t)1));
			}
		}
#elif __BIT_IN == __RB_VAR
		int red; // Color red (1), black (0)
	  public:
		Node* link (const int right) { return _link[right]; }
		static void setlink(Node* &ptrToSet, Node* valueToGiveIt) { ptrToSet = valueToGiveIt; }
		void setlink (const int right, Node* n) { _link[right] = n; }
		static bool isRed (const Node* self) { return (self->red); }
		static void setRed (Node* self, bool value) { self->red = value; }
		static void* getValue (const Node* self) { return (self->value); }
		static void setValue (Node* self, void* value) { self->value = value; }
#endif
		static Node* alloc () { return (Node*)malloc (sizeof (Node)); }
		static Node* create (void* value) {
			return Node::init (Node::alloc (), value);
		}
		static Node* init (Node* self, void* value) {
			if (self) {
				setRed (self, 1);
				self->setlink (0, NULL); self->setlink (1, NULL);
				setValue (self, value);
			}
			return self;
		}
		static void dealloc (Node* self) {
			if (self) {
				free (self);
			}
		}
		static Node*& _getRawLink(Node* n, int dir) { return n->_link[dir]; }

	  public:
		// OOP convenience
		Node () {
			#if __BIT_IN == __RB_VALUE
			setRed (0);
			#endif
			#if __VALUE == __VALUE_PTR
			value = NULL;
			#endif
			setlink (0, NULL); setlink (1, NULL);
		}
		void init (void* value) { Node::init (this, value); }
		void init () { Node::init (this, NULL); }
		void dealloc () { dealloc (this); }

		bool isRed () const { return isRed (this); }
		void setRed (bool red) { setRed (this, red); }
		void* getValue () const { return getValue (this); }
		void setValue (void* value) { setValue (this, value); }
	};
	struct iter {
		RBT* tree;
		Node* n;                        // Current node
		Node* path[RB_ITER_MAX_HEIGHT]; // Traversal path. kept track of because there's no parent node
		size_t top;                     // Top of stack
		void* info;                     // User provided, not used by rb_iter.
		static void* first(iter* self) { return start (self, 0); }
		static void* last (iter* self) { return start (self, 1); }
		static void* next (iter* self) { return move (self, 1); }
		static void* prev (iter* self) { return move (self, 0); }

	  private:
		/**
		* Internal function, init traversal object, dir determines whether
		* to begin traversal at the smallest or largest valued node.
		*/
		static void* start (iter* self, int dir) {
			if(!self) return NULL;
			void* result = NULL;
			self->n = self->tree->root; // start at the root
			self->top = 0;
			if (self->n != NULL) {
				while (self->n->link (dir) != NULL) {
					self->path[self->top++] = self->n; // keep track of traversal in a list (so a parent ptr isnt needed)
					self->n = self->n->link (dir); // move along the given direction
				}
			}
			result = self->n == NULL ? NULL : Node::getValue (self->n);
			return result;
		}

		/** Traverse the tree in the user-specified direction (0 asc, 1 desc) */
		static void* move (iter* self, int dir) {
			if (self->n->link (dir) != NULL) { // travel in the given direction if possible
				self->path[self->top++] = self->n;
				self->n = self->n->link (dir);
				while (self->n->link (!dir) != NULL) {
					self->path[self->top++] = self->n;
					self->n = self->n->link (!dir);
				}
			} else {
				// it the specified direction isn't possible, go backwards up the tree
				Node* last = NULL;
				do {
					if (self->top == 0) {
						self->n = NULL;
						break;
					}
					last = self->n;
					self->n = self->path[--self->top];
				} while (last == self->n->link (dir));
			}
			return self->n == NULL ? NULL : Node::getValue (self->n); //self->n->value;
		}

	  public:
		// OOP convenience
		iter () : tree (NULL), n (NULL), info (NULL) {}
		iter (RBT* tree) : tree (tree), n (NULL), info (NULL) {}
		static iter* alloc () { return (iter*)malloc (sizeof (iter)); }

		static iter* init (iter* self, RBT* tree) {
			if (self) {
				self->tree = tree;
				self->n = NULL;
				self->top = 0;
			}
			return self;
		}

		static iter* create (RBT* tree) { return init (alloc (), tree); }

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

	Node* root;

#if __VALUE == __VALUE_PTR
	typedef int (*node_cmp_f) (RBT* self, void* value_a, void* value_b);
	typedef void (*node_f) (RBT* self, Node* node);
	node_cmp_f cmp;
#endif

	size_t _size;
	void* info; // User provided, not used by RBT.

	static int node_cmp_ptr_cb (RBT* self, void* value_a, void* value_b) {
		return (value_a > value_b) - (value_a < value_b);
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
					if (that->link (0) == NULL) { // No left links, just kill that node and move on
						save = that->link (1);
						node_cb (self, that);
						that = NULL;
					} else { // Rotate away the left link and check again
						save = that->link (0);
						that->setlink (0, save->link (1));
						save->setlink (1, that);
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
			Node* it = self->root;
			int cmp = 0;
			while (it) {
				if ((cmp = self->cmp (self, it->getValue(), value))) {
					// If tree supports duplicates, they should be chained to the right subtree for this to work
					it = it->link (cmp < 0);
				} else {
					break;
				}
			}
			result = it ? Node::getValue (it) //it->value
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
				t->setlink (1, self->root);
				q = self->root;
				// Search down the tree for a place to insert
				while (1) {
					if (q == NULL) {
						// Insert node at the first null link.
						p->setlink (dir, what);
						q = what;
						result = 1;
					} else if (rb_node_is_red (q->link (0)) && rb_node_is_red (q->link (1))) {
						// Simple red violation: color flip
						q->setRed (1);
						q->link (0)->setRed (0);
						q->link (1)->setRed (0);
					}
					if (rb_node_is_red (q) && rb_node_is_red (p)) {
						// Hard red violation: rotations necessary
						int dir2 = t->link (1) == g;
						if (q == p->link (last)) {
							t->setlink (dir2, rb_node_rotate (g, !last));
						} else {
							t->setlink (dir2, rb_node_rotate2 (g, !last));
						}
					}
					// Stop working if we inserted a node
					if (result == 1 && self->cmp (self, q->getValue(), what->getValue()) == 0) {
						break;
					}
					last = dir;
					dir = self->cmp (self, q->getValue(), what->getValue()) < 0;
					// Move the helpers down
					if (g != NULL) {
						t = g;
					}
					g = p, p = q;
					q = q->link (dir);
				}
				// Update the root (it may be different)
				self->root = (head.link (1));
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
			Node head;                     // False tree root... why is this not a **?
			Node *cursor, *p, *g;               // Helpers
			Node* foundn = NULL;           // Found item
			Node **ptrToF = NULL, **ptrToQ = NULL;
			int dir = 1;
			// Set up our helpers
			cursor = &head;
			g = p = NULL;
			cursor->setlink (1, self->root);
			// Search and push a red node down to fix red violations as we go
			while (cursor->link (dir) != NULL) {
				int last = dir;
				// Move the helpers down
				g = p, p = cursor;
				ptrToQ = &(cursor->_link[dir]);//&Node::_getRawLink(q, dir);
				cursor = cursor->_link[dir];
				if(*ptrToQ != cursor){ printf("uh oh %016zx != %016zx\n", (size_t)cursor, (size_t)(*ptrToQ)); int i=0;i=1/i;}
				dir = self->cmp (self, cursor->getValue(), value) < 0;
				// Save the node with matching value and keep going; we'll do removal tasks at the end
				if (self->cmp (self, cursor->getValue(), value) == 0) {
					ptrToF = ptrToQ; foundn = cursor;
				}
				// Push the red node down with rotations and color flips
				if (!rb_node_is_red (cursor) && !rb_node_is_red (cursor->link (dir))) {
					if (rb_node_is_red (cursor->link (!dir))) {
						Node* n = rb_node_rotate (cursor, dir);
						p->setlink (last, n);
						p = n;
					} else if (!rb_node_is_red (cursor->link (!dir))) {
						Node* s = p->link (!last);
						if (s) {
							if (!rb_node_is_red (s->link (!last)) && !rb_node_is_red (s->link (last))) {
								// Color flip
								p->setRed (0);
								s->setRed (1);
								cursor->setRed (1);
							} else {
								int dir2 = g->link (1) == p;
								if (rb_node_is_red (s->link (last))) {
									g->setlink (dir2, rb_node_rotate2 (p, last));
								} else if (rb_node_is_red (s->link (!last))) {
									g->setlink (dir2, rb_node_rotate (p, last));
								}
								// Ensure correct coloring
								cursor->setRed (1);
								g->link (dir2)->setRed (1);
								g->link (dir2)->link (0)->setRed (0);
								g->link (dir2)->link (1)->setRed (0);
							}
						}
					}
				}
			}
			// Replace and remove the saved node
			if (foundn) {
				// if(f == q){
				// 	printf("no need to swap with itself\n");
				// }
				// else {
					int buffer[64];
					int position = 0;
					self->printTree();
					printNodesPointingAt(self->root, cursor, buffer, position); printf("<-q0 %d\n", cursor->isRed());
					printNodesPointingAt(self->root, foundn, buffer, position); printf("<-f0 %d\n", foundn->isRed());

					// swapNodes(ptrToF, ptrToQ); printf("~~~swapped~~~\n");
					// printNodesPointingAt(self->root, cursor, buffer, position); printf("<-q1 %d\n", cursor->isRed());
					// printNodesPointingAt(self->root, foundn, buffer, position); printf("<-f1 %d\n", foundn->isRed());
					// swapNodes(ptrToF, ptrToQ); printf("~~~swapped back~~~\n");
					// printNodesPointingAt(self->root, cursor, buffer, position); printf("<-q2 %d\n", cursor->isRed());
					// printNodesPointingAt(self->root, foundn, buffer, position); printf("<-f2 %d\n", foundn->isRed());

					void* tmp = Node::getValue (foundn);         //f->value;
					Node::setValue (foundn, Node::getValue (cursor)); //f->value = q->value;
					Node::setValue (cursor, tmp);                //q->value = tmp;

					p->setlink (p->link (1) == cursor, cursor->link (cursor->link (0) == NULL));
					if (node_cb) {
						node_cb (self, cursor);
					}

					// swapNodes(ptrToF, ptrToQ);
					// p->setlink (p->link (1) == f, f->link (f->link (0) == NULL));
					// if (node_cb) {
					// 	node_cb (self, f);
					// }

					cursor = NULL;
					ptrToQ = NULL;
					printf("swapped!\n");
				// }
			}
			// Update the root (it may be different)
			self->root = head.link (1);
			// Make the root black for simplified logic
			if (self->root != NULL) {
				self->root->setRed (1);
			}
			--self->_size;
		}
		return 1;
	}
	static void printNodesPointingAt(Node* tree, Node* search, int* path, int& position) {
		Node *link;
		for(int i=0;i<2;++i) {
			Node *link = tree->link(i);
			if(link) {
				path[position++] = i;
				if(link == search) {
					printf(" %zx:", (size_t)tree);
					for(int i=0;i<position;++i){printf("%d", path[i]);}
				}
				printNodesPointingAt(link, search, path, position);
				--position;
			}
		}
	}

	static void swapNodes(Node** ptrToA, Node** ptrToB) {
		Node *a = *ptrToA, *b = *ptrToB;
		// Node::setlink(*ptrToA, b);
		// Node::setlink(*ptrToB, a);
		Node* temp;
		for(int i = 0; i < 2; ++i) {
			temp = a->link(i); a->setlink(i, b->link(i)); b->setlink(i, temp);
		}
		int t;
		t    = a->isRed(); a->setRed(    b->isRed()); b->setRed(    t   );

		if(true || *ptrToA != b) { Node::setlink(*ptrToA, b); }
		if(true || *ptrToB != a) { Node::setlink(*ptrToB, a); }

	}

  private:
	static Node* rb_node_rotate (Node* self, int dir) {
		Node* result = NULL;
		if (self) {
			result = self->link (!dir);
			self->setlink (!dir, result->link (dir));
			result->setlink (dir, self);
			self->setRed (1);
			result->setRed (0);
		}
		return result;
	}

	static Node* rb_node_rotate2 (Node* self, int dir) {
		Node* result = NULL;
		if (self) {
			self->setlink (!dir, rb_node_rotate (self->link (!dir), !dir));
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
			Node* ln = root->link (0);
			Node* rn = root->link (1);
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
			if ((ln != NULL && self->cmp (self, ln->getValue(), root->getValue()) >= 0)
			|| (rn != NULL && self->cmp (self, rn->getValue(), root->getValue()) <= 0)) {
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

	/** @return how deep the heap goes (how many levels) */
	static int getDepth(Node* n) {
		if(n == NULL) return 0;
		int d = 1;
		int depths[2];
		for(int i = 0; i < 2; ++i) {
			depths[i] = getDepth(n->link(i));
		}
		d += (depths[0] > depths[1])?depths[0]:depths[1];
		return d;
	}
  public:
	void printTree() {
		if(root == 0) return;
		// determine how big the pyramid needs to be
		int depth = getDepth(root);
		int maxElements = 1;
		int inRow = 1;
		for(int i = 0; i < depth; ++i) {
			maxElements += inRow;
			inRow *= 2;
		}
		struct nodeAbbrev { void* value; bool isRed; };
		nodeAbbrev * pyramidBuffer = new nodeAbbrev[maxElements];
		void* defaultValue = (void*)0xb44df00d;
		for(int i = 0; i < maxElements; ++i) {
			pyramidBuffer[i].value = defaultValue;
			pyramidBuffer[i].isRed = false;
		}
		inRow = 1;
		int index = 0;
		bool couldHaveAValue;
		void* value;
		bool isRed;
		Node * cursor;
		// fill data into the pyramid
		for(int d = 0; d < depth; ++d) {
			for(int col = 0; col < inRow; ++col) {
				cursor = root;
				couldHaveAValue = true;
				for(int binaryDigit = 0; couldHaveAValue && binaryDigit < d; binaryDigit++) {
					int whichSide = ((col >> (d-binaryDigit-1)) & 1);
					cursor = cursor->link(whichSide);
					couldHaveAValue = cursor != 0;
				}
				value = (couldHaveAValue)?cursor->getValue():defaultValue;
				isRed = (couldHaveAValue)?cursor->isRed():false;
				pyramidBuffer[index].value = value;
				pyramidBuffer[index].isRed = isRed;
				index++;
			}
			if(d+1 < depth)
				inRow *= 2;
		}
		int NUMCHARS = 3;
		int maxWidth = (NUMCHARS+1)*inRow;
		inRow = 1;
		int leadSpace;
		int betweenValues;
		index = 0;
		// print the pyramid
		for(int d = 0; d < depth; ++d) {
			betweenValues = (maxWidth/inRow)-NUMCHARS;
			leadSpace = (betweenValues)/2;
			for(int i = 0; i < leadSpace; ++i) { putchar(' '); }
			for(int n = 0; n < inRow; ++n) {
				if(pyramidBuffer[index].value != defaultValue) {
					printf("%02d%c", pyramidBuffer[index].value, (pyramidBuffer[index].isRed?'R':' '));
				}
				else {
					printf("...");
				}
				index++;
				if(n+1 < inRow) {
					for(int i = 0; i < betweenValues; ++i) { putchar(' '); }
				}
			}
			printf("\n");
			inRow *= 2;
		}
		delete [] pyramidBuffer;
	}

  public:
	// OOP convenience
	void* find (void* value) { return find (this, value); }
	int insert (void* value) { return insert (this, value); }
	int remove (void* value) { return remove (this, value); }
	size_t size () { return size (this); }
	iter* createIter () { return iter::create (this); }
};