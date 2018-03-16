#pragma once

struct SDL_RWops;
class SeqSerializer;

class SeqLibrary;
class SeqScene;
class SeqSelection;

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

	void New();
	void Open();
	void OpenFrom();
	void Save();
	void SaveAs();

	char* GetPath();
	void SetPath(const char *fullPath);

	SeqLibrary* GetLibrary();

	SeqScene* CreateScene(const char *name);
	void AddScene(SeqScene *scene);
	void RemoveScene(SeqScene *scene);
	int SceneCount();
	SeqScene* GetScene(const int index);
	SeqScene* GetSceneById(const int id);
	SeqScene* GetPreviewScene();

	SeqSelection* NextClipSelection();
	void DeactivateAllClipSelections();

	int NextWindowId();
	void AddWindowSequencer();
	void AddWindowLibrary();
	void AddWindowVideo();
	void RemoveWindow(SeqWindow *window);

	void Update();
	void Draw();
	void DrawDialogs();

	void Undo();
	bool CanUndo();
	void Redo();
	bool CanRedo();

	void AddAction(const SeqAction action);
	void DoAndForgetAction(const SeqAction action);
	void AddActionHandler(SeqActionHandler *handler);
	void RemoveActionHandler(SeqActionHandler *handler);

private:
	void Clear();
	void CreateDefault();

	int NextSceneId();

	void DoAction(const SeqAction action);
	void UndoAction(const SeqAction action);
	void ExecuteAction(const SeqAction action, const SeqActionExecution execution);

	int Serialize(SeqSerializer *serializer);
	int Deserialize(SeqSerializer *serializer);

public:
	int width, height;

private:
	const int version = 1;
	char *fullPath;

	int nextWindowId;
	SeqLibrary *library;
	SeqList<SeqScene*> *scenes;
	SeqList<SeqSelection*> *clipSelectionPool;
	SeqList<SeqWindow*> *windows;

	SeqList<SeqActionHandler*> *actionHandlers;
	SeqList<SeqAction> *actions;
	int actionCursor = 0;
	int nextSceneId = 0;
};
