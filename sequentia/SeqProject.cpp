#include <imgui.h>
#include <SDL.h>

#include "SeqProjectHeaders.h"
#include "SeqWindow.h"
#include "SeqUILibrary.h"
#include "SeqUISequencer.h"
#include "SeqUIVideo.h"
#include "SeqDialogs.h"
#include "SeqSerializerBin.h"
#include "SeqPath.h"
#include "SeqUtils.h"
#include "SeqString.h"
#include "SeqList.h"

SeqProject::SeqProject()
{
	library = new SeqLibrary();
	channels = new SeqList<SeqChannel*>();
	clipProxyPool = new SeqList<SeqClipProxy*>();

	windows = new SeqList<SeqWindow*>();

	actionHandlers = new SeqList<SeqActionHandler*>();
	actions = new SeqList<SeqAction>();
	
	fullPath = "";
}

SeqProject::~SeqProject()
{
	Clear();
	delete windows;
	delete actions;
	delete actionHandlers;
	delete channels;
	delete clipProxyPool;
	delete library;
	delete[] fullPath;
}

void SeqProject::Clear()
{
	for (int i = 0; i < windows->Count(); i++)
		delete windows->Get(i);

	for (int i = 0; i < actions->Count(); i++)
		delete actions->Get(i).data;

	for (int i = 0; i < channels->Count(); i++)
		delete channels->Get(i);

	for (int i = 0; i < clipProxyPool->Count(); i++)
		delete clipProxyPool->Get(i);

	windows->Clear();
	actions->Clear();
	actionHandlers->Clear();
	channels->Clear();
	clipProxyPool->Clear();
	library->Clear();

	nextWindowId = 0;
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

void SeqProject::SetPath(char *fullPath)
{
	char *oldPath = this->fullPath;
	this->fullPath = SeqPath::Normalize(fullPath);
	library->UpdatePaths(oldPath, fullPath);
}

SeqLibrary* SeqProject::GetLibrary()
{
	return library;
}

void SeqProject::AddChannel(SeqChannelType type, char *name)
{
	SeqChannel* channel = new SeqChannel(library, name, type);
	channels->Add(channel);
	if (channel->actionId == -1)
		channel->actionId = NextActionId();
	else if (channel->actionId >= nextActionId)
		nextActionId = channel->actionId + 1;
}

void SeqProject::RemoveChannel(const int index)
{
	channels->RemoveAt(index);
}

int SeqProject::GetChannelCount()
{
	return channels->Count();
}

SeqChannel* SeqProject::GetChannel(const int index)
{
	return channels->Get(index);
}

int SeqProject::GetChannelIndexByActionId(const int id)
{
	for (int i = 0; i < channels->Count(); i++)
		if (channels->Get(i)->actionId == id)
			return i;
	return -1;
}

SeqClipProxy* SeqProject::NextClipProxy()
{
	// reuse proxy
	for (int i = 0; i < clipProxyPool->Count(); i++)
	{
		SeqClipProxy *proxy = clipProxyPool->Get(i);
		if (!proxy->IsActive())
		{
			return proxy;
		}
	}
	// make new proxy
	SeqClipProxy *proxy = new SeqClipProxy();
	clipProxyPool->Add(proxy);
	return proxy;
}

void SeqProject::DeactivateAllClipProxies()
{
	for (int i = 0; i < clipProxyPool->Count(); i++)
		clipProxyPool->Get(i)->Deactivate();
}

int SeqProject::NextWindowId()
{
	return nextWindowId++;
}

void SeqProject::AddWindowSequencer()
{
	windows->Add(new SeqUISequencer(this));
}

void SeqProject::AddWindowLibrary()
{
	windows->Add(new SeqUILibrary(this, library));
}

void SeqProject::AddWindowVideo()
{
	windows->Add(new SeqUIVideo(this, library));
}

void SeqProject::RemoveWindow(SeqWindow *window)
{
	windows->Remove(window);
	delete window;
}

double SeqProject::GetLength()
{
	return 60.54321;
}

void SeqProject::Update()
{
	library->Update();
}

void SeqProject::Draw()
{
	SeqDialogs::Draw(this);
	for (int i = 0; i < windows->Count(); i++)
	{
		windows->Get(i)->Draw();
	}
}

int SeqProject::NextActionId()
{
	// TODO: on overflow rearrange action ids.
	return nextActionId++;
}

void SeqProject::Undo()
{
	if (actionCursor > 0)
	{
		actionCursor--;
		UndoAction(actions->Get(actionCursor));
	}
}

void SeqProject::Redo()
{
	if (actionCursor < actions->Count())
	{
		DoAction(actions->Get(actionCursor));
		actionCursor++;
	}
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

int SeqProject::GetActionCount()
{
	return actions->Count();
}

int SeqProject::GetActionCursor()
{
	return actionCursor;
}

SeqAction SeqProject::GetAction(const int index)
{
	return actions->Get(index);
}

void SeqProject::DoAction(const SeqAction action)
{
	// do the action
	ExecuteAction(action, action.execution);
	// fire events
	for (int i = 0; i < actionHandlers->Count(); i++)
	{
		actionHandlers->Get(i)->ActionDone(action);
	}
}

void SeqProject::UndoAction(const SeqAction action)
{
	// invert execution
	SeqActionExecution execution = action.execution;
	if (execution == SeqActionExecution::Do)
		execution = SeqActionExecution::Undo;
	else
		execution = SeqActionExecution::Do;
	// do the action
	ExecuteAction(action, execution);
	// fire events
	for (int i = 0; i < actionHandlers->Count(); i++)
	{
		actionHandlers->Get(i)->ActionUndone(action);
	}
}

void SeqProject::ExecuteAction(const SeqAction action, const SeqActionExecution execution)
{
	switch (action.type)
	{
		case SeqActionType::AddChannel:
		{
			if (execution == SeqActionExecution::Do)
			{
				SeqActionAddChannel *data = (SeqActionAddChannel*)action.data;
				AddChannel(data->type, SeqString::Copy(data->name));
			}
			else
			{
				RemoveChannel(channels->Count() - 1);
			}
			break;
		}
		case SeqActionType::AddLibraryLink:
		{
			char *fullPath = SeqString::Copy((char*)action.data);
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
		case SeqActionType::AddClipToChannel:
		{
			SeqActionAddClipToChannel *data = (SeqActionAddClipToChannel*)action.data;
			SeqChannel* channel = GetChannel(GetChannelIndexByActionId(data->channelId));
			if (execution == SeqActionExecution::Do)
			{
				SeqClip* clip = new SeqClip(library, library->GetLink(data->libraryLinkIndex));
				clip->location.leftTime = data->leftTime;
				clip->location.rightTime = data->rightTime;
				clip->location.startTime = data->startTime;
				channel->AddClip(clip);
				if (data->clipId == -1)
					data->clipId = clip->actionId;
				else
					clip->actionId = data->clipId;
			}
			else
			{
				int clipIndex = channel->GetClipIndexByActionId(data->clipId);
				SeqClip *clip = channel->GetClip(clipIndex);
				channel->RemoveClipAt(clipIndex);
				delete clip;
			}
			break;
		}
		case SeqActionType::MoveClipToChannel:
		{
			SeqActionMoveClipToChannel *data = (SeqActionMoveClipToChannel*)action.data;
			SeqChannel* fromChannel = GetChannel(GetChannelIndexByActionId(data->fromChannelId));
			SeqChannel* toChannel = GetChannel(GetChannelIndexByActionId(data->toChannelId));
			if (execution == SeqActionExecution::Do)
			{
				SeqClip* clip = fromChannel->GetClip(fromChannel->GetClipIndexByActionId(data->fromClipId));
				clip->SetParent(toChannel);
				if (data->toClipId == -1)
					data->toClipId = clip->actionId;
				else
					clip->actionId = data->toClipId;
				clip->isHidden = false;
			}
			else
			{
				SeqClip* clip = toChannel->GetClip(toChannel->GetClipIndexByActionId(data->toClipId));
				clip->SetParent(fromChannel);
				clip->actionId = data->fromClipId;
				clip->isHidden = false;
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
	
	// channels
	serializer->Write(channels->Count());
	for (int i = 0; i < channels->Count(); i++)
		channels->Get(i)->Serialize(serializer);
	
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

	// channels
	count = serializer->ReadInt();
	nextActionId = count;
	for (int i = 0; i < count; i++)
	{
		SeqChannel *channel = new SeqChannel(library, serializer);
		channel->actionId = i;
		channels->Add(channel);
	}

	// ui sequencers
	count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
	{
		int windowType = serializer->ReadInt();
		switch ((SeqWindowType)windowType)
		{
			case SeqWindowType::Library:
				windows->Add(new SeqUILibrary(this, library, serializer));
				break;
			case SeqWindowType::Video:
				windows->Add(new SeqUIVideo(this, library, serializer));
				break;
			case SeqWindowType::Sequencer:
				windows->Add(new SeqUISequencer(this, serializer));
				break;
		}
	}

	return 0;
}
