#pragma once
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BOUNDSCHECK(index, count)	if(index < 0 || index >= count) {printf("oob %d out of %d: %s:%d\n", index, count, __FILE__, __LINE__);int i=0;i=1/i;}

template<typename TYPE>
class List {
private:
	int length;
	TYPE * data;
public:
	List():length(0),data(NULL){}
	List(List & toCopy):length(0),data(NULL) { Copy(toCopy); }
	void Copy(const List<TYPE> & toCopy) {
		SetLength(toCopy.Length());
		memcpy(data, toCopy.data, sizeof(TYPE)*length);
	}
	void Move(List & moving) {
		data = moving.data; length = moving.length;
		moving.data = NULL; moving.length = 0;
	}
	List(List && moving) { Move(moving); }
	List(int size):length(0),data(NULL){
		SetLength(size);
	}
	List(int size, const TYPE & value):length(0),data(NULL){
		SetLength(size);
		SetAll(value);
	}
	~List() { Release(); }

	static int IndexOf(const TYPE & value, const TYPE * data, const int start, const int limit) {
		for(int i = start; i < limit; ++i) {
			if(data[i] == value) { return i; }
		}
		return -1;
	}
	int IndexOf(const TYPE & value, const int start, const int limit) const {
		return IndexOf(value, data, start, limit);
	}
	int IndexOf(const TYPE & value) const { return IndexOf(value, 0, length); }
	void Release() {
		if(data != NULL){
			DELMEM_ARR(data);
			data = NULL;
		}
		length = 0;
	}
	TYPE * SetLength(const int size) {
		TYPE * newData = NEWMEM_ARR(TYPE, size);
		if(newData == NULL) {
			return NULL;
		}
		if(data != NULL){
			int limit = (length<size)?length:size;
			for(int i = 0; i < limit; ++i){ newData[i] = data[i]; }
			DELMEM_ARR(data);
			data = NULL;
		}
		data = newData;
		length = size;
		return data;
	}
	TYPE * GetData() { return data; }
	void SetAll(const TYPE & value) {
		for(int i = 0; i < length; ++i) { data[i] = value; }
	}
	int Length() const { return length; }
	TYPE & Get(const int index) {
		BOUNDSCHECK(index, length);
		return data[index];
	}
	TYPE GetAt(const int index) const {
		BOUNDSCHECK(index, length);
		return data[index];
	}
	void Set(const int index, const TYPE & value){
		BOUNDSCHECK(index, length);
		data[index] = value;
	}
	TYPE & operator[](const int index) { return Get(index); }
private:
	static void swap(TYPE* a, TYPE* b) { TYPE c = *a; *a = *b; *b = c; }
	template<typename INDEXABLE>
	static int partition (INDEXABLE & arr, int low, int high)  {
		int pivot = arr[high];    // pivot
		int i = (low - 1);        // Index of smaller element
		for (int j = low; j <= high- 1; j++) {
			if (arr[j] <= pivot)  {
				i++;    // increment index of smaller element
				swap(&arr[i], &arr[j]);
			}
		}
		swap(&arr[i + 1], &arr[high]);
		return (i + 1);
	}
public:
	template<typename INDEXABLE>
	static void quickSort(INDEXABLE & arr, int low, int high) {
		if (low < high) {
			int pi = partition(arr, low, high);
			quickSort(arr, low, pi - 1);
			quickSort(arr, pi + 1, high);
		}
	}
	void Sort() { quickSort<TYPE*>(data,0,Length()-1); }
	TYPE Sum() const {
		TYPE total = 0;
		for(int i = 0; i < length; ++i) { total += data[i]; }
		return total;
	}
};

