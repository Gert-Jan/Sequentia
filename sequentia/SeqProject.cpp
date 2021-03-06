#include <imgui.h>
#include <SDL.h>

#include "Sequentia.h"
#include "SeqProjectHeaders.h"
#include "SeqMediaType.h"
#include "SeqPlayer.h"
#include "SeqWindow.h"
#include "SeqUILibrary.h"
#include "SeqUISequencer.h"
#include "SeqUIVideo.h"
#include "SeqDialogs.h"
#include "SeqSerializerBin.h"
#include "SeqPath.h"
#include "SeqString.h"
#include "SeqList.h"

SeqProject::SeqProject():
	nextWindowId(0),
	actionCursor(0),
	nextSceneId(0),
	fullPath(""),
	width(1920),
	height(1080)
{
	library = new SeqLibrary();
	scenes = new SeqList<SeqScene*>();
	clipSelectionPool = new SeqList<SeqSelection*>();

	windows = new SeqList<SeqWindow*>();

	actionHandlers = new SeqList<SeqActionHandler*>();
	actions = new SeqList<SeqAction>();
}

SeqProject::~SeqProject()
{
	Clear();
	delete windows;
	delete actions;
	delete actionHandlers;
	delete scenes;
	delete clipSelectionPool;
	delete library;
	delete[] fullPath;
}

void SeqProject::Clear()
{
	for (int i = 0; i < windows->Count(); i++)
		delete windows->Get(i);

	for (int i = 0; i < actions->Count(); i++)
		delete actions->Get(i).data;

	for (int i = 0; i < scenes->Count(); i++)
		delete scenes->Get(i);

	for (int i = 0; i < clipSelectionPool->Count(); i++)
		delete clipSelectionPool->Get(i);

	windows->Clear();
	actions->Clear();
	actionHandlers->Clear();
	scenes->Clear();
	clipSelectionPool->Clear();
	library->Clear();

	nextWindowId = 0;
	nextSceneId = 0;
}

void SeqProject::CreateDefault()
{
	// preview scene
	SeqScene *previewScene = CreateScene("preview");
	previewScene->AddChannel(SeqMediaType::Video, "video");
	previewScene->AddChannel(SeqMediaType::Audio, "audio");
	AddScene(previewScene);

	// master scene
	SeqScene *masterScene = CreateScene("master");
	for (int i = 0; i < 5; i++)
	{
		masterScene->AddChannel(SeqMediaType::Video, "Video");
		masterScene->AddChannel(SeqMediaType::Audio, "Audio");
	}
	AddScene(masterScene);

	// windows
	AddWindowSequencer();
	AddWindowLibrary();
	AddWindowVideo();
}

void SeqProject::New()
{
	Clear();
	CreateDefault();
}

void SeqProject::Open()
{

	if (SeqString::IsEmpty(fullPath))
	{
		OpenFrom();
		return;
	}

	SDL_RWops *file = SDL_RWFromFile(fullPath, "r");
	if (file)
	{
		SeqSerializer *serializer = new SeqSerializerBin(file);
		Clear();
		Deserialize(serializer);
		delete serializer;
		SDL_RWclose(file);
	}
	else
	{
		SeqDialogs::ShowError(const_cast<char*>(SDL_GetError()));
	}
}

void SeqProject::OpenFrom()
{
	SeqDialogs::ShowRequestProjectPath(fullPath, RequestPathAction::Open);
}

void SeqProject::Save()
{
	if (SeqString::IsEmpty(fullPath))
	{
		SaveAs();
		return;
	}
	
	int error = SeqPath::CreateDir(fullPath);
	if (error != 0 && error != EEXIST)
	{
		SeqDialogs::ShowError("Error while creating a directory: %s", error);
	}
	else
	{
		SDL_RWops *file = SDL_RWFromFile(fullPath, "wb");
		if (file)
		{
			SeqSerializer *serializer = new SeqSerializerBin(file);
			Serialize(serializer);
			delete serializer;
			SDL_RWclose(file);
		}
		else
		{
			SeqDialogs::ShowError(const_cast<char*>(SDL_GetError()));
		}
	}
}

