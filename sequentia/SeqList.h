#pragma once

template<class T>
class SeqList
{
public:
	SeqList();
	SeqList(int initialCapacity);
	~SeqList();

	void Add(T item);
	void AddCopy(const T *source, const int addCount, const int sourceOffset);
	int Remove(const T item);
	void InsertAt(T item, const int index);
	void RemoveAt(const int index);
	void RemoveLast();
	T ReplaceAt(T item, const int index);
	T Get(const int index);
	T* GetPtr(const int index);
	void Set(const int index, T item);
	int IndexOf(T item);
	int Count();
	void Clear();

private:
	void EnsureCapacity(const int requiredCapacity);

private:
	T *data;
	int capacity;
	int count;
};

#include "SeqListImpl.h"; 
