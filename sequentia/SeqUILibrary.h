#pragma once

#include "SeqWindow.h";
#include "SeqAction.h";

class SeqProject;
class SeqLibrary;
class SeqLibraryLink;
class SeqSerializer;

class SeqUILibrary : public SeqWindow, SeqActionHandler
{
public:
	SeqUILibrary(SeqProject *project, SeqLibrary *library);
	SeqUILibrary(SeqProject *project, SeqLibrary *library, SeqSerializer *serializer);
	~SeqUILibrary();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	SeqWindowType GetWindowType();
	void Draw();
	void Serialize(SeqSerializer *serializer);

private:
	void Init();
	void AddContextMenu(SeqLibraryLink *link);
	void Deserialize(SeqSerializer *serializer);

private:
	char *name;
	SeqProject *project;
	SeqLibrary *library;
};