void SeqProject::SaveAs()
{
	SeqDialogs::ShowRequestProjectPath(fullPath, RequestPathAction::Save);
}

char* SeqProject::GetPath()
{
	return this->fullPath;
}

void SeqProject::SetPath(const char *fullPath)
{
	char *oldPath = this->fullPath;
	this->fullPath = SeqPath::Normalize(fullPath);
	library->UpdatePaths(oldPath, fullPath);
}

SeqLibrary* SeqProject::GetLibrary()
{
	return library;
}

SeqScene* SeqProject::CreateScene(const char *name)
{
	return new SeqScene(NextSceneId(), name);
}

void SeqProject::AddScene(SeqScene *scene)
{
	scenes->Add(scene);
}

void SeqProject::RemoveScene(SeqScene *scene)
{
	scenes->Remove(scene);
}

int SeqProject::SceneCount()
{
	return scenes->Count();
}

SeqScene* SeqProject::GetScene(const int index)
{
	return scenes->Get(index);
}

SeqScene* SeqProject::GetSceneById(const int id)
{
	for (int i = 0; i < scenes->Count(); i++)
		if (scenes->Get(i)->id == id)
			return scenes->Get(i);
	return nullptr;
}

SeqScene* SeqProject::GetPreviewScene()
{
	return scenes->Get(0);
}

SeqSelection* SeqProject::NextClipSelection()
{
	// reuse selection
	for (int i = 0; i < clipSelectionPool->Count(); i++)
	{
		SeqSelection *selection = clipSelectionPool->Get(i);
		if (!selection->IsActive())
		{
			return selection;
		}
	}
	// make new selection
	SeqSelection *selection = new SeqSelection();
	clipSelectionPool->Add(selection);
	return selection;
}

void SeqProject::DeactivateAllClipSelections()
{
	for (int i = 0; i < clipSelectionPool->Count(); i++)
		clipSelectionPool->Get(i)->Deactivate();
}

int SeqProject::NextWindowId()
{
	return nextWindowId++;
}

void SeqProject::AddWindowSequencer()
{
	windows->Add(new SeqUISequencer(scenes->Get(1)));
}

void SeqProject::AddWindowLibrary()
{
	windows->Add(new SeqUILibrary());
}

void SeqProject::AddWindowVideo()
{
	windows->Add(new SeqUIVideo());
}

void SeqProject::RemoveWindow(SeqWindow *window)
{
	windows->Remove(window);
	delete window;
}

void SeqProject::Update()
{
	library->Update();
	for (int i = 0; i < scenes->Count(); i++)
		scenes->Get(i)->player->Update();
}

void SeqProject::Draw()
{
	for (int i = 0; i < scenes->Count(); i++)
		scenes->Get(i)->player->Render();
	SeqDialogs::Draw();
	for (int i = 0; i < windows->Count(); i++)
	{
		windows->Get(i)->Draw();
	}
}

int SeqProject::NextSceneId()
{
	return nextSceneId++;
}

void SeqProject::Undo()
{
	if (CanUndo())
	{
		actionCursor--;
		UndoAction(actions->Get(actionCursor));
	}
}

bool SeqProject::CanUndo()
{
	return actionCursor > 0;
}

void SeqProject::Redo()
{
	if (CanRedo())
	{
		DoAction(actions->Get(actionCursor));
		actionCursor++;
	}
}

bool SeqProject::CanRedo()
{
	return actionCursor < actions->Count();
}

void SeqProject::AddAction(SeqAction action)
{
	// overwrite redo list
	for (int i = actions->Count() - 1; i >= actionCursor; i--)
	{
		delete actions->Get(i).data;
		actions->RemoveAt(i);
	}
	// add the action in the undo list
	actions->Add(action);
	actionCursor++;
	// execute action
	DoAction(action);
}

