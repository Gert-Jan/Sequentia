#include <imgui.h>
#include <SDL.h>
#include "SeqProjectHeaders.h";
#include "SeqDialogs.h";
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
	for (int i = 0; i < uiLibraries->Count(); i++)
		delete uiLibraries->Get(i);
	delete uiLibraries;
	for (int i = 0; i < uiSequencers->Count(); i++)
		delete uiSequencers->Get(i);
	delete uiSequencers;

	for (int i = 0; i < actions->Count(); i++)
		delete actions->Get(i).data;
	delete actions;
	delete actionHandlers;
	
	delete channels;
	delete library;

	delete[] fullPath;
}

void SeqProject::Open()
{

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
		SDL_RWops *file = SDL_RWFromFile(fullPath, "w+b");
		if (file)
		{
			SDL_RWwrite(file, this, 100, 1);
			SDL_RWclose(file);
		}
		else
		{
			SeqDialogs::ShowError(const_cast<char*>(SDL_GetError()), 0);
		}
	}
}

void SeqProject::SaveAs()
{
	SeqDialogs::ShowRequestProjectPath(fullPath);
}

void SeqProject::SetPath(char *fullPath)
{
	this->fullPath = fullPath;
	library->UpdateRelativePaths(fullPath);
}

int SeqProject::Serialize()
{
	return 0;
}

int SeqProject::Deserialize()
{
	
	return 0;
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