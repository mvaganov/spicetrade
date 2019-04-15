#pragma once
#include "mem.h"
#include "list.h"
#include <stdio.h>
#include "rb_tree.h"

#define RBTKV	RBT

template<typename KEY, typename VAL>
class Dictionary {
public:
	struct KVP {
		KEY key; VAL value;
		KVP(){}
		KVP(KEY key, VAL value):key(key),value(value){}
		KVP(KEY key):key(key){}
	};
private:
    RBTKV* tree;
    static int RBNodeCompare_KVP(RBTKV *self, RBTKV::node *a, RBTKV::node *b) {
        KVP* ka = (KVP*)a->value;
        KVP* kb = (KVP*)b->value;
        if(ka->key < kb->key) return -1;
        if(ka->key > kb->key) return 1;
        return 0;
    }
    struct KVP * CreateAndAddKVP(const KEY key, const VAL & value, bool set) {
        if(set) {
            KVP search(key, 0);
            struct KVP *f = (KVP*)tree->find(&search);
            if(f != NULL) {
                f->value = value;
                return f;
            }
        }
        KVP * kvp = NewKVP(key,value);
        tree->insert(kvp);
        return kvp;
    }
    static KVP* NewKVP(const KEY key, const VAL & value) {
        return NEWMEM(KVP(key, value));
    }
    static void FreeKVP(KVP* kvp){
        DELMEM(kvp);
    }
    static void RBNodeDelete_KVP(RBTKV *self, RBTKV::node *node) {
        FreeKVP((KVP*)node->value);
    }
public:
    Dictionary():tree(0) {
        tree = RBTKV::create(RBNodeCompare_KVP);
    }
	void Remove(const KEY key) {
        KVP kvp(key);
        RBTKV::remove_with_cb(tree, &kvp, RBNodeDelete_KVP);
	}
	void Set(const KEY key, const VAL & value) {
        CreateAndAddKVP(key, value, true);
	}
	/** this works if KEY is a pointer */
	VAL Get(const KEY & key) const {
        KVP search(key, 0);
        struct KVP *f = (KVP*)tree->find(&search);
		return (f != NULL) ? f->value : NULL;
	}
	/** this works if KEY is not a pointer */
	VAL* GetPtr(const KEY & key) const {
        KVP search(key, 0);
        struct KVP *f = (KVP*)tree->find(&search);
		return (f != NULL) ? &(f->value) : NULL;
	}

	void ToArray(KVP * list) {
		int index = 0;
        RBTKV::iter *iter = tree->createIter();
	    if (iter) {
            for (KVP *v = (KVP*)iter->first(); v; v = (KVP*)iter->next()) {
                list[index++] = *v;
            }
            iter->dealloc();
        }
	}
    void Clear(){
        Release();
        tree = RBTKV::create(RBNodeCompare_KVP);
    }
    List<KVP> & CreateArray() {
        List<KVP> list(Count());
        ToArray(list.GetData());
        return list;
    }
	int Count() const { return RBTKV::size(tree); }
    void Release(){
        if(tree != NULL) {
            RBTKV::dealloc(tree, RBNodeDelete_KVP);
        }
    }
	~Dictionary() {
        Release();
	}
};