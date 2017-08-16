#pragma once

template<class T>
class SeqList
{
public:
	SeqList();
	SeqList(int initialCapacity);
	~SeqList();

	void Add(T item);
	void InsertAt(T item, const int index);
	void RemoveAt(const int index);
	T Get(const int index);
	void Set(const int index, T item);
	int Count();

private:
	void ensureCapacity(int requiredCapacity);

private:
	T *data;
	int capacity;
	int count;
};

#include "SeqListImpl.h"; 
