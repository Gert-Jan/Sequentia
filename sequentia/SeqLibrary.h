#pragma once

class SeqProject;
template<class T>
class SeqList;
class SeqSerializer;

struct SeqLibraryLink
{
	char *fullPath;
	char *relativePath;
};

class SeqLibrary
{
public:
	SeqLibrary(SeqProject *project);
	~SeqLibrary();

	void Clear();
	
	void AddLink(char *fullPath);
	void UpdateRelativePaths(char *projectFullPath);

	void Serialize(SeqSerializer *serializer);
	void Deserialize(SeqSerializer *serializer);

private:
	SeqProject *project;
	SeqList<SeqLibraryLink> *links;
};