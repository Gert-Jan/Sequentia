#pragma once

template<class T>
class SeqList;

class SeqString
{
private:
	static SeqList<SeqList<char>*>* SplitResultBuffer;
	static int SplitResultBufferCount;
	static void AddToSplitResultBuffer(const int index, const char *string, const int count, const int offset);

public:
	static int BufferLen;
	static char Buffer[1024];

public:
	static void SetBuffer(const char *string, const int count);
	static void SetBuffer(const char *string);
	static char* Format(const char *format, ...);
	static void FormatBuffer(const char *format, ...);
	static char* Copy(const char* string);
	static char* CopyBuffer();
	static bool Equals(const char *string1, const char *string2);
	static bool EqualsBuffer(const char *string);
	static int Find(const char *string, const char *phrase, const int startAt = 0);
	static int FindReverse(const char *string, const char *phrase, const int startAt = -1);
	static void ReplaceBuffer(const char* phrase, const char *with);
	static SeqList<SeqList<char>*>* Split(const char *string, const char *separator);
};
