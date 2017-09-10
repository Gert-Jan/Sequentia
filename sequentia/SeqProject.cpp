#include <imgui.h>
#include <SDL.h>

#include "SeqProjectHeaders.h";
#include "SeqDialogs.h";
#include "SeqSerializerBin.h";
#include "SeqPath.h";
#include "SeqUtils.h";
#include "SeqString.h";
#include "SeqList.h";

SeqProject::SeqProject()
{
	library = new SeqLibrary(this);
	channels = new SeqList<SeqChannel>();

	actionHandlers = new SeqList<SeqActionHandler*>();
	actions = new SeqList<SeqAction>();
	
	uiSequencers = new SeqList<SeqUISequencer*>();
	uiLibraries = new SeqList<SeqUILibrary*>();

	fullPath = "";

	// test data
	uiSequencers->Add(new SeqUISequencer(this));
	uiLibraries->Add(new SeqUILibrary(this));
}

SeqProject::~SeqProject()
{
	Clear();
	delete uiLibraries;
	delete uiSequencers;
	delete actions;
	delete actionHandlers;
	delete channels;
	delete library;
	delete[] fullPath;
}

void SeqProject::Clear()
{
	for (int i = 0; i < uiLibraries->Count(); i++)
		delete uiLibraries->Get(i);
	
	for (int i = 0; i < uiSequencers->Count(); i++)
		delete uiSequencers->Get(i);

	for (int i = 0; i < actions->Count(); i++)
		delete actions->Get(i).data;

	uiLibraries->Clear();
	uiSequencers->Clear();
	actions->Clear();
	actionHandlers->Clear();
	channels->Clear();
	library->Clear();

	SeqUILibrary::nextWindowId = 0;
	SeqUISequencer::nextWindowId = 0;
}

void SeqProject::Open()
{

	if (fullPath == "")
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
	if (fullPath == "")
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

void SeqProject::SetPath(char *fullPath)
{
	this->fullPath = fullPath;
	library->UpdateRelativePaths(fullPath);
}

void SeqProject::AddAction(SeqAction action)
{
	// overwrite redo list
	while (actionCursor < actions->Count())
	{
		actionCursor--;
		delete actions->Get(actionCursor).data;
		actions->RemoveAt(actionCursor);
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
	switch (action.type)
	{
		case SeqActionType::AddChannel:
		{
			SeqActionAddChannel *data = (SeqActionAddChannel*)action.data;
			AddChannel(data->type, data->name);
			break;
		}
		case SeqActionType::RemoveChannel:
		{
			RemoveChannel(channels->Count() - 1);
			break;
		}
	}
	// fire events
	for (int i = 0; i < actionHandlers->Count(); i++)
	{
		actionHandlers->Get(i)->ActionDone(action);
	}
}

void SeqProject::UndoAction(const SeqAction action)
{
	switch (action.type)
	{
		case SeqActionType::AddChannel:
			RemoveChannel(channels->Count() - 1);
			break;
		case SeqActionType::RemoveChannel:
			SeqActionAddChannel *data = (SeqActionAddChannel*)action.data;
			AddChannel(data->type, data->name);
			break;
	}
	// fire events
	for (int i = 0; i < actionHandlers->Count(); i++)
	{
		actionHandlers->Get(i)->ActionUndone(action);
	}
}

void SeqProject::AddChannel(SeqChannelType type, char *name)
{
	SeqChannel channel = SeqChannel(name, type);
	channels->Add(channel);
}

void SeqProject::RemoveChannel(const int index)
{
	channels->RemoveAt(index);
}

int SeqProject::GetChannelCount()
{
	return channels->Count();
}

SeqChannel SeqProject::GetChannel(const int index)
{
	return channels->Get(index);
}

void SeqProject::AddSequencer()
{
	uiSequencers->Add(new SeqUISequencer(this));
}

void SeqProject::RemoveSequencer(SeqUISequencer *sequencer)
{
	uiSequencers->Remove(sequencer);
	delete sequencer;
}

void SeqProject::AddLibrary()
{
	uiLibraries->Add(new SeqUILibrary(this));
}

void SeqProject::RemoveLibrary(SeqUILibrary *library)
{
	uiLibraries->Remove(library);
	delete library;
}

double SeqProject::GetLength()
{
	return 60.54321;
}

void SeqProject::Draw()
{
	SeqDialogs::Draw(this);
	for (int i = 0; i < uiSequencers->Count(); i++)
	{
		uiSequencers->Get(i)->Draw();
	}
	for (int i = 0; i < uiLibraries->Count(); i++)
	{
		uiLibraries->Get(i)->Draw();
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
		channels->Get(i).Serialize(serializer);
	
	// ui sequencers
	serializer->Write(uiSequencers->Count());
	for (int i = 0; i < uiSequencers->Count(); i++)
		uiSequencers->Get(i)->Serialize(serializer);

	// ui libraries
	serializer->Write(uiLibraries->Count());
	for (int i = 0; i < uiLibraries->Count(); i++)
		uiLibraries->Get(i)->Serialize(serializer);

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
	for (int i = 0; i < count; i++)
		channels->Add(SeqChannel(serializer));

	// ui sequencers
	count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
		uiSequencers->Add(new SeqUISequencer(this, serializer));

	// ui libraries
	count = serializer->ReadInt();
	for (int i = 0; i < count; i++)
		uiLibraries->Add(new SeqUILibrary(this, serializer));

	return 0;
}
