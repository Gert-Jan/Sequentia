#pragma once

template<class T>
class SeqList;

class SeqString
{
public:
	SeqString(int initLen = 32);
	SeqString(char *string);
	void Set(const char *string, const int count);
	void Set(const char *string);
	void Clear();
	void Format(const char *format, ...);
	char* Copy();
	void Replace(const char* phrase, const char *with);
	bool Equals(const char *string);

	static char* Copy(const char* string);
	static bool Equals(const char *string1, const char *string2);
	static bool IsEmpty(const char *string);
	static int Find(const char *string, const char *phrase, const int startAt = 0);
	static int FindReverse(const char *string, const char *phrase, const int startAt = -1);

private:
	bool EnsureCapacity(int requiredLen);

public:
	int BufferLen;
	char *Buffer;

	static SeqString *Temp;
};
