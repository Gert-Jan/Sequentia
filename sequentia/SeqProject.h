#pragma once

struct SDL_RWops;
class SeqSerializer;

class SeqLibrary;
class SeqChannel;
enum class SeqChannelType;
class SeqClipProxy;

class SeqWindow;

struct SeqAction;
class SeqActionHandler;
enum class SeqActionExecution;

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

	SeqLibrary* GetLibrary();

	int GetChannelCount();
	SeqChannel* GetChannel(const int index);
	int GetChannelIndexByActionId(const int id);

	SeqClipProxy* NextClipProxy();
	void DeactivateAllClipProxies();

	int NextWindowId();
	void AddWindowSequencer();
	void AddWindowLibrary();
	void AddWindowVideo();
	void RemoveWindow(SeqWindow *window);

	double GetLength();

	void Update();
	void Draw();
	void DrawDialogs();

	void Undo();
	bool CanUndo();
	void Redo();
	bool CanRedo();

	void AddAction(const SeqAction action);
	void AddActionHandler(SeqActionHandler *handler);
	void RemoveActionHandler(SeqActionHandler *handler);

private:
	int NextActionId();
	void AddChannel(SeqChannelType type, char *name);
	void RemoveChannel(const int index);

	void DoAction(const SeqAction action);
	void UndoAction(const SeqAction action);
	void ExecuteAction(const SeqAction action, const SeqActionExecution execution);

	int Serialize(SeqSerializer *serializer);
	int Deserialize(SeqSerializer *serializer);

private:
	const int version = 1;
	char *fullPath;
	int nextWindowId;
	SeqLibrary *library;
	SeqList<SeqChannel*> *channels;
	SeqList<SeqClipProxy*> *clipProxyPool;
	SeqList<SeqWindow*> *windows;

	SeqList<SeqActionHandler*> *actionHandlers;
	SeqList<SeqAction> *actions;
	int actionCursor = 0;
	int nextActionId = 0;
};
