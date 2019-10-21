#pragma once
#include "list.h"
#include "mem.h"
#include "rb_tree.h"
#include <stdio.h>

#define RBTKV RBT
//<KEY,VAL>

// template<typename KEY, typename VAL>
// int RBNodeCompare_KVP (RBT<KEY,VAL>* self, RBT<KEY,VAL>::Node* a, RBT<KEY,VAL>::Node* b) {
// 	KVP* ka = (KVP*)a->value;
// 	KVP* kb = (KVP*)b->value;
// 	if (ka->key < kb->key)
// 		return -1;
// 	if (ka->key > kb->key)
// 		return 1;
// 	return 0;
// }

template<class KEY, class VAL>
class Dictionary {
  public:
//	class RB : public RBT<KEY,VAL>{};
  private:
	RBTKV* tree;
  public:
	struct KVP {
		KEY key;
		VAL value;
		KVP () {}
		KVP (KEY key, VAL value) : key (key), value (value) {}
		KVP (KEY key) : key (key) {}
	};

  private:
	struct KVP* CreateAndAddKVP (const KEY key, const VAL& value, bool set) {
		if (set) {
			KVP search (key, 0);
			struct KVP* f = (KVP*)tree->find (&search);
			if (f != NULL) {
				f->value = value;
				return f;
			}
		}
		KVP* kvp = NewKVP (key, value);
		tree->insert (kvp);
		return kvp;
	}
	static KVP* NewKVP (const KEY key, const VAL& value) {
		return NEWMEM (KVP (key, value));
	}
	static void FreeKVP (KVP* kvp) {
		DELMEM (kvp);
	}
	// auto is used here because C++ has weird ineractions with template-argumented sub-types
	static void RBNodeDelete_KVP (RBTKV* self, RBTKV::Node* node) {
		FreeKVP ((KVP*)node->getValue());
	}

	static int RBNodeCompare_KVP (RBTKV* self, void* a, void* b) {
		KVP* ka = (KVP*)a;
		KVP* kb = (KVP*)b;
		if (ka->key < kb->key)
			return -1;
		if (ka->key > kb->key)
			return 1;
		return 0;
	}
  public:
	Dictionary () : tree (0) {
		tree = RBTKV::create (RBNodeCompare_KVP);
	}
	void Remove (const KEY key) {
		KVP kvp (key);
		RBTKV::remove_with_cb (tree, &kvp, RBNodeDelete_KVP);
	}
	void Set (const KEY key, const VAL& value) {
		CreateAndAddKVP (key, value, true);
	}
	/** this works if KEY is a pointer */
	VAL Get (const KEY& key) const {
		KVP search (key, 0);
		struct KVP* f = (KVP*)tree->find (&search);
		return (f != NULL) ? (VAL)f->value : (VAL)NULL;
	}
	/** this works if KEY is not a pointer */
	VAL* GetPtr (const KEY& key) const {
		KVP search (key, 0);
		struct KVP* f = (KVP*)tree->find (&search);
		return (f != NULL) ? &(f->value) : NULL;
	}
	VAL & operator[](const KEY& key) {
		KVP search (key, 0);
		struct KVP* f = (KVP*)tree->find (&search);
		return f->value;
	}

	void ToArray (KVP* list) {
		auto iter = tree->createIter ();
		if (iter) {
			int i = 0, count = tree->size();
			for (KVP* v = (KVP*)iter->first (); i < count; v = (KVP*)iter->next ()) {
				list[i++] = *v;
			}
			// for (KVP* v = (KVP*)iter->first (); v; v = (KVP*)iter->next ()) {
			// 	list[index++] = *v;
			// }
			iter->dealloc ();
		}

	}
	void Clear () {
		Release ();
		tree = RBTKV::create (RBNodeCompare_KVP);
	}
	List<KVP>& CreateArray () {
		List<KVP> list (Count ());
		ToArray (list.GetData ());
		return list;
	}
	int Count () const { return RBTKV::size (tree); }
	void Release () {
		if (tree != NULL) {
			RBTKV::dealloc (tree, RBNodeDelete_KVP);
		}
	}
	~Dictionary () {
		Release ();
	}
};