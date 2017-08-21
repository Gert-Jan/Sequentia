#pragma once

class SeqProject;

class SeqUILibrary : SeqActionHandler
{
public:
	SeqUILibrary(SeqProject *project, int windowId);
	SeqUILibrary(SeqProject *project);
	~SeqUILibrary();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	void Draw();

private:
	void Init();
	
private:
	static int nextWindowId;
	int windowId = 0;
	char *name;
	SeqProject *project;
};
