#pragma once

struct SDL_RWops;
class SeqSerializer;

class SeqLibrary;
class SeqChannel;
enum SeqChannelType;

class SeqAction;
class SeqActionHandler;

class SeqUILibrary;
class SeqUISequencer;

template<class T>
class SeqList;

class SeqProject
{
public:
	SeqProject();
	~SeqProject();

	void Clear();
	void Open();
	void OpenFrom();
	void Save();
	void SaveAs();

	char* GetPath();
	void SetPath(char *fullPath);

	void AddAction(const SeqAction action);
	void AddActionHandler(SeqActionHandler *handler);
	void RemoveActionHandler(SeqActionHandler *handler);
	int GetActionCount();
	int GetActionCursor();
	SeqAction GetAction(const int index);
	void DoAction(const SeqAction action);
	void UndoAction(const SeqAction action);

	int GetChannelCount();
	SeqChannel GetChannel(const int index);

	void AddSequencer();
	void RemoveSequencer(SeqUISequencer *sequencer);
	void AddLibrary();
	void RemoveLibrary(SeqUILibrary *library);

	double GetLength();

	void Draw();
	void DrawDialogs();

private:
	void AddChannel(SeqChannelType type, char *name);
	void RemoveChannel(const int index);

	int Serialize(SeqSerializer *serializer);
	int Deserialize(SeqSerializer *serializer);

private:
	const int version = 1;
	char *fullPath;
	SeqLibrary *library;
	SeqList<SeqChannel> *channels;
	SeqList<SeqUISequencer*> *uiSequencers;
	SeqList<SeqUILibrary*> *uiLibraries;

	SeqList<SeqActionHandler*> *actionHandlers;
	SeqList<SeqAction> *actions;
	int actionCursor = 0;
};
