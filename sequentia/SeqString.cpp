#include "SeqString.h";
#include "SeqList.h";
#include "SeqUtils.h";
#include <SDL.h>
#include <stdio.h>
#include <cstdarg>
#include <string>

SeqList<SeqList<char>*>* SeqString::SplitResultBuffer = new SeqList<SeqList<char>*>();
int SeqString::SplitResultBufferCount = 0;
char SeqString::Buffer[1024] = "";

void SeqString::SetBuffer(char *string, int count)
{
	count = SDL_min(SEQ_COUNT(Buffer) - 1, count);
	strncpy(Buffer, string, count);
	Buffer[count + 1] = 0;
}

char* SeqString::Format(char *format, ...)
{
	va_list args;
	va_start(args, format);
	int length = vsnprintf((char*)&Buffer, SEQ_COUNT(Buffer), format, args);
	va_end(args);
	if (length < 0)
	{
		return "ERROR in SeqStringFormat";
	}
	else
	{
		char* string = new char[length + 1];
		memcpy(string, &Buffer, length);
		string[length] = 0;
		Buffer[0] = 0;
		return string;
	}
}

char* SeqString::Copy(char *string)
{
	size_t size = strlen(string) + 1;
	char* copy = new char[size];
	strcpy_s(copy, size, string);
	return copy;
}

char* SeqString::CopyBuffer()
{
	size_t size = strlen(Buffer) + 1;
	char* copy = new char[size];
	strcpy_s(copy, size, Buffer);
	return copy;
}

bool SeqString::Equals(char *string1, char *string2)
{
	return strcmp(string1, string2) == 0;
}

bool SeqString::EqualsBuffer(char *string)
{
	return strcmp(string, Buffer) == 0;
}

int SeqString::Find(char *string, char *phrase, int startAt)
{
	int phrasePos = 0;
	int phraseLen = strlen(phrase);
	int current = startAt;
	while (string[current] != 0)
	{
		if (string[current] == phrase[phrasePos])
		{
			phrasePos++;
			if (phrasePos == phraseLen)
				return current - phraseLen + 1;
		}
		else
		{
			phrasePos = 0;
		}
		current++;
	}
	return -1;
}

int SeqString::FindReverse(char *string, char *phrase, int startAt)
{
	if (startAt == -1)
		startAt = strlen(string);
	int phraseLen = strlen(phrase);
	int phrasePos = phraseLen - 1;
	int current = startAt;
	while (current >= 0)
	{
		if (string[current] == phrase[phrasePos])
		{
			if (phrasePos == 0)
				return current - phraseLen + 1;
			phrasePos--;
		}
		else
		{
			phrasePos = phraseLen - 1;
		}
		current--;
	}
	return -1;
}

SeqList<SeqList<char>*>* SeqString::Split(char *string, char *separator)
{
	int previousSplit = 0;
	int splitIndex = 0;
	int separatorLen = strlen(separator);
	int separatorPos = Find(string, separator, 0);

	// prepare the result buffer
	SplitResultBuffer->Clear();
	
	// find all split positions
	while (separatorPos > -1)
	{
		// add split string to buffer
		AddToSplitResultBuffer(splitIndex, string, separatorPos - previousSplit, previousSplit);
		previousSplit = separatorPos + separatorLen;
		splitIndex++;
		separatorPos = Find(string, separator, previousSplit);
	}
	// add the last 
	AddToSplitResultBuffer(splitIndex, string, strlen(string) - previousSplit, previousSplit);
	return SplitResultBuffer;
}

void SeqString::AddToSplitResultBuffer(int index, char *string, int count, int offset)
{
	// ensure the result buffer is ready  
	if (index >= SplitResultBufferCount)
	{
		SplitResultBuffer->Add(new SeqList<char>(count + 1));
		SplitResultBufferCount++;
	}
	SeqList<char>* result = SplitResultBuffer->Get(index);
	result->Clear();
	// copy in result
	result->AddCopy(string, count, offset);
	result->Add(0);
}