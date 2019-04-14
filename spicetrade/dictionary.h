#pragma once
#include "mem.h"
#include "list.h"
#include <stdio.h>
#include "rb_tree.h"

template<typename KTYPE, typename VTYPE>
class Dictionary {
public:
	struct KVP {
		KTYPE key; VTYPE value;
		KVP(){}
		KVP(KTYPE key, VTYPE value):key(key),value(value){}
		KVP(KTYPE key):key(key){}
	};
private:
    struct rb_tree* tree;
    static int RBNodeCompare_KVP(struct rb_tree *self, struct rb_node *a, struct rb_node *b) {
        KVP* ka = (KVP*)a->value;
        KVP* kb = (KVP*)b->value;
        if(ka->key < kb->key) return -1;
        if(ka->key > kb->key) return 1;
        return 0;
    }
    struct KVP * CreateAndAddKVP(const KTYPE key, const VTYPE & value, bool set) {
        if(set) {
            KVP search(key, 0);
            struct KVP *f = (KVP*)rb_tree_find(tree, &search);
            if(f != NULL) {
                f->value = value;
                return f;
            }
        }
        KVP * kvp = NewKVP(key,value);
        rb_tree_insert(tree, kvp);
        return kvp;
    }
    static KVP* NewKVP(const KTYPE key, const VTYPE & value) {
        return NEWMEM(KVP(key, value));
    }
    static void FreeKVP(KVP* kvp){
        DELMEM(kvp);
    }
    static void RBNodeDelete_KVP(struct rb_tree *self, struct rb_node *node) {
        FreeKVP((KVP*)node->value);
    }
public:
    Dictionary():tree(0) {
        tree = rb_tree_create(RBNodeCompare_KVP);
    }
	void Remove(const KTYPE key) {
        KVP kvp(key);
        rb_tree_remove_with_cb(tree, &kvp, RBNodeDelete_KVP);
	}
	void Set(const KTYPE key, const VTYPE & value) {
        CreateAndAddKVP(key, value, true);
	}
	/** this works if KTYPE is a pointer */
	VTYPE Get(const KTYPE & key) const {
        KVP search(key, 0);
        struct KVP *f = (KVP*)rb_tree_find(tree, &search);
		return (f != NULL) ? f->value : NULL;
	}
	/** this works if KTYPE is not a pointer */
	VTYPE* GetPtr(const KTYPE & key) const {
        KVP search(key, 0);
        struct KVP *f = (KVP*)rb_tree_find(tree, &search);
		return (f != NULL) ? &(f->value) : NULL;
	}

	void ToArray(KVP * list) {
		int index = 0;
        struct rb_iter *iter = rb_iter_create();
	    if (iter) {
            for (struct KVP *v = (KVP*)rb_iter_first(iter, tree); v; v = (KVP*)rb_iter_next(iter)) {
                list[index++] = *v;
            }
            rb_iter_dealloc(iter);
        }
	}
    void Clear(){
        Release();
        tree = rb_tree_create(RBNodeCompare_KVP);
    }
    List<KVP> & CreateArray() {
        List<KVP> list(Count());
        ToArray(list.GetData());
        return list;
    }
	int Count() const { return tree->size; }
    void Release(){
        if(tree != NULL) {
            rb_tree_dealloc(tree, RBNodeDelete_KVP);
        }
    }
	~Dictionary() {
        Release();
	}
};