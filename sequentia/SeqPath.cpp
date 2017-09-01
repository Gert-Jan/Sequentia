#include "SeqPath.h";
#include "SeqList.h";
#include "SeqString.h";
#include <SDL.h>
#include <dirent.h>
#include <direct.h>
#include <errno.h>

int SeqPath::CreateDir(char *path)
{
	int error = 0;
	int pos = SeqString::Find(path, "/", 0);
	while (pos > -1)
	{
		SeqString::SetBuffer(path, pos);
		error = mkdir(SeqString::Buffer);
		if (error == -1)
			error = errno;
		pos = SeqString::Find(path, "/", pos + 1);
	}
	return error;
}

bool SeqPath::FileExists(char *path)
{
	struct stat buffer;
	return stat(path, &buffer) == 0;
}