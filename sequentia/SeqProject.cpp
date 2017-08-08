#include "SeqProject.h";

SeqProject::SeqProject()
{
	actionHandlers = new SeqList<SeqActionHandler*>();
	actions = new SeqList<SeqAction>();
	channels = new SeqList<SeqChannel>();
	uiSequencer = new SeqUISequencer(this);
}

SeqProject::~SeqProject()
{
	delete actionHandlers;
	delete actions;
	delete channels;
	delete uiSequencer;
}

void SeqProject::AddAction(const SeqAction action)
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
		}
			break;
		case SeqActionType::RemoveChannel:
			RemoveChannel(channels->Count() - 1);
			break;
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

double SeqProject::GetLength()
{
	return 60.54321;
}

void SeqProject::Draw()
{
	uiSequencer->Draw();
}
