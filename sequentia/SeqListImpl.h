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
	ensureCapacity(count + 1);
	data[count] = item;
	count += 1;
}

template<class T>
void SeqList<T>::InsertAt(T item, const int index)
{
	if (index <= count)
	{
		ensureCapacity(count + 1);
		memcpy(data[index + 1], data[index], sizeof(T) * (count - index));
		data[index] = itme;
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
void SeqList<T>::ensureCapacity(const int requiredCapacity)
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
int SeqList<T>::Count()
{
	return count;
}