void SeqProject::AddActionHandler(SeqActionHandler *handler)
{
	actionHandlers->Add(handler);
}

void SeqProject::RemoveActionHandler(SeqActionHandler *handler)
{
	actionHandlers->Remove(handler);
}

void SeqProject::DoAndForgetAction(const SeqAction action)
{
	// do action
	DoAction(action);
	// forget action
	delete action.data;
}

void SeqProject::DoAction(const SeqAction action)
{
	SeqActionExecution execution = action.execution;
	// fire events
	for (int i = 0; i < actionHandlers->Count(); i++)
	{
		actionHandlers->Get(i)->PreExecuteAction(action.type, execution, action.data);
	}
	// do the action
	ExecuteAction(action, execution);
}

void SeqProject::UndoAction(const SeqAction action)
{
	// invert execution
	SeqActionExecution execution = action.execution;
	if (execution == SeqActionExecution::Do)
		execution = SeqActionExecution::Undo;
	else
		execution = SeqActionExecution::Do;
	// fire events
	for (int i = 0; i < actionHandlers->Count(); i++)
	{
		actionHandlers->Get(i)->PreExecuteAction(action.type, execution, action.data);
	}
	// do the action
	ExecuteAction(action, execution);
}

void SeqProject::ExecuteAction(const SeqAction action, const SeqActionExecution execution)
{
	switch (action.type)
	{
		case SeqActionType::AddChannel:
		{
			SeqActionAddChannel *data = (SeqActionAddChannel*)action.data;
			SeqScene* scene = GetSceneById(data->sceneId);
			if (execution == SeqActionExecution::Do)
			{
				scene->AddChannel(data->type, SeqString::Copy(data->name));
			}
			else
			{
				scene->RemoveChannel(scene->ChannelCount() - 1);
				scene->RefreshLastClip();
			}
			break;
		}
		case SeqActionType::AddLibraryLink:
		{
			char *fullPath = (char*)action.data;
			if (execution == SeqActionExecution::Do)
			{
				library->AddLink(fullPath);
			}
			else
			{
				library->RemoveLink(fullPath);
			}
			break;
		}
		case SeqActionType::AddClipGroup:
		{
			SeqActionAddClipGroup *data = (SeqActionAddClipGroup*)action.data;
			SeqScene* scene = GetSceneById(data->sceneId);
			if (execution == SeqActionExecution::Do)
			{
				scene->AddClipGroup();
				SeqClipGroup *group = scene->GetClipGroup(scene->ClipGroupCount() - 1);
				data->groupId = group->actionId;
			}
			else
			{
				scene->RemoveClipGroup(scene->GetClipGroupIndexByActionId(data->groupId));
			}
			break;
		}
		case SeqActionType::AddClipToChannel:
		{
			SeqActionAddClipToChannel *data = (SeqActionAddClipToChannel*)action.data;
			SeqScene *scene = GetSceneById(data->sceneId);
			SeqChannel *channel = scene->GetChannelByActionId(data->channelId);
			if (execution == SeqActionExecution::Do)
			{
				SeqLibraryLink *link = library->GetLink(data->libraryLinkIndex);
				SeqClip* clip = new SeqClip(link, data->streamIndex);
				clip->location.leftTime = data->leftTime;
				clip->location.rightTime = data->rightTime;
				clip->location.startTime = data->startTime;
				channel->AddClip(clip);
				if (data->clipId == -1)
					data->clipId = clip->actionId;
				else
					clip->actionId = data->clipId;
				scene->RefreshLastClip();
			}
			else
			{
				int clipIndex = channel->GetClipIndexByActionId(data->clipId);
				SeqClip *clip = channel->GetClip(clipIndex);
				SeqSelection *dragProxy = Sequentia::GetDragClipSelection();
				if (dragProxy != nullptr && dragProxy->GetClip() == clip)
				{
					Sequentia::SetDragClip(nullptr);
				}
				channel->RemoveClipAt(clipIndex);
				delete clip;
				scene->RefreshLastClip();
			}
			break;
		}
		case SeqActionType::AddClipToGroup:
		{
			SeqActionAddClipToGroup *data = (SeqActionAddClipToGroup*)action.data;
			SeqScene *scene = GetSceneById(data->sceneId);
			SeqChannel *channel = scene->GetChannelByActionId(data->channelId);
			SeqClipGroup *group = scene->GetClipGroupByActionId(data->groupId);
			SeqClip *clip = channel->GetClipByActionId(data->clipId);
			if (execution == SeqActionExecution::Do)
			{
				group->AddClip(clip);
			}
			else
			{
				group->RemoveClip(clip);
			}
			break;
		}
		case SeqActionType::MoveClip:
		{
			SeqActionMoveClip *data = (SeqActionMoveClip*)action.data;
			SeqScene *fromScene = GetSceneById(data->fromSceneId);
			SeqScene *toScene = GetSceneById(data->toSceneId);
			SeqChannel* fromChannel = fromScene->GetChannelByActionId(data->fromChannelId);
			SeqChannel* toChannel = toScene->GetChannelByActionId(data->toChannelId);
			if (execution == SeqActionExecution::Do)
			{
				SeqClip* clip = fromChannel->GetClip(fromChannel->GetClipIndexByActionId(data->fromClipId));
				clip->SetParent(toChannel);
				if (data->toClipId == -1)
					data->toClipId = clip->actionId;
				else
					clip->actionId = data->toClipId;
				clip->SetPosition(data->toLeftTime);
				clip->isHidden = false;
				fromScene->RefreshLastClip();
			}
			else
			{
				SeqClip* clip = toChannel->GetClip(toChannel->GetClipIndexByActionId(data->toClipId));
				clip->SetParent(fromChannel);
				clip->actionId = data->fromClipId;
				clip->SetPosition(data->fromLeftTime);
				clip->isHidden = false;
				toScene->RefreshLastClip();
			}
			break;
		}
	}
}

