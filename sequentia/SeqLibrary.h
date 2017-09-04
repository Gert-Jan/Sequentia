#pragma once

class SeqProject;
template<class T>
class SeqList;

struct SeqLibraryLink
{
	bool isDirectory;
	char *fullPath;
	char *relativePath;
};

class SeqLibrary
{
public:
	SeqLibrary(SeqProject *project);
	~SeqLibrary();

	void AddLink(char *fullPath);
	void UpdateRelativePaths(char *projectFullPath);

private:
	SeqProject *project;
	SeqList<SeqLibraryLink> *links;
};