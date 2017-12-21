#include "SeqString.h"
#include "SeqList.h"
#include <SDL.h>
#include <stdio.h>
#include <cstdarg>
#include <string>

SeqString* SeqString::Temp = new SeqString(1024);

SeqString::SeqString(int initLen)
{
	BufferLen = SDL_max(initLen, 4);
	Buffer = new char[BufferLen];
}

SeqString::SeqString(char *string)
{
	BufferLen = strlen(string);
	Buffer = new char[BufferLen];
	strcpy_s(Buffer, BufferLen, string);
}

void SeqString::Set(const char *string, const int count)
{
	EnsureCapacity(count);
	memcpy_s(Buffer, BufferLen, string, count);
	Buffer[count] = 0;
}

void SeqString::Set(const char *string)
{
	Set(string, strlen(string));
}

void SeqString::Clear()
{
	Buffer[0] = 0;
}

void SeqString::Format(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int length = vsnprintf(Buffer, BufferLen, format, args);
	// handle errors
	if (length < 0)
	{
		Set("ERROR in SeqString::Format()");
		return;
	}
	// increase buffer size if needed and retry
	else if (EnsureCapacity(length))
	{
		length = vsnprintf(Buffer, BufferLen, format, args);
	}
	va_end(args);
	// end string with /0
	Buffer[length] = 0;
}

char* SeqString::Copy(const char *string)
{
	size_t size = strlen(string) + 1;
	char* copy = new char[size];
	strcpy_s(copy, size, string);
	return copy;
}

char* SeqString::Copy()
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

bool SeqString::Equals(const char *string)
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

void SeqString::Replace(const char* phrase, const char *with)
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
			EnsureCapacity(BufferLen + delta);
			memcpy(&Buffer[pos + phraseLen + delta], &Buffer[pos + phraseLen], len - (pos + phraseLen));
		}
		memcpy(&Buffer[pos], with, withLen);
		pos = Find(Buffer, phrase, pos + withLen);
	}
}

bool SeqString::EnsureCapacity(int requiredLen)
{
	if (BufferLen < requiredLen + 1)
	{
		int newLen = BufferLen;
		while (newLen < requiredLen + 1)
			newLen = newLen * 2;
		char *newBuffer = new char[newLen];
		memcpy_s(newBuffer, newLen, Buffer, BufferLen);
		delete[] Buffer;
		Buffer = newBuffer;
		BufferLen = newLen;
		return true;
	}
	else
	{
		return false;
	}
}