int SeqProject::Serialize(SeqSerializer *serializer)
{
	serializer->SetApplicationVersion(version);
	serializer->SetSerializedVersion(version);

	// project
	serializer->Write(version);
	serializer->Write(fullPath);
	
	// library
	library->Serialize(serializer);
	
	// scenes
	serializer->Write(scenes->Count());
	for (int i = 0; i < scenes->Count(); i++)
		scenes->Get(i)->Serialize(serializer);
	
	// windows
	serializer->Write(windows->Count());
	for  (int i = 0; i < windows->Count(); i++)
	{
		SeqWindow *window = windows->Get(i);
		serializer->Write((int)window->GetWindowType());
		window->Serialize(serializer);
	}

	return 0;
}

int SeqProject::Deserialize(SeqSerializer *serializer)
{
	int count = 0;

	// project
	serializer->SetApplicationVersion(version);
	serializer->SetSerializedVersion(serializer->ReadInt());
	fullPath = serializer->ReadString();
	
	// library
	library->Deserialize(serializer);

	// scenes
	count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
	{
		SeqScene *scene = new SeqScene(serializer);
		scenes->Add(scene);
		nextSceneId = SDL_max(scene->id, nextSceneId) + 1;
	}

	// ui sequencers
	count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
	{
		int windowType = serializer->ReadInt();
		switch ((SeqWindowType)windowType)
		{
			case SeqWindowType::Library:
				windows->Add(new SeqUILibrary(serializer));
				break;
			case SeqWindowType::Video:
				windows->Add(new SeqUIVideo(serializer));
				break;
			case SeqWindowType::Sequencer:
				windows->Add(new SeqUISequencer(scenes->Get(0), serializer));
				break;
		}
	}

	return 0;
}
