#include "SeqPath.h"
#include "SeqList.h"
#include "SeqString.h"
#include <SDL.h>
#include <direct.h>
#include <errno.h>

char* SeqPath::Normalize(const char *path)
{
	SeqString::Temp->Set(path);
	SeqString::Temp->Replace("\\", "/");
	return SeqString::Temp->Copy();
}

int SeqPath::CreateDir(const char *path)
{
	int error = 0;
	size_t pos = SeqString::Find(path, "/", 0);
	while (pos > -1)
	{
		SeqString::Temp->Set(path, pos);
		error = _mkdir(SeqString::Temp->Buffer);
		if (error == -1)
			error = errno;
		pos = SeqString::Find(path, "/", pos + 1);
	}
	return error;
}

bool SeqPath::FileExists(const char *path)
{
	struct stat buffer;
	return stat(path, &buffer) == 0;
}

bool SeqPath::IsDir(const char *normalizedPath)
{
	return normalizedPath[strlen(normalizedPath)] == '/';
}

char* SeqPath::GetDir(const char *path)
{
	char* normalizedPath = Normalize(path);
	if (!IsDir(normalizedPath))
	{
		size_t pos = SeqString::FindReverse(normalizedPath, "/");
		SeqString::Temp->Set(path);
		SeqString::Temp->Buffer[pos + 1] = 0;
		return SeqString::Temp->Copy();
	}
	else
	{
		return normalizedPath;
	}
}
