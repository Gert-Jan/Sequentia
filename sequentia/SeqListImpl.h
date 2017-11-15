#include <string>;

template<class T>
SeqList<T>::SeqList() : SeqList(16)
{
}

template<class T>
SeqList<T>::SeqList(int initialCapacity) :
	capacity(initialCapacity),
	count(0)
{
	if (capacity < 1)
		capacity = 1;
	data = new T[capacity];
}

template<class T>
SeqList<T>::~SeqList()
{
	delete[] data;
}

template<class T>
void SeqList<T>::Add(T item)
{
	EnsureCapacity(count + 1);
	data[count] = item;
	count += 1;
}

template<class T>
void SeqList<T>::AddCopy(T *source, int addCount, int sourceOffset)
{
	EnsureCapacity(count + addCount);
	memcpy(&data[count], &source[sourceOffset], sizeof(T) * addCount);
	count += addCount;
}

template<class T>
int SeqList<T>::Remove(T item)
{
	for (int i = 0; i < count; i++)
	{
		if (data[i] == item)
		{
			RemoveAt(i);
			return i;
		}
	}
	return -1;
}

template<class T>
void SeqList<T>::InsertAt(T item, const int index)
{
	if (index <= count)
	{
		EnsureCapacity(count + 1);
		memcpy(data[index + 1], data[index], sizeof(T) * (count - index));
		data[index] = item;
		count += 1;
	}
}

template<class T>
void SeqList<T>::RemoveAt(const int index)
{
	if (index < count)
	{
		memcpy(&data[index], &data[index + 1], sizeof(T) * (count - index - 1));
		count -= 1;
	}
}

template<class T>
void SeqList<T>::RemoveLast()
{
	count = max(0, count - 1);
}

template<class T>
T SeqList<T>::ReplaceAt(T item, const int index)
{
	T previous = data[index];
	data[index] = item;
	return previous;
}

template<class T>
void SeqList<T>::EnsureCapacity(const int requiredCapacity)
{
	if (requiredCapacity > capacity)
	{
		int newCapacity = capacity * 2;
		while (requiredCapacity > newCapacity)
			newCapacity = newCapacity * 2;
		T* newData = new T[newCapacity];
		T* deleteData = data;
		memcpy(newData, data, sizeof(T) * count);
		data = newData;
		delete[] deleteData;
		capacity = newCapacity;
	}
}

template<class T>
T SeqList<T>::Get(const int index)
{
	return data[index];
}

template<class T>
void SeqList<T>::Set(const int index, T value)
{
	data[index] = value;
}

template<class T>
int SeqList<T>::Count()
{
	return count;
}

template<class T>
void SeqList<T>::Clear()
{
	count = 0;
}
