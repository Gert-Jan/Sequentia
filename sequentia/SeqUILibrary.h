#pragma once

class SeqProject;
class SeqSerializer;

class SeqUILibrary : SeqActionHandler
{
public:
	SeqUILibrary(SeqProject *project);
	SeqUILibrary(SeqProject *project, SeqSerializer *serializer);
	SeqUILibrary(SeqProject *project, int windowId);
	~SeqUILibrary();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	void Draw();
	void Serialize(SeqSerializer *serializer);

private:
	void Init();
	void Deserialize(SeqSerializer *serializer);

public:
	static int nextWindowId;

private:
	int windowId = 0;
	char *name;
	SeqProject *project;
};
