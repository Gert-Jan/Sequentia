#pragma once

class SeqPath
{
public:
	static char* Normalize(char *path);
	static int CreateDir(char *path);
	static bool FileExists(char *path);
	static bool IsDir(char *normalizedPath);
};