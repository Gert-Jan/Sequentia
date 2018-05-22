#pragma once

class SeqPath
{
public:
	static char* Normalize(const char *path);
	static int CreateDir(const char *path);
	static bool FileExists(const char *path);
	static bool IsDir(const char *normalizedPath);
	static char* GetDir(const char *path);
};