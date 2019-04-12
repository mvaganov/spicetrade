#include "mem.h"
#include <stdio.h>
#define BOUNDSCHECK(index, count)     if(index < 0 || index >= count) {printf("oob\n");int i=0;i=1/i;}

template<typename TYPE>
class List {
private:
    int length;
    TYPE * data;
public:
    List():length(0),data(nullptr){}
    List(int size):length(0),data(nullptr){
        SetLength(size);
    }
    List(int size, const TYPE & value):length(0),data(nullptr){
        SetLength(size);
        SetAll(value);
    }
    ~List() {
        if(data != nullptr){
            DELMEM_ARR(data);
            data = nullptr;
        }
        length = 0;
    }
    void SetLength(const int size) {
        TYPE * newData = NEWMEM_ARR(TYPE, size);
        if(data != nullptr){
            int limit = (length<size)?length:size;
            for(int i = 0; i < limit; ++i){ newData[i] = data[i]; }
            DELMEM_ARR(data);
            data = nullptr;
        }
        data = newData;
        length = size;
    }
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
};

template<typename TYPE>
class VList : List<TYPE> {
private:
    int count;
public:
    VList():List<TYPE>::List(),count(0) {}
    VList(const int capacity):count(0),List<TYPE>::List(capacity){}
    void Clear(){count = 0;}
    void Add(const TYPE & value) {
        if(count >= this->Length()) {
            int newLength = this->Length()+16;
            this->SetLength(newLength);
        }
        List<TYPE>::Set(count, value);
        count++;
    }
    void RemoveAt(const int index) {
        for(int i = index; i < count-1; ++i) { Set(i, Get(i+1)); }
        count--;
    }
    int Count() const {return count;}
    TYPE & Get(const int index) {
        BOUNDSCHECK(index, count);
        return List<TYPE>::Get(index);
    }
    void Set(const int index, TYPE value){
        BOUNDSCHECK(index, count);
        List<TYPE>::Set(index, value);
    }
    TYPE & operator[](const int index) { return Get(index); }
};