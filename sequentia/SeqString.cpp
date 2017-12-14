#include "SeqString.h"
#include "SeqList.h"
#include "SeqUtils.h"
#include <SDL.h>
#include <stdio.h>
#include <cstdarg>
#include <string>

SeqList<SeqList<char>*>* SeqString::SplitResultBuffer = new SeqList<SeqList<char>*>();
int SeqString::SplitResultBufferCount = 0;
int SeqString::BufferLen = 1024;
char SeqString::Buffer[1024] = "";

void SeqString::SetBuffer(const char *string, const int count)
{
	int cappedCount = SDL_min(BufferLen - 1, count);
	strncpy_s(Buffer, string, cappedCount);
	Buffer[cappedCount] = 0;
}

void SeqString::SetBuffer(const char *string)
{
	SetBuffer(string, strlen(string));
}

char* SeqString::Format(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int length = vsnprintf((char*)&Buffer, BufferLen, format, args);
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

void SeqString::FormatBuffer(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int length = vsnprintf((char*)&Buffer, BufferLen, format, args);
	va_end(args);
	if (length < 0)
	{
		SetBuffer("ERROR in SeqStringFormat");
	}
	else
	{
		Buffer[length] = 0;
	}
}

char* SeqString::Copy(const char *string)
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

bool SeqString::Equals(const char *string1, const char *string2)
{
	return strcmp(string1, string2) == 0;
}

bool SeqString::EqualsBuffer(const char *string)
{
	return strcmp(string, Buffer) == 0;
}

bool SeqString::IsEmpty(const char *string)
{
	return string[0] == '\0';
}

int SeqString::Find(const char *string, const char *phrase, int startAt)
{
	int phrasePos = 0;
	int phraseLen = (int)strlen(phrase);
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

int SeqString::FindReverse(const char *string, const char *phrase, const int startAt)
{
	int phraseLen = (int)strlen(phrase);
	int phrasePos = phraseLen - 1;
	int current = startAt == -1 ? (int)strlen(string) : startAt;
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

void SeqString::ReplaceBuffer(const char* phrase, const char *with)
{
	int pos = Find(Buffer, phrase, 0);
	int phraseLen = (int)strlen(phrase);
	int withLen = (int)strlen(with);
	int delta = withLen - phraseLen;
	int len = (int)strlen(Buffer) + 1;

	while (pos > -1)
	{
		if (delta != 0)
		{
			memcpy(&Buffer[pos + phraseLen + delta], &Buffer[pos + phraseLen], len - (pos + phraseLen));
		}
		memcpy(&Buffer[pos], with, withLen);
		pos = Find(Buffer, phrase, pos + withLen);
	}
}

SeqList<SeqList<char>*>* SeqString::Split(const char *string, const char *separator)
{
	int previousSplit = 0;
	int splitIndex = 0;
	int separatorLen = (int)strlen(separator);
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
	AddToSplitResultBuffer(splitIndex, string, (int)strlen(string) - previousSplit, previousSplit);
	return SplitResultBuffer;
}

void SeqString::AddToSplitResultBuffer(const int index, const char *string, const int count, const int offset)
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
