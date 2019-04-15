//
// Based on Julienne Walker's <http://eternallyconfuzzled.com/> rb_tree
// implementation.
//
// Modified by Mirek Rusin <http://github.com/mirek/rb_tree>.
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

#include "rb_tree.h"

// RBT::node

// RBT::node * RBT::node::alloc () {
//     return (RBT::node*)malloc(sizeof(RBT::node));
// }

// RBT::node * RBT::node::init (RBT::node *self, void *value) {
//     if (self) {
//         self->setRed(1);
//         self->link[0] = self->link[1] = NULL;
//         self->value = value;
//     }
//     return self;
// }

RBT::node * RBT::node::create (void *value) {
    return RBT::node::init(RBT::node::alloc(), value);
}

void RBT::node::dealloc (RBT::node *self) {
    if (self) {
        free(self);
    }
}

/** won't just use ->isRed() because NULL nodes must always count as black. */
static int rb_node_is_red (const RBT::node *self) {
    return self ? self->isRed() : 0;
}

static RBT::node * rb_node_rotate (RBT::node *self, int dir) {
    RBT::node *result = NULL;
    if (self) {
        result = self->link[!dir];
        self->link[!dir] = result->link[dir];
        result->link[dir] = self;
        self->setRed(1);
        result->setRed(0);
    }
    return result;
}

static RBT::node * rb_node_rotate2 (RBT::node *self, int dir) {
    RBT::node *result = NULL;
    if (self) {
        self->link[!dir] = rb_node_rotate(self->link[!dir], !dir);
        result = rb_node_rotate(self, dir);
    }
    return result;
}

// RBT - default callbacks

int RBT::node_cmp_ptr_cb (RBT *self, RBT::node *a, RBT::node *b) {
    return (a->value > b->value) - (a->value < b->value);
}

void RBT::node_dealloc_cb (RBT *self, RBT::node *node) {
    if (self && node) {
        RBT::node::dealloc(node);
    }
}

// RBT

RBT * RBT::alloc () {
    return (RBT*)malloc(sizeof(RBT));
}

RBT * RBT::init (RBT *self, RBT::node_cmp_f node_cmp_cb) {
    if (self) {
        self->root = NULL;
        self->_size = 0;
        self->cmp = node_cmp_cb ? node_cmp_cb : node_cmp_ptr_cb;
    }
    return self;
}

RBT * RBT::create (RBT::node_cmp_f node_cb) {
    return RBT::init(alloc(), node_cb);
}

void RBT::dealloc (RBT *self, RBT::node_f node_cb) {
    if (self) {
        if (node_cb) {
            RBT::node *node = self->root;
            RBT::node *save = NULL;
            // Rotate away the left links so that we can treat this like the destruction of a linked list
            while (node) {
                if (node->link[0] == NULL) {
                    // No left links, just kill the node and move on
                    save = node->link[1];
                    node_cb(self, node);
                    node = NULL;
                } else {
                    // Rotate away the left link and check again
                    save = node->link[0];
                    node->link[0] = save->link[1];
                    save->link[1] = node;
                }
                node = save;
            }
        }
        free(self);
    }
}

int test (RBT *self, RBT::node *root) {
    int lh, rh;
    if ( root == NULL )
        return 1;
    else {
        RBT::node *ln = root->link[0];
        RBT::node *rn = root->link[1];
        // Consecutive red links
        if (root->isRed()) {
            if (ln->isRed() || rn->isRed()) {
                printf("Red violation");
                return 0;
            }
        }
        lh = test(self, ln);
        rh = test(self, rn);
        // Invalid binary search tree
        if ( ( ln != NULL && self->cmp(self, ln, root) >= 0 )
        ||   ( rn != NULL && self->cmp(self, rn, root) <= 0)) {
            puts ( "Binary tree violation" );
            return 0;
        }
        
        // Black height mismatch
        if ( lh != 0 && rh != 0 && lh != rh ) {
            puts ( "Black violation" );
            return 0;
        }
        
        // Only count black links
        if ( lh != 0 && rh != 0 )
            return rb_node_is_red ( root ) ? lh : lh + 1;
        else
            return 0;
    }
}

void * RBT::find(RBT *self, void *value) {
    void *result = NULL;
    if (self) {
        RBT::node node;
        node.value = value;
        RBT::node *it = self->root;
        int cmp = 0;
        while (it) {
            if ((cmp = self->cmp(self, it, &node))) {
                // If the tree supports duplicates, they should be chained to the right subtree for this to work
                it = it->link[cmp < 0];
            } else {
                break;
            }
        }
        result = it ? it->value : NULL;
    }
    return result;
}

// Creates (malloc'ates) 
int RBT::insert (RBT *self, void *value) {
    return RBT::insert_node(self, RBT::node::create(value));
}

