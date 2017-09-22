#pragma once

class SeqProject;
template<class T>
class SeqList;
class SeqSerializer;
class SeqLibrary;

struct SeqLibraryLink
{
	char *fullPath;
};

class SeqLibrary
{
public:
	SeqLibrary();
	~SeqLibrary();

	void Clear();
	
	void AddLink(char *fullPath);
	void RemoveLink(const int index);
	void RemoveLink(char *fullPath);
	int LinkCount();
	SeqLibraryLink GetLink(const int index);
	void UpdatePaths(char *oldProjectFullPath, char *newProjectFullPath);

	void Serialize(SeqSerializer *serializer);
	void Deserialize(SeqSerializer *serializer);

private:
	int GetLinkIndex(char *fullPath);

private:
	SeqList<SeqLibraryLink> *links;
};