template<typename TYPE>
class VList : protected List<TYPE> {
private:
	int count;
public:
	VList():List<TYPE>::List(),count(0) {}
	VList(VList & toCopy):List<TYPE>::List(),count(0) { Copy(toCopy); }
	void Copy(VList & toCopy) {
		List<TYPE>::SetLength(toCopy.List<TYPE>::Length());
		count = toCopy.count;
		memcpy(GetData(), toCopy.GetData(), sizeof(TYPE)*count);
	}
	VList(VList && moving) {
		List<TYPE>::Move(moving);
		count = moving.count;
		moving.count = 0;
	}
	VList(const int capacity):count(0),List<TYPE>::List(capacity){}
	TYPE * GetData() { return List<TYPE>::GetData(); }
	bool EnsureCapacity(int size) {
		if(size >= this->Length()) {
			int newLength = this->Length();
			while(newLength < size) { newLength += 16; }
			return this->SetLength(newLength) != NULL;
		}
		return true;
	}
	void Clear(){count = 0;}
	void SetCount(int count) {
		EnsureCapacity(count);
		this->count = count;
	}
	bool Add(const TYPE & value) {
		if(!EnsureCapacity(count+1)) return false;
		List<TYPE>::Set(count++, value);
		return true;
	}
	bool Insert(const int index, TYPE & value) {
		if(!EnsureCapacity(count+1)) return false;
		for(int i = count; i > index; --i) {
			Set(i, Get(i-1));
		}
		++count;
		Set(index, value);
		return true;
	}
	bool Insert(const int index, TYPE * value, int count) {
		if(!EnsureCapacity(this->count+count)) return false;
		this->count += count;
		for(int i = this->count-1; i >= index+count; --i) {
			Set(i, Get(i-count));
		}
		for(int i = 0; i < count; ++i) {
			Set(index+i, value[i]);
		}
		return true;
	}
	TYPE * GetAdd(const TYPE & value) {
		if(!EnsureCapacity(count+1)) return NULL;
		List<TYPE>::Set(count, value);
		count++;
		return &Get(count-1);
	}
	void RemoveAt(const int index) {
		for(int i = index; i < count-1; ++i) { Set(i, Get(i+1)); }
		count--;
	}
	TYPE PopLast() {
		count--;
		return List<TYPE>::GetAt(count);
	}
	int Count() const {return count;}
	int IndexOf(const TYPE & value, const int start, const int limit) const {
		return List<TYPE>::IndexOf(value, start, limit);
	}
	int IndexOf(const TYPE & value) const { return IndexOf(value, 0, count); }
	TYPE & Get(const int index) {
		BOUNDSCHECK(index, count);
		return List<TYPE>::Get(index);
	}
	void Set(const int index, TYPE value) {
		BOUNDSCHECK(index, count);
		List<TYPE>::Set(index, value);
	}
	TYPE & operator[](const int index) { return Get(index); }
	void Sort() { List<TYPE>::quickSort(*this,0,Count()-1); }
};

// like a linked-list, but memory stable, and with slightly more overhead
template<typename TYPE>
class PageList {
private:
	int pageSize, count;
	VList<TYPE*> pages;
public:
	PageList():pageSize(64),count(0){}
	PageList(int pageSize):pageSize(pageSize),count(0){}
	~PageList() { Release(); }
	void Release() {
		for(int i = 0; i < pages.Count(); ++i) {
			DELMEM_ARR(pages.Get(i));
			pages.Set(i, NULL);
		}
		pages.Clear();
	}
	bool EnsureCapacity(int size) {
		while(size >= pageSize * pages.Count()) {
			TYPE* page = NEWMEM_ARR(TYPE, pageSize);
			if(page == NULL) return false;
			pages.Add(page);
		}
		return true;
	}
	void Clear() { count = 0;}
	bool Add(const TYPE & value) {
		if(!EnsureCapacity(count+1)) return false;
		Set(count, value);
		count++;
	}
	TYPE * GetAdd(const TYPE & value) {
		if(!EnsureCapacity(count+1)) return NULL;
		count++;
		Set(count-1, value);
		return &Get(count-1);
	}
	void RemoveAt(const int index) {
		div_t divresult = div (index,pageSize);
		TYPE* list = pages.Get(divresult.quot);
		for(int col = divresult.rem; col < pageSize; ++col) {
			list[col] = list[col+1];
		}
		if(pages.Count() > divresult.quot) {
			TYPE* nextList = pages.Get(divresult.quot+1);
			list[pageSize-1] = nextList[0];
			list = nextList;
		}
		for(int row = divresult.quot+1; row < pages.Count(); ++row) {
			for(int col = 0; col < pageSize; ++col) {
				list[pageSize-1] = list[col+1];
			}
			if(pages.Count() > row+1) {
				TYPE* nextList = pages.Get(row+1);
				list[pageSize-1] = nextList[0];
				list = nextList;
			}
		}
		count--;
	}
	int Count() const {return count;}
	int IndexOf(const TYPE & value, int start, const int limit) const {
		div_t divresult = div (start,pageSize);
		start = divresult.rem;
		int found = -1, rowlimit = pageSize, lastRow = limit/pageSize;
		for(int row = divresult.quot; row < pages.Count(); ++row) {
			if(row == lastRow) { rowlimit = limit - pageSize*row; }
			found = List<TYPE>::IndexOf(value, pages.GetAt(row), start, rowlimit);
			if(found != -1){
				found += row*pageSize;
				return found;
			}
			start = 0;
		}
		return -1;
	}
	int IndexOf(const TYPE & value) const { return IndexOf(value, 0, count); }
	TYPE & Get(const int index) {
		BOUNDSCHECK(index, count);
		div_t divresult = div (index,pageSize);
		return pages.Get(divresult.quot)[divresult.rem];
	}
	void Set(const int index, TYPE value) {
		BOUNDSCHECK(index, count);
		div_t divresult = div (index,pageSize);
		pages.Get(divresult.quot)[divresult.rem] = value;
	}
	TYPE & operator[](const int index) { return Get(index); }
	void Sort() { List<TYPE>::quickSort(*this,0,Count()-1); }
};