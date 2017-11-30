#pragma once

struct SDL_RWops;
class SeqSerializer;

class SeqLibrary;
class SeqChannel;
enum class SeqChannelType;

class SeqWindow;

class SeqAction;
class SeqActionHandler;

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

	void Undo();
	void Redo();

	void AddAction(const SeqAction action);
	void AddActionHandler(SeqActionHandler *handler);
	void RemoveActionHandler(SeqActionHandler *handler);
	int GetActionCount();
	int GetActionCursor();
	SeqAction GetAction(const int index);
	void DoAction(const SeqAction action);
	void UndoAction(const SeqAction action);

	int GetChannelCount();
	SeqChannel* GetChannel(const int index);

	int NextWindowId();
	void AddWindowSequencer();
	void AddWindowLibrary();
	void AddWindowVideo();
	void RemoveWindow(SeqWindow *window);

	double GetLength();

	void Update();
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
	int nextWindowId;
	SeqLibrary *library;
	SeqList<SeqChannel*> *channels;
	SeqList<SeqWindow*> *windows;

	SeqList<SeqActionHandler*> *actionHandlers;
	SeqList<SeqAction> *actions;
	int actionCursor = 0;
};