/** Returns 1 on success, 0 otherwise. */
int RBT::insert_node (RBT *self, RBT::node *node) {
    int result = 0;
    if (self && node) {
        if (self->root == NULL) {
            self->root = node;
            result = 1;
        } else {
            RBT::node head;     // False tree root
            RBT::node *g, *t;   // Grandparent & parent
            RBT::node *p, *q;   // Iterator & parent
            int dir = 0, last = 0;

            // Set up our helpers
            t = &head;
            g = p = NULL;
            q = t->link[1] = self->root;

            // Search down the tree for a place to insert
            while (1) {
                if (q == NULL) {

                    // Insert node at the first null link.
                    p->link[dir] = q = node;
                } else if (rb_node_is_red(q->link[0]) && rb_node_is_red(q->link[1])) {
                
                    // Simple red violation: color flip
                    q->setRed(1);
                    q->link[0]->setRed(0);
                    q->link[1]->setRed(0);
                }

                if (rb_node_is_red(q) && rb_node_is_red(p)) {

                    // Hard red violation: rotations necessary
                    int dir2 = t->link[1] == g;
                    if (q == p->link[last]) {
                        t->link[dir2] = rb_node_rotate(g, !last);
                    } else {
                        t->link[dir2] = rb_node_rotate2(g, !last);
                    }
                }
          
                // Stop working if we inserted a node. This
                // check also disallows duplicates in the tree
                if (self->cmp(self, q, node) == 0) {
                    break;
                }

                last = dir;
                dir = self->cmp(self, q, node) < 0;

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
        self->root->setRed(0);
        ++self->_size;
    }
    
    return 1;
}

// Returns 1 if the value was removed, 0 otherwise. Optional node callback
// can be provided to dealloc node and/or user data. Use RBT_node_dealloc
// default callback to deallocate node created by RBT_insert(...).
int
RBT::remove_with_cb (RBT *self, void *value, RBT::node_f node_cb) {
    if (self->root != NULL) {
        RBT::node head;// False tree root
        RBT::node node;// = { .value = value }; // Value wrapper node
        node.value = value;
        RBT::node *q, *p, *g; // Helpers
        RBT::node *f = NULL;  // Found item
        int dir = 1;

        // Set up our helpers
        q = &head;
        g = p = NULL;
        q->link[1] = self->root;
    
        // Search and push a red node down
        // to fix red violations as we go
        while (q->link[dir] != NULL) {
            int last = dir;

            // Move the helpers down
            g = p, p = q;
            q = q->link[dir];
            dir = self->cmp(self, q, &node) < 0;
      
            // Save the node with matching value and keep
            // going; we'll do removal tasks at the end
            if (self->cmp(self, q, &node) == 0) {
                f = q;
            }

            // Push the red node down with rotations and color flips
            if (!rb_node_is_red(q) && !rb_node_is_red(q->link[dir])) {
                if (rb_node_is_red(q->link[!dir])) {
                    p = p->link[last] = rb_node_rotate(q, dir);
                } else if (!rb_node_is_red(q->link[!dir])) {
                    RBT::node *s = p->link[!last];
                    if (s) {
                        if (!rb_node_is_red(s->link[!last]) && !rb_node_is_red(s->link[last])) {
                            // Color flip
                            p->setRed(0);
                            s->setRed(1);
                            q->setRed(1);
                        } else {
                            int dir2 = g->link[1] == p;
                            if (rb_node_is_red(s->link[last])) {
                                g->link[dir2] = rb_node_rotate2(p, last);
                            } else if (rb_node_is_red(s->link[!last])) {
                                g->link[dir2] = rb_node_rotate(p, last);
                            }
                            
                            // Ensure correct coloring
                            q->setRed(1);
                            g->link[dir2]->setRed(1);
                            g->link[dir2]->link[0]->setRed(0);
                            g->link[dir2]->link[1]->setRed(0);
                        }
                    }
                }
            }
        }

        // Replace and remove the saved node
        if (f) {
            void *tmp = f->value;
            f->value = q->value;
            q->value = tmp;
            
            p->link[p->link[1] == q] = q->link[q->link[0] == NULL];
            
            if (node_cb) {
                node_cb(self, q);
            }
            q = NULL;
        }

        // Update the root (it may be different)
        self->root = head.link[1];

        // Make the root black for simplified logic
        if (self->root != NULL) {
            self->root->setRed(1);
        }

        --self->_size;
    }
    return 1;
}

int
RBT::remove (RBT *self, void *value) {
    int result = 0;
    if (self) {
        result = RBT::remove_with_cb(self, value, node_dealloc_cb);
    }
    return result;
}

size_t
RBT::size (RBT *self) {
    size_t result = 0;
    if (self) {
        result = self->_size;
    }
    return result;
}

// RBT::iter

RBT::iter * RBT::iter::alloc () {
    return (RBT::iter*)malloc(sizeof(RBT::iter));
}

RBT::iter *
RBT::iter::init (RBT::iter *self, RBT *tree) {
    if (self) {
        self->tree = tree;
        self->n = NULL;
        self->top = 0;
    }
    return self;
}

RBT::iter *
RBT::iter::create (RBT *tree) {
    return init(alloc(), tree);
}

void
RBT::iter::dealloc (RBT::iter *self) {
    if (self) {
        free(self);
    }
}

// Internal function, init traversal object, dir determines whether
// to begin traversal at the smallest or largest valued node.
void *
RBT::iter::start (RBT::iter *self, int dir) {
    void *result = NULL;
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

        result = self->n == NULL ? NULL : self->n->value;
    }
    return result;
}

// Traverse a red black tree in the user-specified direction (0 asc, 1 desc)
void *
RBT::iter::move (RBT::iter *self, int dir) {
    if (self->n->link[dir] != NULL) {
        // Continue down this branch
        self->path[self->top++] = self->n;
        self->n = self->n->link[dir];
        while ( self->n->link[!dir] != NULL ) {
            self->path[self->top++] = self->n;
            self->n = self->n->link[!dir];
        }
    } else {
        
        // Move to the next branch
        RBT::node *last = NULL;
        do {
            if (self->top == 0) {
                self->n = NULL;
                break;
            }
            last = self->n;
            self->n = self->path[--self->top];
        } while (last == self->n->link[dir]);
    }
    return self->n == NULL ? NULL : self->n->value;
}

void *
RBT::iter::first (RBT::iter *self) {
    return RBT::iter::start(self, 0);
}

void *
RBT::iter::last (RBT::iter *self) {
    return RBT::iter::start(self, 1);
}

void *
RBT::iter::next (RBT::iter *self) {
    return RBT::iter::move(self, 1);
}

void *
RBT::iter::prev (RBT::iter *self) {
    return RBT::iter::move(self, 0);
}