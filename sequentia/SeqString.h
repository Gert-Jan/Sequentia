#pragma once

template<class T>
class SeqList;

class SeqString
{
private:
	static SeqList<SeqList<char>*>* SplitResultBuffer;
	static int SplitResultBufferCount;
	static void AddToSplitResultBuffer(int index, char *string, int count, int offset);

public:
	static int BufferLen;
	static char Buffer[1024];

public:
	static void SetBuffer(char *string, int count);
	static void SetBuffer(char *string);
	static char* Format(char *format, ...);
	static void FormatBuffer(char *format, ...);
	static char* Copy(char* string);
	static char* CopyBuffer();
	static bool Equals(char *string1, char *string2);
	static bool EqualsBuffer(char *string);
	static int Find(char *string, char *phrase, int startAt = 0);
	static int FindReverse(char *string, char *phrase, int startAt = -1);
	static void Replace(char* phrase, char *with);
	static SeqList<SeqList<char>*>* Split(char *string, char *separator);
};
